#include "gdal_priv.h"
#include "raster.hpp"

namespace duckdb {

Raster::Raster(GDALDataset *dataset) : dataset_(dataset) {
}

int Raster::GetRasterXSize() const {
	return dataset_->GetRasterXSize();
}

int Raster::GetRasterYSize() const {
	return dataset_->GetRasterYSize();
}

int Raster::GetRasterCount() const {
	return dataset_->GetRasterCount();
}

int32_t Raster::GetSrid() const {

	int32_t srid = 0; // SRID_UNKNOWN

	const char *proj_def = dataset_->GetProjectionRef();
	if (proj_def) {
		OGRSpatialReference spatial_ref;
		spatial_ref.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

		if (spatial_ref.importFromWkt(proj_def) == OGRERR_NONE && spatial_ref.AutoIdentifyEPSG() == OGRERR_NONE) {

			const char *code = spatial_ref.GetAuthorityCode(nullptr);
			if (code) {
				srid = atoi(code);
			}
		}
	}
	return srid;
}

bool Raster::GetGeoTransform(double *matrix) const {

	if (dataset_->GetGeoTransform(matrix) != CE_None) {
		// Using default geotransform matrix (0, 1, 0, 0, 0, -1)
		matrix[0] = 0;
		matrix[1] = 1;
		matrix[2] = 0;
		matrix[3] = 0;
		matrix[4] = 0;
		matrix[5] = -1;
		return false;
	}
	return true;
}

bool Raster::GetInvGeoTransform(double *inv_matrix) const {
	double gt[6] = {0};
	GetGeoTransform(gt);

	if (!GDALInvGeoTransform(gt, inv_matrix)) {
		return false;
	}
	return true;
}

std::string Raster::GetLastErrorMsg() {
	return std::string(CPLGetLastErrorMsg());
}

} // namespace duckdb
