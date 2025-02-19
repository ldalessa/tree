add_library(pc_ingest_coffee OBJECT coffee.cpp)

target_compile_features(pc_ingest_coffee PRIVATE cxx_std_23)
target_compile_options(pc_ingest_coffee PRIVATE -Wall -Werror)
target_include_directories(pc_ingest_coffee PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(pc_ingest_coffee PRIVATE
  pc::ingest::commercial
  pc::ingest::cyber
  pc::ingest::social
  pc::ingest::uses)

add_library(pc::ingest::coffee ALIAS pc_ingest_coffee)
