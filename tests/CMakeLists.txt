if (TREE_WITH_MMIO_TESTS)
  include(FetchContent)

  FetchContent_Declare(mmio
	GIT_REPOSITORY     https://github.com/ldalessa/mmio-cxx.git
	GIT_TAG            main)
  FetchContent_MakeAvailable(mmio)

  add_executable(mmio mmio.cpp)
  target_link_libraries(mmio tree mmio::mmio)

  
  add_executable(globs globs.cpp)
  target_link_libraries(globs tree mmio::mmio)

  add_executable(bubble bubble.cpp)
  target_link_libraries(bubble tree mmio::mmio)
endif (TREE_WITH_MMIO_TESTS)

add_executable(test test.cpp)
target_link_libraries(test tree)

if (TREE_WITH_CONSTEXPR_CHECKS)
  target_compile_definitions(test PRIVATE TREE_WITH_CONSTEXPR_CHECKS)
endif (TREE_WITH_CONSTEXPR_CHECKS)


