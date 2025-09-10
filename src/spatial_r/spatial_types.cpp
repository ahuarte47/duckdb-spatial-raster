#include "spatial_types.hpp"
#include "duckdb/main/extension/extension_loader.hpp"

namespace duckdb {

LogicalType SpatialTypes::POINT_2D() {
	auto type = LogicalType::STRUCT({{"x", LogicalType::DOUBLE}, {"y", LogicalType::DOUBLE}});
	type.SetAlias("POINT_2D");
	return type;
}

LogicalType SpatialTypes::GEOMETRY() {
	auto blob_type = LogicalType(LogicalTypeId::BLOB);
	blob_type.SetAlias("GEOMETRY");
	return blob_type;
}

LogicalType SpatialTypes::WKB_BLOB() {
	auto blob_type = LogicalType(LogicalTypeId::BLOB);
	blob_type.SetAlias("WKB_BLOB");
	return blob_type;
}

void SpatialTypes::Register(ExtensionLoader &loader) {

	// POINT_2D
	loader.RegisterType("RPOINT_2D", SpatialTypes::POINT_2D());

	// GEOMETRY
	loader.RegisterType("RGEOMETRY", SpatialTypes::GEOMETRY());

	// WKB_BLOB
	loader.RegisterType("RWKB_BLOB", SpatialTypes::WKB_BLOB());
}

} // namespace duckdb
