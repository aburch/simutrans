/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "script_loader.h"
#include "script.h"
#include "api/api.h"
#include "export_objs.h"
#include "../dataobj/environment.h"
#include "../utils/cbuffer.h"



bool script_loader_t::load_base_script(script_vm_t *script, const char* base)
{
	cbuffer_t buf;
	buf.printf("%sscript/%s", env_t::data_dir, base);
	if (const char* err = script->call_script(buf)) {
		// should not happen
		dbg->error("load_base_script", "error [%s] calling %s", err, (const char*)buf);
		return false;
	}
	return true;
}


script_vm_t* script_loader_t::start_vm(const char* base_file_name, const char* logfile_name, const char* include_path, bool is_scenario)
{
	script_vm_t* script = new script_vm_t(include_path, logfile_name);
	// load global stuff
	// constants must be known compile time
	export_global_constants(script->get_vm());

	// load scripting base definitions
	bool ok = load_base_script(script, "script_base.nut");
	// load specific base definitions
	ok = ok  &&  load_base_script(script, base_file_name);

	// register api functions
	if (ok) {
		register_export_function(script->get_vm(), is_scenario);
		if (script->get_error()) {
			dbg->error("start_vm", "error [%s] calling register_export_function", script->get_error());
			ok = false;
		}
	}
	if (ok) {
		return script;
	}
	else {
		delete script;
		return NULL;
	}
}


void script_loader_t::load_compatibility_script(script_vm_t *script)
{
	// check api version
	plainstring api_version;
	if (const char* err = script->call_function(script_vm_t::FORCE, "get_api_version", api_version)) {
		dbg->warning("scenario_t::init", "error [%s] calling get_api_version", err);
		api_version = "120.1";
	}
	if (api_version != "*") {
		// load scenario compatibility script
		if (load_base_script(script, "script_compat.nut")) {
			plainstring dummy;
			// call compatibility function
			if (const char* err = script->call_function(script_vm_t::FORCE, "compat", dummy, api_version) ) {
				dbg->warning("scenario_t::init", "error [%s] calling compat", err);
			}
		}
	}
}
