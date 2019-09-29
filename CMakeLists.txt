cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
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

if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/external/include/json.hpp)
    execute_process(COMMAND wget https://github.com/nlohmann/json/releases/download/v3.7.0/json.hpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/include)
endif ()
include_directories(external/include)

install(
    CODE
    "execute_process(COMMAND ./prepare.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})"
)

enable_testing()

include_directories(include)
add_subdirectory(src)
