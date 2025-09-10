#pragma once

namespace duckdb {

class ExtensionLoader;

struct RasterScalarFunctions {
public:
	static void Register(ExtensionLoader &loader);
};

} // namespace duckdb
