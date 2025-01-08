#
# This file is part of the Simutrans project under the artistic licence.
# (see licence.txt)
#

add_executable(makeobj
	makeobj.cc
)

target_compile_options(makeobj PRIVATE ${SIMUTRANS_COMMON_COMPILE_OPTIONS})
target_compile_definitions(makeobj PRIVATE MAKEOBJ=1 COLOUR_DEPTH=0)
target_compile_definitions(makeobj PRIVATE MSG_LEVEL=${SIMUTRANS_MSG_LEVEL})

if (SIMUTRANS_USE_SYSLOG)
	target_compile_definitions(simutrans PRIVATE SYSLOG=1)
endif ()

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 4 AND SIMUTRANS_BUILD_32BIT)
	target_compile_options(makeobj PRIVATE -m32)
	set_target_properties(makeobj PROPERTIES LINK_FLAGS "-m32")
endif ()

target_link_libraries(makeobj
	PNG::PNG
)


# These source files are unique to makeobj
target_sources(makeobj PRIVATE
	../descriptor/writer/bridge_writer.cc
	../descriptor/writer/building_writer.cc
	../descriptor/writer/citycar_writer.cc
	../descriptor/writer/crossing_writer.cc
	../descriptor/writer/factory_writer.cc
	../descriptor/writer/get_climate.cc
	../descriptor/writer/get_waytype.cc
	../descriptor/writer/good_writer.cc
	../descriptor/writer/groundobj_writer.cc
	../descriptor/writer/ground_writer.cc
	../descriptor/writer/image_writer.cc
	../descriptor/writer/imagelist2d_writer.cc
	../descriptor/writer/imagelist_writer.cc
	../descriptor/writer/obj_node.cc
	../descriptor/writer/obj_writer.cc
	../descriptor/writer/pedestrian_writer.cc
	../descriptor/writer/roadsign_writer.cc
	../descriptor/writer/root_writer.cc
	../descriptor/writer/sim_writer.cc
	../descriptor/writer/skin_writer.cc
	../descriptor/writer/sound_writer.cc
	../descriptor/writer/text_writer.cc
	../descriptor/writer/tree_writer.cc
	../descriptor/writer/tunnel_writer.cc
	../descriptor/writer/vehicle_writer.cc
	../descriptor/writer/way_writer.cc
	../descriptor/writer/way_obj_writer.cc
	../descriptor/writer/xref_writer.cc
)

# These source files produce the same object code in makeobj and simutrans
target_sources(makeobj PRIVATE
	../descriptor/image.cc
	../dataobj/freelist.cc
	../io/raw_image.cc
	../simio.cc
	../simdebug.cc
	../simmem.cc
	../utils/simstring.cc
	../utils/searchfolder.cc
)

# These source files produce different object code in makeobj and simutrans
target_sources(makeobj PRIVATE
	../dataobj/tabfile.cc
	../io/classify_file.cc
	../io/raw_image_bmp.cc
	../io/raw_image_png.cc
	../io/raw_image_ppm.cc
	../utils/log.cc
)

