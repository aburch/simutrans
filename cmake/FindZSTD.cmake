#
# This file is part of the Simutrans-Extended project under the Artistic License.
# (see LICENSE.txt)
#
#
# - Find zstd compression library.
#
# This module defines the following variables:
#   ZSTD_FOUND           - true if zstd was found
#   ZSTD_INCLUDE_DIRS    - Include directories needed for zstd
#   ZSTD_LIBRARIES       - Libraries to link to when using zstd
#
# Additionally, this module defines the IMPORTED target ZSTD::ZSTD,
# if zstd has been found.
#

include(FindPackageHandleStandardArgs)

if (ZSTD_INCLUDE_DIRS AND ZSTD_LIBRARIES)
	# already in cache, be silent
	set(ZSTD_FIND_QUIETLY TRUE)
endif (ZSTD_INCLUDE_DIRS AND ZSTD_LIBRARIES)

find_path(ZSTD_INCLUDE_DIR zstd.h
	/usr/local/include
	/usr/include
	$ENV{MINGDIR}/include
)

set(ZSTD_NAMES zstd)
find_library(ZSTD_LIBRARY
	NAMES ${ZSTD_NAMES}
	PATHS /usr/local/lib /usr/lib $ENV{MINGDIR}/lib
)

find_package_handle_standard_args(ZSTD
	FOUND_VAR ZSTD_FOUND
	REQUIRED_VARS ZSTD_LIBRARY ZSTD_INCLUDE_DIR
)

set(ZSTD_INCLUDE_DIRS "${ZSTD_INCLUDE_DIR}")
set(ZSTD_LIBRARIES "${ZSTD_LIBRARY}")

if (ZSTD_FOUND OR NOT ZSTD_FIND_REQUIRED)
	# Only show variables when zstd is required and not found
	mark_as_advanced(ZSTD_INCLUDE_DIRS ZSTD_LIBRARIES ZSTD_INCLUDE_DIR ZSTD_LIBRARY)
endif (ZSTD_FOUND OR NOT ZSTD_FIND_REQUIRED)

if (ZSTD_FOUND AND NOT TARGET ZSTD::ZSTD)
	add_library(ZSTD::ZSTD UNKNOWN IMPORTED)

	set_target_properties(ZSTD::ZSTD PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIRS}")
	set_property(TARGET ZSTD::ZSTD APPEND PROPERTY IMPORTED_LOCATION "${ZSTD_LIBRARIES}")
endif (ZSTD_FOUND AND NOT TARGET ZSTD::ZSTD)
