#include "raster_file_functions.hpp"
#include "../raster.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/common/vector_operations/generic_executor.hpp"
// Spatial
#include "spatial/util/function_builder.hpp"
// GDAL
#include "gdal_priv.h"
#include "../modules/gdal/gdal_dataset_factory.hpp"
#include "../modules/gdal/gdal_context_state.hpp"

namespace duckdb {

namespace {

//======================================================================================================================
// RT_File
//======================================================================================================================

struct RT_File {

	static void RasterFromFile(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &context = state.GetContext();

		UnaryExecutor::Execute<string_t, uintptr_t>(args.data[0], result, args.size(), [&](string_t input) {
			auto &ctx_state = GDALClientContextState::GetOrCreate(context);
			auto raw_file_name = input.GetString();

			GDALDataset *dataset = GDALDatasetFactory::FromFile(raw_file_name);
			if (dataset == nullptr) {
				auto error = Raster::GetLastErrorMsg();
				throw IOException("Could not open file: " + raw_file_name + " (" + error + ")");
			}

			ctx_state.GetDatasetRegistry(context).RegisterDataset(dataset);
			return CastPointerToValue(dataset);
		});
	}

	static void RasterAsFile_01(DataChunk &args, ExpressionState &state, Vector &result) {

		TernaryExecutor::Execute<uintptr_t, string_t, string_t, bool>(
		    args.data[0], args.data[1], args.data[2], result, args.size(),
		    [&](uintptr_t input, string_t file_name, string_t driver_name) {
			    GDALDataset *dataset = reinterpret_cast<GDALDataset *>(input);
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

			    GDALDataset *dataset = reinterpret_cast<GDALDataset *>(input);
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
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
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
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});
	}
};

} // namespace

// ######################################################################################################################
//  Register File functions
// ######################################################################################################################

void RasterFileFunctions::Register(DatabaseInstance &db) {
	RT_File::Register(db);
}

} // namespace duckdb
