#
# This file is part of the Simutrans project under the artistic licence.
# (see licence.txt)
#

add_executable(nettool
	nettool.cc
)

target_compile_options(nettool PRIVATE ${SIMUTRANS_COMMON_COMPILE_OPTIONS})
target_compile_definitions(nettool PRIVATE NETTOOL=1 COLOUR_DEPTH=0)
target_compile_definitions(nettool PRIVATE MSG_LEVEL=${SIMUTRANS_MSG_LEVEL})

if (SIMUTRANS_USE_SYSLOG)
	target_compile_definitions(nettool PRIVATE SYSLOG=1)
endif ()

if (NOT SIMUTRANS_USE_IPV6)
	target_compile_definitions(nettool PRIVATE USE_IP4_ONLY=1)
endif ()

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 4 AND SIMUTRANS_BUILD_32BIT)
	target_compile_options(nettool PRIVATE -m32)
	set_target_properties(nettool PROPERTIES LINK_FLAGS "-m32")
endif ()

# these source files produce the same object code in nettool and simutrans
target_sources(nettool PRIVATE
	../dataobj/freelist.cc
	../network/memory_rw.cc
	../network/network_address.cc
	../network/network_cmd.cc
	../network/network_packet.cc
	../network/network_socket_list.cc
	../simdebug.cc
	../simmem.cc
	../utils/simstring.cc
	../utils/fetchopt.cc
	../utils/sha1.cc
	../utils/sha1_hash.cc
)

# these source files produce different object code in nettool and simutrans
target_sources(nettool PRIVATE
	../utils/log.cc
	../network/network.cc
	../network/network_file_transfer.cc
)

if (WIN32)
	target_link_libraries(nettool PRIVATE ws2_32)
endif (WIN32)
