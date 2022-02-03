/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "dynamic_string.h"
#include "script.h"

#include "../sys/simsys.h"
#include "../world/simworld.h"
#include "../network/network.h"
#include "../network/network_cmd_scenario.h"
#include "../dataobj/environment.h"
#include "../dataobj/loadsave.h"
#include "../player/simplay.h"
#include "../tpl/plainstringhashtable_tpl.h"
#include "../utils/cbuffer.h"
#include "../utils/simstring.h"

uint32 const CACHE_TIME = 10000; // 10s

struct cached_string_t {
	plainstring result;
	uint32 time;
	dynamic_string *listener;
	cached_string_t(const char* str, uint32 t, dynamic_string *l) : result(str), time(t), listener(l) {}

	cached_string_t(loadsave_t* file)
	{
		time = dr_time() - CACHE_TIME;
		listener = NULL;
		rdwr(file);
	}

	void rdwr(loadsave_t* file)
	{
		file->rdwr_str(result);
	}
};

static plainstringhashtable_tpl<cached_string_t*> cached_results;


cached_string_t* get_cached_result(const char* function, uint32 cache_time)
{
	cached_string_t *entry = cached_results.get(function);

	if (entry) {
		if (dr_time() - entry->time > cache_time) {
			entry = NULL;
		}
	}
	return entry;
}


dynamic_string::~dynamic_string()
{
	FOR(plainstringhashtable_tpl<cached_string_t*>, &iter, cached_results) {
		if (iter.value->listener == this) {
			iter.value->listener = NULL;
		}
	}
}


void dynamic_string::init(script_vm_t *script)
{
	while(!cached_results.empty()) {
		delete cached_results.remove_first();
	}
	script->register_callback(&dynamic_string::record_result, "dynamicstring_record_result");
}


void dynamic_string::rdwr_cache(loadsave_t *file)
{
	uint32 count = cached_results.get_count();
	file->rdwr_long(count);

	if (file->is_loading()) {
		// clear list
		while(!cached_results.empty()) {
			delete cached_results.remove_first();
		}
		for(uint32 i=0; i<count; i++) {
			plainstring key;
			file->rdwr_str(key);
			cached_results.set(key, new cached_string_t(file));
		}
	}
	else {
		FOR(plainstringhashtable_tpl<cached_string_t*>, &iter, cached_results) {
			file->rdwr_str(iter.key);
			iter.value->rdwr(file);
		}
	}
}


void dynamic_string::update(script_vm_t *script, player_t *player, bool force_update)
{
	cbuffer_t buf;
	buf.printf("%s(%d)", method, player ? player->get_player_nr() : PLAYER_UNOWNED);
	const char* function = (const char*)buf;
	plainstring s;

	if (script) {
		cached_string_t *entry = NULL;
		if (!force_update) {
			entry = get_cached_result(function, CACHE_TIME);
		}
		if (entry) {
			// valid cache entry
			s = entry->result.c_str();
		}
		else {
			script->prepare_callback("dynamicstring_record_result", 2, (const char*)buf, (const char*)"");

			// call script
			const char* err = script->call_function(script_vm_t::QUEUE, method, s, (uint8)(player ? player->get_player_nr() : PLAYER_UNOWNED));
			if (!script_vm_t::is_call_suspended(err)) {
				if ( s != (const char*)str) {
					changed = true;
					str = s;
				}
			}
			else {
				// activate listener
				cached_string_t *entry = cached_results.get((const char*)buf);
				if (entry == NULL) {
					cached_results.set((const char*)buf, new cached_string_t(NULL, dr_time(), this));
				}
				else {
					entry->listener = this;
					// return cached value for now; will be changed after callback returns
					s = entry->result.c_str();
				}
			}
			script->clear_pending_callback();
			// .. and store result
			record_result(function, s);
		}
	}
	else {
		if (env_t::networkmode  &&  !env_t::server) {
			s = dynamic_string::fetch_result( function, NULL, this, force_update);
			if ( s == NULL) {
				s = "Waiting for server response...";
			}
		}
	}
	if ( s != (const char*)str) {
		str = s;
		changed = true;
	}
}


const char* dynamic_string::fetch_result(const char* function, script_vm_t *script, dynamic_string *listener, bool force_update)
{
	//dbg->warning("dynamic_string::fetch_result", "function = '%s'", function);
	cached_string_t *entry = cached_results.get(function);

	bool const needs_update = entry == NULL  ||  force_update  ||  (dr_time() - entry->time > CACHE_TIME);

	if (needs_update) {
		if (script) {
			// suspended calls have to be caught by script callbacks
			plainstring str;
			if(call_script(function, script, str)) {
				record_result(function, str);
			}
		}
		else {
			if (entry == NULL) {
				cached_string_t *old = cached_results.set(function, new cached_string_t(NULL, dr_time(), listener));
				assert(old == NULL);
				(void)old;
			}
			// send request
			nwc_scenario_t *nwc = new nwc_scenario_t();
			nwc->function = function;
			nwc->what     = nwc_scenario_t::CALL_SCRIPT_ANSWER;
			network_send_server(nwc);
		}
		// entry maybe invalid now, fetch again
		entry = cached_results.get(function);
	}
	return entry ? entry->result.c_str() : NULL;
}


bool dynamic_string::record_result(const char* function, plainstring result)
{
	cached_string_t *new_entry = new cached_string_t(result, dr_time(), NULL);
	cached_string_t *entry = cached_results.set(function, new_entry);
	if (entry == NULL) {
		return false;
	}
	if (dynamic_string *dyn = entry->listener) {
		new_entry->listener = dyn;
		if (dyn->str != (const char*) result) {
			dyn->str = result;
			dyn->changed = true;
		}
	}
	delete entry;
	return true;
}


/// Struct for checking number of parameters
struct method_param_t {
	const char* name;
	int nparam;
};

/// Static table with script methods that can be called
static const method_param_t scenario_methods[] = {
	{ "is_scenario_completed", 1 },
	{ "get_info_text", 1 },
	{ "get_goal_text", 1 },
	{ "get_rule_text", 1 },
	{ "get_result_text", 1 },
	{ "get_about_text", 1 },
	{ "get_short_description", 1}
};


void get_scenario_completion(plainstring &res, int player_nr)
{
	cbuffer_t buf;
	sint32 percent = script_api::welt->get_scenario()->get_completion(player_nr);
	buf.printf("%d", percent);
	res = (const char*)buf;
}


bool dynamic_string::call_script(const char* function, script_vm_t* script, plainstring& str)
{
	int nparams = -1;

	size_t n = strcspn(function, "(");
	char *method = new char[n+1];
	tstrncpy(method, function, n+1);

	// check method name
	for(uint i=0; i< lengthof(scenario_methods); i++) {
		if (strcmp(method, scenario_methods[i].name)==0) {
			nparams = scenario_methods[i].nparam;
			break;
		}
	}
	if (nparams == -1) {
		// unknown method
		delete [] method;
		return false;
	}
	// scan for params
	int params[] = {0, 0, 0, 0};
	assert( nparams+1 < (int)lengthof(params) );

	const char* p = function+n;
	if (*p) p++;
	bool ok = true;

	for(int i=0; i<nparams; i++) {
		if (p  &&  *p) {
			params[i] = atoi(p);
			p = strchr(function, ',');
		}
		else {
			ok = false;
		}
	}

	if (ok) {
		const char *err = NULL;
		switch(nparams) {
			case 0:
				err = script->call_function(script_vm_t::QUEUE, method, str);
				break;

			case 1:
				if ( strcmp(method, "is_scenario_completed")==0 ) {
					get_scenario_completion(str, params[0]);
				}
				else {
					err = script->call_function(script_vm_t::QUEUE, method, str, params[0]);
				}
				break;

			default:
				assert(0);
		}
		if (script_vm_t::is_call_suspended(err)) {
			ok = false;
		}
	}

	// cleanup
	delete [] method;

	return ok;
}
