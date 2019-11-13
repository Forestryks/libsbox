cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(libsbox)

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
    logger.cpp)

target_link_libraries(libsboxd pthread)

install(
    TARGETS libsboxd
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ
)

add_library(
    sbox
    libsbox.cpp utils.cpp context_manager.cpp
)

install(
    TARGETS sbox
    DESTINATION lib
)
