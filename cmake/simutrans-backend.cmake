#
# This file is part of the Simutrans-Extended project under the Artistic License.
# (see LICENSE.txt)
#

find_package(SDL2 MODULE REQUIRED)


if (SDL2_FOUND)
	list(APPEND AVAILABLE_BACKENDS "sdl2")
	mark_as_advanced(SDL2_DIR)
endif (SDL2_FOUND)

if (WIN32 OR MINGW)
	list(APPEND AVIALIABLE_BACKENDS "gdi")
endif ()

list(APPEND AVAILABLE_BACKENDS "none")

string(REGEX MATCH "^[^;][^;]*" FIRST_BACKEND "${AVAILABLE_BACKENDS}")
set(SIMUTRANS_BACKEND "${FIRST_BACKEND}" CACHE STRING "Graphics backend")
set_property(CACHE SIMUTRANS_BACKEND PROPERTY STRINGS ${AVAILABLE_BACKENDS})

if (SIMUTRANS_BACKEND EQUAL "none")
	set(SIMUTRANS_BUILD_HEADLESS ON)
endif ()
