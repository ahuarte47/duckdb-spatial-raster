#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/vector.hpp"
#include "spatial_r/spatial_types.hpp"

namespace duckdb {

class ExtensionLoader;
struct LogicalType;

struct RasterTypes {
	static LogicalType RASTER();
	static LogicalType RASTER_COORD();

	static void Register(ExtensionLoader &loader);
};

//! Position of a cell in a Raster (upper left corner as column and row)
struct RasterCoord {
	int32_t col;
	int32_t row;
	explicit RasterCoord(int32_t col, int32_t row) : col(col), row(row) {
	}
};

} // namespace duckdb
