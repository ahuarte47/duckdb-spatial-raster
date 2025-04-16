#pragma once

namespace duckdb {

class DatabaseInstance;

struct GdalModule {
public:
	static void Register(DatabaseInstance &db);
};

} // namespace duckdb
