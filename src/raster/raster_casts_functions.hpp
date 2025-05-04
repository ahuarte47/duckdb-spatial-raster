#pragma once

namespace duckdb {

class DatabaseInstance;

struct GdalRasterCastsFunctions {
public:
	static void Register(DatabaseInstance &db);
};

} // namespace duckdb
