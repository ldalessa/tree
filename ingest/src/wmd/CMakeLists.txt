add_library(pc_ingest_wmd OBJECT
  parse_tuple.cpp
  parse_cycles.cpp
  print_as_csv.cpp)

target_compile_features(pc_ingest_wmd PRIVATE cxx_std_23)
target_compile_options(pc_ingest_wmd PRIVATE -Wall -Werror)
target_include_directories(pc_ingest_wmd PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(pc_ingest_wmd PUBLIC gmap::common)

add_library(pc::ingest::wmd ALIAS pc_ingest_wmd)
