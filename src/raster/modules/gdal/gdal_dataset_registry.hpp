#pragma once

#include "gdal_priv.h"
#include <mutex>

namespace duckdb {

class GDALThreadSafeDataset;

//! A registry of Rasters (GDALDatasets) where items are released.
//! This takes ownership of items registered.
class GDALDatasetRegistry {
public:
	//! Constructor
	GDALDatasetRegistry();
	//! Destructor
	~GDALDatasetRegistry();

	//! Register a GDALDataset
	void RegisterDataset(GDALThreadSafeDataset *dataset);

private:
	std::vector<GDALThreadSafeDataset *> datasets_;
	std::mutex lock_;
};

} // namespace duckdb
