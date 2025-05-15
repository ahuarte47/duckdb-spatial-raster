#pragma once

namespace duckdb {

class DatabaseInstance;

struct RasterCastsFunctions {
public:
	static void Register(DatabaseInstance &db);
};

} // namespace duckdb
