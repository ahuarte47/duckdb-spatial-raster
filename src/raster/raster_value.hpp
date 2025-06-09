#pragma once

#include "duckdb/common/types/value.hpp"

namespace duckdb {

class GDALThreadSafeDataset;

//! This Value object holds a Raster (GDALDataset) instance
class RasterValue : public Value {
public:
	//! Returns the pointer to the dataset
	GDALThreadSafeDataset *operator->() const;
	//! Returns the pointer to the dataset
	GDALThreadSafeDataset *get() const;

	//! Create a RASTER value
	static Value CreateValue(GDALThreadSafeDataset *dataset);
};

} // namespace duckdb
