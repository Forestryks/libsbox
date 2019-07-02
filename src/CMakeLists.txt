cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
project(libsbox)

add_library(sbox init.cpp die.cpp execution_target.cpp io.cpp execution_context.cpp fs.cpp signal.cpp cgroup.cpp)

install(
    TARGETS sbox
    DESTINATION lib/x86_64-linux-gnu
)

add_executable(invoke invoke.cpp)
target_link_libraries(invoke sbox)

install(
    TARGETS invoke
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ SETUID SETGID
)

install(
    CODE
    "execute_process(COMMAND chown root:invokers ${CMAKE_INSTALL_PREFIX}/bin/invoke)"
)

install(
    CODE
    "execute_process(COMMAND chmod +s ${CMAKE_INSTALL_PREFIX}/bin/invoke)"
)
