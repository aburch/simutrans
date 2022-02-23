
install(TARGETS simutrans RUNTIME DESTINATION "${CMAKE_BINARY_DIR}/simutrans/" BUNDLE DESTINATION "${CMAKE_BINARY_DIR}/simutrans/")

install(DIRECTORY "${CMAKE_SOURCE_DIR}/simutrans" DESTINATION "${CMAKE_BINARY_DIR}")

#
# Download language files
#
if (MSVC)
	# MSVC has no variable on the install target path at execution time, which is why we expand the directories at creation time!
	install(CODE "execute_process(COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/tools/get_lang_files.ps1 WORKING_DIRECTORY ${simutrans_BINARY_DIR})")
else ()
	install(CODE "execute_process(COMMAND sh ${CMAKE_SOURCE_DIR}/tools/get_lang_files.sh WORKING_DIRECTORY \${CMAKE_BINARY_DIR})")
endif ()

#
# Pak installer
#
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

#
# Install pak64 if requested or needed
#
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

#
# System Installation (Linux only)
#
if (UNIX AND NOT APPLE AND NOT OPTION_BUNDLE_LIBRARIES)

	include(GNUInstallDirs)

	if (USE_GAMES_DATADIR)
		set(INSTALL_DATADIR "${CMAKE_INSTALL_DATADIR}/games")
	else ()
		set(INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR})
	endif ()

	install(TARGETS simutrans RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

	install(DIRECTORY ${CMAKE_BINARY_DIR}/simutrans
		DESTINATION ${INSTALL_DATADIR}
		REGEX "simutrans/simutrans|simutrans/get_pak.sh" EXCLUDE
	)
	install(FILES ${CMAKE_BINARY_DIR}/simutrans/get_pak.sh DESTINATION ${INSTALL_DATADIR}/simutrans PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_EXECUTE WORLD_READ)

	install(FILES ${CMAKE_SOURCE_DIR}/src/simutrans/simutrans.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
	install(FILES ${CMAKE_SOURCE_DIR}/src/simutrans/.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications RENAME simutrans.desktop)

endif ()
