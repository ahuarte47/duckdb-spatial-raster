set(EXTENSION_SOURCES
    ${EXTENSION_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/proj_module.cpp
    ${CMAKE_SOURCE_DIR}/../duckdb-spatial/src/spatial/modules/proj/proj_db.c
PARENT_SCOPE)
