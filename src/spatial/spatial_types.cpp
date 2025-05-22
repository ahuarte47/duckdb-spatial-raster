#include "spatial_types.hpp"
#include "duckdb/main/extension_util.hpp"

namespace duckdb {

LogicalType GeoTypes::POINT_2D() {
	auto type = LogicalType::STRUCT({{"x", LogicalType::DOUBLE}, {"y", LogicalType::DOUBLE}});
	type.SetAlias("RPOINT_2D");
	return type;
}

void GeoTypes::Register(DatabaseInstance &db) {

	// POINT_2D
	ExtensionUtil::RegisterType(db, "RPOINT_2D", GeoTypes::POINT_2D());
}

} // namespace duckdb
