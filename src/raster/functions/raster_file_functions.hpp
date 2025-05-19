#pragma once

namespace duckdb {

class DatabaseInstance;

struct RasterFileFunctions {
public:
	static void Register(DatabaseInstance &db);
};

} // namespace duckdb
