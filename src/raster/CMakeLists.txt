set(EXTENSION_SOURCES
    ${EXTENSION_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/spatial_raster_extension.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gdal_module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gdal_dataset_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gdal_dataset_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gdal_context_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raster_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raster_value.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raster_table_functions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raster_casts_functions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../duckdb-spatial/src/spatial/util/function_builder.cpp
PARENT_SCOPE)
