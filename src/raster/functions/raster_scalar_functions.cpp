#include "raster_scalar_functions.hpp"
#include "../raster.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"
// Spatial
#include "spatial/util/function_builder.hpp"
// GDAL
#include "gdal_priv.h"

namespace duckdb {

namespace {

//======================================================================================================================
// RT_Srid
//======================================================================================================================

struct RT_Srid {
	static void GetSridFunction(DataChunk &args, ExpressionState &state, Vector &result) {

		UnaryExecutor::Execute<uintptr_t, int32_t>(args.data[0], result, args.size(), [&](uintptr_t input) {
			Raster raster(reinterpret_cast<GDALDataset *>(input));
			return raster.GetSrid();
		});
	}

	static void Register(DatabaseInstance &db) {

		FunctionBuilder::RegisterScalar(db, "RT_Srid", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::INTEGER);
				variant.SetFunction(GetSridFunction);
			});
			func.SetDescription(R"(
				Returns the spatial reference identifier (EPSG code) of the raster.
                Refer to [EPSG](https://spatialreference.org/ref/epsg/) for more details.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});
	}
};

} // namespace

// ######################################################################################################################
//  Register Scalar functions
// ######################################################################################################################

void RasterScalarFunctions::Register(DatabaseInstance &db) {
	RT_Srid::Register(db);
}

} // namespace duckdb
