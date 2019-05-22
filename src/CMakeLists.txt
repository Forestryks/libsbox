cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)

add_executable(libsbox main.cpp init.cpp conf.cpp die.cpp execution_target.cpp io.cpp bind.cpp execution_context.cpp fs.cpp signal.cpp cgroup.cpp)
