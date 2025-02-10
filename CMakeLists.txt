cmake_minimum_required(VERSION 3.31.5)

set(CMAKE_CXX_FLAGS "-stdlib=libc++")
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

project(TREE CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} ${CXXFLAGS}")
set(CMAKE_CXX_MODULE_STD 1)

option(TREE_WITH_CONSTEXPR_CHECKS "run constexpr checks" ON)
option(TREE_WITH_MMIO_TESTS "build and run mmio tests" ON)

add_subdirectory(tree)
add_subdirectory(tests)
