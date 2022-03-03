if (UNIX AND NOT APPLE AND NOT OPTION_BUNDLE_LIBRARIES AND NOT SINGLE_INSTALL)
	# System Installation (Linux only)
	include(GNUInstallDirs)

	if (USE_GAMES_DATADIR)
		set(SIMUTRANS_BASE_DIR "${CMAKE_INSTALL_DATADIR}/games/simutrans")
	else ()
		set(SIMUTRANS_BASE_DIR "${CMAKE_INSTALL_DATADIR}/simutrans")
	endif ()
	set(SIMUTRANS_BIN_DIR "${CMAKE_INSTALL_BINDIR}")
	set(SIMUTRANS_OUTPUT_DIR "${CMAKE_INSTALL_PREFIX}")

	install(FILES ${CMAKE_SOURCE_DIR}/src/simutrans/simutrans.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
	install(FILES ${CMAKE_SOURCE_DIR}/src/simutrans/.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications RENAME simutrans.desktop)
else ()
	# Portable installation
	set(SIMUTRANS_BASE_DIR "${CMAKE_BINARY_DIR}/simutrans")
	set(SIMUTRANS_BIN_DIR "${CMAKE_BINARY_DIR}/simutrans")
	set(SIMUTRANS_OUTPUT_DIR "")
endif ()


install(TARGETS simutrans RUNTIME DESTINATION "${SIMUTRANS_BIN_DIR}" BUNDLE DESTINATION "${SIMUTRANS_BIN_DIR}")

install(DIRECTORY "${CMAKE_SOURCE_DIR}/simutrans/" DESTINATION ${SIMUTRANS_BASE_DIR} REGEX "get_pak.sh" EXCLUDE)

#
# Download language files
#
if (MSVC)
	# MSVC has no variable on the install target path at execution time, which is why we expand the directories at creation time!
	install(CODE "execute_process(COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/tools/get_lang_files.ps1 WORKING_DIRECTORY ${SIMUTRANS_OUTPUT_DIR}/${SIMUTRANS_BASE_DIR}/..)")
else ()
	install(CODE "execute_process(COMMAND sh ${CMAKE_SOURCE_DIR}/tools/get_lang_files.sh WORKING_DIRECTORY ${SIMUTRANS_OUTPUT_DIR}/${SIMUTRANS_BASE_DIR}/.. )")
endif ()

#
# Pak installer
#
if (NOT WIN32)
	install(FILES "${CMAKE_SOURCE_DIR}/tools/get_pak.sh" DESTINATION "${SIMUTRANS_BASE_DIR}"
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
	install(FILES "${CMAKE_SOURCE_DIR}/src/Windows/nsis/download-paksets.exe" DESTINATION "${SIMUTRANS_BASE_DIR}")
endif ()

#
# Install pak64 if requested or needed
#
if (SIMUTRANS_INSTALL_PAK64)
	if (MSVC)
		install(CODE
		"if(NOT EXISTS ${SIMUTRANS_BASE_DIR}/pak)
		execute_process(COMMAND powershell -Command \"Remove-Item \'${SIMUTRANS_BASE_DIR}/pak\' -Recurse\" WORKING_DIRECTORY ${SIMUTRANS_OUTPUT_DIR}/${SIMUTRANS_BASE_DIR})
		file(STRINGS ${CMAKE_SOURCE_DIR}/src/simutrans/paksetinfo.h URLpak64 REGEX \"/pak64/\")
		string( REGEX REPLACE \"^.[\\t ]*\\\"\" \"\" URLpak64 \${URLpak64})
		string( REGEX REPLACE \"\\\", .*\$\" \"\" URLpak64 \${URLpak64})
		message(\"install pak to \" ${SIMUTRANS_BASE_DIR})
		execute_process(COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/tools/get_pak.ps1 \${URLpak64} WORKING_DIRECTORY ${SIMUTRANS_OUTPUT_DIR}/${SIMUTRANS_BASE_DIR})
		endif ()
		")
	else ()
		# install pak64 with the bundle
		install(CODE
		"file(STRINGS  ${CMAKE_SOURCE_DIR}/src/simutrans/paksetinfo.h URLpak64 REGEX \"/pak64/\")
		 string( REGEX REPLACE \"^.[\\t ]*\\\"\" \"\" URLpak64 \${URLpak64})
		 string( REGEX REPLACE \"\\\", .*\$\" \"\" URLpak64 \${URLpak64})
		 execute_process(COMMAND sh ${CMAKE_SOURCE_DIR}/tools/get_pak.sh \${URLpak64} WORKING_DIRECTORY ${SIMUTRANS_OUTPUT_DIR}/${SIMUTRANS_BASE_DIR})
		")
	endif ()
endif ()

#
# Bundle libraries on linux if requested
#
if (OPTION_BUNDLE_LIBRARIES AND UNIX AND NOT APPLE)
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
