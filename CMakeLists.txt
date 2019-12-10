cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(libsbox)

set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Winit-self -Wmissing-declarations -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wno-unused "
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(
        CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wlogical-op -Wnoexcept -Wstrict-null-sentinel"
    )
endif ()

set(
    CMAKE_INSTALL_PREFIX
    /usr
)

set(
    LIBSBOX_INCLUDE_DIR
    ${PROJECT_SOURCE_DIR}/include
)

set(
    LIBSBOX_SOURCE_DIR
    ${PROJECT_SOURCE_DIR}/src
)

set(RAPIDJSON_REPO_DIR "${PROJECT_BINARY_DIR}/rapidjson")
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/external/include/rapidjson)
    execute_process(COMMAND git clone --depth=1 https://github.com/Tencent/rapidjson.git ${RAPIDJSON_REPO_DIR}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
    file(COPY ${RAPIDJSON_REPO_DIR}/include/rapidjson DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/external/include)
    file(REMOVE_RECURSE ${RAPIDJSON_REPO_DIR})
endif ()

include_directories(SYSTEM external/include)
include_directories(include)

add_subdirectory(src)
add_subdirectory(tests)

install(
    CODE
    "execute_process(COMMAND ./prepare.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})"
)

install(
    FILES include/libsbox.h
    DESTINATION include
)

install(
    DIRECTORY include/libsbox
    DESTINATION include
)