# Set copyright text
string(TIMESTAMP YEAR "%Y")
set(COPYRIGHT "Copyright 1997-${YEAR} by the Simutrans Team")

# Get version number
file(READ ${SOURCE_DIR}src/simutrans/simversion.h VERSION_FILE)

string(REGEX MATCH "VERSION_MAJOR ([0-9]*)" _ ${VERSION_FILE})
set(VERSION ${CMAKE_MATCH_1})

# Copy the icon file
set(ICON ${SOURCE_DIR}src/OSX/simutrans.icns)
target_sources(simutrans PRIVATE ${ICON})
set_source_files_properties(${ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

# Bundle information
set_target_properties(simutrans PROPERTIES
	MACOSX_BUNDLE_BUNDLE_NAME simutrans
	MACOSX_BUNDLE_BUNDLE_VERSION ${VERSION}
	MACOSX_BUNDLE_COPYRIGHT ${COPYRIGHT}
	MACOSX_BUNDLE_GUI_IDENTIFIER org.simutrans.simutrans
	MACOSX_BUNDLE_ICON_FILE simutrans
	MACOSX_BUNDLE_INFO_STRING "Simutrans ${VERSION}, ${COPYRIGHT}"
	MACOSX_BUNDLE_LONG_VERSION_STRING ${VERSION}
	MACOSX_BUNDLE_SHORT_VERSION_STRING ${VERSION}
)

# Change the install directory from /usr/local to the build directory, so it's easier to package.
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

install(CODE "
	include(BundleUtilities)
	fixup_bundle(\"${CMAKE_BINARY_DIR}/simutrans/simutrans.app\" \"\" \"\") 
")
