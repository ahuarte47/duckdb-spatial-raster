#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/vector.hpp"

namespace duckdb {

class DatabaseInstance;
struct LogicalType;

struct RasterTypes {
	static LogicalType RASTER();
	static LogicalType RASTER_XY();
	static LogicalType RASTER_COORD();

	static void Register(DatabaseInstance &db);
};

//! Location of a XY coordinate in geographic coordinates
struct PointXY {
	double x;
	double y;
	explicit PointXY(double x, double y) : x(x), y(y) {
	}

	// Approximate equality
	bool operator==(const PointXY &other) const {
		return std::abs(x - other.x) < 1e-6 && std::abs(y - other.y) < 1e-6;
	}
};

//! Position of a cell in a Raster (upper left corner as column and row)
struct RasterCoord {
	int32_t col;
	int32_t row;
	explicit RasterCoord(int32_t col, int32_t row) : col(col), row(row) {
	}
};

} // namespace duckdb
