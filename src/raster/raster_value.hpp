#pragma once

#include "duckdb/common/types/value.hpp"

class GDALDataset;

namespace duckdb {

//! This Value object holds a Raster (GDALDataset) instance
class RasterValue : public Value {
public:
	//! Returns the pointer to the dataset
	GDALDataset *operator->() const;
	//! Returns the pointer to the dataset
	GDALDataset *get() const;

	//! Create a RASTER value
	static Value CreateValue(GDALDataset *dataset);
};

} // namespace duckdb
