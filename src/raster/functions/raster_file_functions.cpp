#include "raster_file_functions.hpp"
#include "raster.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/common/vector_operations/generic_executor.hpp"
// Spatial
#include "spatial/util/function_builder.hpp"
// GDAL
#include "gdal_priv.h"
#include "modules/gdal/gdal_dataset_factory.hpp"
#include "modules/gdal/gdal_context_state.hpp"
#include "modules/gdal/gdal_dataset_ts.hpp"

namespace duckdb {

namespace {

//======================================================================================================================
// RT_File
//======================================================================================================================

struct RT_File {

	static void RasterFromFile(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 1);

		auto &context = state.GetContext();
		auto &ctx_state = GDALClientContextState::GetOrCreate(context);

		UnaryExecutor::Execute<string_t, uintptr_t>(args.data[0], result, args.size(), [&](string_t input) {
			auto raw_file_name = input.GetString();

			GDALThreadSafeDataset *dataset = GDALDatasetFactory::FromFile(raw_file_name);
			if (dataset == nullptr) {
				auto error = Raster::GetLastErrorMsg();
				throw IOException("Could not open file: " + raw_file_name + " (" + error + ")");
			}

			ctx_state.GetDatasetRegistry().RegisterDataset(dataset);
			return CastPointerToValue(dataset);
		});
	}

	static void RasterAsFile_01(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 3);

		TernaryExecutor::Execute<uintptr_t, string_t, string_t, bool>(
		    args.data[0], args.data[1], args.data[2], result, args.size(),
		    [&](uintptr_t input, string_t file_name, string_t driver_name) {
			    GDALThreadSafeDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input);
			    auto raw_file_name = file_name.GetString();

			    auto gdal_driver_name = driver_name.GetString();
			    if (gdal_driver_name.empty()) {
				    throw InvalidInputException("Driver name must be specified");
			    }

			    if (!GDALDatasetFactory::WriteFile(dataset, raw_file_name, gdal_driver_name)) {
				    auto error = Raster::GetLastErrorMsg();
				    if (error.length()) {
					    throw IOException("Could not save file: " + raw_file_name + " (" + error + ")");
				    }
				    return false;
			    }
			    return true;
		    });
	}

	static void RasterAsFile_02(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 4);

		using POINTER_TYPE = PrimitiveType<uintptr_t>;
		using STRING_TYPE = PrimitiveType<string_t>;
		using LIST_TYPE = PrimitiveType<list_entry_t>;
		using BOOL_TYPE = PrimitiveType<bool>;

		auto &p1 = args.data[0];
		auto &p2 = args.data[1];
		auto &p3 = args.data[2];
		auto &p4 = args.data[3];
		auto &p4_entry = ListVector::GetEntry(p4);

		GenericExecutor::ExecuteQuaternary<POINTER_TYPE, STRING_TYPE, STRING_TYPE, LIST_TYPE, BOOL_TYPE>(
		    p1, p2, p3, p4, result, args.size(),
		    [&](POINTER_TYPE p1, STRING_TYPE p2, STRING_TYPE p3, LIST_TYPE p4_offlen) {
			    auto input = p1.val;
			    auto file_name = p2.val;
			    auto driver_name = p3.val;
			    auto offlen = p4_offlen.val;

			    GDALThreadSafeDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input);
			    auto raw_file_name = file_name.GetString();

			    auto gdal_driver_name = driver_name.GetString();
			    if (gdal_driver_name.empty()) {
				    throw InvalidInputException("Driver name must be specified");
			    }

			    auto options = std::vector<std::string>();

			    for (idx_t i = offlen.offset; i < offlen.offset + offlen.length; i++) {
				    const auto &child_value = p4_entry.GetValue(i);
				    const auto option = child_value.ToString();
				    options.emplace_back(option);
			    }

			    if (!GDALDatasetFactory::WriteFile(dataset, raw_file_name, gdal_driver_name, options)) {
				    auto error = Raster::GetLastErrorMsg();
				    if (error.length()) {
					    throw IOException("Could not save file: " + raw_file_name + " (" + error + ")");
				    }
				    return false;
			    }
			    return true;
		    });
	}

	static void Register(DatabaseInstance &db) {

		FunctionBuilder::RegisterScalar(db, "RT_RasterFromFile", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("path", LogicalType::VARCHAR);
				variant.SetReturnType(RasterTypes::RASTER());
				variant.SetFunction(RasterFromFile);
			});
			func.SetDescription(R"(
				Loads a raster from a file path.
			)");
			func.SetExample(R"(
				WITH __input AS (
					SELECT
						RT_RasterFromFile(file) AS raster
					FROM
						glob('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				)
				SELECT
					RT_RasterAsFile(raster, './test/data/rasterasfile.tiff', 'Gtiff') AS result
				FROM
					__input
				;
				┌─────────┐
				│ result  │
				│ boolean │
				├─────────┤
				│ true    │
				└─────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "serialization");
		});

		FunctionBuilder::RegisterScalar(db, "RT_RasterAsFile", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("file_name", LogicalType::VARCHAR);
				variant.AddParameter("driver_name", LogicalType::VARCHAR);
				variant.SetReturnType(LogicalType::BOOLEAN);
				variant.SetFunction(RasterAsFile_01);
			});
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("file_name", LogicalType::VARCHAR);
				variant.AddParameter("driver_name", LogicalType::VARCHAR);
				variant.AddParameter("write_options", LogicalType::LIST(LogicalType::VARCHAR));
				variant.SetReturnType(LogicalType::BOOLEAN);
				variant.SetFunction(RasterAsFile_02);
			});
			func.SetDescription(R"(
				Writes a raster to a file path.
				`write_options` is optional, an array of parameters for the GDAL driver specified.
			)");
			func.SetExample(R"(
				WITH __input AS (
					SELECT
						RT_RasterFromFile(file) AS raster
					FROM
						glob('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				)
				SELECT
					RT_RasterAsFile(raster, './test/data/rasterasfile.tiff', 'Gtiff') AS result
				FROM
					__input
				;
				┌─────────┐
				│ result  │
				│ boolean │
				├─────────┤
				│ true    │
				└─────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "serialization");
		});
	}
};

//======================================================================================================================
// RT_Blob
//======================================================================================================================

struct RT_Blob {

	static void RasterFromBlob_00(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 1);

		auto &context = state.GetContext();
		auto &ctx_state = GDALClientContextState::GetOrCreate(context);

		UnaryExecutor::Execute<string_t, uintptr_t>(args.data[0], result, args.size(), [&](string_t mblob) {
			auto blob_ptr = mblob.GetData();
			auto blob_size = mblob.GetSize();

			std::vector<std::string> allowed_drivers = {"GTiff"};

			GDALThreadSafeDataset *dataset = GDALDatasetFactory::FromBlob(blob_ptr, blob_size, allowed_drivers);
			if (dataset == nullptr) {
				auto error = Raster::GetLastErrorMsg();
				throw IOException("Could not open file from a Blob (" + error + ")");
			}

			ctx_state.GetDatasetRegistry().RegisterDataset(dataset);
			return CastPointerToValue(dataset);
		});
	}

	static void RasterFromBlob_01(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 2);

		auto &context = state.GetContext();
		auto &ctx_state = GDALClientContextState::GetOrCreate(context);

		BinaryExecutor::Execute<string_t, string_t, uintptr_t>(
		    args.data[0], args.data[1], result, args.size(), [&](string_t mblob, string_t driver_name) {
			    auto blob_ptr = mblob.GetData();
			    auto blob_size = mblob.GetSize();

			    auto gdal_driver_name = driver_name.GetString();
			    if (gdal_driver_name.empty()) {
				    throw InvalidInputException("Driver name must be specified");
			    }

			    std::vector<std::string> allowed_drivers = {gdal_driver_name};

			    GDALThreadSafeDataset *dataset = GDALDatasetFactory::FromBlob(blob_ptr, blob_size, allowed_drivers);
			    if (dataset == nullptr) {
				    auto error = Raster::GetLastErrorMsg();
				    throw IOException("Could not open file from a Blob (" + error + ")");
			    }

			    ctx_state.GetDatasetRegistry().RegisterDataset(dataset);
			    return CastPointerToValue(dataset);
		    });
	}

	static void RasterFromBlob_02(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 2);

		auto &context = state.GetContext();
		auto &ctx_state = GDALClientContextState::GetOrCreate(context);

		using BLOB_TYPE = PrimitiveType<string_t>;
		using LIST_TYPE = PrimitiveType<list_entry_t>;
		using POINTER_TYPE = PrimitiveType<uintptr_t>;

		auto &p1 = args.data[0];
		auto &p2 = args.data[1];
		auto &p2_entry = ListVector::GetEntry(p2);

		GenericExecutor::ExecuteBinary<BLOB_TYPE, LIST_TYPE, POINTER_TYPE>(
		    p1, p2, result, args.size(), [&](BLOB_TYPE p1, LIST_TYPE p2_offlen) {
			    auto blob_ptr = p1.val.GetData();
			    auto blob_size = p1.val.GetSize();
			    auto offlen = p2_offlen.val;

			    auto allowed_drivers = std::vector<std::string>();

			    for (idx_t i = offlen.offset; i < offlen.offset + offlen.length; i++) {
				    const auto &child_value = p2_entry.GetValue(i);
				    const auto option = child_value.ToString();
				    allowed_drivers.emplace_back(option);
			    }

			    if (allowed_drivers.empty()) {
				    throw InvalidInputException("Driver name[s] must be specified");
			    }

			    GDALThreadSafeDataset *dataset = GDALDatasetFactory::FromBlob(blob_ptr, blob_size, allowed_drivers);
			    if (dataset == nullptr) {
				    auto error = Raster::GetLastErrorMsg();
				    throw IOException("Could not open file from a Blob (" + error + ")");
			    }

			    ctx_state.GetDatasetRegistry().RegisterDataset(dataset);
			    return CastPointerToValue(dataset);
		    });
	}

	static void RasterAsBlob_00(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 1);

		UnaryExecutor::Execute<uintptr_t, string_t>(args.data[0], result, args.size(), [&](uintptr_t input) {
			GDALThreadSafeDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input);

			uint64_t blob_size = 0;
			const char *blob_ptr = GDALDatasetFactory::WriteBlob(dataset, "GTiff", blob_size);

			if (!blob_ptr) {
				auto error = Raster::GetLastErrorMsg();
				if (error.length()) {
					throw IOException("Could not save raster as a Blob (" + error + ")");
				}
				return string_t();
			}

			return string_t(blob_ptr, static_cast<uint32_t>(blob_size));
		});
	}

	static void RasterAsBlob_01(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 2);

		BinaryExecutor::Execute<uintptr_t, string_t, string_t>(
		    args.data[0], args.data[1], result, args.size(), [&](uintptr_t input, string_t driver_name) {
			    GDALThreadSafeDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input);

			    auto gdal_driver_name = driver_name.GetString();
			    if (gdal_driver_name.empty()) {
				    throw InvalidInputException("Driver name must be specified");
			    }

			    uint64_t blob_size = 0;
			    const char *blob_ptr = GDALDatasetFactory::WriteBlob(dataset, gdal_driver_name, blob_size);

			    if (!blob_ptr) {
				    auto error = Raster::GetLastErrorMsg();
				    if (error.length()) {
					    throw IOException("Could not save raster as a Blob (" + error + ")");
				    }
				    return string_t();
			    }

			    return string_t(blob_ptr, static_cast<uint32_t>(blob_size));
		    });
	}

	static void RasterAsBlob_02(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 3);

		using POINTER_TYPE = PrimitiveType<uintptr_t>;
		using STRING_TYPE = PrimitiveType<string_t>;
		using LIST_TYPE = PrimitiveType<list_entry_t>;
		using BLOB_TYPE = PrimitiveType<string_t>;

		auto &p1 = args.data[0];
		auto &p2 = args.data[1];
		auto &p3 = args.data[2];
		auto &p3_entry = ListVector::GetEntry(p3);

		GenericExecutor::ExecuteTernary<POINTER_TYPE, STRING_TYPE, LIST_TYPE, BLOB_TYPE>(
		    p1, p2, p3, result, args.size(), [&](POINTER_TYPE p1, STRING_TYPE p2, LIST_TYPE p3_offlen) {
			    auto input = p1.val;
			    auto driver_name = p2.val;
			    auto offlen = p3_offlen.val;

			    GDALThreadSafeDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input);

			    auto gdal_driver_name = driver_name.GetString();
			    if (gdal_driver_name.empty()) {
				    throw InvalidInputException("Driver name must be specified");
			    }

			    auto options = std::vector<std::string>();

			    for (idx_t i = offlen.offset; i < offlen.offset + offlen.length; i++) {
				    const auto &child_value = p3_entry.GetValue(i);
				    const auto option = child_value.ToString();
				    options.emplace_back(option);
			    }

			    uint64_t blob_size = 0;
			    const char *blob_ptr = GDALDatasetFactory::WriteBlob(dataset, gdal_driver_name, blob_size, options);

			    if (!blob_ptr) {
				    auto error = Raster::GetLastErrorMsg();
				    if (error.length()) {
					    throw IOException("Could not save raster as a Blob (" + error + ")");
				    }
				    return string_t();
			    }

			    return string_t(blob_ptr, static_cast<uint32_t>(blob_size));
		    });
	}

	static void Register(DatabaseInstance &db) {

		FunctionBuilder::RegisterScalar(db, "RT_RasterFromBlob", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("blob", LogicalType::BLOB);
				variant.SetReturnType(RasterTypes::RASTER());
				variant.SetFunction(RasterFromBlob_00);
			});
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("blob", LogicalType::BLOB);
				variant.AddParameter("driver_name", LogicalType::VARCHAR);
				variant.SetReturnType(RasterTypes::RASTER());
				variant.SetFunction(RasterFromBlob_01);
			});
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("blob", LogicalType::BLOB);
				variant.AddParameter("driver_names", LogicalType::LIST(LogicalType::VARCHAR));
				variant.SetReturnType(RasterTypes::RASTER());
				variant.SetFunction(RasterFromBlob_02);
			});
			func.SetDescription(R"(
				Loads a raster from a blob.
				`driver_name` is optional, 'GTiff' format by default.
			)");
			func.SetExample(R"(
				WITH __input AS (
					SELECT
						RT_RasterFromFile(file) AS raster
					FROM
						glob('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				),
				__blob AS (
					SELECT
						RT_RasterAsBlob(raster, 'COG') AS result
					FROM
						__input
				)
				SELECT
					RT_GetGeometry(RT_RasterFromBlob(result, ['COG', 'GTiff']))
				FROM
					__blob
				;
				┌────────────────────────────────────────────────────────────────────────────────────────────┐
				│         rt_getgeometry(rt_rasterfromblob(result, main.list_value('COG', 'GTiff')))         │
				│                                          geometry                                          │
				├────────────────────────────────────────────────────────────────────────────────────────────┤
				│ POLYGON ((541020 4690200, 541020 4796640, 609780 4796640, 609780 4690200, 541020 4690200)) │
				└────────────────────────────────────────────────────────────────────────────────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "serialization");
		});

		FunctionBuilder::RegisterScalar(db, "RT_RasterAsBlob", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::BLOB);
				variant.SetFunction(RasterAsBlob_00);
			});
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("driver_name", LogicalType::VARCHAR);
				variant.SetReturnType(LogicalType::BLOB);
				variant.SetFunction(RasterAsBlob_01);
			});
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("driver_name", LogicalType::VARCHAR);
				variant.AddParameter("write_options", LogicalType::LIST(LogicalType::VARCHAR));
				variant.SetReturnType(LogicalType::BLOB);
				variant.SetFunction(RasterAsBlob_02);
			});
			func.SetDescription(R"(
				Writes a raster to a blob.
				`driver_name` is optional, 'GTiff' format by default.
				`write_options` is optional, an array of parameters for the GDAL driver specified.
			)");
			func.SetExample(R"(
				WITH __input AS (
					SELECT
						RT_RasterFromFile(file) AS raster
					FROM
						glob('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				)
				SELECT
					RT_RasterAsBlob(raster, 'Gtiff') AS result
				FROM
					__input
				;
				┌───────────────────────────────────────────────┐
				│                   result                      │
				│                    blob                       │
				├───────────────────────────────────────────────┤
				│ II*\x00\xC0\x00\x00\x00GDAL_STRUCTURAL…       │
				└───────────────────────────────────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "serialization");
		});
	}
};

} // namespace

// ######################################################################################################################
//  Register File functions
// ######################################################################################################################

void RasterFileFunctions::Register(DatabaseInstance &db) {
	RT_File::Register(db);
	RT_Blob::Register(db);
}

} // namespace duckdb
