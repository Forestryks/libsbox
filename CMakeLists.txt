cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
project(libsbox)

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

include_directories(include)
add_subdirectory(src)
add_subdirectory(include)
