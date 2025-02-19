add_library(pc_ingest_edge_list OBJECT
  parse_tuple.cpp
  print_as_csv.cpp)

target_compile_features(pc_ingest_edge_list PRIVATE cxx_std_23)
target_compile_options(pc_ingest_edge_list PRIVATE -Wall -Werror)
target_include_directories(pc_ingest_edge_list PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(pc_ingest_edge_list PUBLIC gmap::common)

add_library(pc::ingest::edge_list ALIAS pc_ingest_edge_list)
