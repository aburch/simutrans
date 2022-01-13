#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#
# set the correct triplet when compiling on MSVC using VCPKG
#

# MSVC variable is not defined until the first call to project
if (CMAKE_GENERATOR MATCHES "Visual Studio.*" OR CMAKE_GENERATOR MATCHES "Ninja" AND WIN32)
	if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "CMAKE_TOOLCHAIN_FILE")
	elseif (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
		set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/build/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "CMAKE_TOOLCHAIN_FILE")
	endif ()
endif ()


# to built static libaries on vcpkg (exists on Linux too), we need to modify the VCPKG triplet (only if vcpkg)
STRING(TOLOWER "${CMAKE_TOOLCHAIN_FILE}" CMAKE_TOOLCHAIN_FILE_LC)
if (CMAKE_TOOLCHAIN_FILE_LC MATCHES ".*vcpkg.cmake")
	if (CMAKE_GENERATOR MATCHES "Visual Studio.*" AND "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "")
		# default is 32bit for now
		message( WARNING "No architecture givem => assume x86 build")
		set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "" FORCE)
		set(VCPKG_TARGET_TRIPLET "x86-windows-static" CACHE STRING "Default target is 32bit static builds" FORCE)
	endif ()

	if (DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
		set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
	endif ()
	
	# if not defined, then guess
	if (NOT DEFINED VCPKG_TARGET_TRIPLET)
		if (DEFINED CMAKE_INSTALL_PREFIX)
			if (CMAKE_INSTALL_PREFIX MATCHES ".*x86-.*")
				set(VCPKG_TARGET_TRIPLET "x86-windows-static" CACHE STRING "Default target is 32bit static builds")
			elseif (CMAKE_INSTALL_PREFIX MATCHES ".*arm64-.*")
					set(VCPKG_TARGET_TRIPLET "arm64-windows-static" CACHE STRING "Default target is 64bit arm static builds")
			else ()
					set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "Default target is 64bit static builds")
			endif ()
		elseif (CMAKE_GENERATOR MATCHES "Visual Studio.*")
			if (CMAKE_GENERATOR_PLATFORM MATCHES "Win32")
				set(VCPKG_TARGET_TRIPLET "x86-windows-static" CACHE STRING "Default target is 32bit static builds")
			elseif (CMAKE_GENERATOR_PLATFORM MATCHES "ARM64")
					set(VCPKG_TARGET_TRIPLET "arm64-windows-static" CACHE STRING "Default target is 64bit arm static builds")
			else ()
					set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "Default target is 64bit static builds")
			endif ()
		else ()
			message(FATAL_ERROR "Please specify VCPKG triplet!")
		endif ()
	else ()

		# defined, but check if static
		if (WIN32 AND NOT VCPKG_TARGET_TRIPLET MATCHES ".*-static")
			string(CONCAT STATIC_TRIPLET ${VCPKG_TARGET_TRIPLET} "-static")
			set(VCPKG_TARGET_TRIPLET ${STATIC_TRIPLET} CACHE STRING "Default target is static build" FORCE)
			message( WARNING "Static build preferred on windows => make static target " ${STATIC_TRIPLET} )
		endif ()
	endif ()
 	message( "-- VCPKG: triplet=" ${VCPKG_TARGET_TRIPLET} " platform=" ${CMAKE_GENERATOR_PLATFORM}) 
else ()
	if (CMAKE_GENERATOR MATCHES "Visual Studio.*" OR CMAKE_GENERATOR MATCHES "Ninja" AND WIN32)
		message(WARNING  "CMake will fail without setting CMAKE_TOOLCHAIN_FILE!")
	endif ()
endif ()
