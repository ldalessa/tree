# We need to use main for now for c++26 support.
include(FetchContent)
FetchContent_Declare(CLI11
  GIT_REPOSITORY     https://github.com/CLIUtils/CLI11.git
  GIT_TAG            main)
FetchContent_MakeAvailable(CLI11)

add_library(tree INTERFACE
  exit_scope.hpp
  options.hpp
  testing.hpp
  tree.hpp
  types.hpp
  Glob.hpp
  GlobTreeNode.hpp
  Key.hpp
  SPSCQueue.hpp
  TreeNode.hpp
  TopLevelTreeNode.hpp
)

target_include_directories(tree INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)
target_compile_features(tree INTERFACE cxx_std_26)
target_link_libraries(tree INTERFACE CLI11::CLI11)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_libraries(tree INTERFACE atomic)
endif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
