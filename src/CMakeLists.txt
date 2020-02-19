add_subdirectory(schema)

set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/schema/generated/request_schema.c PROPERTY GENERATED 1)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/schema/generated/response_schema.c PROPERTY GENERATED 1)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/schema/)

add_executable(
    libsboxd
    libsboxd.cpp
    libsbox.cpp
    context_manager.cpp
    utils.cpp
    shared_memory.cpp
    shared_mutex.cpp
    shared_cond.cpp
    shared_id_getter.cpp
    shared_barrier.cpp
    config.cpp
    signals.cpp
    daemon.cpp
    worker.cpp
    container.cpp
    cgroup_controller.cpp
    bind.cpp
    logger.cpp
    schema/generated/request_schema.c
    schema/generated/response_schema.c
    schema_validator.cpp
    error.cpp)

target_link_libraries(libsboxd pthread)

install(
    TARGETS libsboxd
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ
)

add_dependencies(libsboxd json_schemas)

set(
    LIBRARY_SOURCES
    libsbox.cpp
    utils.cpp
    context_manager.cpp
    error.cpp
    schema_validator.cpp
    schema/generated/response_schema.c
)

add_library(
    sbox-static
    ${LIBRARY_SOURCES}
)

SET_TARGET_PROPERTIES(
    sbox-static PROPERTIES
    OUTPUT_NAME sbox CLEAN_DIRECT_OUTPUT 1
)

add_dependencies(sbox-static json_schemas)

add_library(
    sbox-shared SHARED
    ${LIBRARY_SOURCES}
)

SET_TARGET_PROPERTIES(
    sbox-shared PROPERTIES
    OUTPUT_NAME sbox CLEAN_DIRECT_OUTPUT 1
)

add_dependencies(sbox-shared json_schemas)

target_compile_options(
    sbox-static
    PUBLIC -fPIC
)

install(
    TARGETS sbox-static
    DESTINATION lib
)

install(
    TARGETS sbox-shared
    DESTINATION lib
)

add_library(libsbox::sbox-static ALIAS sbox-static)
