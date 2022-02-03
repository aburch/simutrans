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
	../simutrans/descriptor/writer/bridge_writer.cc
	../simutrans/descriptor/writer/building_writer.cc
	../simutrans/descriptor/writer/citycar_writer.cc
	../simutrans/descriptor/writer/crossing_writer.cc
	../simutrans/descriptor/writer/factory_writer.cc
	../simutrans/descriptor/writer/get_climate.cc
	../simutrans/descriptor/writer/get_waytype.cc
	../simutrans/descriptor/writer/good_writer.cc
	../simutrans/descriptor/writer/groundobj_writer.cc
	../simutrans/descriptor/writer/ground_writer.cc
	../simutrans/descriptor/writer/image_writer.cc
	../simutrans/descriptor/writer/imagelist2d_writer.cc
	../simutrans/descriptor/writer/imagelist_writer.cc
	../simutrans/descriptor/writer/obj_node.cc
	../simutrans/descriptor/writer/obj_writer.cc
	../simutrans/descriptor/writer/pedestrian_writer.cc
	../simutrans/descriptor/writer/roadsign_writer.cc
	../simutrans/descriptor/writer/root_writer.cc
	../simutrans/descriptor/writer/sim_writer.cc
	../simutrans/descriptor/writer/skin_writer.cc
	../simutrans/descriptor/writer/sound_writer.cc
	../simutrans/descriptor/writer/text_writer.cc
	../simutrans/descriptor/writer/tree_writer.cc
	../simutrans/descriptor/writer/tunnel_writer.cc
	../simutrans/descriptor/writer/vehicle_writer.cc
	../simutrans/descriptor/writer/way_writer.cc
	../simutrans/descriptor/writer/way_obj_writer.cc
	../simutrans/descriptor/writer/xref_writer.cc
)

# These source files produce the same object code in makeobj and simutrans
target_sources(makeobj PRIVATE
	../simutrans/descriptor/image.cc
	../simutrans/dataobj/freelist.cc
	../simutrans/io/raw_image.cc
	../simutrans/simio.cc
	../simutrans/simdebug.cc
	../simutrans/simmem.cc
	../simutrans/utils/simstring.cc
	../simutrans/utils/searchfolder.cc
)

# These source files produce different object code in makeobj and simutrans
target_sources(makeobj PRIVATE
	../simutrans/dataobj/tabfile.cc
	../simutrans/io/classify_file.cc
	../simutrans/io/raw_image_bmp.cc
	../simutrans/io/raw_image_png.cc
	../simutrans/io/raw_image_ppm.cc
	../simutrans/utils/log.cc
)

