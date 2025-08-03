# This file is included by DuckDB's build system. It specifies which extension to load

# Extension from this repo
duckdb_extension_load(spatial_raster
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
    LOAD_TESTS
)

# Any extra extensions that should be built
# e.g.: duckdb_extension_load(json)

# Add the spatial extension to test integration with spatial-raster
duckdb_extension_load(spatial
    SOURCE_DIR  ${CMAKE_CURRENT_LIST_DIR}/duckdb-spatial
    INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/duckdb-spatial/src/spatial
)
