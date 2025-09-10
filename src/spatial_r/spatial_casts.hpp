#pragma once

namespace duckdb {

class ExtensionLoader;

struct SpatialCastsFunctions {
public:
	static void Register(ExtensionLoader &loader);
};

} // namespace duckdb
