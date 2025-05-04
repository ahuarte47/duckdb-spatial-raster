#include "raster_value.hpp"
#include "raster_types.hpp"
#include "gdal_priv.h"

namespace duckdb {

Value RasterValue::CreateValue(GDALDataset *dataset) {
	Value value = Value::POINTER(CastPointerToValue(dataset));
	value.Reinterpret(RasterTypes::RASTER());
	return value;
}

GDALDataset *RasterValue::operator->() const {
	GDALDataset *dataset = reinterpret_cast<GDALDataset *>(GetValueUnsafe<uint64_t>());
	return dataset;
}

GDALDataset *RasterValue::get() const {
	GDALDataset *dataset = reinterpret_cast<GDALDataset *>(GetValueUnsafe<uint64_t>());
	return dataset;
}

} // namespace duckdb
