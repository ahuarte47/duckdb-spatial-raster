#include "raster_types.hpp"
#include "raster_casts.hpp"
#include "raster.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/common/vector_operations/generic_executor.hpp"
// Spatial
#include "spatial_r/spatial_types.hpp"
#include "spatial/geometry/geometry_serialization.hpp"
#include "spatial/geometry/sgl.hpp"
// GDAL
#include "gdal_priv.h"

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
	// RASTER -> GEOMETRY
	//------------------------------------------------------------------------------------------------------------------

	static bool RasterToGeometryCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters) {

		UnaryExecutor::Execute<uintptr_t, string_t>(source, result, count, [&](uintptr_t &input) {
			Raster raster(reinterpret_cast<GDALThreadSafeDataset *>(input));
			Boundary2D boundary = raster.GetGeometry();

			// We can create the geometry polygon directly on the stack.
			double buffer[10];
			double *buffer_p = buffer;
			for (const auto &point : boundary.points) {
				*buffer_p++ = point.x;
				*buffer_p++ = point.y;
			}
			sgl::geometry ring(sgl::geometry_type::LINESTRING, false, false);
			ring.set_vertex_array(buffer, 5);
			sgl::geometry polygon(sgl::geometry_type::POLYGON, false, false);
			polygon.append_part(&ring);

			// Serialize the geometry into a blob
			const auto size = Serde::GetRequiredSize(polygon);
			auto blob = StringVector::EmptyString(result, size);
			Serde::Serialize(polygon, blob.GetDataWriteable(), size);
			blob.Finalize();
			return blob;
		});
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

		// RASTER -> GEOMETRY
		ExtensionUtil::RegisterCastFunction(db, RasterTypes::RASTER(), GeoTypes::GEOMETRY(), RasterToGeometryCast, 1);

		// RASTER -> POINTER is implicitly castable
		ExtensionUtil::RegisterCastFunction(db, RasterTypes::RASTER(), LogicalType::POINTER,
		                                    DefaultCasts::ReinterpretCast, 1);

		// POINTER -> RASTER is implicitly castable
		ExtensionUtil::RegisterCastFunction(db, LogicalType::POINTER, RasterTypes::RASTER(),
		                                    DefaultCasts::ReinterpretCast, 1);

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
