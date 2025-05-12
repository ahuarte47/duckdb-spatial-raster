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

#include "proj.h"
#include "sqlite3.h"

// We embed the whole proj.db in the proj_db.c file, which we then link into the extension binary
// We can then use the sqlite3 "memvfs" (which we also statically link to) to point to the proj.db database in memory
// To genereate the proj_db.c file, we use the following command:
// `xxd -i proj.db > proj_db.c`
// Then rename the array to proj_db and the length to proj_db_len if necessary
// We link these from the proj_db.c file externally instead of #include:ing so our IDE doesnt go haywire
extern "C" unsigned char proj_db[];
extern "C" unsigned int proj_db_len;
extern "C" int sqlite3_memvfs_init(sqlite3 *, char **, const sqlite3_api_routines *);

// Specialize hash for std::pair<std::string, std::string> so we can use it as a key in an unordered_map
template <>
struct std::hash<std::pair<std::string, std::string>> {
	size_t operator()(pair<string, string> const &v) const noexcept {
		const auto lhs = std::hash<string> {}(v.first);
		const auto rhs = std::hash<string> {}(v.second);
		// Shift by one so we dont match the hash of the reversed pair
		return lhs ^ (rhs << 1);
	}
};

namespace duckdb {

// ######################################################################################################################
//  PROJ Module & SQLITE VFS Registration
// ######################################################################################################################

struct ProjModule {
	static void RegisterVFS(DatabaseInstance &db);
};

// IMPORTANT: Make sure this module is loaded before any other modules that use proj (like GDAL)
void ProjModule::RegisterVFS(DatabaseInstance &db) {

	// Initialization lock around global proj state
	static mutex lock;

	lock_guard<mutex> g(lock);

	// we use the sqlite "memvfs" to store the proj.db database in the extension binary itself
	// this way we don't have to worry about the user having the proj.db database installed
	// on their system. We therefore have to tell proj to use memvfs as the sqlite3 vfs and
	// point it to the segment of the binary that contains the proj.db database

	sqlite3_initialize();
	sqlite3_memvfs_init(nullptr, nullptr, nullptr);
	const auto vfs = sqlite3_vfs_find("memvfs");
	if (!vfs) {
		throw InternalException("Could not find sqlite memvfs extension");
	}
	sqlite3_vfs_register(vfs, 0);

	// We set the default context proj.db path to the one in the binary here
	// Otherwise GDAL will try to load the proj.db from the system
	// Any PJ_CONTEXT we create after this will inherit these settings (on this thread?)
	const auto path = StringUtil::Format("file:/proj.db?ptr=%llu&sz=%lu&max=%lu", static_cast<void *>(proj_db),
	                                     proj_db_len, proj_db_len);

	proj_context_set_sqlite3_vfs_name(nullptr, "memvfs");

	const auto ok = proj_context_set_database_path(nullptr, path.c_str(), nullptr, nullptr);
	if (!ok) {
		throw InternalException("Could not set proj.db path");
	}
}

// ######################################################################################################################
//  Functions Registration
// ######################################################################################################################

static void LoadInternal(DatabaseInstance &instance) {

	// Register the VFS for the proj.db database
	ProjModule::RegisterVFS(instance);

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
