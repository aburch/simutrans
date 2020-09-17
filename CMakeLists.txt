#
# This file is part of the Simutrans-Extended project under the Artistic License.
# (see LICENSE.txt)
#

cmake_minimum_required(VERSION 3.8)

# Disable in-source builds
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "Building Simutrans in-source is not supported. "
		"Please delete ${CMAKE_SOURCE_DIR}/CMakeCache.txt and configure in a different "
		"(preferrably empty) directory.")
endif (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

set(CMAKE_WARN_DEPRECATED ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")


find_package(CCache)
if (CCache_FOUND)
	option(SIMUTRANS_USE_CCACHE "Use CCache compiler cache to improve recompilation speed" ON)
	if (SIMUTRANS_USE_CCACHE)
		set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCache_EXECUTABLE}")
		set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK    "${CCache_EXECUTABLE}")
	endif (SIMUTRANS_USE_CCACHE)
endif (CCache_FOUND)


project(simutrans-extended LANGUAGES C CXX)

include(simutrans-revision)

include(TestBigEndian)
TEST_BIG_ENDIAN(SIMUTRANS_BIG_ENDIAN)


# Force C++11 everywhere
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)


#
# Dependencies
#
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(PNG)
find_package(Freetype)
find_package(MiniUPNP)
find_package(ZSTD)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads)


#
# Configuration options
#
include(simutrans-backend)

if (CMAKE_USE_PTHREADS_INIT)
	option(SIMUTRANS_MULTI_THREAD "Use multiple threads for drawing" ON)
else (CMAKE_USE_PTHREADS_INIT)
	set(SIMUTRANS_MULTI_THREAD OFF)
endif (CMAKE_USE_PTHREADS_INIT)

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
	option(SIMUTRANS_BUILD_32BIT "Build 32 or 64 bit executable" OFF)
endif ()


option(SIMUTRANS_VALGRIND_SUPPORT  "Add support for valgrind \"memcheck\" tool" OFF)

if (Freetype_FOUND)
	option(SIMUTRANS_USE_FREETYPE "Enable TrueType font support using freetype library" OFF)
endif (Freetype_FOUND)

if (MiniUPNP_FOUND)
	option(SIMUTRANS_USE_UPNP "Use MiniUPNP for easier server setup" ON)
endif (MiniUPNP_FOUND)

if (ZSTD_FOUND)
	option(SIMUTRANS_USE_ZSTD "Enable support for zstd save file compression (slightly larger save files, but faster)" ON)
endif (ZSTD_FOUND)

option(SIMUTRANS_ENABLE_PROFILING "Enable profiling code" OFF)
option(SIMUTRANS_USE_SYSLOG "Enable logging to syslog" OFF)
option(SIMUTRANS_ENABLE_IPV6 "Enable connections using IPv6 in addition to IPv4" ON)
option(SIMUTRANS_ENABLE_RANDOMNESS "Disable to make get_random_seed() always return the same number" ON)
option(SIMUTRANS_DEBUG_SIMRAND "Debug the random number generator" OFF)

set(SIMUTRANS_MSG_LEVEL 0 CACHE STRING "Message verbosity level")
set_property(CACHE SIMUTRANS_MSG_LEVEL PROPERTY STRINGS 0 1 2 3 4)

# CMake does not fill in CMAKE_BUILD_TYPE automatically.
if (NOT MSVC)
    if (CMAKE_BUILD_TYPE STREQUAL "")
        set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type. Valid values are Debug Release MinSizeRel RelWithDebInfo" FORCE)
    endif ()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)
endif (NOT MSVC)


#
# output directory
#
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/simutrans)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/simutrans)


#
# sources
#
add_executable(simutrans-extended
	bauer/brueckenbauer.cc
	bauer/fabrikbauer.cc
	bauer/goods_manager.cc
	bauer/hausbauer.cc
	bauer/tunnelbauer.cc
	bauer/vehikelbauer.cc
	bauer/wegbauer.cc
	boden/boden.cc
	boden/brueckenboden.cc
	boden/fundament.cc
	boden/grund.cc
	boden/monorailboden.cc
	boden/tunnelboden.cc
	boden/wasser.cc
	boden/wege/kanal.cc
	boden/wege/maglev.cc
	boden/wege/monorail.cc
	boden/wege/narrowgauge.cc
	boden/wege/runway.cc
	boden/wege/schiene.cc
	boden/wege/strasse.cc
	boden/wege/weg.cc
	convoy.cc
	dataobj/crossing_logic.cc
	dataobj/environment.cc
	dataobj/freelist.cc
	dataobj/gameinfo.cc
	dataobj/height_map_loader.cc
	dataobj/koord3d.cc
	dataobj/koord.cc
	dataobj/livery_scheme.cc
	dataobj/loadsave.cc
	dataobj/marker.cc
	dataobj/objlist.cc
	dataobj/powernet.cc
	dataobj/rect.cc
	dataobj/replace_data.cc
	dataobj/ribi.cc
	dataobj/route.cc
	dataobj/scenario.cc
	dataobj/schedule.cc
	dataobj/settings.cc
	dataobj/tabfile.cc
	dataobj/translator.cc
	descriptor/bridge_desc.cc
	descriptor/building_desc.cc
	descriptor/factory_desc.cc
	descriptor/goods_desc.cc
	descriptor/ground_desc.cc
	descriptor/image.cc
	descriptor/obj_base_desc.cc
	descriptor/reader/bridge_reader.cc
	descriptor/reader/building_reader.cc
	descriptor/reader/citycar_reader.cc
	descriptor/reader/crossing_reader.cc
	descriptor/reader/factory_reader.cc
	descriptor/reader/good_reader.cc
	descriptor/reader/groundobj_reader.cc
	descriptor/reader/ground_reader.cc
	descriptor/reader/imagelist2d_reader.cc
	descriptor/reader/imagelist3d_reader.cc
	descriptor/reader/imagelist_reader.cc
	descriptor/reader/image_reader.cc
	descriptor/reader/obj_reader.cc
	descriptor/reader/pedestrian_reader.cc
	descriptor/reader/roadsign_reader.cc
	descriptor/reader/root_reader.cc
	descriptor/reader/sim_reader.cc
	descriptor/reader/skin_reader.cc
	descriptor/reader/sound_reader.cc
	descriptor/reader/text_reader.cc
	descriptor/reader/tree_reader.cc
	descriptor/reader/tunnel_reader.cc
	descriptor/reader/vehicle_reader.cc
	descriptor/reader/way_obj_reader.cc
	descriptor/reader/way_reader.cc
	descriptor/reader/xref_reader.cc
	descriptor/sound_desc.cc
	descriptor/tunnel_desc.cc
	descriptor/vehicle_desc.cc
	descriptor/way_desc.cc
	display/font.cc
	display/simview.cc
	display/viewport.cc
	finder/placefinder.cc
	freight_list_sorter.cc
	gui/ai_option_t.cc
	gui/banner.cc
	gui/base_info.cc
	gui/baum_edit.cc
	gui/citybuilding_edit.cc
	gui/city_info.cc
	gui/citylist_frame_t.cc
	gui/citylist_stats_t.cc
	gui/climates.cc
	gui/components/gui_button.cc
	gui/components/gui_chart.cc
	gui/components/gui_combobox.cc
	gui/components/gui_component.cc
	gui/components/gui_component_table.cc
	gui/components/gui_container.cc
	gui/components/gui_convoiinfo.cc
	gui/components/gui_convoy_assembler.cc
	gui/components/gui_convoy_formation.cc
	gui/components/gui_convoy_label.cc
	gui/components/gui_convoy_payloadinfo.cc
	gui/components/gui_factory_storage_info.cc
	gui/components/gui_fixedwidth_textarea.cc
	gui/components/gui_flowtext.cc
	gui/components/gui_halthandled_lines.cc
	gui/components/gui_image.cc
	gui/components/gui_image_list.cc
	gui/components/gui_label.cc
	gui/components/gui_map_preview.cc
	gui/components/gui_numberinput.cc
	gui/components/gui_obj_view_t.cc
	gui/components/gui_scrollbar.cc
	gui/components/gui_scrolled_list.cc
	gui/components/gui_scrollpane.cc
	gui/components/gui_speedbar.cc
	gui/components/gui_table.cc
	gui/components/gui_tab_panel.cc
	gui/components/gui_textarea.cc
	gui/components/gui_textinput.cc
	gui/components/gui_world_view_t.cc
	gui/convoi_detail_t.cc
	gui/convoi_filter_frame.cc
	gui/convoi_frame.cc
	gui/convoi_info_t.cc
	gui/convoy_item.cc
	gui/curiosity_edit.cc
	gui/curiositylist_frame_t.cc
	gui/curiositylist_stats_t.cc
	gui/depot_frame.cc
	gui/display_settings.cc
	gui/enlarge_map_frame_t.cc
	gui/extend_edit.cc
	gui/fabrik_info.cc
	gui/factory_chart.cc
	gui/factory_edit.cc
	gui/factorylist_frame_t.cc
	gui/factorylist_stats_t.cc
	gui/goods_frame_t.cc
	gui/goods_stats_t.cc
	gui/ground_info.cc
	gui/gui_frame.cc
	gui/gui_theme.cc
	gui/halt_detail.cc
	gui/halt_info.cc
	gui/halt_list_filter_frame.cc
	gui/halt_list_frame.cc
	gui/halt_list_stats.cc
	gui/help_frame.cc
	gui/jump_frame.cc
	gui/karte.cc
	gui/kennfarbe.cc
	gui/label_info.cc
	gui/labellist_frame_t.cc
	gui/labellist_stats_t.cc
	gui/line_class_manager.cc
	gui/line_item.cc
	gui/line_management_gui.cc
	gui/load_relief_frame.cc
	gui/loadsave_frame.cc
	gui/map_frame.cc
	gui/messagebox.cc
	gui/message_frame_t.cc
	gui/message_option_t.cc
	gui/message_stats_t.cc
	gui/money_frame.cc
	gui/obj_info.cc
	gui/onewaysign_info.cc
	gui/optionen.cc
	gui/overtaking_mode.cc
	gui/pakselector.cc
	gui/password_frame.cc
	gui/player_frame_t.cc
	gui/privatesign_info.cc
	gui/replace_frame.cc
	gui/savegame_frame.cc
	gui/scenario_frame.cc
	gui/scenario_info.cc
	gui/schedule_gui.cc
	gui/schedule_list.cc
	gui/schiene_info.cc
	gui/server_frame.cc
	gui/settings_frame.cc
	gui/settings_stats.cc
	gui/signal_info.cc
	gui/signal_spacing.cc
	gui/simwin.cc
	gui/sound_frame.cc
	gui/sprachen.cc
	gui/station_building_select.cc
	gui/themeselector.cc
	gui/times_history.cc
	gui/times_history_container.cc
	gui/times_history_entry.cc
	gui/tool_selector.cc
	gui/trafficlight_info.cc
	gui/vehicle_class_manager.cc
	gui/welt.cc
	network/checksum.cc
	network/memory_rw.cc
	network/network_address.cc
	network/network.cc
	network/network_cmd.cc
	network/network_cmd_ingame.cc
	network/network_cmd_scenario.cc
	network/network_cmp_pakset.cc
	network/network_file_transfer.cc
	network/network_packet.cc
	network/network_socket_list.cc
	network/pakset_info.cc
	network/pwd_hash.cc
	obj/baum.cc
	obj/bruecke.cc
	obj/crossing.cc
	obj/field.cc
	obj/gebaeude.cc
	obj/groundobj.cc
	obj/label.cc
	obj/leitung2.cc
	obj/pillar.cc
	obj/roadsign.cc
	obj/signal.cc
	obj/tunnel.cc
	obj/wayobj.cc
	obj/wolke.cc
	obj/zeiger.cc
	old_blockmanager.cc
	path_explorer.cc
	player/ai.cc
	player/ai_goods.cc
	player/ai_passenger.cc
	player/finance.cc
	player/simplay.cc
	script/api/api_city.cc
	script/api/api_const.cc
	script/api/api_convoy.cc
	script/api/api_factory.cc
	script/api/api_gui.cc
	script/api/api_halt.cc
	script/api/api_include.cc
	script/api/api_line.cc
	script/api/api_map_objects.cc
	script/api/api_obj_desc_base.cc
	script/api/api_obj_desc.cc
	script/api/api_player.cc
	script/api/api_scenario.cc
	script/api/api_schedule.cc
	script/api/api_settings.cc
	script/api/api_simple.cc
	script/api/api_tiles.cc
	script/api/api_world.cc
	script/api_class.cc
	script/api/export_desc.cc
	script/api_function.cc
	script/api/get_next.cc
	script/api_param.cc
	script/dynamic_string.cc
	script/export_objs.cc
	script/script.cc
	simcity.cc
	simconvoi.cc
	simdebug.cc
	simdepot.cc
	simevent.cc
	simfab.cc
	simhalt.cc
	siminteraction.cc
	simintr.cc
	simio.cc
	simline.cc
	simlinemgmt.cc
	simloadingscreen.cc
	simmain.cc
	simmem.cc
	simmenu.cc
	simmesg.cc
	simobj.cc
	simplan.cc
	simsignalbox.cc
	simskin.cc
	simsound.cc
	sys/simsys.cc
	simticker.cc
	simtool.cc
	simunits.cc
	simware.cc
	simworld.cc
	squirrel/sq_extensions.cc
	squirrel/sqstdlib/sqstdaux.cc
	squirrel/sqstdlib/sqstdblob.cc
	squirrel/sqstdlib/sqstdio.cc
	squirrel/sqstdlib/sqstdmath.cc
	squirrel/sqstdlib/sqstdrex.cc
	squirrel/sqstdlib/sqstdstream.cc
	squirrel/sqstdlib/sqstdstring.cc
	squirrel/sqstdlib/sqstdsystem.cc
	squirrel/squirrel/sqapi.cc
	squirrel/squirrel/sqbaselib.cc
	squirrel/squirrel/sqclass.cc
	squirrel/squirrel/sqcompiler.cc
	squirrel/squirrel/sqdebug.cc
	squirrel/squirrel/sqfuncstate.cc
	squirrel/squirrel/sqlexer.cc
	squirrel/squirrel/sqmem.cc
	squirrel/squirrel/sqobject.cc
	squirrel/squirrel/sqstate.cc
	squirrel/squirrel/sqtable.cc
	squirrel/squirrel/sqvm.cc
	unicode.cc
	utils/cbuffer_t.cc
	utils/csv.cc
	utils/float32e8_t.cc
	utils/log.cc
	utils/searchfolder.cc
	utils/sha1.cc
	utils/simrandom.cc
	utils/simstring.cc
	utils/simthread.cc
	vehicle/movingobj.cc
	vehicle/simpeople.cc
	vehicle/simroadtraffic.cc
	vehicle/simvehicle.cc
)

if (WIN32)
	target_sources(simutrans-extended PRIVATE sys/clipboard_w32.cc)
else ()
	target_sources(simutrans-extended PRIVATE sys/clipboard_internal.cc)
endif ()

if (SIMUTRANS_MULTI_THREAD)
	target_compile_definitions(simutrans-extended PRIVATE MULTI_THREAD=1)
	target_link_libraries(simutrans-extended PRIVATE Threads::Threads)
endif (SIMUTRANS_MULTI_THREAD)


# Backup sound and music routines if the selected backend does not provide any routines
if (WIN32)
	set(BACKUP_SOUND sound/win32_sound.cc)
	set(BACKUP_MUSIC music/w32_midi.cc)
else (WIN32)
	set(BACKUP_SOUND sound/no_sound.cc)
	set(BACKUP_MUSIC music/no_midi.cc)
endif (WIN32)


if (SIMUTRANS_BACKEND STREQUAL "sdl2")
	target_sources(simutrans-extended PRIVATE display/simgraph16.cc sys/simsys_s2.cc sound/sdl2_sound.cc ${BACKUP_MUSIC})
	target_link_libraries(simutrans-extended PRIVATE SDL2::Core)
	target_compile_definitions(simutrans-extended PRIVATE COLOUR_DEPTH=16)

	if (WIN32)
		target_sources(simutrans-extended PRIVATE sys/simsys_w32_png.cc)
	endif ()

elseif (SIMUTRANS_BACKEND STREQUAL "mixer_sdl2")
	target_sources(simutrans-extended PRIVATE display/simgraph16.cc sys/simsys_s2.cc sound/sdl2_mixer_sound.cc music/sdl2_mixer_midi.cc)
	target_link_libraries(simutrans-extended PRIVATE SDL2::Core SDL2::Mixer)
	target_compile_definitions(simutrans-extended PRIVATE COLOUR_DEPTH=16)

	if (WIN32)
		target_sources(simutrans-extended PRIVATE sys/simsys_w32_png.cc)
	endif ()

elseif (SIMUTRANS_BACKEND STREQUAL "gdi")
	target_sources(simutrans-extended PRIVATE display/simgraph16.cc sys/simsys_w.cc sys/simsys_w32_png.cc ${BACKUP_SOUND} ${BACKUP_MUSIC})
	target_link_libraries(simutrans-extended PRIVATE imm32)
	target_compile_definitions(simutrans-extended PRIVATE COLOUR_DEPTH=16)

else ()
	if (NOT SIMUTRANS_BACKEND STREQUAL "none")
		message(WARNING "Unknown backend '${SIMUTRANS_BACKEND}', falling back to headless compilation")
	endif ()

	target_sources(simutrans-extended PRIVATE display/simgraph0.cc sys/simsys_posix.cc sound/no_sound.cc music/no_midi.cc)
	target_compile_definitions(simutrans-extended PRIVATE COLOUR_DEPTH=0)
endif ()


#
# Link dependencies
#
target_link_libraries(simutrans-extended PRIVATE
	ZLIB::ZLIB
	BZip2::BZip2
)

if (WIN32)
	target_link_libraries(simutrans-extended PRIVATE ws2_32 winmm)
endif (WIN32)

if (SIMUTRANS_USE_FREETYPE)
	target_compile_definitions(simutrans-extended PRIVATE USE_FREETYPE=1)
	target_include_directories(simutrans-extended PRIVATE ${FREETYPE_INCLUDE_DIRS})
	target_link_libraries(simutrans-extended PRIVATE ${FREETYPE_LIBRARIES})
endif (SIMUTRANS_USE_FREETYPE)

if (SIMUTRANS_USE_UPNP)
	target_compile_definitions(simutrans-extended PRIVATE USE_UPNP=1)
	target_link_libraries(simutrans-extended PRIVATE MiniUPNP::MiniUPNP)
endif (SIMUTRANS_USE_UPNP)

if (SIMUTRANS_USE_ZSTD)
	target_compile_definitions(simutrans-extended PRIVATE USE_ZSTD=1)
	target_link_libraries(simutrans-extended PRIVATE ZSTD::ZSTD)
endif (SIMUTRANS_USE_ZSTD)


#
# compile options
#
if (SIMUTRANS_BIG_ENDIAN)
	target_compile_definitions(simutrans-extended PRIVATE SIM_BIG_ENDIAN=1)
endif ()

if (SIMUTRANS_VALGRIND_SUPPORT)
	add_definitions(-DUSE_VALGRIND_MEMCHECK=1)
endif ()

if (SIMUTRANS_ENABLE_PROFILING)
	add_definitions(-DPROFILE=1)
endif ()

include(simutrans-compile-options)
target_compile_options(simutrans-extended PRIVATE ${SIMUTRANS_COMMON_COMPILE_OPTIONS})

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
	if (SIMUTRANS_BUILD_32BIT)
		target_compile_options(simutrans-extended PRIVATE -m32)
		set_target_properties(simutrans-extended PROPERTIES LINK_FLAGS "-m32")

	else ()
		# For now, emit a warning when compiling as 64 bit binary
		message(WARNING "Simutrans is preferrably compiled as a 32 bit binary!")
	endif (SIMUTRANS_BUILD_32BIT)
endif (NOT CMAKE_SIZEOF_VOID_P EQUAL 4)


target_compile_definitions(simutrans-extended PRIVATE $<IF:$<CONFIG:Debug>,DEBUG=1,NDEBUG>)
target_compile_definitions(simutrans-extended PRIVATE ENABLE_WATERWAY_SIGNS=0 AUTOJOIN_PUBLIC=0)
target_compile_definitions(simutrans-extended PRIVATE MSG_LEVEL=${SIMUTRANS_MSG_LEVEL})

if (SIMUTRANS_WITH_REVISION)
	target_compile_definitions(simutrans-extended PRIVATE REVISION=${SIMUTRANS_SHA_SHORT})
endif ()

if (SIMUTRANS_USE_SYSLOG)
	target_compile_definitions(simutrans-extended PRIVATE SYSLOG=1)
endif ()

if (NOT SIMUTRANS_USE_IPV6)
	target_compile_definitions(simutrans-extended PRIVATE USE_IP4_ONLY=1)
endif ()

if (NOT SIMUTRANS_ENABLE_RANDOMNESS)
	target_compile_definitions(simutrans-extended PRIVATE DISABLE_RANDOMNESS=1)
endif ()

if (SIMUTRANS_DEBUG_SIMRAND)
	target_compile_definitions(simutrans-extended PRIVATE DEBUG_SIMRAND_CALLS=1)
endif ()

if (APPLE)
	target_compile_definitions(simutrans-extended PRIVATE ALT_SDL_DIR=1)
endif (APPLE)


#
# Installation
#
install(TARGETS simutrans-extended RUNTIME DESTINATION simutrans)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/simutrans/" DESTINATION "simutrans")

install(FILES get_pak.sh DESTINATION "simutrans"
	PERMISSIONS
		OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE
)


#
# Nettool/Makeobj
#
if (PNG_FOUND)
	add_subdirectory(makeobj)
endif ()

add_subdirectory(nettools)



#
# Pakckaging
#

include(InstallRequiredSystemLibraries)

if (WIN32 AND NOT UNIX)
    set(CPACK_GENERATOR "ZIP;NSIS")
else ()
    set(CPACK_GENERATOR "ZIP;TGZ")
endif()

set(SIMUTRANS_PKG_NAME "simutrans-extended")

if (WIN32 AND NOT UNIX)
    set(SIMUTRANS_PKG_PLATFORM "win")
else ()
    set(SIMUTRANS_PKG_PLATFORM "linux")
endif ()

math(EXPR SIMUTRANS_PKG_BITNESS "${CMAKE_SIZEOF_VOID_P} * 8")

# actual configuration
set(SIMUTRANS_VERSION "${SIMUTRANS_SHA_SHORT}-nightly")

set(CPACK_PACKAGE_NAME                ${SIMUTRANS_PKG_NAME})
set(CPACK_PACKAGE_VERSION             "${SIMUTRANS_VERSION}")
set(CPACK_PACKAGE_VENDOR              "Simutrans-Extended developers")
set(CPACK_PACKAGE_CONTACT             "https://forum.simutrans.com/index.php/board,53.0.html")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simutrans-Extended - an extended version of the popular game Simutrans")
set(CPACK_PACKAGE_DESCRIPTION_FILE    "${CMAKE_SOURCE_DIR}/readme.txt")
set(CPACK_RESOURCE_FILE_LICENSE       "${CMAKE_SOURCE_DIR}/LICENSE.txt")

set(CPACK_PACKAGE_FILE_NAME "${SIMUTRANS_PKG_NAME}-${SIMUTRANS_VERSION}-${SIMUTRANS_PKG_PLATFORM}${SIMUTRANS_PKG_BITNESS}")

if (WIN32 AND NOT UNIX)
    # There is a bug in NSI that does not handle full unix paths properly.
    # Make sure there is at least one set of four (4) backlasshes.
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${SIMUTRANS_PKG_NAME}\\\\${SIMUTRANS_VERSION}\\\\")
    set(CPACK_NSIS_DISPLAY_NAME "Simutrans Extended")
    set(CPACK_NSIS_MODIFY_PATH ON)

else (WIN32 AND NOT UNIX)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "")
    set(CPACK_STRIP_FILES "simutrans/simutrans-extended")
    set(CPACK_SOURCE_STRIP_FILES "")

    # deb specific
    string(REGEX REPLACE "^v" "" CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
endif (WIN32 AND NOT UNIX)

include(CPack)
