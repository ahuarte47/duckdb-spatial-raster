#pragma once

namespace duckdb {

class DatabaseInstance;

struct RasterAggregateFunctions {
public:
	static void Register(DatabaseInstance &db);
};

} // namespace duckdb
