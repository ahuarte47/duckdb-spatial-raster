#pragma once

#include <string>

namespace duckdb {

//! A wrapper of a GDALDataset with useful methods to manage raster data.
//! Does not take ownership of the pointer.
class Raster {
public:
	//! Get the last error message.
	static std::string GetLastErrorMsg();
};

} // namespace duckdb
