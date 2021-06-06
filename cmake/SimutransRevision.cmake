#
# This file is part of the Simutrans project under the artistic licence.
# (see licence.txt)
#

include(FindSubversion)
include(FindGit)


if (Subversion_FOUND OR Git_FOUND)
	option(SIMUTRANS_WITH_REVISION "Build executable with SVN/git revision information" ON)
endif (Subversion_FOUND OR Git_FOUND)

# We have to try Git first because Subversion_WC_INFO does not fail silently if
# this repository is not a Subversion repository
# assume this is from aburch/simutrans!
if (GIT_FOUND)
	execute_process(WORKING_DIRECTORY ${SOURCE_DIR} COMMAND ${GIT_EXECUTABLE} log -1 RESULT_VARIABLE res_var OUTPUT_VARIABLE SIMUTRANS_GIT_WC_REVISION)
	if ( ${res_var} EQUAL 0 )
		message( "git log -1 ok:" ${SIMUTRANS_GIT_WC_REVISION})
		string( REGEX REPLACE "\n" " " TEMP1 ${SIMUTRANS_GIT_WC_REVISION})
		string( REGEX REPLACE "^.*trunk\@" "" TEMP2 ${TEMP1})
		string( REGEX REPLACE " .*$" "" SIMUTRANS_WC_REVISION ${TEMP2})
	endif()
endif ()

if ( NOT SIMUTRANS_WC_REVISION AND Subversion_FOUND )
	execute_process(WORKING_DIRECTORY ${SOURCE_DIR} COMMAND ${SVN_EXECUTABLE} svn info RESULT_VARIABLE res_var OUTPUT_VARIABLE SIMUTRANS_GIT_WC_REVISION)
	if ( ${res_var} EQUAL 0 )
        message( "svn ok:" ${SIMUTRANS_GIT_WC_REVISION})
		string( REGEX REPLACE "\n" " " TEMP1 ${SIMUTRANS_GIT_WC_REVISION})
		string( REGEX REPLACE "^.*Revision: " "" TEMP2 ${TEMP1})
		string( REGEX REPLACE " .*$" "" SIMUTRANS_WC_REVISION ${TEMP2})
	endif()
endif ()

if (SIMUTRANS_WC_REVISION)
	# write a file with the SVNVERSION define
	file(WRITE revision.h.txt "#define REVISION ${SIMUTRANS_WC_REVISION}\n")
	# copy the file to the final header only if the version changes
	# reduces needless rebuilds
	execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different revision.h.txt ${SOURCE_DIR}/revision.h)

	message(STATUS "Compiling Simutrans revision ${SIMUTRANS_WC_REVISION} ...")
else ()
	message(WARNING "Could not find revision information because this repository "
		"is neither a Subversion nor a Git repository. Revision information "
		"will be unavailable.")
	file(WRITE ${SOURCE_DIR}/revision.h "#define REVISION \n")
endif ()

