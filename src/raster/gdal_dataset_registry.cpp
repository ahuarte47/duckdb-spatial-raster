#include "gdal_dataset_registry.hpp"

namespace duckdb {

GDALDatasetRegistry::GDALDatasetRegistry() {
}

GDALDatasetRegistry::~GDALDatasetRegistry() {

	// Release items in reverse order, first children, then parent ones
	for (auto it = datasets_.rbegin(); it != datasets_.rend(); ++it) {
		auto datasetUniquePtr = std::move(*it);
		datasetUniquePtr.reset();
	}
}

void GDALDatasetRegistry::RegisterDataset(GDALDataset *dataset) {
	datasets_.emplace_back(GDALDatasetUniquePtr(dataset));
}

} // namespace duckdb
