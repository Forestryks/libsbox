cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
project(libsbox)

SET(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -Wextra"
)

set(
    CMAKE_INSTALL_PREFIX
    /usr
)

set(
    CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin
    CACHE PATH "Binary output dir"
)

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin
    CACHE PATH "Binary output dir"
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/lib
    CACHE PATH "Library output"
)

find_path(JSON_INCLUDES external/json/single_include/nlohmann)
if ((NOT JSON_INCLUDES) OR (NOT EXISTS ${JSON_INCLUDES}))
    execute_process(COMMAND git submodule update --init -- external/json
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()
include_directories(external/json/single_include/nlohmann)

enable_testing()

include_directories(include)
add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(tests)
