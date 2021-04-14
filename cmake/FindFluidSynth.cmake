#
# This file is part of the Simutrans-Extended project under the Artistic License.
# (see LICENSE.txt)
#
#
# - Find fluidsynth midi synthetizer library.
#
# This module defines the following variables:
#   FluidSynth_FOUND           - true if fluidsynth was found
#   FluidSynth_INCLUDE_DIRS    - Include directories needed for fluidsynth
#   FluidSynth_LIBRARIES       - Libraries to link to when using fluidsynth
#
# Additionally, this module defines the IMPORTED target FluidSynth::FluidSynth,
# if fluidsynth has been found.
#

include(FindPackageHandleStandardArgs)

if (FluidSynth_INCLUDE_DIRS AND FluidSynth_LIBRARIES)
	# already in cache, be silent
	set(FluidSynth_FIND_QUIETLY TRUE)
endif (FluidSynth_INCLUDE_DIRS AND FluidSynth_LIBRARIES)

find_path(FluidSynth_INCLUDE_DIR fluidsynth.h
	/usr/local/include
	/usr/include
	$ENV{MINGDIR}/include
)

set(FluidSynth_NAMES fluidsynth)
find_library(FluidSynth_LIBRARY
	NAMES ${FluidSynth_NAMES}
	PATHS /usr/local/lib /usr/lib $ENV{MINGDIR}/lib
)

find_package_handle_standard_args(FluidSynth
	FOUND_VAR FluidSynth_FOUND
	REQUIRED_VARS FluidSynth_LIBRARY FluidSynth_INCLUDE_DIR
)

set(FluidSynth_INCLUDE_DIRS "${FluidSynth_INCLUDE_DIR}")
set(FluidSynth_LIBRARIES "${FluidSynth_LIBRARY}")

if (FluidSynth_FOUND OR NOT FluidSynth_FIND_REQUIRED)
	# Only show variables when fluidsynth is required and not found
	mark_as_advanced(FluidSynth_INCLUDE_DIRS FluidSynth_LIBRARIES FluidSynth_INCLUDE_DIR FluidSynth_LIBRARY)
endif (FluidSynth_FOUND OR NOT FluidSynth_FIND_REQUIRED)

if (FluidSynth_FOUND AND NOT TARGET FluidSynth::FluidSynth)
	add_library(FluidSynth::FluidSynth UNKNOWN IMPORTED)

	set_target_properties(FluidSynth::FluidSynth PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FluidSynth_INCLUDE_DIRS}")
	set_property(TARGET FluidSynth::FluidSynth APPEND PROPERTY IMPORTED_LOCATION "${FluidSynth_LIBRARIES}")
endif (FluidSynth_FOUND AND NOT TARGET FluidSynth::FluidSynth)
