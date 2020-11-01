#
# This file is part of the Simutrans-Extended project under the Artistic License.
# (see LICENSE.txt)
#

find_package(Git)

if (Git_FOUND)
	option(SIMUTRANS_WITH_REVISION "Build executable with git commit information" ON)
endif (Git_FOUND)

if (Git_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse --short=7 HEAD
		OUTPUT_VARIABLE SIMUTRANS_SHA_SHORT
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
		OUTPUT_VARIABLE SIMUTRANS_BRANCH_NAME
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif (Git_FOUND)


if (SIMUTRANS_SHA_SHORT AND SIMUTRANS_BRANCH_NAME)
	message(STATUS "Configuring Simutrans-Extended (commit ${SIMUTRANS_SHA_SHORT} on ${SIMUTRANS_BRANCH_NAME}) ...")
else ()
	message(WARNING "Could not find revision information because this repository "
		"is not a  Git repository. Commit and branch information will be unavailable.")
endif ()
