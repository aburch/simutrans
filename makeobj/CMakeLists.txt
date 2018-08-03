#
# This file is part of the Simutrans project under the artistic licence.
# (see licence.txt)
#

add_executable(makeobj-extended
	makeobj.cc
)

target_compile_options(makeobj-extended PRIVATE ${SIMUTRANS_COMMON_COMPILE_OPTIONS})
target_compile_definitions(makeobj-extended PRIVATE MAKEOBJ=1 COLOUR_DEPTH=0)


target_link_libraries(makeobj-extended
	ZLIB::ZLIB
	PNG::PNG
	BZip2::BZip2
)


# These source files are unique to makeobj-extended
target_sources(makeobj-extended PRIVATE
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
	../descriptor/writer/imagelist3d_writer.cc
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
	../utils/dr_rdpng.cc
)

# These source files produce the same object code in makeobj-extended and simutrans
target_sources(makeobj-extended PRIVATE
	../descriptor/image.cc
	../dataobj/freelist.cc
	../simdebug.cc
	../simmem.cc
	../utils/simstring.cc
	../utils/searchfolder.cc
)

# These source files produce different object code in makeobj-extended and simutrans
target_sources(makeobj-extended PRIVATE
	../dataobj/tabfile.cc
	../utils/log.cc
)

