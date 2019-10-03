cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(libsbox)

add_executable(libsboxd
    libsbox.cpp
    config.cpp
    daemon.cpp
    utils.cpp
    proxy.cpp
    signals.cpp
    context.cpp
    shared_memory.cpp
    shared_mutex.cpp
    shared_cond.cpp
    shared_counter.cpp
    shared_barrier.cpp
    worker.cpp
    target.cpp
    bind.cpp
    cgroup.cpp
)

target_link_libraries(libsboxd pthread)

#add_library(sbox init.cpp die.cpp execution_target.cpp io.cpp execution_context.cpp fs.cpp signal.cpp cgroup.cpp)
#
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
