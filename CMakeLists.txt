cmake_minimum_required(VERSION 3.10)
project(libsbox)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

add_executable(libsbox main.cpp libsbox.cpp fs.cpp)
