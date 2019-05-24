cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
project(libsbox)

install(
    FILES libsbox.h
    DESTINATION include
)

install(
    DIRECTORY libsbox
    DESTINATION include
)
