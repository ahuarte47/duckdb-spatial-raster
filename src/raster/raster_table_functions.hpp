#pragma once

namespace duckdb {

class DatabaseInstance;

struct GdalRasterTableFunctions {
public:
	static void Register(DatabaseInstance &db);
};

} // namespace duckdb
