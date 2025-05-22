#pragma once

namespace duckdb {

class DatabaseInstance;

struct SpatialCastsFunctions {
public:
	static void Register(DatabaseInstance &db);
};

} // namespace duckdb
