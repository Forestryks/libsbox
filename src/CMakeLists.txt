cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)

add_executable(libsbox main.cpp fs.cpp execution_context.cpp die.cpp init.cpp status.cpp)
