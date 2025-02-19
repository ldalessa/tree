# Build the mmio library that's available from online.
add_library(pc_ingest_mmio_contrib OBJECT mmio.c)
target_compile_features(pc_ingest_mmio_contrib PRIVATE c_std_99)
add_library(pc::ingest::mmio_c ALIAS pc_ingest_mmio_contrib)

# Build the ingest mmio library.
add_library(pc_ingest_mmio
  OBJECT
    FileReader.cpp
	parse_tuple.cpp
	print_tuple.cpp)
  
target_compile_features(pc_ingest_mmio PRIVATE cxx_std_23)
target_compile_options(pc_ingest_mmio PRIVATE -Wall -Werror)
target_include_directories(pc_ingest_mmio PUBLIC ${PROJECT_SOURCE_DIR}/ingest/include)

add_library(pc::ingest::mmio ALIAS pc_ingest_mmio)
