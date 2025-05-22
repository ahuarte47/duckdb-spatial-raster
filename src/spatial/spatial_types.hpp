#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/vector.hpp"

namespace duckdb {

class DatabaseInstance;
struct LogicalType;

struct GeoTypes {
	static LogicalType POINT_2D();

	static void Register(DatabaseInstance &db);
};

//! Location of a XY coordinate in geographic coordinates
struct Point2D {
	double x;
	double y;
	explicit Point2D(double x, double y) : x(x), y(y) {
	}

	// Approximate equality
	bool operator==(const Point2D &other) const {
		return std::abs(x - other.x) < 1e-6 && std::abs(y - other.y) < 1e-6;
	}
};

} // namespace duckdb
