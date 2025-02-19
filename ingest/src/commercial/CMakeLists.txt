add_library(pc_ingest_commercial OBJECT
  parse_tuple.cpp
  parse_cycles.cpp
  print_tuple.cpp)

target_compile_features(pc_ingest_commercial PRIVATE cxx_std_23)
target_compile_options(pc_ingest_commercial PRIVATE -Wall -Werror)
target_include_directories(pc_ingest_commercial PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(pc_ingest_commercial PUBLIC gmap::common)

add_library(pc::ingest::commercial ALIAS pc_ingest_commercial)
