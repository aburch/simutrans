/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../sys/simsys.h"
#include "../simconst.h"
#include "../simtypes.h"
#include "../simdebug.h"

#include "../player/simplay.h"
#include "../world/simworld.h"
#include "../simmesg.h"
#include "../simmem.h"
#include "../tool/simmenu.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../network/network.h"
#include "../network/network_cmd_scenario.h"
#include "../dataobj/schedule.h"

#include "../utils/cbuffer.h"

// error popup
#include "../gui/simwin.h"
#include "../gui/scenario_info.h"

// scripting
#include "../script/script.h"
#include "../script/script_loader.h"
#include "../script/api/api.h"
#include "../script/api_param.h"
#include "../script/api_class.h"
#include "../script/api/api_simple.h"

#include "../tpl/plainstringhashtable_tpl.h"

#include "scenario.h"

#include <stdarg.h>

// cache the scenario text files
static plainstringhashtable_tpl<plainstring> cached_text_files;


scenario_t::scenario_t(karte_t *w) :
	description_text("get_short_description"),
	info_text("get_info_text"),
	goal_text("get_goal_text"),
	rule_text("get_rule_text"),
	result_text("get_result_text"),
	about_text("get_about_text"),
	debug_text("get_debug_text")
{
	welt = w;
	what_scenario = 0;

	script = NULL;
	won = false;
	lost = false;
	rdwr_error = false;
	need_toolbar_update = false;

	cached_text_files.clear();
}


scenario_t::~scenario_t()
{
	delete script;
	clear_rules();
	cached_text_files.clear();
}


const char* scenario_t::init( const char *scenario_base, const char *scenario_name_, karte_t *welt )
{
	this->welt = welt;
	scenario_name = scenario_name_;

	// path to scenario files
	cbuffer_t buf;
	buf.printf("%s%s/", scenario_base, scenario_name_);
	scenario_path = buf;

	// scenario script file
	buf.append("scenario.nut");
	if (!load_script( buf )) {
		dbg->warning("scenario_t::init", "could not load script file %s", (const char*)buf);
		return "Loading scenario script failed";
	}

	const char *err = NULL;
	plainstring mapfile;

	// load savegame
	if ((err = script->call_function(script_vm_t::FORCE, "get_map_file", mapfile))) {
		dbg->warning("scenario_t::init", "error [%s] calling get_map_file", err);
		return "No scenario map specified";
	}

	// if savegame-string == "<attach>" then do not load a savegame, just attach to running game.
	if ( strcmp(mapfile, "<attach>") ) {
		// savegame location
		buf.clear();
		buf.printf("%s%s/%s", scenario_base, scenario_name_, mapfile.c_str());
		if (!welt->load( buf )) {
			dbg->warning("scenario_t::init", "error loading savegame %s", err, (const char*)buf);
			return "Could not load scenario map!";
		}
		// set savegame name
		buf.clear();
		buf.printf("%s.sve", scenario_name.c_str());
		welt->get_settings().set_filename( strdup(buf) );
		welt->type_of_generation = karte_t::SCENARIO_WORLD;
		// re-initialize coordinate and rotation handling
		script_api::coordinate_transform_t::initialize();
	}

	script_loader_t::load_compatibility_script(script);

	// load translations
	translator::load_files_from_folder( scenario_path.c_str(), "scenario" );
	cached_text_files.clear();

	what_scenario = SCRIPTED;

	// callback
	script->register_callback(&scenario_t::set_completion, "scenario_t_set_completed");

	// register ourselves
	welt->set_scenario(this);
	welt->get_message()->clear();
	welt->get_chat_message()->clear();

	// set start time
	sint32 const time = welt->get_current_month();
	welt->get_settings().set_starting_year( time / 12);
	welt->get_settings().set_starting_month( time % 12);

	// set my player number to PLAYER_UNOWNED
	script->set_my_player(PLAYER_UNOWNED);

	// now call startup function
	if ((err = script->call_function(script_vm_t::QUEUE, "start"))) {
		dbg->warning("scenario_t::init", "error [%s] calling start", err);
	}

	return NULL;
}


bool scenario_t::load_script(const char* filename)
{
	delete script;
	// start vm
	script = script_loader_t::start_vm("scenario_base.nut", "script-scenario.log", scenario_path.c_str(), true);
	if (script == NULL) {
		return false;
	}

	// init strings
	dynamic_string::init(script);
	// register callback
	if (env_t::server) {
		nwc_scenario_t::init(script);
	}

	// load scenario definition
	if (const char* err = script->call_script(filename)) {
		dbg->error("scenario_t::load_script", "error [%s] calling %s", err, filename);
		return false;
	}
	return true;
}


void scenario_t::koord_sq2w(koord &k) const
{
	script_api::coordinate_transform_t::koord_sq2w(k);
}


const char* scenario_t::get_forbidden_text()
{
	static cbuffer_t buf;
	buf.clear();
	buf.append("<h1>Forbidden stuff:</h1><br>");
	for(uint pnr=0; pnr<MAX_PLAYER_COUNT;  pnr++) {
		for(uint32 i=0; i<forbidden_tools[pnr].get_count(); i++) {
			scenario_t::forbidden_t &f = *forbidden_tools[pnr][i];
			buf.printf("[%d] Player = %d, Tool = %d", i, pnr, f.toolnr);
			if (f.waytype!=invalid_wt) {
				buf.printf(", Waytype = %d", f.waytype);
			}
			if (f.parameter_hash != 0) {
				buf.printf(", Default_parameter_hash = \"%ld\", ", f.parameter_hash);
			}
			if (f.type == forbidden_t::forbid_tool_rect) {
				if (-128<f.hmin ||  f.hmax<127) {
					buf.printf(", Cube = (%s,%d) x ", f.pos_nw.get_str(), f.hmin);
					buf.printf("(%s,%d)", f.pos_se.get_str(), f.hmax);
				}
				else {
					buf.printf(", Rect = (%s) x ", f.pos_nw.get_str());
					buf.printf("(%s)", f.pos_se.get_str());
				}
			}
			buf.printf("<br>");
		}
	}
	return buf;
}


bool scenario_t::forbidden_t::operator <(const forbidden_t &other) const
{
	bool lt = type < other.type;
	if (!lt  &&  type == other.type) {
		sint32 diff = (sint32)toolnr - (sint32)other.toolnr;
		if (diff == 0) {
			diff = (sint32)waytype - (sint32)other.waytype;
		}
		if (diff == 0) {
			diff = (sint32)parameter_hash - (sint32)other.parameter_hash;
		}
		lt = diff < 0;
	}
	return lt;
}


bool scenario_t::forbidden_t::operator ==(const forbidden_t &other) const
{
	bool eq = (type == other.type) && (waytype == other.waytype) && (toolnr == other.toolnr) && (parameter_hash == other.parameter_hash);
	if (eq) {
		switch (type) {
			case forbid_tool_rect:
				eq = eq && (hmin == other.hmin) && (hmax == other.hmax);
				eq = eq && (pos_nw == other.pos_nw);
				eq = eq && (pos_se == other.pos_se);
				/* FALLTHROUGH */
			case forbid_tool:
				break;
		}
	}
	return eq;
}


scenario_t::forbidden_t::forbidden_t(const forbidden_t& other) :
	type(other.type), toolnr(other.toolnr),
	waytype(other.waytype), parameter_hash(other.parameter_hash), pos_nw(other.pos_nw), pos_se(other.pos_se),
	hmin(other.hmin), hmax(other.hmax), error(other.error)
{
}


void scenario_t::forbidden_t::rotate90(const sint16 y_size)
{
	switch(type) {
		case forbid_tool_rect: {
			pos_nw.rotate90(y_size);
			pos_se.rotate90(y_size);
			sint16 x = pos_nw.x; pos_nw.x = pos_se.x; pos_se.x = x;
		}
		default: ;
	}
}


uint32 scenario_t::find_first(const forbidden_t &other, uint player_nr) const
{
	if (forbidden_tools[player_nr].empty()  ||  *forbidden_tools[player_nr].back() < other) {
		// empty vector, or everything is smaller
		return forbidden_tools[player_nr].get_count();
	}
	if (other < *forbidden_tools[player_nr][0]) {
		// everything is larger
		return forbidden_tools[player_nr].get_count();
	}
	else if ( other <= *forbidden_tools[player_nr][0] ) {
		return 0;
	}
	// now: low < other <= high
	uint32 low = 0, high = forbidden_tools[player_nr].get_count()-1;
	while(low+1 < high) {
		uint32 mid = (low+high) / 2;
		if (*forbidden_tools[player_nr][mid] < other) {
			low = mid;
			// now low < other
		}
		else {
			high = mid;
			// now other <= high
		}
	};
	// still: low < other <= high
	bool notok = other < *forbidden_tools[player_nr][high];
	return high;
}


// only match type and toolnumber and return frist match (other could follow)
uint32 scenario_t::find_first_type_tool_wt(const forbidden_t& other, uint player_nr) const
{
	if (forbidden_tools[player_nr].empty()) {
		// empty
		return 0;
	}
	forbidden_t &b = *forbidden_tools[player_nr].back();
	if(forbidden_tools[player_nr].back()->type < other.type  ||  forbidden_tools[player_nr].back()->toolnr < other.toolnr) {
		// everything is smaller
		return forbidden_tools[player_nr].get_count();
	}
	if (forbidden_tools[player_nr][0]->type > other.type  ||  forbidden_tools[player_nr][0]->toolnr > other.toolnr  ||  forbidden_tools[player_nr][0]->waytype > other.waytype) {
		// everything is smaller
		return forbidden_tools[player_nr].get_count();
	}
	else if (forbidden_tools[player_nr][0]->type == other.type  &&  forbidden_tools[player_nr][0]->toolnr == other.toolnr  &&  (forbidden_tools[player_nr][0]->waytype == invalid_wt  ||  forbidden_tools[player_nr][0]->waytype==other.waytype)  ) {
		// first is matching
		return 0;
	}
	// now binary search: low < other <= high
	uint32 low = 0, high = forbidden_tools[player_nr].get_count() - 1;
	while (low + 1 < high) {
		uint32 mid = (low + high) / 2;
		if (*forbidden_tools[player_nr][mid] < other) {
			low = mid;
			// now low < other
		}
		else {
			high = mid;
			// now other <= high
		}
	};
	// did we find something?
	bool ok = forbidden_tools[player_nr][high]->type == other.type  &&  forbidden_tools[player_nr][high]->toolnr == other.toolnr;
	return ok ? high : forbidden_tools[player_nr].get_count();
}


void scenario_t::intern_forbid(forbidden_t* test, uint player_nr, bool add_rule)
{
	bool changed = false;
	forbidden_t::forbid_type type = test->type;

	bool current_add = add_rule;
	for (int i=0; i<1+add_rule; i++) {
		if(add_rule  &&  type!=forbidden_t::forbid_tool) {
			if (type == forbidden_t::allow_tool_rect) {
				// before adding an allow rule, remove a identical forbid rule
				test->type = (i == 0) ? forbidden_t::forbid_tool_rect : forbidden_t::allow_tool_rect;
				current_add = i;
			}
			else if (type == forbidden_t::forbid_tool_rect) {
				// before adding a forbind rule, remove a identical allowed rule
				test->type = (i == 0) ? forbidden_t::allow_tool_rect : forbidden_t::forbid_tool_rect;
			}
			current_add = i; // first pass remove, next pass add
		}

		for (uint32 i = find_first(*test, player_nr); i < forbidden_tools[player_nr].get_count() && *forbidden_tools[player_nr][i] <= *test; i++) {
			if (*test == *forbidden_tools[player_nr][i]) {
				// entry exists already
				delete test;
				if (!current_add) {
					delete forbidden_tools[player_nr][i];
					forbidden_tools[player_nr].remove_at(i);
					changed = true;
				}
				goto end;
			}
		}
	}
	// entry does not exist
	if (add_rule) {
		forbidden_tools[player_nr].insert_ordered(test, scenario_t::forbidden_t::compare);
		changed = true;
	}
end:
	if (changed  &&  type==forbidden_t::forbid_tool) {
		need_toolbar_update = true;
	}
}

void scenario_t::call_forbid_tool(forbidden_t *test, uint player_nr, bool forbid)
{
	if (env_t::server) {
		// send information over network
		nwc_scenario_rules_t *nws = new nwc_scenario_rules_t(welt->get_sync_steps() + 1, welt->get_map_counter());
		nws->rule = test;
		nws->forbid = forbid;
		nws->player_nr = player_nr;
		network_send_all(nws, false);
	}
	else {
		// directly apply
		intern_forbid(test, player_nr, forbid);
	}
}

void scenario_t::forbid_tool(uint8 player_nr, uint16 tool_id)
{
	forbidden_t *test = new forbidden_t(forbidden_t::forbid_tool, tool_id, invalid_wt);
	call_forbid_tool(test, player_nr, true);
}


void scenario_t::clear_forbid_tool(uint8 player_nr, uint16 tool_id)
{
	forbidden_t *test = new forbidden_t(forbidden_t::forbid_tool, tool_id, invalid_wt);
	call_forbid_tool(test, player_nr, false);
}


void scenario_t::forbid_way_tool(uint8 player_nr, uint16 tool_id, waytype_t wt, const char* param)
{
	forbidden_t *test = new forbidden_t(forbidden_t::forbid_tool, tool_id, wt, param);
	call_forbid_tool(test, player_nr, true);
}


void scenario_t::clear_forbid_way_tool(uint8 player_nr, uint16 tool_id, waytype_t wt, const char* param)
{
	forbidden_t *test = new forbidden_t(forbidden_t::forbid_tool, tool_id, wt, param);
	call_forbid_tool(test, player_nr, false);
}


void scenario_t::forbid_way_tool_rect(uint8 player_nr, uint16 tool_id, waytype_t wt, const char* param, koord pos_nw, koord pos_se, plainstring err)
{
	forbid_way_tool_cube(player_nr, tool_id, wt, param, koord3d(pos_nw, -128), koord3d(pos_se, 127), err);
}


void scenario_t::clear_way_tool_rect(uint8 player_nr, uint16 tool_id, waytype_t wt, const char* param, koord pos_nw, koord pos_se, bool allow)
{
	clear_way_tool_cube(player_nr, tool_id, wt, param, koord3d(pos_nw, -128), koord3d(pos_se, 127), allow);
}


void scenario_t::allow_way_tool_rect(uint8 player_nr, uint16 tool_id, waytype_t wt, const char* param, koord pos_nw, koord pos_se)
{
	allow_way_tool_cube(player_nr, tool_id, wt, param, koord3d(pos_nw, -128), koord3d(pos_se, 127));
}


void scenario_t::forbid_way_tool_cube(uint8 player_nr, uint16 tool_id, waytype_t wt, const char* param, koord3d pos_nw_0, koord3d pos_se_0, plainstring err)
{
	koord pos_nw( min(pos_nw_0.x, pos_se_0.x), min(pos_nw_0.y, pos_se_0.y));
	koord pos_se( max(pos_nw_0.x, pos_se_0.x), max(pos_nw_0.y, pos_se_0.y));
	sint8 hmin( min(pos_nw_0.z, pos_se_0.z) );
	sint8 hmax( max(pos_nw_0.z, pos_se_0.z) );

	forbidden_t *test = new forbidden_t(tool_id, wt, param, pos_nw, pos_se, hmin, hmax);
	test->error = err;
	call_forbid_tool(test, player_nr, true);
}


void scenario_t::clear_way_tool_cube(uint8 player_nr, uint16 tool_id, waytype_t wt, const char* param, koord3d pos_nw_0, koord3d pos_se_0, bool allow)
{
	koord pos_nw(min(pos_nw_0.x, pos_se_0.x), min(pos_nw_0.y, pos_se_0.y));
	koord pos_se(max(pos_nw_0.x, pos_se_0.x), max(pos_nw_0.y, pos_se_0.y));
	sint8 hmin(min(pos_nw_0.z, pos_se_0.z));
	sint8 hmax(max(pos_nw_0.z, pos_se_0.z));

	forbidden_t* test = new forbidden_t(tool_id, wt, param, pos_nw, pos_se, hmin, hmax);
	test->type = allow ? forbidden_t::allow_tool_rect : forbidden_t::forbid_tool_rect;
	call_forbid_tool(test, player_nr, false);
}


void scenario_t::allow_way_tool_cube(uint8 player_nr, uint16 tool_id, waytype_t wt, const char *param, koord3d pos_nw_0, koord3d pos_se_0)
{
	koord pos_nw( min(pos_nw_0.x, pos_se_0.x), min(pos_nw_0.y, pos_se_0.y));
	koord pos_se( max(pos_nw_0.x, pos_se_0.x), max(pos_nw_0.y, pos_se_0.y));
	sint8 hmin( min(pos_nw_0.z, pos_se_0.z) );
	sint8 hmax( max(pos_nw_0.z, pos_se_0.z) );

	forbidden_t *test = new forbidden_t(tool_id, wt, param, pos_nw, pos_se, hmin, hmax);
	test->type = forbidden_t::allow_tool_rect;
	call_forbid_tool(test, player_nr, true);
}


void scenario_t::clear_rules()
{
	for (uint pnr = 0; pnr < MAX_PLAYER_COUNT; pnr++) {
		clear_ptr_vector(forbidden_tools[pnr]);
	}
	need_toolbar_update = true;
}


bool scenario_t::is_tool_allowed(const player_t* player, uint16 tool_id, sint16 wt, const char* param)
{
	if (what_scenario != SCRIPTED  &&  what_scenario != SCRIPTED_NETWORK) {
		return true;
	}
	// first test the list
	uint8 player_nr = player ? player->get_player_nr() : PLAYER_UNOWNED;
	while (1) {
		if (!forbidden_tools[player_nr].empty()) {
			forbidden_t test1(forbidden_t::forbid_tool, tool_id, wt, param);
			uint32 p_hash = test1.parameter_hash;

			// find if there is a general ban of this tool
			for (uint32 i = find_first_type_tool_wt(test1, player_nr); i < forbidden_tools[player_nr].get_count(); i++) {
				// there is something, we need to test more
				forbidden_t const& f = *forbidden_tools[player_nr][i];
				if (f.type != forbidden_t::forbid_tool || f.toolnr != tool_id) {
					// reached end of forbidden tools with this id => done
					break;
				}
				if (f.waytype == invalid_wt || f.waytype == wt) {
					if (f.parameter_hash == 0 || f.parameter_hash == p_hash) {
						// parameter matches too => forbidden
						const char* err = f.error.c_str();
						if (err == NULL) {
							err = "";
						}
						return err;
					}
				}
			}
		}
		if (player_nr == PLAYER_UNOWNED) {
			break;
		}
		// retry with unowned rules
		player_nr = PLAYER_UNOWNED;
	}

	// then call script if available
	if (what_scenario == SCRIPTED) {
		bool ok = true;
		const char* err = script->call_function(script_vm_t::FORCE, "is_tool_allowed", ok, (uint8)(player  ?  player->get_player_nr() : PLAYER_UNOWNED), tool_id, wt, param);
		return err != NULL  ||  ok;
	}

	return true;
}


const char* scenario_t::is_work_allowed_here(const player_t* player, uint16 tool_id, sint16 wt, const char* param, koord3d pos)
{
	if (what_scenario != SCRIPTED  &&  what_scenario != SCRIPTED_NETWORK) {
		return NULL;
	}

	// first test for allowed tools
	uint8 player_nr = player ? player->get_player_nr() : PLAYER_UNOWNED;
	while(1) {
		if (!forbidden_tools[player_nr].empty()) {
			// only area rules can allow
			forbidden_t test2(forbidden_t::allow_tool_rect, tool_id, wt, param);
			uint32 p_hash = test2.parameter_hash;
			for (uint32 i = find_first_type_tool_wt(test2, player_nr); i < forbidden_tools[player_nr].get_count(); i++) {
				// there is something, we need to test more
				forbidden_t const& f = *forbidden_tools[player_nr][i];
				if (f.type != forbidden_t::forbid_tool_rect || f.toolnr != tool_id) {
					// reached end of forbidden tools with this id => done
					break;
				}
				if (f.waytype == invalid_wt  ||  f.waytype == wt) {
					if (f.parameter_hash == 0  ||  f.parameter_hash == p_hash) {
						// parameter matches too => check rectangle
						if (f.pos_nw.x <= pos.x && f.pos_nw.y <= pos.y && pos.x <= f.pos_se.x && pos.y <= f.pos_se.y) {
							// check height
							if (f.hmin <= pos.z && pos.z <= f.hmax) {
								goto allowed_tool_rect;
							}
						}
					}
				}
			}
		}
		if (player_nr == PLAYER_UNOWNED) {
			break;
		}
		// retry with unowned rules
		player_nr = PLAYER_UNOWNED;
	}

	// then test for forbidden tools
	player_nr = player ? player->get_player_nr() : PLAYER_UNOWNED;
	while(1) {
		if (!forbidden_tools[player_nr].empty()) {
			forbidden_t test1(forbidden_t::forbid_tool, tool_id, wt, param);
			uint32 p_hash = test1.parameter_hash;

			// find if there is a general ban of this tool
			for (uint32 i = find_first_type_tool_wt(test1, player_nr); i < forbidden_tools[player_nr].get_count(); i++) {
				// there is something, we need to test more
				forbidden_t const& f = *forbidden_tools[player_nr][i];
				if (f.type != forbidden_t::forbid_tool || f.toolnr != tool_id) {
					// reached end of forbidden tools with this id => done
					break;
				}
				if (f.waytype == invalid_wt  ||  f.waytype == wt) {
					if (f.parameter_hash == 0  ||  f.parameter_hash == p_hash) {
						// parameter matches too => forbidden
						const char* err = f.error.c_str();
						if (err == NULL) {
							err = "";
						}
						return err;
					}
				}
			}

			// now check the area tools
			forbidden_t test2(forbidden_t::forbid_tool_rect, tool_id, wt, param);
			for (uint32 i = find_first_type_tool_wt(test2, player_nr); i < forbidden_tools[player_nr].get_count(); i++) {
				// there is something, we need to test more
				forbidden_t const& f = *forbidden_tools[player_nr][i];
				if (f.type != forbidden_t::forbid_tool_rect || f.toolnr != tool_id) {
					// reached end of forbidden tools with this id => done
					break;
				}
				if (f.waytype == invalid_wt  ||  f.waytype == wt) {
					if (f.parameter_hash == 0  ||  f.parameter_hash == p_hash) {
						// parameter matches too => check rectangle
						if (f.pos_nw.x <= pos.x && f.pos_nw.y <= pos.y && pos.x <= f.pos_se.x && pos.y <= f.pos_se.y) {
							// check height
							if (f.hmin <= pos.z && pos.z <= f.hmax) {
								const char* err = f.error.c_str();
								if (err == NULL) {
									err = "";
								}
								return err;
							}
						}
					}
				}
			}
			forbidden_t test3(forbidden_t::allow_tool_rect, tool_id, wt, param);
			for (uint32 i = find_first_type_tool_wt(test3, player_nr); i < forbidden_tools[player_nr].get_count(); i++) {
				// there is something, we need to test more
				forbidden_t const& f = *forbidden_tools[player_nr][i];
				if (f.type != forbidden_t::allow_tool_rect || f.toolnr != tool_id) {
					// reached end of forbidden tools with this id => done
					break;
				}
				if (f.waytype == invalid_wt  ||  f.waytype == wt) {
					if (f.parameter_hash == 0  ||  f.parameter_hash == p_hash) {
						// parameter matches too => check rectangle
						if (f.pos_nw.x <= pos.x && f.pos_nw.y <= pos.y && pos.x <= f.pos_se.x && pos.y <= f.pos_se.y) {
							// check height
							if (f.hmin <= pos.z && pos.z <= f.hmax) {
								return NULL;
							}
						}
					}
				}
			}
		}
		if (player_nr == PLAYER_UNOWNED) {
			break;
		}
		// retry with unowned rules
		player_nr = PLAYER_UNOWNED;
	}

allowed_tool_rect:
	// then call the script
	// cannot be done for two_click_tool_t's as they depend on routefinding,
	// which is done per client
	if (what_scenario == SCRIPTED) {
		koord3d start_pos = pos;
		bool is_drag_tool = false;
		bool is_ctrl = false;
		bool is_shift = false;
		uint8 player_nr = player ? player->get_player_nr() : PLAYER_UNOWNED;
		if(player_nr != PLAYER_UNOWNED){
			if (two_click_tool_t *two_tool = dynamic_cast<two_click_tool_t*>(welt->get_tool(player_nr))) {
				start_pos = two_tool->get_start_pos();
				is_drag_tool = !two_tool->is_first_click();
				is_ctrl = two_tool->is_ctrl_pressed();
				is_shift = two_tool->is_shift_pressed();
			}
			else if (tool_t *tool = dynamic_cast<tool_t*>(welt->get_tool(player_nr))) {
				is_ctrl = tool->is_ctrl_pressed();
				is_shift = tool->is_shift_pressed();
			}
		}
		static plainstring msg;
		const char *err = script->call_function(script_vm_t::FORCE, "is_work_allowed_here", msg, player_nr, tool_id, param, pos, script_api::mytool_data_t(start_pos, is_drag_tool, is_ctrl, is_shift));

		return err == NULL ? msg.c_str() : NULL;
	}
	return NULL;
}


const char* scenario_t::is_schedule_allowed(const player_t* player, const schedule_t* schedule)
{
	// sanity checks
	if (schedule == NULL) {
		return "";
	}
	if (env_t::server) {
		// networkgame: allowed
		return NULL;
	}
	// call script
	if (what_scenario == SCRIPTED) {
		static plainstring msg;
		const char *err = script->call_function(script_vm_t::FORCE, "is_schedule_allowed", msg, (uint8)(player  ?  player->get_player_nr() : PLAYER_UNOWNED), schedule);

		return err == NULL ? msg.c_str() : NULL;
	}
	return NULL;
}


const char* scenario_t::is_convoy_allowed(const player_t* player, convoihandle_t cnv, depot_t* depot)
{
	// sanity checks
	if (!cnv.is_bound()  ||  depot == NULL) {
		return "";
	}
	if (env_t::server) {
		// networkgame: allowed
		return NULL;
	}
	// call script
	if (what_scenario == SCRIPTED) {
		static plainstring msg;
		const char *err = script->call_function(script_vm_t::FORCE, "is_convoy_allowed", msg, (uint8)(player  ?  player->get_player_nr() : PLAYER_UNOWNED), cnv, (obj_t*)depot);

		return err == NULL ? msg.c_str() : NULL;
	}
	return NULL;

}


bool scenario_t::is_tool_enabled(const player_t * player, uint16 tool_id, sint16 wt, const char *param)
{
	if (what_scenario != SCRIPTED  &&  what_scenario != SCRIPTED_NETWORK) {
		return true;
	}
	// then call script if available
	if (what_scenario == SCRIPTED) {
		bool ok = true;
		const char* err = script->call_function(script_vm_t::FORCE, "is_tool_active", ok, (uint8)(player ? player->get_player_nr() : PLAYER_UNOWNED), tool_id, wt, param);
		return err != NULL || ok;
	}
	return true;
}


const char* scenario_t::jump_to_link_executed(koord3d pos)
{
	if (env_t::server) {
		// networkgame: allowed
		return NULL;
	}
	// call script
	if (what_scenario == SCRIPTED) {
		static plainstring msg;
		const char *err = script->call_function(script_vm_t::FORCE, "jump_to_link_executed", msg, pos);

		return err == NULL ? msg.c_str() : NULL;
	}
	return NULL;
}

const char* scenario_t::get_error_text()
{
	if (script) {
		return script->get_error();
	}
	return NULL;
}


void scenario_t::step()
{
	if (!script) {
		// update texts at clients if info window open
		if (env_t::networkmode  &&  !env_t::server  &&  win_get_magic(magic_scenario_info)) {
			update_scenario_texts();
		}
		return;
	}

	uint16 new_won = 0;
	uint16 new_lost = 0;

	// first check, whether win/loss state of any player changed
	for(uint32 i=0; i<PLAYER_UNOWNED; i++) {
		player_t *player = welt->get_player(i);
		uint16 mask = 1 << i;
		// player exists and has not won/lost yet
		if (player  &&  (((won | lost) & mask)==0)) {
			sint32 percentage = 0;
			// callback
			script->prepare_callback("scenario_t_set_completed", 2, i, percentage );
			// call script
			const char *err = script->call_function(script_vm_t::QUEUE, "is_scenario_completed", percentage, i);
			// clear callback
			script->clear_pending_callback();

			// script might have deleted the player
			player = welt->get_player(i);
			if (player == NULL) {
				continue;
			}
			// call completed?
			if (script_vm_t::is_call_suspended(err)) {
				continue;
			}
			player->set_scenario_completion(percentage);
			// won ?
			if (percentage >= 100) {
				new_won |= mask;
			}
			// lost ?
			else if (percentage < 0) {
				new_lost |= mask;
			}
		}
	}

	update_won_lost(new_won, new_lost);

	// update texts
	if (win_get_magic(magic_scenario_info) ) {
		update_scenario_texts();
	}

	// update toolbars if necessary
	if (need_toolbar_update) {
		tool_t::update_toolbars();
		need_toolbar_update = false;

		// reset active tool if now forbidden
		// check scenario conditions for all players
		for(uint8 player_nr = 0; player_nr < PLAYER_UNOWNED; player_nr++) {
			if (player_t *player = welt->get_player(player_nr)) {
				tool_t *tool = welt->get_tool(player_nr);

				if (!is_tool_allowed(player, tool->get_id(), tool->get_waytype(), tool->get_default_param())) {
					welt->local_set_tool(tool_t::general_tool[TOOL_QUERY], player);
				}
			}
		}
	}
}


void scenario_t::new_month()
{
	if (script) {
		script->call_function(script_vm_t::QUEUE, "new_month");
	}
}

void scenario_t::new_year()
{
	if (script) {
		script->call_function(script_vm_t::QUEUE, "new_year");
	}
}


void scenario_t::update_won_lost(uint16 new_won, uint16 new_lost)
{
	// server sends the new state to the clients
	if (env_t::server  &&  (new_won | new_lost)) {
		nwc_scenario_t *nwc = new nwc_scenario_t();
		nwc->won = new_won;
		nwc->lost = new_lost;
		nwc->what = nwc_scenario_t::UPDATE_WON_LOST;
		network_send_all(nwc, true);
	}

	// we are the champions
	if (new_won) {
		won |= new_won;
		// those are the losers
		new_lost = ~new_won;
	}
	if (new_lost) {
		lost |= new_lost;
	}

	// notify active player
	if ( (new_won|new_lost) & (1<<welt->get_active_player_nr()) ) {
		// most likely result text has changed, force update
		result_text.update(script, welt->get_active_player(), true);

		open_info_win();
	}
}


void scenario_t::update_scenario_texts()
{
	player_t *player = welt->get_active_player();
	info_text.update(script, player);
	goal_text.update(script, player);
	rule_text.update(script, player);
	result_text.update(script, player);
	about_text.update(script, player);
	debug_text.update(script, player);
	description_text.update(script, player);
}


plainstring scenario_t::load_language_file(const char* filename)
{
	if (filename == NULL) {
		return "(null)";
	}
	std::string path = scenario_path.c_str();
	// try user language
	std::string wanted_file = path + translator::get_lang()->iso + PATH_SEPARATOR + filename;

	const plainstring& cached = cached_text_files.get(wanted_file.c_str());
	if (cached != NULL) {
		// file already cached
		return cached;
	}
	// not cached: try to read file
	FILE* file = dr_fopen(wanted_file.c_str(), "rb");
	if (file == NULL) {
		// try English
		file = dr_fopen((path + "en" + PATH_SEPARATOR + filename).c_str(), "rb");
	}
	if (file == NULL) {
		// try scenario directory
		file = dr_fopen((path + filename).c_str(), "rb");
	}

	plainstring text = "";
	if (file) {
		fseek(file,0,SEEK_END);
		long len = ftell(file);
		if(len>0) {
			char* const buf = MALLOCN(char, len + 1);
			fseek(file,0,SEEK_SET);
			if (fread(buf, 1, len, file) == (size_t)len) {
				buf[len] = '\0';
				text = buf;
			}
			free(buf);
		}
		fclose(file);
	}
	// store text to cache
	cached_text_files.put(wanted_file.c_str(), text);

	return text;
}

bool scenario_t::open_info_win(const char* tab) const
{
	// pop up for the win
	scenario_info_t *si = (scenario_info_t*)win_get_magic(magic_scenario_info);
	if (si == NULL) {
		si = new scenario_info_t();
		if (create_win(si, w_info, magic_scenario_info) < 0) {
			// failed
			return false;
		}
	}
	si->open_tab(tab);
	return true; // dummy return value
}


void scenario_t::rdwr(loadsave_t *file)
{
	file->rdwr_short(what_scenario);
	if (file->is_version_less(111, 5)) {
		uint32 city_nr = 0;
		file->rdwr_long(city_nr);
		sint64 factor = 0;
		file->rdwr_longlong(factor);
		koord k(0,0);
		k.rdwr( file );
	}

	if (what_scenario != SCRIPTED  &&  what_scenario != SCRIPTED_NETWORK) {
		if (file->is_loading()) {
			what_scenario = 0;
		}
		return;
	}

	script_api::coordinate_transform_t::rdwr(file);
	file->rdwr_short(won);
	file->rdwr_short(lost);
	file->rdwr_str(scenario_name);

	// load scripts and scenario files
	if (what_scenario == SCRIPTED) {
		if (file->is_loading()) {
			// load persistent scenario data
			plainstring str;
			file->rdwr_str(str);
			dbg->warning("scenario_t::rdwr", "loaded persistent scenario data: %s", str.c_str());
			if (env_t::networkmode   &&  !env_t::server) {
				// client playing network scenario game:
				// script files are not available
				what_scenario = SCRIPTED_NETWORK;
				script = NULL;
			}
			else {
				// load script
				cbuffer_t script_filename;
				// assume error
				rdwr_error = true;
				// try addon directory first
				if (env_t::default_settings.get_with_private_paks()) {
					scenario_path = ( std::string("addons/") + env_t::pak_name + "scenario" + PATH_SEPARATOR + scenario_name.c_str() + "/").c_str();
					script_filename.printf("%sscenario.nut", scenario_path.c_str());
					rdwr_error = !load_script(script_filename);
				}

				// failed, try scenario from pakset directory
				if (rdwr_error) {
					scenario_path = (env_t::pak_dir + "scenario/" + scenario_name.c_str() + PATH_SEPARATOR).c_str();
					script_filename.clear();
					script_filename.printf("%sscenario.nut", scenario_path.c_str());
					rdwr_error = !load_script(script_filename);
				}

				if (!rdwr_error) {
					script_loader_t::load_compatibility_script(script);
					// restore persistent data
					const char* err = script->eval_string(str);
					if (err) {
						dbg->warning("scenario_t::rdwr", "error [%s] evaluating persistent scenario data", err);
						rdwr_error = true;
					}
					// load translations
					translator::load_files_from_folder( scenario_path.c_str(), "scenario" );
					// callback
					script->register_callback(&scenario_t::set_completion, "scenario_t_set_completed");
				}
				else {
					dbg->warning("scenario_t::rdwr", "could not load script file %s", (const char*)script_filename);
				}
			}
		}
		else {
			plainstring str;
			script->call_function(script_vm_t::FORCEX, "save", str);
			dbg->warning("scenario_t::rdwr", "write persistent scenario data: %s", str.c_str());
			file->rdwr_str(str);
		}
	}

	// load forbidden tool
	if (file->is_loading()) {
		clear_rules();
	}

	for (uint pnr = 0; pnr < MAX_PLAYER_COUNT; pnr++) {
		uint32 count = forbidden_tools[pnr].get_count();
		file->rdwr_long(count);

		for (uint32 i = 0; i < count; i++) {
			if (file->is_loading()) {
				forbidden_tools[pnr].append(new forbidden_t());
			}
			forbidden_tools[pnr][i]->rdwr(file);
		}
	}

	// cached strings
	if (file->is_version_atleast(120, 3)) {
		dynamic_string::rdwr_cache(file);
	}

	if (what_scenario == SCRIPTED  &&  file->is_loading()  &&  !rdwr_error) {
		const char* err = script->call_function(script_vm_t::FORCEX, "resume_game");
		if (err) {
			dbg->warning("scenario_t::rdwr", "error [%s] calling resume_game", err);
			rdwr_error = true;
		}
	}
	// client side of scripted game but not on a client
	if ( (what_scenario == SCRIPTED_NETWORK)  ^  (env_t::networkmode  &&  env_t::server==0) ) {
		what_scenario = 0;
		rdwr_error = true;
	}
}

void scenario_t::rotate90(const sint16 y_size)
{
	for (uint pnr = 0; pnr < MAX_PLAYER_COUNT; pnr++) {
		for (uint32 i = 0; i < forbidden_tools[pnr].get_count(); i++) {
			forbidden_tools[pnr][i]->rotate90(y_size);
		}
	}
}


// return percentage completed
sint32 scenario_t::get_completion(int player_nr)
{
	if ( what_scenario == 0  ||  player_nr < 0  ||  player_nr >= PLAYER_UNOWNED) {
		return 0;
	}
	// check if won / lost
	uint32 pl = player_nr;
	if (won & (1<<player_nr)) {
		return 100;
	}
	else if (lost & (1<<player_nr)) {
		return -1;
	}

	sint32 percentage = 0;
	player_t *player = welt->get_player(player_nr);

	if ( what_scenario == SCRIPTED ) {
		// take cached value
		if (player) {
			percentage = player->get_scenario_completion();
		}
	}
	else if ( what_scenario == SCRIPTED_NETWORK ) {
		cbuffer_t buf;
		buf.printf("is_scenario_completed(%d)", pl);
		const char *ret = dynamic_string::fetch_result((const char*)buf, NULL, NULL);
		percentage = ret ? atoi(ret) : 0;
		// cache value
		if (player) {
			player->set_scenario_completion(percentage);
		}
	}
	return min( 100, percentage);
}


bool scenario_t::set_completion(sint32 player_nr, sint32 percentage)
{
	if ( player_nr < 0  ||  player_nr >= PLAYER_UNOWNED) {
		return false;
	}
	player_t *player = welt->get_player(player_nr);
	if (player == NULL) {
		return false;
	}
	player->set_scenario_completion(percentage - player->get_scenario_completion());

	uint16 mask = 1 << player_nr;
	// check won/lost
	if (percentage < 0  &&  (lost & mask)==0) {
		update_won_lost(0, mask);
	}
	else if (percentage >= 100  &&  (won & mask)==0) {
		update_won_lost(mask, 0);
	}
	return true;
}


const char* scenario_t::eval_string(const char* squirrel_string) const
{
	if (what_scenario == SCRIPTED) {
		return script->eval_string(squirrel_string);
	}
	return "";
}
