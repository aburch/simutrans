#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#
#
# Locate MiniUPNP library.
# This module defines
#  MiniUPNP_FOUND, if miniupnp library and headers have been found
#  MiniUPNP_LIBRARY, the miniupnp variant
#  MiniUPNP_INCLUDE_DIR, where to find miniupnpc.h and family)
#  MiniUPNP_VERSION, the API version of MiniUPNP
#


if (MiniUPNP_INCLUDE_DIR AND MiniUPNP_LIBRARY)
	# Already in cache, be silent
	set(MiniUPNP_FIND_QUIETLY TRUE)
endif ()

find_path(MiniUPNP_INCLUDE_DIR miniupnpc.h
	HINTS $ENV{MINIUPNP_INCLUDE_DIR}
	PATH_SUFFIXES miniupnpc
)

find_library(MiniUPNP_LIBRARY miniupnpc
	HINTS $ENV{MINIUPNP_LIBRARY}
)

find_library(MiniUPNP_STATIC_LIBRARY libminiupnpc.a
	HINTS $ENV{MINIUPNP_STATIC_LIBRARY}
)

set(MiniUPNP_INCLUDE_DIRS ${MiniUPNP_INCLUDE_DIR})
set(MiniUPNP_LIBRARIES ${MiniUPNP_LIBRARY})
set(MiniUPNP_STATIC_LIBRARIES ${MiniUPNP_STATIC_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	MiniUPNP DEFAULT_MSG
	MiniUPNP_LIBRARY
	MiniUPNP_INCLUDE_DIR
)

if (MiniUPNP_FOUND)
	file(STRINGS "${MiniUPNP_INCLUDE_DIR}/miniupnpc.h" MiniUPNP_API_VERSION_STR REGEX "^#define[\t ]+MINIUPNPC_API_VERSION[\t ]+[0-9]+")
	if (MiniUPNP_API_VERSION_STR MATCHES "^#define[\t ]+MINIUPNPC_API_VERSION[\t ]+([0-9]+)")
		set(MiniUPNP_API_VERSION "${CMAKE_MATCH_1}")
	endif()

	if (MiniUPNP_API_VERSION GREATER_EQUAL 10 AND NOT MiniUPNP_FIND_QUIETLY)
		message(STATUS "Found MiniUPNP API version ${MiniUPNP_API_VERSION}")
	endif()
endif()

mark_as_advanced(MiniUPNP_INCLUDE_DIR MiniUPNP_LIBRARY MiniUPNP_STATIC_LIBRARY)

if (MiniUPNP_FOUND)
	if (NOT TARGET MiniUPNP::MiniUPNP)
		add_library(MiniUPNP::MiniUPNP UNKNOWN IMPORTED)
	endif (NOT TARGET MiniUPNP::MiniUPNP)

	set_target_properties(MiniUPNP::MiniUPNP PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${MiniUPNP_INCLUDE_DIRS}")
	set_target_properties(MiniUPNP::MiniUPNP PROPERTIES IMPORTED_LOCATION "${MiniUPNP_LIBRARIES}")
	if (MINGW)
	    # MinGW needs an additional link flags 
	    set_property(TARGET MiniUPNP::MiniUPNP PROPERTY INTERFACE_LINK_LIBRARIES "-liphlpapi -lws2_32")
	endif (MINGW)
	if (MSVC)
	    set_property(TARGET MiniUPNP::MiniUPNP PROPERTY INTERFACE_LINK_LIBRARIES "iphlpapi.lib")
	endif (MSVC)
endif (MiniUPNP_FOUND)

