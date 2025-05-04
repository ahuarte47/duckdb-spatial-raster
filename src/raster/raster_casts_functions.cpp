#include "raster_types.hpp"
#include "raster_casts_functions.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"

namespace duckdb {

namespace {

//======================================================================================================================
// RASTER Casts
//======================================================================================================================

struct RasterCasts {

	//------------------------------------------------------------------------------------------------------------------
	// RASTER -> VARCHAR
	//------------------------------------------------------------------------------------------------------------------

	static bool RasterToVarcharCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters) {
		UnaryExecutor::Execute<uintptr_t, string_t>(source, result, count,
		                                            [&](uintptr_t &input) { return string_t("RASTER"); });
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------
	// Register
	//------------------------------------------------------------------------------------------------------------------

	static void Register(DatabaseInstance &db) {
		// RASTER -> VARCHAR
		ExtensionUtil::RegisterCastFunction(db, RasterTypes::RASTER(), LogicalType::VARCHAR, RasterToVarcharCast, 1);
	}
};

} // namespace

// ######################################################################################################################
//  Register
// ######################################################################################################################

void GdalRasterCastsFunctions::Register(DatabaseInstance &db) {
	RasterCasts::Register(db);
}

} // namespace duckdb
