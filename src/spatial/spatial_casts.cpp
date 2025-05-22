#include "spatial_types.hpp"
#include "spatial_casts.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/common/vector_operations/generic_executor.hpp"

namespace duckdb {

namespace {

//======================================================================================================================
// Geometry Casts
//======================================================================================================================

struct GeometryCasts {

	///-----------------------------------------------------------------------------------------------------------------
	// POINT_2D -> VARCHAR
	//------------------------------------------------------------------------------------------------------------------

	static bool PointToVarcharCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters) {

		using POINT_TYPE = StructTypeBinary<double_t, double_t>;
		using VARCHAR_TYPE = PrimitiveType<string_t>;

		GenericExecutor::ExecuteUnary<POINT_TYPE, VARCHAR_TYPE>(source, result, count, [&](POINT_TYPE &input) {
			auto x = input.a_val;
			auto y = input.b_val;
			return StringVector::AddString(result, StringUtil::Format("POINT (%.f %.f)", x, y));
		});
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------
	// Register
	//------------------------------------------------------------------------------------------------------------------

	static void Register(DatabaseInstance &db) {

		// POINT_2D -> VARCHAR
		ExtensionUtil::RegisterCastFunction(db, GeoTypes::POINT_2D(), LogicalType::VARCHAR, PointToVarcharCast, 1);
	}
};

} // namespace

// ######################################################################################################################
//  Register
// ######################################################################################################################

void SpatialCastsFunctions::Register(DatabaseInstance &db) {
	GeometryCasts::Register(db);
}

} // namespace duckdb
