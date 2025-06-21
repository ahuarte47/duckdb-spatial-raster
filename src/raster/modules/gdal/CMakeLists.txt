set(EXTENSION_SOURCES
    ${EXTENSION_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/gdal_module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gdal_dataset_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gdal_dataset_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gdal_context_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gdal_dataset_ts.cpp
    PARENT_SCOPE)
