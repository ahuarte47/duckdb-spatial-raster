#include "pixel_type.hpp"

namespace duckdb {

std::string PixelTypes::GetPixelTypeName(const PixelType &pixel_type) {
	switch (pixel_type) {
	case Byte:
		return "Byte";
	case Int8:
		return "Int8";
	case UInt16:
		return "UInt16";
	case Int16:
		return "Int16";
	case UInt32:
		return "UInt32";
	case Int32:
		return "Int32";
	case UInt64:
		return "UInt64";
	case Int64:
		return "Int64";
	case Float32:
		return "Float32";
	case Float64:
		return "Float64";
	case CInt16:
		return "CInt16";
	case CInt32:
		return "CInt32";
	case CFloat32:
		return "CFloat32";
	case CFloat64:
		return "CFloat64";
	case TypeCount:
	case Unknown:
	default:
		return "Unknown";
	}
}

} // namespace duckdb
