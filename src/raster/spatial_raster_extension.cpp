#define DUCKDB_EXTENSION_MAIN

#include "spatial_raster_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

#include "gdal_module.hpp"
#include "raster_types.hpp"
#include "raster_table_functions.hpp"
#include "raster_casts_functions.hpp"

namespace duckdb {

static void LoadInternal(DatabaseInstance &instance) {

	// Register the GDAL module for RASTER
	GdalModule::Register(instance);

	// Register the types
	RasterTypes::Register(instance);

	// Register the Table functions
	GdalRasterTableFunctions::Register(instance);

	// Register the Casts functions
	GdalRasterCastsFunctions::Register(instance);
}

void SpatialRasterExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string SpatialRasterExtension::Name() {
	return "spatial_raster";
}

std::string SpatialRasterExtension::Version() const {
#ifdef EXT_VERSION_SPATIAL_RASTER
	return EXT_VERSION_SPATIAL_RASTER;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void spatial_raster_init(duckdb::DatabaseInstance &db) {
	duckdb::DuckDB db_wrapper(db);
	db_wrapper.LoadExtension<duckdb::SpatialRasterExtension>();
}

DUCKDB_EXTENSION_API const char *spatial_raster_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
