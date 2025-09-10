#pragma once

namespace duckdb {

class ExtensionLoader;

struct RasterBandFunctions {
public:
	static void Register(ExtensionLoader &loader);
};

} // namespace duckdb
