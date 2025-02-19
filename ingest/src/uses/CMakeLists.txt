add_library(pc_ingest_uses OBJECT
  parse_tuple.cpp
  parse_cycles.cpp
  print_tuple.cpp)

target_compile_features(pc_ingest_uses PRIVATE cxx_std_23)
target_compile_options(pc_ingest_uses PRIVATE -Wall -Werror)
target_include_directories(pc_ingest_uses PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(pc_ingest_uses PUBLIC gmap::common)

add_library(pc::ingest::uses ALIAS pc_ingest_uses)
