cmake_minimum_required(VERSION 3.3)
project(tpfpatch_store)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        src/main/cpp/test.cc)

add_executable(tpfpatch_store ${SOURCE_FILES})