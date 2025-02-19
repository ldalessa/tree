include(FetchContent)

FetchContent_Declare(
  moodycamel
  GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
  GIT_TAG master)
FetchContent_MakeAvailable(moodycamel)

add_executable(mmio mmio.cpp)
target_link_libraries(mmio tree pc::ingest)
  
add_executable(globs globs.cpp)
target_link_libraries(globs tree pc::ingest)

add_executable(bubble bubble.cpp)
target_link_libraries(bubble tree pc::ingest concurrentqueue)

add_executable(test test.cpp)
target_link_libraries(test tree)

if (TREE_WITH_CONSTEXPR_CHECKS)
  target_compile_definitions(test PRIVATE TREE_WITH_CONSTEXPR_CHECKS)
endif (TREE_WITH_CONSTEXPR_CHECKS)
