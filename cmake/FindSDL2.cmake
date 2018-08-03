#
# This file is part of the Simutrans-Extended project under the Artistic License.
# (see LICENSE.txt)
#

include(FindPackageHandleStandardArgs)

find_package(SDL2 CONFIG)

find_program(SDL2_CONFIG_EXECUTABLE sdl2-config)

if (SDL2_CONFIG_EXECUTABLE)
	execute_process(COMMAND "${SDL2_CONFIG_EXECUTABLE}" --version
		OUTPUT_VARIABLE SDL2_VERSION
	)

	string(STRIP "${SDL2_VERSION}" SDL2_VERSION) # Chop off trailing newline
else (SDL2_CONFIG_EXECUTABLE)
	set(SDL2_VERSION "")
endif (SDL2_CONFIG_EXECUTABLE)


find_package_handle_standard_args(SDL2
	FOUND_VAR SDL2_FOUND
	REQUIRED_VARS SDL2_LIBRARIES SDL2_INCLUDE_DIRS
	VERSION_VAR SDL2_VERSION
)


if (SDL2_FOUND)
	if (NOT TARGET SDL2::SDL2)
		add_library(SDL2::SDL2 UNKNOWN IMPORTED)
	endif (NOT TARGET SDL2::SDL2)

	set_target_properties(SDL2::SDL2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}")
	set_target_properties(SDL2::SDL2 PROPERTIES IMPORTED_LOCATION "${SDL2_LIBRARIES}")
endif (SDL2_FOUND)


