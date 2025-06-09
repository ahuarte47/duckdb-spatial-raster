
#include "gdal_dataset_ts.hpp"

// GDAL
#include "gdal_priv.h"

namespace duckdb {

GDALThreadSafeDataset::GDALThreadSafeDataset(GDALDataset *dataset) : dataset_(dataset) {
}

GDALThreadSafeDataset::~GDALThreadSafeDataset() {
	if (dataset_) {
		dataset_->Close();
		delete dataset_;
		dataset_ = nullptr;
	}
}

void GDALThreadSafeDataset::AcquireMutex() {
	lock_.lock();
}

void GDALThreadSafeDataset::ReleaseMutex() {
	lock_.unlock();
}

} // namespace duckdb
