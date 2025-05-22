#pragma once

#include "gdal_priv.h"
#include <mutex>

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
	std::mutex lock_;
};

} // namespace duckdb
