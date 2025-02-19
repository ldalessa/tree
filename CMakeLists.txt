cmake_minimum_required(VERSION 3.31.5)

project(TREE CXX C)

option(TREE_WITH_CONSTEXPR_CHECKS "run constexpr checks" ON)

add_subdirectory(ingest)
add_subdirectory(tree)
add_subdirectory(tests)
