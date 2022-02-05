#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#
if (CCache_FOUND)
	option(SIMUTRANS_USE_CCACHE "Use CCache compiler cache to improve recompilation speed" ON)
	if (SIMUTRANS_USE_CCACHE)
		set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCache_EXECUTABLE}")
		set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK    "${CCache_EXECUTABLE}")
	endif (SIMUTRANS_USE_CCACHE)
endif (CCache_FOUND)

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
	option(SIMUTRANS_USE_FREETYPE "Enable TrueType font support using freetype library" ON)
endif (Freetype_FOUND)

if (MiniUPNP_FOUND)
	option(SIMUTRANS_USE_UPNP "Use MiniUPNP for easier server setup" ON)
endif (MiniUPNP_FOUND)

if (ZSTD_FOUND)
	option(SIMUTRANS_USE_ZSTD "Enable support for zstd save file compression (larger save files than bzip2, but faster)" ON)
endif (ZSTD_FOUND)

if (FluidSynth_FOUND AND NOT WIN32)
	option(SIMUTRANS_USE_FLUIDSYNTH_MIDI "Enable FluidSynth for MIDI playback" ON)
endif (FluidSynth_FOUND AND NOT WIN32)

option(SIMUTRANS_INSTALL_PAK64 "Download pak64 on install" OFF)
option(SIMUTRANS_ENABLE_PROFILING "Enable profiling code" OFF)
option(SIMUTRANS_USE_SYSLOG "Enable logging to syslog" OFF)
option(SIMUTRANS_USE_IP4_ONLY "Use only IPv4" OFF)
option(SIMUTRANS_STEAM_BUILT "Compile a Steam build" OFF)
option(DEBUG_FLUSH_BUFFER "Highlite areas changes since last redraw" OFF)
option(ENABLE_WATERWAY_SIGNS "Allow private signs on watersways" OFF)
option(AUTOJOIN_PUBLIC "Join when making things public" OFF)

if(NOT SIMUTRANS_DEBUG_LEVEL)
	set(SIMUTRANS_DEBUG_LEVEL $<CONFIG:Debug>)
endif ()

if(NOT SIMUTRANS_MSG_LEVEL)
	set(SIMUTRANS_MSG_LEVEL 3 CACHE STRING "Message verbosity level")
endif ()
set_property(CACHE SIMUTRANS_MSG_LEVEL PROPERTY STRINGS 0 1 2 3 4)

if(OPTION_BUNDLE_LIBRARIES AND UNIX AND NOT APPLE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "\$ORIGIN/lib")
    # otherwise RUNPATH will be set instead of RPATH, which can lead to issues
	add_link_options("-Wl,--disable-new-dtags")
endif()

include(CheckCXXCompilerFlag)

#
# This function adds all supported compiler flags to result_list
# Example: SIMUTRANS_CHECK_CXX_COMPILER_FLAGS(COMMON_COMPILE_OPTIONS -Wall -Wextra -Werror)
# will add -Wall -Wextra -Werror to COMMON_COMPILE_OPTIONS.
#
function(SIMUTRANS_CHECK_CXX_COMPILER_FLAGS result_list)
	set(temp_list "")
	foreach (flag ${ARGN})
		# We cannot check for -Wno-* or -fno-* as this won't throw a warning so we must check for -W* or -f* directly
		string(REGEX REPLACE "^(-[Wf])no-" "\\1" sanitizedFlag ${flag})
		set(cachedVarName ${sanitizedFlag})
		string(REPLACE "+" "X" cachedVarName ${cachedVarName})
		string(REGEX REPLACE "[-=]" "_" cachedVarName ${cachedVarName})

		if (NOT ${CMAKE_CXX_COMPILER_ID}_${cachedVarName}_CHECKED)
			check_cxx_compiler_flag(${sanitizedFlag} CXX_FLAG_${cachedVarName}_SUPPORTED)
			set(${CMAKE_CXX_COMPILER_ID}_${cachedVarName}_CHECKED YES CACHE INTERNAL "")
		endif()

		if (CXX_FLAG_${cachedVarName}_SUPPORTED)
			list(APPEND temp_list ${flag})
		endif (CXX_FLAG_${cachedVarName}_SUPPORTED)

		unset(cachedVarName)
		unset(sanitizedFlag)
	endforeach ()

	if (NOT ${result_list})
		set(${result_list} ${temp_list} PARENT_SCOPE)
	elseif (temp_list)
		set(${result_list} "${${result_list}};${temp_list}" PARENT_SCOPE)
	endif ()
endfunction()

if (MSVC)
	SIMUTRANS_CHECK_CXX_COMPILER_FLAGS(SIMUTRANS_COMMON_COMPILE_OPTIONS
		/W3
4250;4373;4800;4996;26812;26451
		/wd4244  # C4244: 'conversion_type': conversion from 'type1' to 'type2', possible loss of data
		/wd4267  # C4267: '=': conversion from 'type1' to 'type2', possible loss of data
		/wd4068  # C4068: unknown pragma
		/wd4250  # C4250: same name in derived class belong to derived class (silly)
		/wd26812 # Prefer 'enum class' over 'enum' (silly)
		/wd26451 # Arithmetic overflow: Using operator 'operator' on a size-a byte value and then casting the result to a size-b byte value. (widely used in finances)
		/wdD9025
#		/MT # static multithreded libaries
		/MP # parallel builds
	)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")

	foreach(CompilerFlag ${CompilerFlags})
		string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()

	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
	add_definitions(-DNOMINMAX)
	add_definitions(-DWIN32_LEAN_AND_MEAN)

	if (CMAKE_SIZEOF_VOID_P EQUAL 4)
		add_link_options(/LARGEADDRESSAWARE)
	endif ()

else (MSVC) # Assume GCC/Clang
	SIMUTRANS_CHECK_CXX_COMPILER_FLAGS(SIMUTRANS_COMMON_COMPILE_OPTIONS
		-Wall
		-Wextra
		-Wformat=2
		-Wundef
		-Wmissing-include-dirs
		-Wcast-qual
		-Wpointer-arith
		-Wcast-align
		-Walloca
		-Wduplicated-cond
	)

	SIMUTRANS_CHECK_CXX_COMPILER_FLAGS(SIMUTRANS_COMMON_COMPILE_OPTIONS
		-Wno-format-nonliteral       # Mostly for translator
		-Wno-overloaded-virtual      # For makeobj
		-Wno-deprecated-declarations # auto_ptr for squirrel
		-Wno-deprecated-copy         # for squirrel
		-Wno-cast-align              # for squirrel
		-Wno-return-std-move         # for squirrel
	)

	# only add large address linking to 32 bin windows programs
	if (WIN32)
		if (CMAKE_SIZEOF_VOID_P EQUAL 4)
			add_link_options(-Wl,--large-address-aware)
		endif ()
	endif ()

	if (SIMUTRANS_PROFILE)
		SIMUTRANS_CHECK_CXX_COMPILER_FLAGS(SIMUTRANS_COMMON_COMPILE_OPTIONS
			-pg -fno-inline -fno-schedule-insns
		)
	endif (SIMUTRANS_PROFILE)
endif (MSVC)
