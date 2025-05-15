#pragma once

namespace duckdb {

class DatabaseInstance;

struct RasterTableFunctions {
public:
	static void Register(DatabaseInstance &db);
};

} // namespace duckdb
