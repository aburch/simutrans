#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#
# set the correct triplet when compiling on MSVC using VCPKG
#

if (MSVC AND DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif ()

# to built static libaries on windows, we need to modify the VCPKG triplet (if given)
if (DEFINED CMAKE_TOOLCHAIN_FILE)
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
		if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win32")
			set(VCPKG_TARGET_TRIPLET "x86-windows-static" CACHE STRING "Default target is 32bit static builds")

		else ()
			if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "ARM64")
				set(VCPKG_TARGET_TRIPLET "x86-windows-static" CACHE STRING "Default target is 64bit arm static builds")
			else ()
				set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "Default target is 64bit static builds")
			endif ()
		endif ()
	else ()

		# defined, but check if static
		string(FIND ${VCPKG_TARGET_TRIPLET} "-static" STATIC_TRIPLET)
		if (WIN32 AND ${STATIC_TRIPLET} EQUAL -1)
			string(CONCAT STATIC_TRIPLET ${VCPKG_TARGET_TRIPLET} "-static")
			set(VCPKG_TARGET_TRIPLET ${STATIC_TRIPLET} CACHE STRING "Default target is static build" FORCE)
			message( WARNING "Static build preferred on windows => make static target " ${STATIC_TRIPLET} )
		endif ()
	endif ()

else ()
	if (MSVC)
		message(WARNING  "CMake will fail without setting CMAKE_TOOLCHAIN_FILE!")
	endif ()
endif ()
