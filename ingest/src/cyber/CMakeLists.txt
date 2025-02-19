add_library(pc_ingest_cyber OBJECT
  parse_tuple.cpp
  parse_cycles.cpp)

target_compile_features(pc_ingest_cyber PRIVATE cxx_std_23)
target_compile_options(pc_ingest_cyber PRIVATE -Wall -Werror)
target_include_directories(pc_ingest_cyber PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(pc_ingest_cyber PUBLIC gmap::common)

add_library(pc::ingest::cyber ALIAS pc_ingest_cyber)
