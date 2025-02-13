# We need to use main for now for c++26 support.
include(FetchContent)
FetchContent_Declare(CLI11
  GIT_REPOSITORY     https://github.com/CLIUtils/CLI11.git
  GIT_TAG            main)
FetchContent_MakeAvailable(CLI11)

add_library(tree)
target_sources(tree
	PUBLIC
		FILE_SET cxx_modules TYPE CXX_MODULES FILES
		exit_scope.cppm
		options.cppm
		testing.cppm
		tree.cppm
		types.cppm
		Key.cppm
		TreeNode.cppm)

target_compile_features(tree PUBLIC cxx_std_26)

target_compile_options(tree PUBLIC
	--include=${CMAKE_CURRENT_SOURCE_DIR}/FWD.hpp
	--include=${CMAKE_CURRENT_SOURCE_DIR}/ARROW.hpp)

if (TREE_WITH_CONSTEXPR_CHECKS)
  target_compile_options(tree PRIVATE -DTREE_WITH_CONSTEXPR_CHECKS)
endif (TREE_WITH_CONSTEXPR_CHECKS)
  
target_link_libraries(tree PUBLIC CLI11::CLI11)
