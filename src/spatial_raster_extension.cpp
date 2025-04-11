#define DUCKDB_EXTENSION_MAIN

#include "spatial_raster_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

// OpenSSL linked through vcpkg
#include <openssl/opensslv.h>

namespace duckdb {

inline void SpatialRasterScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) {
			return StringVector::AddString(result, "SpatialRaster "+name.GetString()+" 🐥");
        });
}

inline void SpatialRasterOpenSSLVersionScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) {
			return StringVector::AddString(result, "SpatialRaster " + name.GetString() +
                                                     ", my linked OpenSSL version is " +
                                                     OPENSSL_VERSION_TEXT );
        });
}

static void LoadInternal(DatabaseInstance &instance) {
    // Register a scalar function
    auto spatial_raster_scalar_function = ScalarFunction("spatial_raster", {LogicalType::VARCHAR}, LogicalType::VARCHAR, SpatialRasterScalarFun);
    ExtensionUtil::RegisterFunction(instance, spatial_raster_scalar_function);

    // Register another scalar function
    auto spatial_raster_openssl_version_scalar_function = ScalarFunction("spatial_raster_openssl_version", {LogicalType::VARCHAR},
                                                LogicalType::VARCHAR, SpatialRasterOpenSSLVersionScalarFun);
    ExtensionUtil::RegisterFunction(instance, spatial_raster_openssl_version_scalar_function);
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
