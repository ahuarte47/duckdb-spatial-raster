set(EXTENSION_SOURCES
    ${EXTENSION_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/spatial_raster_extension.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gdal_module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raster_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raster_table_functions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../duckdb-spatial/src/spatial/util/function_builder.cpp
PARENT_SCOPE)
