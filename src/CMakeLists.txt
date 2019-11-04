cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(libsbox)

add_executable(
    libsboxd
    libsboxd.cpp
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
    bind.cpp bind.h)

target_link_libraries(libsboxd pthread)

#install(
#    TARGETS sbox
#    DESTINATION lib/x86_64-linux-gnu
#)
#
#add_executable(invoke invoke.cpp)
#target_link_libraries(invoke sbox)
#
install(
    TARGETS libsboxd
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ
)

#
#install(
#	CODE
#	"execute_process(COMMAND groupadd invokers)"
#)
#
#install(
#    CODE
#    "execute_process(COMMAND chown root:invokers ${CMAKE_INSTALL_PREFIX}/bin/invoke)"
#)
#
#install(
#    CODE
#    "execute_process(COMMAND chmod +s ${CMAKE_INSTALL_PREFIX}/bin/invoke)"
#)
