#include "raster_types.hpp"
#include "duckdb/main/extension_util.hpp"

namespace duckdb {

LogicalType RasterTypes::RASTER() {
	auto type = LogicalType(LogicalTypeId::POINTER);
	type.SetAlias("RASTER");
	return type;
}

LogicalType RasterTypes::RASTER_XY() {
	auto type = LogicalType::STRUCT({{"x", LogicalType::DOUBLE}, {"y", LogicalType::DOUBLE}});
	type.SetAlias("RASTER_XY");
	return type;
}

LogicalType RasterTypes::RASTER_COORD() {
	auto type = LogicalType::STRUCT({{"col", LogicalType::INTEGER}, {"row", LogicalType::INTEGER}});
	type.SetAlias("RASTER_COORD");
	return type;
}

void RasterTypes::Register(DatabaseInstance &db) {

	// RASTER
	ExtensionUtil::RegisterType(db, "RASTER", RasterTypes::RASTER());

	// RASTER_XY (POINT_2D)
	ExtensionUtil::RegisterType(db, "RASTER_XY", RasterTypes::RASTER_XY());

	// RASTER_COORD
	ExtensionUtil::RegisterType(db, "RASTER_COORD", RasterTypes::RASTER_COORD());
}

} // namespace duckdb
