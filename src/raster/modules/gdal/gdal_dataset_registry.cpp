#include "gdal_dataset_registry.hpp"
#include "gdal_dataset_ts.hpp"

namespace duckdb {

GDALDatasetRegistry::GDALDatasetRegistry() {
}

GDALDatasetRegistry::~GDALDatasetRegistry() {

	// Release items in reverse order, first children, then parent ones
	for (auto it = datasets_.rbegin(); it != datasets_.rend(); ++it) {
		auto dataset = std::move(*it);
		delete dataset;
	}
}

void GDALDatasetRegistry::RegisterDataset(GDALThreadSafeDataset *dataset) {
	std::lock_guard<std::mutex> guard(lock_);
	datasets_.emplace_back(dataset);
}

} // namespace duckdb
