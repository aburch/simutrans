//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


map.file = "empty-16x16.sve"

scenario.short_description = "Automated tests"
scenario.author = "ceeac"
scenario.version = "0.1"


//
// Includes
//

include("test_helpers")
include("all_tests")


//
// test runner stuff
//

local num_tests_done = 0
local num_tests = all_tests.len()
local error_msg = null


function run_all_tests()
{
	num_tests_done = 0
	error_msg = null

	print("============================================================")
	print("== Running tests ... =======================================")
	print("============================================================")

	foreach (i,test_func in all_tests) {
		local func_name = test_func.getinfos().name
		print("[" + (num_tests_done+1) + "/" + num_tests + "] " + func_name)
		test_func() // run the test
		++num_tests_done
	}

	print("Tests completed successfully.")
}


//
// Required for scenario
//

function get_rule_text(pl)
{
	return ttext("Don't touch.")
}


function get_goal_text(pl)
{
	return ttext("Wait for all tests to pass.")
}


function get_info_text(pl)
{
	if (error_msg != null) {
		// an error ocurred
		return error_msg
	}

	local msg = ttext("{ndone}/{ntests} completed.")
	msg.ndone = num_tests_done
	msg.ntests = num_tests
	return msg
}


function get_result_text(pl)
{
	return get_info_text(pl)
}


function is_tool_allowed(pl, tool_id, wt, name)
{
	return true
}


function start()
{
	run_all_tests()
}


function resume_game()
{
	run_all_tests()
}


function is_scenario_completed(pl)
{
	if (error_msg != null) {
		return -1
	}
	else if (num_tests == 0) {
		return 100
	}

	return (100*num_tests_done) / num_tests
}
