#include "raster_types.hpp"
#include "duckdb/main/extension/extension_loader.hpp"

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

void RasterTypes::Register(ExtensionLoader &loader) {

	// RASTER
	loader.RegisterType("RASTER", RasterTypes::RASTER());

	// RASTER_COORD
	loader.RegisterType("RASTER_COORD", RasterTypes::RASTER_COORD());
}

} // namespace duckdb
