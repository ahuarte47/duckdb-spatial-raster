#include "raster_value.hpp"
#include "raster_types.hpp"
#include "modules/gdal/gdal_dataset_ts.hpp"

namespace duckdb {

Value RasterValue::CreateValue(GDALThreadSafeDataset *dataset) {
	Value value = Value::POINTER(CastPointerToValue(dataset));
	value.Reinterpret(RasterTypes::RASTER());
	return value;
}

GDALThreadSafeDataset *RasterValue::operator->() const {
	GDALThreadSafeDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(GetValueUnsafe<uint64_t>());
	return dataset;
}

GDALThreadSafeDataset *RasterValue::get() const {
	GDALThreadSafeDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(GetValueUnsafe<uint64_t>());
	return dataset;
}

} // namespace duckdb
