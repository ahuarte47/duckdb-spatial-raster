#pragma once

namespace duckdb {

class DatabaseInstance;

struct RasterBandFunctions {
public:
	static void Register(DatabaseInstance &db);
};

} // namespace duckdb
