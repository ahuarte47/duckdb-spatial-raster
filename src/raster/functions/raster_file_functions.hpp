#pragma once

namespace duckdb {

class ExtensionLoader;

struct RasterFileFunctions {
public:
	static void Register(ExtensionLoader &loader);
};

} // namespace duckdb
