#include "raster_types.hpp"
#include "raster_casts.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/common/vector_operations/generic_executor.hpp"

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
	// RASTER_COORD -> VARCHAR
	//------------------------------------------------------------------------------------------------------------------

	static bool CoordToVarcharCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters) {

		using COORD_TYPE = StructTypeBinary<int32_t, int32_t>;
		using VARCHAR_TYPE = PrimitiveType<string_t>;

		GenericExecutor::ExecuteUnary<COORD_TYPE, VARCHAR_TYPE>(source, result, count, [&](COORD_TYPE &input) {
			auto col = input.a_val;
			auto row = input.b_val;
			return StringVector::AddString(result, StringUtil::Format("COORD (%d, %d)", col, row));
		});
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------
	// Register
	//------------------------------------------------------------------------------------------------------------------

	static void Register(DatabaseInstance &db) {
		// RASTER -> VARCHAR
		ExtensionUtil::RegisterCastFunction(db, RasterTypes::RASTER(), LogicalType::VARCHAR, RasterToVarcharCast, 1);

		// RASTER_COORD -> VARCHAR
		ExtensionUtil::RegisterCastFunction(db, RasterTypes::RASTER_COORD(), LogicalType::VARCHAR, CoordToVarcharCast,
		                                    1);
	}
};

} // namespace

// ######################################################################################################################
//  Register
// ######################################################################################################################

void RasterCastsFunctions::Register(DatabaseInstance &db) {
	RasterCasts::Register(db);
}

} // namespace duckdb
