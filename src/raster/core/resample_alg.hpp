#pragma once

#include "duckdb/common/string.hpp"

namespace duckdb {

//! Supported Warp Resampling Algorithm (GDALResampleAlg).
typedef enum {
	NearestNeighbour = 0, /**< Nearest neighbour (select on one input pixel) */
	Bilinear = 1,         /**< Bilinear (2x2 kernel) */
	Cubic = 2,            /**< Cubic Convolution Approximation (4x4 kernel) */
	CubicSpline = 3,      /**< Cubic B-Spline Approximation (4x4 kernel) */
	Lanczos = 4,          /**< Lanczos windowed sinc interpolation (6x6 kernel) */
	Average = 5,          /**< Average (computes the weighted average of all non-NODATA contributing pixels) */
	Mode = 6,             /**< Mode (selects the value which appears most often of all the sampled points) */
	Max = 8,              /**< Max (selects maximum of all non-NODATA contributing pixels) */
	Min = 9,              /**< Min (selects minimum of all non-NODATA contributing pixels) */
	Med = 10,             /**< Med (selects median of all non-NODATA contributing pixels) */
	Q1 = 11,              /**< Q1 (selects first quartile of all non-NODATA contributing pixels) */
	Q3 = 12,              /**< Q3 (selects third quartile of all non-NODATA contributing pixels) */
	Sum = 13,             /**< Sum (weighed sum of all non-NODATA contributing pixels) */
	RMS = 14,             /**< RMS (weighted root mean square (quadratic mean) of all non-NODATA contributing pixels) */
} ResampleAlg;

struct ResampleAlgs {

	//! Returns the name of given ResampleAlg
	static std::string GetResampleAlgName(const ResampleAlg &resample_alg);
};

} // namespace duckdb
