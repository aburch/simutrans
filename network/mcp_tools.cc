/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/**
 * MCP tool: run_squirrel
 *
 * A single tool that accepts arbitrary Squirrel script code and executes it
 * inside a dedicated script VM that has the full Simutrans scripting API
 * (ai_base) available.  The script should return a value; that value is
 * converted to a string and returned to the MCP client as {"result":"..."}.
 *
 * A fresh VM is created for each call and destroyed afterwards, so no state
 * (global variables, defined functions, etc.) leaks between invocations.
 */

#include "mcp_tools.h"
#include "mcp_server.h"

#include <stdio.h>
#include <string.h>
#include <string>

#include "../script/script.h"
#include "../script/script_loader.h"
#include "../dataobj/environment.h"
#include "../utils/cbuffer_t.h"
#include "../utils/plainstring.h"
#include "../simworld.h"


// ---------------------------------------------------------------------------
// Minimal JSON helpers (output only)
// ---------------------------------------------------------------------------

static std::string jesc(const std::string &s) { return mcp_server_t::json_escape(s); }
static std::string jstr(const std::string &s)  { return "\"" + jesc(s) + "\""; }
static std::string jstr(const char *s)          { return s ? jstr(std::string(s)) : "null"; }


// ---------------------------------------------------------------------------
// Tool implementation
// ---------------------------------------------------------------------------

static std::string tool_run_squirrel(const std::string &code, int player_nr)
{
	// Create a fresh VM for each call so no state leaks between invocations.
	cbuffer_t ai_path;
	ai_path.printf("%sai/", env_t::data_dir);

	script_vm_t *vm = script_loader_t::start_vm("ai_base.nut", "script-mcp.log",
	                                              ai_path, /*is_scenario=*/false,
	                                              /*enable_io=*/false);
	if (!vm) {
		return "{\"error\":\"failed to initialize Squirrel VM\"}";
	}
	vm->set_my_player((uint8)player_nr);
	vm->pause_on_error = false;

	// Wrap the user code in a named function so we can call it with
	// call_function<plainstring>(FORCEX, ...) and capture the return value.
	std::string wrapped =
		"function __mcp_fn__() {\n"
		+ code +
		"\n}";

	std::string result_json;

	// Step 1: define __mcp_fn__ in the VM
	const char *err = vm->eval_string(wrapped.c_str());
	if (err && strcmp(err, "suspended") != 0) {
		result_json = "{\"error\":" + jstr(err) + "}";
	}
	else {
		// Step 2: call __mcp_fn__ synchronously, capture return value as string
		plainstring result;
		err = vm->call_function(script_vm_t::FORCEX, "__mcp_fn__", result);
		if (err) {
			result_json = "{\"error\":" + jstr(err) + "}";
		}
		else {
			result_json = "{\"result\":" + jstr(result.c_str()) + "}";
		}
	}

	delete vm;
	return result_json;
}


// ---------------------------------------------------------------------------
// Tool registry
// ---------------------------------------------------------------------------

struct tool_def_t {
	const char *name;
	const char *description;
	const char *input_schema_json;
};

static const tool_def_t TOOL_DEFS[] = {
	{
		"run_squirrel",
		"Execute Squirrel script code with the full Simutrans scripting API "
		"(world, player, halt, line, convoy, factory, command_x, etc.) available. "
		"The script should return a value; it is returned as a string result. "
		"Each call runs in a fresh VM; no state persists between calls.",
		"{\"type\":\"object\","
		 "\"properties\":{"
		   "\"code\":{\"type\":\"string\","
		              "\"description\":\"Squirrel script code to execute. "
		              "Use 'return' to pass a value back.\"},"
		   "\"player_nr\":{\"type\":\"integer\","
		                   "\"description\":\"Player context index (0-15), default 1\"}"
		 "},"
		 "\"required\":[\"code\"]}"
	}
};
static const int NUM_TOOLS = sizeof(TOOL_DEFS) / sizeof(TOOL_DEFS[0]);


// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

std::string mcp_tools::tools_list_json()
{
	std::string arr = "[";
	for (int i = 0; i < NUM_TOOLS; i++) {
		if (i > 0) arr += ',';
		arr += std::string("{")
			+ "\"name\":"        + jstr(TOOL_DEFS[i].name)        + ","
			+ "\"description\":" + jstr(TOOL_DEFS[i].description) + ","
			+ "\"inputSchema\":"  + TOOL_DEFS[i].input_schema_json
			+ "}";
	}
	return arr + "]";
}


std::string mcp_tools::tools_call(const std::string &name,
                                   const std::string &args_json,
                                   karte_t           * /*welt*/)
{
	if (name == "run_squirrel") {
		std::string code     = mcp_server_t::json_get_string(args_json, "code");
		std::string player_s = mcp_server_t::json_get_raw(args_json, "player_nr");
		int player_nr = player_s.empty() ? 1 : atoi(player_s.c_str());
		if (player_nr < 0 || player_nr > 15) player_nr = 1;
		return tool_run_squirrel(code, player_nr);
	}

	return "{\"error\":\"unknown tool: " + jesc(name) + "\"}";
}
