# add_subdirectory(coffee)
# add_subdirectory(commercial)
# add_subdirectory(cyber)
# add_subdirectory(social)
# add_subdirectory(uses)
# add_subdirectory(wmd)
# add_subdirectory(edge_list)
add_subdirectory(mmio)

add_library(pc_ingest STATIC)
target_link_libraries(pc_ingest
  PUBLIC
    # pc::ingest::coffee
	# pc::ingest::commercial
	# pc::ingest::cyber
	# pc::ingest::social
	# pc::ingest::uses
	# pc::ingest::wmd
	# pc::ingest::edge_list
	pc::ingest::mmio
	pc::ingest::mmio_c)

target_include_directories(pc_ingest INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

add_library(pc::ingest ALIAS pc_ingest)
