#include "gdal_dataset_factory.hpp"
#include "gdal_priv.h"
#include "modules/gdal/gdal_dataset_ts.hpp"
#include "duckdb/common/types/uuid.hpp"

namespace duckdb {

GDALThreadSafeDataset *GDALDatasetFactory::FromFile(const std::string &file_path,
                                                    const std::vector<std::string> &allowed_drivers,
                                                    const std::vector<std::string> &open_options,
                                                    const std::vector<std::string> &sibling_files) {

	auto gdal_allowed_drivers = GDALDatasetFactory::FromVectorOfStrings(allowed_drivers);
	auto gdal_open_options = GDALDatasetFactory::FromVectorOfStrings(open_options);
	auto gdal_sibling_files = GDALDatasetFactory::FromVectorOfStrings(sibling_files);

	GDALDataset *dataset = GDALDataset::Open(file_path.c_str(), GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR,
	                                         gdal_allowed_drivers.empty() ? nullptr : gdal_allowed_drivers.data(),
	                                         gdal_open_options.empty() ? nullptr : gdal_open_options.data(),
	                                         gdal_sibling_files.empty() ? nullptr : gdal_sibling_files.data());

	return dataset ? new GDALThreadSafeDataset(dataset) : nullptr;
}

//! Get a valid file extension for a GDAL driver
static std::string GetFileExtensionForDriver(const std::string &driver_name) {

	auto driver = GetGDALDriverManager()->GetDriverByName(driver_name.c_str());
	if (!driver) {
		throw InvalidInputException("Unknown driver '%s'", driver_name.c_str());
	}

	const char *extension = driver->GetMetadataItem(GDAL_DMD_EXTENSION);
	if (extension && *extension) {
		return std::string(extension);
	}

	const char *extensions = driver->GetMetadataItem("DMD_EXTENSIONS");
	if (extensions && *extensions) {
		if (extensions && *extensions) {
			std::istringstream iss(extensions);
			std::string first_ext;
			iss >> first_ext;
			return std::string(first_ext);
		}
	}
	return "dat";
}

GDALThreadSafeDataset *GDALDatasetFactory::FromBlob(const char *blob, const uint64_t blob_size,
                                                    const std::vector<std::string> &allowed_drivers,
                                                    const std::vector<std::string> &open_options) {

	if (allowed_drivers.empty()) {
		throw InvalidInputException("Driver name[s] must be specified");
	}

	std::string file_ext = GetFileExtensionForDriver(allowed_drivers[0]);
	std::string mem_file_name = "/vsimem/tmp-" + UUID::ToString(UUID::GenerateRandomUUID()) + "." + file_ext;

	VSIFCloseL(VSIFileFromMemBuffer(mem_file_name.c_str(), (GByte *)(blob), blob_size, FALSE));

	GDALThreadSafeDataset *dataset = GDALDatasetFactory::FromFile(mem_file_name, allowed_drivers, open_options);
	VSIUnlink(mem_file_name.c_str());

	return dataset;
}

bool GDALDatasetFactory::WriteFile(GDALThreadSafeDataset *dataset, const std::string &file_path,
                                   const std::string &driver_name, const std::vector<std::string> &write_options) {

	auto driver = GetGDALDriverManager()->GetDriverByName(driver_name.c_str());

	if (!driver) {
		throw InvalidInputException("Unknown driver '%s'", driver_name.c_str());
	}

	bool copy_available = CSLFetchBoolean(driver->GetMetadata(), GDAL_DCAP_CREATECOPY, FALSE);
	auto gdal_write_options = GDALDatasetFactory::FromVectorOfStrings(write_options);
	auto gdal_options = gdal_write_options.empty() ? nullptr : gdal_write_options.data();

	GDALDatasetUniquePtr output;
	CPLErrorReset();

	if (copy_available) {
		output = GDALDatasetUniquePtr(
		    driver->CreateCopy(file_path.c_str(), dataset->get(), FALSE, gdal_options, NULL, NULL));

		if (output.get() == nullptr) {
			return false;
		}
	} else {
		GDALDataset *dataset_ = dataset->get();

		int cols = dataset_->GetRasterXSize();
		int rows = dataset_->GetRasterYSize();
		int band_count = dataset_->GetRasterCount();

		if (band_count == 0) {
			throw InvalidInputException("Input Raster has no RasterBands");
		}

		GDALRasterBand *raster_band = dataset_->GetRasterBand(1);
		GDALDataType data_type = raster_band->GetRasterDataType();
		int date_type_size = GDALGetDataTypeSize(data_type);

		output =
		    GDALDatasetUniquePtr(driver->Create(file_path.c_str(), cols, rows, band_count, data_type, gdal_options));

		if (output.get() == nullptr) {
			return false;
		}

		double gt[6] = {0, 1, 0, 0, 0, -1};
		dataset_->GetGeoTransform(gt);
		output->SetGeoTransform(gt);

		output->SetProjection(dataset_->GetProjectionRef());
		output->SetMetadata(dataset_->GetMetadata());

		void *pafScanline = CPLMalloc(date_type_size * cols * rows);

		for (int i = 1; i <= band_count; i++) {
			GDALRasterBand *source_band = dataset_->GetRasterBand(i);
			GDALRasterBand *target_band = output->GetRasterBand(i);

			target_band->SetMetadata(source_band->GetMetadata());
			target_band->SetNoDataValue(source_band->GetNoDataValue());
			target_band->SetColorInterpretation(source_band->GetColorInterpretation());

			if (source_band->RasterIO(GF_Read, 0, 0, cols, rows, pafScanline, cols, rows, data_type, 0, 0) != CE_None ||
			    target_band->RasterIO(GF_Write, 0, 0, cols, rows, pafScanline, cols, rows, data_type, 0, 0) !=
			        CE_None) {
				CPLFree(pafScanline);
				return false;
			}
		}

		CPLFree(pafScanline);
	}
	output->FlushCache();

	return true;
}

const char *GDALDatasetFactory::WriteBlob(GDALThreadSafeDataset *dataset, const std::string &driver_name,
                                          uint64_t &blob_size, const std::vector<std::string> &write_options) {

	std::string file_ext = GetFileExtensionForDriver(driver_name);
	std::string mem_file_name = "/vsimem/tmp-" + UUID::ToString(UUID::GenerateRandomUUID()) + "." + file_ext;

	if (!GDALDatasetFactory::WriteFile(dataset, mem_file_name, driver_name, write_options)) {
		blob_size = 0;
		return nullptr;
	}

	// Get stream of bytes of the created memory file
	GUIntBig blob_usize = 0;
	GByte *blob_ptr = VSIGetMemFileBuffer(mem_file_name.c_str(), &blob_usize, TRUE);
	VSIUnlink(mem_file_name.c_str());

	blob_size = static_cast<uint64_t>(blob_usize);
	return (const char *)blob_ptr;
}

std::vector<char const *> GDALDatasetFactory::FromVectorOfStrings(const std::vector<std::string> &input) {
	auto output = std::vector<char const *>();

	if (input.size()) {
		output.reserve(input.size() + 1);

		for (auto it = input.begin(); it != input.end(); ++it) {
			output.push_back((*it).c_str());
		}
		output.push_back(nullptr);
	}
	return output;
}

std::vector<char const *> GDALDatasetFactory::FromNamedParameters(const named_parameter_map_t &input,
                                                                  const std::string &keyname) {
	auto output = std::vector<char const *>();

	auto input_param = input.find(keyname);
	if (input_param != input.end()) {
		output.reserve(input.size() + 1);

		for (auto &param : ListValue::GetChildren(input_param->second)) {
			output.push_back(StringValue::Get(param).c_str());
		}
		output.push_back(nullptr);
	}
	return output;
}

} // namespace duckdb
