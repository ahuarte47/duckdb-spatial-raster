#pragma once

#include "gdal_priv.h"

namespace duckdb {

//! A registry of Rasters (GDALDatasets) where items are released.
//! This takes ownership of items registered.
class GDALDatasetRegistry {
public:
	//! Constructor
	GDALDatasetRegistry();
	//! Destructor
	~GDALDatasetRegistry();

	//! Register a GDALDataset
	void RegisterDataset(GDALDataset *dataset);

private:
	std::vector<GDALDatasetUniquePtr> datasets_;
};

} // namespace duckdb
