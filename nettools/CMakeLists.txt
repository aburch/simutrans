#
# This file is part of the Simutrans project under the artistic licence.
# (see licence.txt)
#

add_executable(nettool-extended
	nettool.cc
)

target_compile_options(nettool-extended PRIVATE ${SIMUTRANS_COMMON_COMPILE_OPTIONS})
target_compile_definitions(nettool-extended PRIVATE NETTOOL=1 COLOUR_DEPTH=0)

# these source files produce the same object code in nettool and simutrans
target_sources(nettool-extended PRIVATE
	../dataobj/freelist.cc
	../network/memory_rw.cc
	../network/network_address.cc
	../network/network_cmd.cc
	../network/network_packet.cc
	../network/network_socket_list.cc
	../network/pwd_hash.cc
	../simdebug.cc
	../simmem.cc
	../utils/simstring.cc
	../utils/fetchopt.cc
	../utils/sha1.cc
)

# these source files produce different object code in nettool and simutrans
target_sources(nettool-extended PRIVATE
	../utils/log.cc
	../network/network.cc
	../network/network_file_transfer.cc
)

if (WIN32)
       target_link_libraries(nettool-extended PRIVATE ws2_32)
endif (WIN32)
