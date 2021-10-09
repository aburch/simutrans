#
# This file is part of the Simutrans project under the artistic licence.
# (see licence.txt)
#

find_package(Subversion QUIET)
find_package(Git QUIET)

# assume this is from aburch/simutrans!
if (Git_FOUND)
	execute_process(WORKING_DIRECTORY "${SOURCE_DIR}"
		COMMAND "${GIT_EXECUTABLE}" log -1
		RESULT_VARIABLE res_var
		OUTPUT_VARIABLE SIMUTRANS_RAW_WC_REVISION
	)

	if (res_var EQUAL 0)
		message( "git log -1 ok:" ${SIMUTRANS_RAW_WC_REVISION})
		string( REGEX REPLACE "[\t\r\n]" " " TEMP1 ${SIMUTRANS_RAW_WC_REVISION})
		string( REGEX REPLACE "^.*trunk\@" "" TEMP2 ${TEMP1})
		string( REGEX REPLACE " .*$" "" SIMUTRANS_WC_REVISION ${TEMP2})

		if(SIMUTRANS_WC_REVISION MATCHES "commit")
			unset(SIMUTRANS_WC_REVISION)
		else ()
			execute_process(WORKING_DIRECTORY "${SOURCE_DIR}"
				COMMAND "${GIT_EXECUTABLE}" describe --always
				RESULT_VARIABLE res_var
				OUTPUT_VARIABLE SIMUTRANS_WC_HASH
			)
		endif()
	endif()
endif ()

if (NOT SIMUTRANS_WC_REVISION AND Subversion_FOUND)
	execute_process(WORKING_DIRECTORY "${SOURCE_DIR}"
		COMMAND svn info --show-item revision
		RESULT_VARIABLE res_var
		OUTPUT_VARIABLE SIMUTRANS_WC_REVISION
		ERROR_VARIABLE dummy
	)

	if (res_var)
		# SVN not found => no result
		unset(SIMUTRANS_WC_REVISION)
	endif ()
endif ()

# Fallback for git commits not submitted to svn (e.g. when using git-svn)
if (NOT SIMUTRANS_WC_REVISION AND Git_FOUND)
	execute_process(WORKING_DIRECTORY "${SOURCE_DIR}"
		COMMAND "${GIT_EXECUTABLE}" rev-list --count --first-parent HEAD
		RESULT_VARIABLE res_var
		OUTPUT_VARIABLE SIMUTRANS_WC_REVISION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if (res_var)
		# not a git repository
		unset(SIMUTRANS_WC_REVISION)
	else ()
		# the number of commit +326 equals the revision ...
		MATH(EXPR res_var "${res_var}+326")

		execute_process(WORKING_DIRECTORY "${SOURCE_DIR}"
			COMMAND "${GIT_EXECUTABLE}" describe --always
			RESULT_VARIABLE res_var
			OUTPUT_VARIABLE SIMUTRANS_WC_HASH
		)
	endif ()
endif ()

if (SIMUTRANS_WC_REVISION)
	# write a file with the SVNVERSION define
	if (NOT SIMUTRANS_WC_HASH)
		set(SIMUTRANS_WC_HASH "0")
	endif ()
	file(WRITE revision.h.txt "#define REVISION ${SIMUTRANS_WC_REVISION}\n#define GIT_HASH 0x${SIMUTRANS_WC_HASH}\n")

	# copy the file to the final header only if the version changes
	# reduces needless rebuilds
	execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different revision.h.txt "${SOURCE_DIR}/revision.h")

	message(STATUS "Compiling Simutrans revision ${SIMUTRANS_WC_REVISION} with hash ${SIMUTRANS_WC_HASH} ...")
else ()
	message(WARNING "Could not find revision information because this repository "
		"is neither a Subversion nor a Git repository. Revision information "
		"will be unavailable.")

	if (NOT EXISTS "${SOURCE_DIR}/revision.h")
		file(WRITE "${SOURCE_DIR}/revision.h" "#define REVISION \n")
	endif ()
endif ()

