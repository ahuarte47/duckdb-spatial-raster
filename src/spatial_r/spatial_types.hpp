#pragma once

#include "duckdb/common/string.hpp"
#include "duckdb/common/vector.hpp"
#include <array>

namespace duckdb {

class DatabaseInstance;
struct LogicalType;

struct SpatialTypes {
	static LogicalType POINT_2D();
	static LogicalType GEOMETRY();
	static LogicalType WKB_BLOB();

	static void Register(DatabaseInstance &db);
};

//! Location of a XY coordinate in geographic coordinates
struct Point2D {
	double x;
	double y;
	explicit Point2D(double x = 0.0, double y = 0.0) : x(x), y(y) {
	}

	// Approximate equality
	bool operator==(const Point2D &other) const {
		return std::abs(x - other.x) < 1e-6 && std::abs(y - other.y) < 1e-6;
	}
};

//! Minimal-Bounding-Box rectangle containing a geographic location
struct BBox2D {
	Point2D min;
	Point2D max;

	explicit BBox2D(const Point2D &min_p, const Point2D &max_p) : min(min_p), max(max_p) {
	}
};

//! Geometry polygon containing the limits of a geographic location
struct Boundary2D {
	std::array<Point2D, 5> points;

	explicit Boundary2D(const std::array<Point2D, 5> &pnts) {
		points = pnts;
	}
};

} // namespace duckdb
