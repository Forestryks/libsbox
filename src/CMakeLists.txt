cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
project(libsbox)

add_library(sbox init.cpp die.cpp execution_target.cpp io.cpp execution_context.cpp fs.cpp signal.cpp cgroup.cpp logger.cpp)

install(
    TARGETS sbox
    DESTINATION lib/x86_64-linux-gnu
)
