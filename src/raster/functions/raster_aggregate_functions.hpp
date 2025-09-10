#pragma once

namespace duckdb {

class ExtensionLoader;

struct RasterAggregateFunctions {
public:
	static void Register(ExtensionLoader &loader);
};

} // namespace duckdb
