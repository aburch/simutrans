#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

cmake_minimum_required(VERSION 3.16)

# Disable in-source builds
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "Building Simutrans in-source is not supported. "
		"Please delete ${CMAKE_SOURCE_DIR}/CMakeCache.txt and configure in a different "
		"(preferrably empty) directory.")
endif (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

set(CMAKE_WARN_DEPRECATED ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

include(SimutransVcpkgTriplet)

project(simutrans LANGUAGES C CXX)

# Force C++14 everywhere
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)

include(TestBigEndian)
TEST_BIG_ENDIAN(SIMUTRANS_BIG_ENDIAN)

if (MINGW)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ON)
	set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
endif ()

if (APPLE)
	list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/readline")
	link_directories(/usr/local/lib)
	set(CMAKE_EXE_LINKER_FLAGS "-framework Cocoa")
endif ()


#
# Dependencies
#
find_package(CCache)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(PNG REQUIRED)
find_package(MiniUPNP)
if (MSVC)
	find_package(ZSTD)
	find_package(SDL2)
	find_package(Freetype)
	find_package(FluidSynth)
else ()
	find_package(PkgConfig MODULE REQUIRED)
	pkg_check_modules(ZSTD IMPORTED_TARGET libzstd)
	pkg_check_modules(SDL2 IMPORTED_TARGET sdl2)
	pkg_check_modules(Freetype IMPORTED_TARGET freetype2)
	pkg_check_modules(FluidSynth IMPORTED_TARGET fluidsynth>=2.1.0)
endif ()

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads)

#
# Configuration options
#
include(SimutransBackend) # Select backend (SDL2 / GDI)
include(SimutransCompileOptions)


# CMake does not fill in CMAKE_BUILD_TYPE automatically.
if (NOT MSVC)
	if (CMAKE_BUILD_TYPE STREQUAL "")
		set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type. Valid values are Debug Release MinSizeRel RelWithDebInfo" FORCE)
	endif ()
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)
endif (NOT MSVC)


#
# sources
#
add_executable(simutrans MACOSX_BUNDLE)
if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	set_target_properties(simutrans PROPERTIES WIN32_EXECUTABLE 1)
endif ()

include(SimutransSourceList)


#
# output directory
#
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/simutrans)
set_target_properties( simutrans PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/simutrans/$<0:> )


#
# Generate revision on compile time (so only if needed)
#
string(FIND ${CMAKE_GENERATOR} "Visual Studio" VS )
if (${VS} EQUAL 0)
	# using a script for MSVC project files
	file(WRITE ${CMAKE_SOURCE_DIR}/revision.h "#define REVISION \n")
	add_custom_command(TARGET simutrans PRE_BUILD WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND cscript.exe /Nologo revision.jse COMMENT "Find SVN revision")
else ()
	# using custom target svnhear that is always built to update the revision.h file using cmake script
	add_custom_target(svnheader ALL DEPENDS svn_header)
	add_custom_command(OUTPUT svn_header COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_MODULE_PATH}/SimutransRevision.cmake)
	set_source_files_properties(${CMAKE_SOURCE_DIR}/revision.h PROPERTIES GENERATED TRUE HEADER_FILE_ONLY TRUE)
	add_dependencies(simutrans svnheader)
endif ()

#
# Libraries
#

target_link_libraries(simutrans PRIVATE PNG::PNG)

if (WIN32)
	target_sources(simutrans PRIVATE src/Windows/simres.rc)
	target_sources(simutrans PRIVATE src/simutrans/sys/clipboard_w32.cc)
	target_link_libraries(simutrans PRIVATE ws2_32 winmm)

	# Backup sound and music routines if the selected backend does not provide any routines
	set(BACKUP_SOUND src/simutrans/sound/win32_sound_xa.cc)
	set(BACKUP_MUSIC src/simutrans/music/w32_midi.cc)
else ()
	if (SIMUTRANS_BACKEND STREQUAL "sdl2")
		target_sources(simutrans PRIVATE src/simutrans/sys/clipboard_s2.cc)
	else ()
		target_sources(simutrans PRIVATE src/simutrans/sys/clipboard_internal.cc)
	endif ()
	if (APPLE)
		target_sources(simutrans PRIVATE src/OSX/translocation.m)
		#Darwin kernel version is used to identify macOS version (16->10.12/Sierra)
		if (CMAKE_HOST_SYSTEM_VERSION VERSION_GREATER_EQUAL 16) #AVF
			set(BACKUP_SOUND src/simutrans/sound/AVF_core-audio_sound.mm)
			#AVF MIDI currently crash Simutrans
			#set(BACKUP_MUSIC src/simutrans/music/AVF_core-audio_midi.mm)
			set(BACKUP_MUSIC src/simutrans/music/no_midi.cc)
		else () #QTKit
			set(BACKUP_SOUND src/simutrans/sound/core-audio_sound.mm)
			set(BACKUP_MUSIC src/simutrans/music/core-audio_midi.mm)
		endif ()
	else ()
		set(BACKUP_SOUND src/simutrans/sound/no_sound.cc)
		set(BACKUP_MUSIC src/simutrans/music/no_midi.cc)
	endif ()
endif ()

if (SIMUTRANS_MULTI_THREAD)
	target_compile_definitions(simutrans PRIVATE MULTI_THREAD=1)
	target_link_libraries(simutrans PRIVATE Threads::Threads)
endif (SIMUTRANS_MULTI_THREAD)

if (SIMUTRANS_BACKEND STREQUAL "sdl2")
	target_sources(simutrans PRIVATE src/simutrans/display/simgraph16.cc src/simutrans/sys/simsys_s2.cc src/simutrans/sound/sdl2_sound.cc)
	target_include_directories(simutrans PRIVATE ${SDL2_INCLUDE_DIRS})
	if (MINGW)
		target_link_libraries(simutrans PRIVATE ${SDL2_STATIC_LIBRARIES})
	elseif (APPLE)
		target_link_libraries(simutrans PRIVATE ${SDL2_LIBRARIES})
	elseif (MSVC)
		target_link_libraries(simutrans PRIVATE SDL2::SDL2)
	else ()
		target_link_libraries(simutrans PRIVATE PkgConfig::SDL2)
	endif ()
	target_compile_definitions(simutrans PRIVATE COLOUR_DEPTH=16)

elseif (SIMUTRANS_BACKEND STREQUAL "gdi")
	target_sources(simutrans PRIVATE src/simutrans/display/simgraph16.cc src/simutrans/sys/simsys_w.cc ${BACKUP_SOUND} )
	target_link_libraries(simutrans PRIVATE imm32 xaudio2_8)
	target_compile_definitions(simutrans PRIVATE COLOUR_DEPTH=16)

else ()
	if (NOT SIMUTRANS_BACKEND STREQUAL "none")
		message(WARNING "Unknown backend '${SIMUTRANS_BACKEND}', falling back to headless compilation")
	endif ()

	target_sources(simutrans PRIVATE src/simutrans/display/simgraph0.cc src/simutrans/sys/simsys_posix.cc src/simutrans/sound/no_sound.cc src/simutrans/music/no_midi.cc)
	target_compile_definitions(simutrans PRIVATE COLOUR_DEPTH=0)
endif ()


#
# Link dependencies
#
target_link_libraries(simutrans PRIVATE ZLIB::ZLIB)
target_link_libraries(simutrans PRIVATE BZip2::BZip2)

if (SIMUTRANS_USE_FREETYPE)
	target_include_directories(simutrans PRIVATE ${Freetype_INCLUDE_DIRS})
	if (MINGW)
		target_link_libraries(simutrans PRIVATE ${Freetype_STATIC_LIBRARIES})
	elseif (APPLE)
		target_link_libraries(simutrans PRIVATE ${Freetype_LIBRARIES})
	elseif (MSVC)
		target_link_libraries(simutrans PRIVATE Freetype::Freetype)
	else ()
		target_link_libraries(simutrans PRIVATE PkgConfig::Freetype)
	endif ()
	target_compile_definitions(simutrans PRIVATE USE_FREETYPE=1)
endif (SIMUTRANS_USE_FREETYPE)

if (SIMUTRANS_USE_UPNP)
	target_compile_definitions(simutrans PRIVATE USE_UPNP=1)
	target_link_libraries(simutrans PRIVATE MiniUPNP::MiniUPNP)
endif (SIMUTRANS_USE_UPNP)

if (SIMUTRANS_USE_ZSTD)
	target_sources(simutrans PRIVATE src/simutrans/io/rdwr/zstd_file_rdwr_stream.cc)
	target_include_directories(simutrans PRIVATE ${ZSTD_INCLUDE_DIRS})
	target_compile_definitions(simutrans PRIVATE USE_ZSTD=1)
	if (MSVC)
		target_link_libraries(simutrans PRIVATE zstd::libzstd_static)
	elseif (MINGW)
		# only 64 bit cmake, 32 bit is broken
		target_link_libraries(simutrans PRIVATE -lzstd)
	else()
		target_link_libraries(simutrans PRIVATE PkgConfig::ZSTD)
	endif ()
endif (SIMUTRANS_USE_ZSTD)

# only consider FluidSynth for SDL2 and GDI backends
if ((SIMUTRANS_BACKEND STREQUAL "sdl2") OR (SIMUTRANS_BACKEND STREQUAL "gdi"))
	if (SIMUTRANS_USE_FLUIDSYNTH_MIDI)
		target_sources(simutrans PRIVATE src/simutrans/music/fluidsynth.cc src/simutrans/gui/loadsoundfont_frame.cc)
		target_compile_definitions(simutrans PRIVATE USE_FLUIDSYNTH_MIDI=1)
		if (MINGW)
			# since fluidsynth 2.2.2 fluidsynth.pc include static libraries, but some are still missing
			target_link_libraries(simutrans PRIVATE ${FluidSynth_STATIC_LIBRARIES} -liconv -lgomp)
		elseif (MSVC)
			target_link_libraries(simutrans PRIVATE ${FluidSynth_LIBRARIES})
		else ()
			target_link_libraries(simutrans PRIVATE PkgConfig::FluidSynth)
		endif (MINGW)
	else ()
		target_sources( simutrans PRIVATE ${BACKUP_MUSIC} )
	endif (SIMUTRANS_USE_FLUIDSYNTH_MIDI)
endif ((SIMUTRANS_BACKEND STREQUAL "sdl2") OR (SIMUTRANS_BACKEND STREQUAL "gdi"))


#
# compile options
#
if (MSVC)
	target_link_options(simutrans PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:LIBCMTD /MANIFEST:NO /LARGEADDRESSAWARE)
endif ()

if (SIMUTRANS_BIG_ENDIAN)
	target_compile_definitions(simutrans PRIVATE SIM_BIG_ENDIAN=1)
endif ()

if (SIMUTRANS_VALGRIND_SUPPORT)
	add_definitions(-DUSE_VALGRIND_MEMCHECK=1)
endif ()

if (SIMUTRANS_ENABLE_PROFILING)
	add_definitions(-DPROFILE=1)
endif ()

target_compile_options(simutrans PRIVATE ${SIMUTRANS_COMMON_COMPILE_OPTIONS})

target_compile_definitions(simutrans PRIVATE $<IF:$<CONFIG:Debug>,DEBUG=1,>)
target_compile_definitions(simutrans PRIVATE $<IF:$<CONFIG:Release>,NDEBUG=1,>)

target_compile_definitions(simutrans PRIVATE MSG_LEVEL=${SIMUTRANS_MSG_LEVEL})

if (SIMUTRANS_AUTOJOIN_PUBLIC)
	target_compile_definitions(simutrans PRIVATE AUTOJOIN_PUBLIC=1)
endif ()

if (SIMUTRANS_ENABLE_WATERWAY_SIGNS)
	target_compile_definitions(simutrans PRIVATE ENABLE_WATERWAY_SIGNS=1)
endif ()

if (SIMUTRANS_USE_SYSLOG)
	target_compile_definitions(simutrans PRIVATE SYSLOG=1)
endif ()

if (SIMUTRANS_DEBUG_FLUSH_BUFFER)
	target_compile_definitions(simutrans PRIVATE DEBUG_FLUSH_BUFFER=1)
endif ()

if (SIMUTRANS_USE_IP4_ONLY)
	target_compile_definitions(simutrans PRIVATE USE_IP4_ONLY=1)
endif ()

if (SIMUTRANS_STEAM_BUILT)
	target_compile_definitions(simutrans PRIVATE STEAM_BUILT=1)
endif ()

if (APPLE)
	target_compile_definitions(simutrans PRIVATE ALT_SDL_DIR=1)
endif (APPLE)

#
# Package
#
if (APPLE)
	include(MacBundle)
endif ()


#
# Tests
#
add_custom_target(test
	$<TARGET_FILE:simutrans>
		-set_workdir ${CMAKE_CURRENT_SOURCE_DIR}/simutrans
		-objects pak
		-scenario automated-tests
		-debug 2
		-lang en
		-fps 100
	DEPENDS simutrans
)


#
# Installation
#
install(TARGETS simutrans RUNTIME DESTINATION "${CMAKE_BINARY_DIR}/simutrans/" BUNDLE DESTINATION "${CMAKE_BINARY_DIR}/simutrans/")

install(DIRECTORY "${CMAKE_SOURCE_DIR}/simutrans" DESTINATION "${CMAKE_BINARY_DIR}")

# download language files
if (MSVC)
	# MSVC has no variable on the install target path at execution time, which is why we expand the directories at creation time!
	install(CODE "execute_process(COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/tools/get_lang_files.ps1 WORKING_DIRECTORY ${simutrans_BINARY_DIR})")
else ()
	install(CODE "execute_process(COMMAND sh ${CMAKE_SOURCE_DIR}/tools/get_lang_files.sh WORKING_DIRECTORY \${CMAKE_BINARY_DIR})")
endif ()

# pak installer
if (NOT WIN32)
	install(FILES ${CMAKE_SOURCE_DIR}/tools/get_pak.sh DESTINATION "${CMAKE_BINARY_DIR}/simutrans/"
		PERMISSIONS
			OWNER_READ OWNER_WRITE OWNER_EXECUTE
			GROUP_READ GROUP_EXECUTE
			WORLD_READ WORLD_EXECUTE
	)
else ()
	# NSIS must be installed manually in the path with the right addons
	if(MINGW)
		install(CODE "execute_process(COMMAND makensis onlineupgrade.nsi WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/Windows/nsis)")
	else ()
		install(CODE "execute_process(COMMAND cmd /k \"$ENV{ProgramFiles\(x86\)}/NSIS/makensis.exe\" onlineupgrade.nsi WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/Windows/nsis)")
	endif ()
	install(FILES "${CMAKE_SOURCE_DIR}/src/Windows/nsis/download-paksets.exe" DESTINATION "${CMAKE_BINARY_DIR}/simutrans")
endif ()

# install pak if needed
if (SIMUTRANS_INSTALL_PAK64)
	if (MSVC)
		install(CODE
		"if(NOT EXISTS ${simutrans_BINARY_DIR}/simutrans/pak)
		execute_process(COMMAND powershell -Command \"Remove-Item \'${simutrans_BINARY_DIR}/simutrans/pak\' -Recurse\" WORKING_DIRECTORY ${simutrans_BINARY_DIR})
		file(STRINGS ${CMAKE_SOURCE_DIR}/src/simutrans/paksetinfo.h URLpak64 REGEX \"/pak64/\")
		string( REGEX REPLACE \"^.[\\t ]*\\\"\" \"\" URLpak64 \${URLpak64})
		string( REGEX REPLACE \"\\\", .*\$\" \"\" URLpak64 \${URLpak64})
		message(\"install pak to \" ${simutrans_BINARY_DIR})
		execute_process(COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/tools/get_pak.ps1 \${URLpak64} WORKING_DIRECTORY ${simutrans_BINARY_DIR}/simutrans)
		endif ()
		")
	else ()
		# install pak64 with the bundle
		install(CODE
		"file(STRINGS  ${CMAKE_SOURCE_DIR}/src/simutrans/paksetinfo.h URLpak64 REGEX \"/pak64/\")
		 string( REGEX REPLACE \"^.[\\t ]*\\\"\" \"\" URLpak64 \${URLpak64})
		 string( REGEX REPLACE \"\\\", .*\$\" \"\" URLpak64 \${URLpak64})
		 execute_process(COMMAND sh ../../${CMAKE_SOURCE_DIR}/tools/get_pak.sh \${URLpak64} WORKING_DIRECTORY \${CMAKE_BINARY_DIR}/simutrans)
		")
	endif ()
endif ()

# bundle libraries on linux if requested
if(OPTION_BUNDLE_LIBRARIES AND UNIX AND NOT APPLE)
	# Steam Runtime already includes some of our libraries
	if (SIMUTRANS_STEAM_BUILT)
		install(CODE [[
			file(GET_RUNTIME_DEPENDENCIES
					RESOLVED_DEPENDENCIES_VAR DEPENDENCIES
					EXECUTABLES "${CMAKE_BINARY_DIR}/simutrans/simutrans"
					PRE_EXCLUDE_REGEXES "libSDL2*|libz.so*|libfreetype*|libpng*|libglib*|libogg*|libpcre*|libvorbis*"
					POST_EXCLUDE_REGEXES "ld-linux|libc.so|libdl.so|libm.so|libgcc_s.so|libpthread.so|libstdc...so|libgomp.so")
		]])
	else ()
		install(CODE [[
			file(GET_RUNTIME_DEPENDENCIES
					RESOLVED_DEPENDENCIES_VAR DEPENDENCIES
					EXECUTABLES "${CMAKE_BINARY_DIR}/simutrans/simutrans"
					PRE_EXCLUDE_REGEXES "libSDL2*|libz.so*"
					POST_EXCLUDE_REGEXES "ld-linux|libc.so|libdl.so|libm.so|libgcc_s.so|libpthread.so|libstdc...so|libgomp.so")
		]])
	endif ()
	install(CODE [[
		file(INSTALL
				DESTINATION "${CMAKE_BINARY_DIR}/simutrans/lib"
				FILES ${DEPENDENCIES}
				FOLLOW_SYMLINK_CHAIN)
	]])
endif ()

#
# Nettool/Makeobj
#
add_subdirectory(src/makeobj EXCLUDE_FROM_ALL)
add_subdirectory(src/nettool EXCLUDE_FROM_ALL)


