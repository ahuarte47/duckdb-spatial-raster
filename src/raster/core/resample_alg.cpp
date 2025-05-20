#include "resample_alg.hpp"

namespace duckdb {

std::string ResampleAlgs::GetResampleAlgName(const ResampleAlg &resample_alg) {
	switch (resample_alg) {
	case NearestNeighbour:
		return "NearestNeighbour";
	case Bilinear:
		return "Bilinear";
	case Cubic:
		return "Cubic";
	case CubicSpline:
		return "CubicSpline";
	case Lanczos:
		return "Lanczos";
	case Average:
		return "Average";
	case Mode:
		return "Mode";
	case Max:
		return "Maximum";
	case Min:
		return "Minimun";
	case Med:
		return "Median";
	case Q1:
		return "Quartile1";
	case Q3:
		return "Quartile3";
	case Sum:
		return "Sum";
	case RMS:
		return "RootMeanSquare";
	default:
		return "Unknown";
	}
}

} // namespace duckdb
