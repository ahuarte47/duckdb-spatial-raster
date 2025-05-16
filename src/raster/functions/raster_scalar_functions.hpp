#pragma once

namespace duckdb {

class DatabaseInstance;

struct RasterScalarFunctions {
public:
	static void Register(DatabaseInstance &db);
};

} // namespace duckdb
