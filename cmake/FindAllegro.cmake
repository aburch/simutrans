#
# This file is part of the Simutrans-Extended project under the Artistic License.
# (see LICENSE.txt)
#
#
# - Find Allegro graphics library.
#
# This module defines the following variables:
#   Allegro_FOUND           - true if Allegro was found
#   Allegro_VERSION_STRING  - version string of Allegro library
#   Allegro_INCLUDE_DIRS    - Include directories needed for Allegro
#   Allegro_LIBRARIES       - Libraries to link to when using Allegro
#
# Additionally, this module defines the IMPORTED target Allegro::Allegro,
# if Allegro has been found.
#

include(FindPackageHandleStandardArgs)

if (Allegro_INCLUDE_DIRS AND Allegro_LIBRARIES)
	# already in cache, be silent
	set(Allegro_FIND_QUIETLY TRUE)
endif (Allegro_INCLUDE_DIRS AND Allegro_LIBRARIES)

find_path(Allegro_INCLUDE_DIR allegro.h
	/usr/local/include
	/usr/include
	$ENV{MINGDIR}/include
)

set(ALLEGRO_NAMES alleg alleglib allegdll)
find_library(Allegro_LIBRARY
	NAMES ${ALLEGRO_NAMES}
	PATHS /usr/local/lib /usr/lib $ENV{MINGDIR}/lib
)

find_program(Allegro_CONFIG NAMES allegro-config)

if (Allegro_CONFIG)
	execute_process(
		COMMAND "${Allegro_CONFIG}" --version
		OUTPUT_VARIABLE Allegro_VERSION_STRING
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif (Allegro_CONFIG)

find_package_handle_standard_args(Allegro
	FOUND_VAR Allegro_FOUND
	REQUIRED_VARS Allegro_LIBRARY Allegro_INCLUDE_DIR
	VERSION_VAR Allegro_VERSION_STRING
)

set(Allegro_INCLUDE_DIRS "${Allegro_INCLUDE_DIR}")
set(Allegro_LIBRARIES "${Allegro_LIBRARY}")

if (Allegro_FOUND OR NOT Allegro_FIND_REQUIRED)
	# Only show variables when Allegro is required and not found
	mark_as_advanced(Allegro_INCLUDE_DIRS Allegro_LIBRARIES Allegro_CONFIG Allegro_INCLUDE_DIR Allegro_LIBRARY)
endif (Allegro_FOUND OR NOT Allegro_FIND_REQUIRED)

if (Allegro_FOUND AND NOT TARGET Allegro::Allegro)
	add_library(Allegro::Allegro UNKNOWN IMPORTED)

	set_target_properties(Allegro::Allegro PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Allegro_INCLUDE_DIRS}")
	set_property(TARGET Allegro::Allegro APPEND PROPERTY IMPORTED_LOCATION "${Allegro_LIBRARIES}")
endif (Allegro_FOUND AND NOT TARGET Allegro::Allegro)
