cmake_minimum_required(VERSION 3.31.5)

project(TREE CXX)

option(TREE_WITH_CONSTEXPR_CHECKS "run constexpr checks" ON)
option(TREE_WITH_MMIO_TESTS "build and run mmio tests" ON)

add_subdirectory(tree)
add_subdirectory(tests)
