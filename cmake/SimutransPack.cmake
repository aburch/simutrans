#
# This file is part of the Simutrans-Extended project under the Artistic License.
# (see LICENSE.txt)
#

include(InstallRequiredSystemLibraries)

if (WIN32 AND NOT UNIX)
	set(CPACK_GENERATOR "ZIP;NSIS")
else ()
	set(CPACK_GENERATOR "ZIP;TGZ")
endif()

set(SIMUTRANS_PKG_NAME "simutrans-extended")

if (WIN32 AND NOT UNIX)
	set(SIMUTRANS_PKG_PLATFORM "win")
elseif (APPLE)
	set(SIMUTRANS_PKG_PLATFORM "mac")
else ()
    set(SIMUTRANS_PKG_PLATFORM "linux")
endif ()

math(EXPR SIMUTRANS_PKG_BITNESS "${CMAKE_SIZEOF_VOID_P} * 8")

# actual configuration
set(SIMUTRANS_VERSION "${SIMUTRANS_SHA_SHORT}-nightly")

set(CPACK_PACKAGE_NAME                ${SIMUTRANS_PKG_NAME})
set(CPACK_PACKAGE_VERSION             "${SIMUTRANS_VERSION}")
set(CPACK_PACKAGE_VENDOR              "Simutrans-Extended developers")
set(CPACK_PACKAGE_CONTACT             "https://forum.simutrans.com/index.php/board,53.0.html")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simutrans-Extended - an extended version of the popular game Simutrans")
set(CPACK_PACKAGE_DESCRIPTION_FILE    "${CMAKE_SOURCE_DIR}/readme.txt")
set(CPACK_RESOURCE_FILE_LICENSE       "${CMAKE_SOURCE_DIR}/LICENSE.txt")

set(CPACK_PACKAGE_FILE_NAME "${SIMUTRANS_PKG_NAME}-${SIMUTRANS_VERSION}-${SIMUTRANS_PKG_PLATFORM}${SIMUTRANS_PKG_BITNESS}")

if (WIN32 AND NOT UNIX)
	# There is a bug in NSI that does not handle full unix paths properly.
	# Make sure there is at least one set of four (4) backslashes.
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "${SIMUTRANS_PKG_NAME}\\\\${SIMUTRANS_VERSION}\\\\")
	set(CPACK_NSIS_DISPLAY_NAME "Simutrans Extended")
	set(CPACK_NSIS_MODIFY_PATH ON)

else (WIN32 AND NOT UNIX)
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "")
	set(CPACK_STRIP_FILES "simutrans/simutrans-extended")
	set(CPACK_SOURCE_STRIP_FILES "")

	# deb specific
	string(REGEX REPLACE "^v" "" CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
endif (WIN32 AND NOT UNIX)

include(CPack)
