#include "raster_types.hpp"
#include "duckdb/main/extension_util.hpp"

namespace duckdb {

LogicalType RasterTypes::RASTER() {
	auto type = LogicalType(LogicalTypeId::POINTER);
	type.SetAlias("RASTER");
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

	// RASTER_COORD
	ExtensionUtil::RegisterType(db, "RASTER_COORD", RasterTypes::RASTER_COORD());
}

} // namespace duckdb
