cmake_minimum_required(VERSION 3.31.5)

set(CMAKE_CXX_FLAGS "-stdlib=libc++")
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

project(TREE CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} ${CXXFLAGS} -stdlib=libc++")
set(CMAKE_CXX_MODULE_STD 1)

add_library(tree)
target_sources(tree
  PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
		exit_scope.cppm
		tree.cppm
		types.cppm
		Key.cppm
		TreeNode.cppm
	      )
        # ttl.cpp
        # concepts.cpp
        # tensor_traits.cpp
        # tspan.cpp
        # index/cstring.cpp
        # index/imap.cpp
        # index/index.cpp
        # index/istring.cpp
        # tensor/extents.cpp
        # tensor/evaluate.cpp
        # tensor/outer.cpp
        # tensor/rank.cpp
        # tensor/tensor.cpp
        # tree/bind.cpp
        # tree/expression.cpp)

target_compile_features(tree PUBLIC cxx_std_26)

target_compile_options(tree PUBLIC
  --include=${CMAKE_CURRENT_SOURCE_DIR}/FWD.hpp
  --include=${CMAKE_CURRENT_SOURCE_DIR}/ARROW.hpp)

add_executable(test test.cpp)
target_link_libraries(test tree)
