#include "gdal_priv.h"
#include "raster.hpp"

namespace duckdb {

std::string Raster::GetLastErrorMsg() {
	return std::string(CPLGetLastErrorMsg());
}

} // namespace duckdb
