if (TREE_WITH_MMIO_TESTS)
  include(FindOpenMP)
  include(FetchContent)

  FetchContent_Declare(mmio
	GIT_REPOSITORY     https://github.com/ldalessa/mmio-cxx.git
	GIT_TAG            main)
  FetchContent_MakeAvailable(mmio)

  add_executable(mmio mmio.cpp)
  target_link_libraries(mmio tree mmio::mmio OpenMP::OpenMP_CXX)
endif  (TREE_WITH_MMIO_TESTS)

add_executable(test test.cpp)
target_link_libraries(test tree)
