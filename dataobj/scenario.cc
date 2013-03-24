#include "../simconst.h"
#include "../simtypes.h"
#include "../simdebug.h"

#include "../player/simplay.h"
#include "../simworld.h"
#include "../simmesg.h"
#include "../simmem.h"
#include "../simmenu.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/network.h"
#include "../dataobj/network_cmd_scenario.h"
#include "../dataobj/fahrplan.h"

#include "../utils/cbuffer_t.h"

// error popup
#include "../simwin.h"
#include "../gui/scenario_info.h"

// scripting
#include "../script/script.h"
#include "../script/export_objs.h"
#include "../script/api/api.h"

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
	about_text("get_about_text")
{
	welt = w;
	what_scenario = 0;

	script = NULL;
	rotation = 0;
	won = false;
	lost = false;
	rdwr_error = false;

	cached_text_files.clear();
}


scenario_t::~scenario_t()
{
	if (script) {
		delete script;
	}
	clear_ptr_vector(forbidden_tools);
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
	if ((err = script->call_function("get_map_file", mapfile))) {
		dbg->warning("scenario_t::init", "error [%s] calling get_map_file", err);
		return "No scenario map specified";
	}
	else {
		// savegame location
		buf.clear();
		buf.printf("%s%s/%s", scenario_base, scenario_name_, mapfile.c_str());
		if (!welt->load( buf )) {
			dbg->warning("scenario_t::init", "error loading savegame %s", err, (const char*)buf);
			return "Could not load scenario map!";
		}
	}

	// set savegame name
	buf.clear();
	buf.printf("%s.sve", scenario_name.c_str());
	welt->get_settings().set_filename( strdup(buf) );

	// load translations
	translator::load_files_from_folder( scenario_path.c_str(), "scenario" );
	cached_text_files.clear();

	what_scenario = SCRIPTED;
	rotation = 0;
	// register ourselves
	welt->set_scenario(this);
	welt->get_message()->clear();

	// set start time
	sint32 const time = welt->get_current_month();
	welt->get_settings().set_starting_year( time / 12);
	welt->get_settings().set_starting_month( time % 12);

	// now call startup function
	if ((err = script->call_function("start"))) {
		dbg->warning("scenario_t::init", "error [%s] calling start", err);
	}

	return NULL;
}


bool scenario_t::load_script(const char* filename)
{
	script = new script_vm_t();
	// load global stuff
	// constants must be known compile time
	export_global_constants(script->get_vm());
	// load scenario base definitions
	char basefile[1024];
	sprintf( basefile, "%sscript/scenario_base.nut", umgebung_t::program_dir );
	const char* err = script->call_script(basefile);
	if (err) { // should not happen ...
		dbg->error("scenario_t::load_script", "error [%s] calling %s", err, basefile);
		return false;
	}
	register_export_function(script->get_vm(), welt);

	// load scenario definition
	err = script->call_script(filename);
	if (err) {
		dbg->error("scenario_t::load_script", "error [%s] calling %s", err, filename);
		return false;
	}
	return true;
}


void scenario_t::koord_w2sq(koord &k) const
{
	switch( rotation ) {
		// 0: do nothing
		case 1: k = koord(k.y, welt->get_size().y-1 - k.x); break;
		case 2: k = koord(welt->get_size().x-1 - k.x, welt->get_size().y-1 - k.y); break;
		case 3: k = koord(welt->get_size().x-1 - k.y, k.x); break;
		default: break;
	}
}


void scenario_t::koord_sq2w(koord &k)
{
	// just rotate back
	rotation = 4 - rotation;
	koord_w2sq(k);
	// restore original rotation
	rotation = 4 - rotation;
}


void scenario_t::ribi_w2sq(ribi_t::ribi &r) const
{
	if (rotation) {
		r = ( ( (r << 4) | r) >> rotation) & 15;
	}
}


void scenario_t::ribi_sq2w(ribi_t::ribi &r) const
{
	if (rotation) {
		r = ( ( (r << 4) | r) << rotation) >> 4 & 15;
	}
}


const char* scenario_t::get_forbidden_text()
{
	static cbuffer_t buf;
	buf.clear();
	buf.append("<h1>Forbidden stuff:</h1><br>");
	for(uint32 i=0; i<forbidden_tools.get_count(); i++) {
		scenario_t::forbidden_t &f = *forbidden_tools[i];
		buf.printf("[%d] Player = %d, Tool = %d", i, f.player_nr, f.toolnr);
		if (f.waytype!=invalid_wt) {
			buf.printf(", Waytype = %d", f.waytype);
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
	return buf;
}


bool scenario_t::forbidden_t::operator <(const forbidden_t &other) const
{
	bool lt = type < other.type;
	if (!lt  &&  type == other.type) {
		sint32 diff = (sint32)player_nr - (sint32)other.player_nr;
		if (diff == 0) {
			diff = (sint32)toolnr - (sint32)other.toolnr;
		}
		if (diff == 0) {
			diff = (sint32)waytype - (sint32)other.waytype;
		}
		lt = diff < 0;
	}
	return lt;
}


bool scenario_t::forbidden_t::operator ==(const forbidden_t &other) const
{
	bool eq =  (type == other.type)  &&  (player_nr == other.player_nr)  &&  (waytype == other.waytype);
	if (eq) {
		switch(type) {
			case forbid_tool_rect:
				eq = eq  &&  (hmin == other.hmin)  &&  (hmax == other.hmax);
				eq = eq  &&  (pos_nw == other.pos_nw);
				eq = eq  &&  (pos_se == other.pos_se);
			case forbid_tool:
				eq = eq  &&  (toolnr == other.toolnr);
				break;
		}
	}
	return eq;
}


scenario_t::forbidden_t::forbidden_t(const forbidden_t& other) :
	type(other.type), player_nr(other.player_nr), toolnr(other.player_nr),
	waytype(other.waytype), pos_nw(other.pos_nw), pos_se(other.pos_se),
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


uint32 scenario_t::find_first(const forbidden_t &other) const
{
	if (forbidden_tools.empty()  ||  *forbidden_tools.back() < other) {
		// empty vector, or everything is smaller
		return forbidden_tools.get_count();
	}
	if (other < (*forbidden_tools[0])) {
		// everything is larger
		return forbidden_tools.get_count();
	}
	else if ( other <= *forbidden_tools[0] ) {
		return 0;
	}
	// now: low < other <= high
	uint32 low = 0, high = forbidden_tools.get_count()-1;
	while(low+1 < high) {
		uint32 mid = (low+high) / 2;
		if (*forbidden_tools[mid] < other) {
			low = mid;
			// now low < other
		}
		else {
			high = mid;
			// now other <= high
		}
	};
	// still: low < other <= high
	bool notok = other < *forbidden_tools[high];
	return notok   ?  forbidden_tools.get_count()  :  high;
}


void scenario_t::intern_forbid(forbidden_t *test, bool forbid)
{
	bool changed = false;
	forbidden_t::forbid_type type = test->type;

	for(uint32 i = find_first(*test); i < forbidden_tools.get_count()  &&  *forbidden_tools[i] <= *test; i++) {
		if (*test == *forbidden_tools[i]) {
			// entry exists already
			delete test;
			if (!forbid) {
				delete forbidden_tools[i];
				forbidden_tools.remove_at(i);
				changed = true;
			}
			goto end;
		}
	}
	// entry does not exist
	if (forbid) {
		forbidden_tools.insert_ordered(test, scenario_t::forbidden_t::compare);
		changed = true;
	}
end:
	if (changed) {
		switch(type) {
			case forbidden_t::forbid_tool:
				werkzeug_t::update_toolbars(welt);
				break;
			default: ;
		}
	}
}

void scenario_t::call_forbid_tool(forbidden_t *test, bool forbid)
{
	if (umgebung_t::server) {
		// send information over network
		nwc_scenario_rules_t *nws = new nwc_scenario_rules_t(welt->get_sync_steps() + 1, welt->get_map_counter());
		nws->rule = test;
		nws->forbid = forbid;
		network_send_all(nws, false);
	}
	else {
		// directly apply
		intern_forbid(test, forbid);
	}
}

void scenario_t::forbid_tool(uint8 player_nr, uint16 wkz_id)
{
	forbidden_t *test = new forbidden_t(forbidden_t::forbid_tool, player_nr, wkz_id, invalid_wt);
	call_forbid_tool(test, true);
}


void scenario_t::allow_tool(uint8 player_nr, uint16 wkz_id)
{
	forbidden_t *test = new forbidden_t(forbidden_t::forbid_tool, player_nr, wkz_id, invalid_wt);
	call_forbid_tool(test, false);
}


void scenario_t::forbid_way_tool(uint8 player_nr, uint16 wkz_id, waytype_t wt)
{
	forbidden_t *test = new forbidden_t(forbidden_t::forbid_tool, player_nr, wkz_id, wt);
	call_forbid_tool(test, true);
}


void scenario_t::allow_way_tool(uint8 player_nr, uint16 wkz_id, waytype_t wt)
{
	forbidden_t *test = new forbidden_t(forbidden_t::forbid_tool, player_nr, wkz_id, wt);
	call_forbid_tool(test, false);
}


void scenario_t::forbid_way_tool_rect(uint8 player_nr, uint16 wkz_id, waytype_t wt, koord pos_nw, koord pos_se, plainstring err)
{
	forbid_way_tool_cube(player_nr, wkz_id, wt, koord3d(pos_nw, -128), koord3d(pos_se, 127), err);
}


void scenario_t::allow_way_tool_rect(uint8 player_nr, uint16 wkz_id, waytype_t wt, koord pos_nw, koord pos_se)
{
	allow_way_tool_cube(player_nr, wkz_id, wt, koord3d(pos_nw, -128), koord3d(pos_se, 127));
}


void scenario_t::forbid_way_tool_cube(uint8 player_nr, uint16 wkz_id, waytype_t wt, koord3d pos_nw_0, koord3d pos_se_0, plainstring err)
{
	koord pos_nw( min(pos_nw_0.x, pos_se_0.x), min(pos_nw_0.y, pos_se_0.y));
	koord pos_se( max(pos_nw_0.x, pos_se_0.x), max(pos_nw_0.y, pos_se_0.y));
	sint8 hmin( min(pos_nw_0.z, pos_se_0.z) );
	sint8 hmax( max(pos_nw_0.z, pos_se_0.z) );

	forbidden_t *test = new forbidden_t(player_nr, wkz_id, wt, pos_nw, pos_se, hmin, hmax);
	test->error = err;
	call_forbid_tool(test, true);
}


void scenario_t::allow_way_tool_cube(uint8 player_nr, uint16 wkz_id, waytype_t wt, koord3d pos_nw_0, koord3d pos_se_0)
{
	koord pos_nw( min(pos_nw_0.x, pos_se_0.x), min(pos_nw_0.y, pos_se_0.y));
	koord pos_se( max(pos_nw_0.x, pos_se_0.x), max(pos_nw_0.y, pos_se_0.y));
	sint8 hmin( min(pos_nw_0.z, pos_se_0.z) );
	sint8 hmax( max(pos_nw_0.z, pos_se_0.z) );

	forbidden_t *test = new forbidden_t(player_nr, wkz_id, wt, pos_nw, pos_se, hmin, hmax);
	call_forbid_tool(test, false);
}


void scenario_t::clear_rules()
{
	clear_ptr_vector(forbidden_tools);
}


bool scenario_t::is_tool_allowed(spieler_t* sp, uint16 wkz_id, sint16 wt)
{
	if (what_scenario != SCRIPTED  &&  what_scenario != SCRIPTED_NETWORK) {
		return true;
	}
	// first test the list
	if (!forbidden_tools.empty()) {
		forbidden_t test(forbidden_t::forbid_tool, PLAYER_UNOWNED, wkz_id, invalid_wt);
		uint8 player_nr = sp  ?  sp->get_player_nr() :  PLAYER_UNOWNED;

		// first test waytype invalid_wt, then wt
		// .. and all players then specific player
		for(uint32 wti = 0; wti<4; wti++) {
			if (find_first(test) < forbidden_tools.get_count()) {
				// there is something, hence forbidden
				return false;
			}
			// logic to test all possible four cases
			sim::swap<sint16>( wt, test.waytype );
			if (test.waytype == invalid_wt) {
				sim::swap<uint8>( player_nr, test.player_nr );
				if (test.player_nr == PLAYER_UNOWNED) {
					break;
				}
			}
		}
	}
	// then call script if available
	if (what_scenario == SCRIPTED) {
		bool ok = true;
		const char* err = script->call_function("is_tool_allowed", ok, (uint8)(sp  ?  sp->get_player_nr() : PLAYER_UNOWNED), wkz_id, wt);
		return err != NULL  ||  ok;
	}

	return true;
}

const char* scenario_t::is_work_allowed_here(spieler_t* sp, uint16 wkz_id, sint16 wt, koord3d pos)
{
	if (what_scenario != SCRIPTED  &&  what_scenario != SCRIPTED_NETWORK) {
		return NULL;
	}

	// first test the list
	if (!forbidden_tools.empty()) {
		forbidden_t test(forbidden_t::forbid_tool_rect, PLAYER_UNOWNED, wkz_id, invalid_wt);
		uint8 player_nr = sp  ?  sp->get_player_nr() :  PLAYER_UNOWNED;

		// first test waytype invalid_wt, then wt
		// .. and all players then specific player
		for(uint32 wti = 0; wti<4; wti++) {
			for(uint32 i = find_first(test); i < forbidden_tools.get_count()  &&  *forbidden_tools[i] <= test; i++) {
				forbidden_t const& f = *forbidden_tools[i];
				// check rectangle
				if (f.pos_nw.x <= pos.x  &&  f.pos_nw.y <= pos.y  &&  pos.x <= f.pos_se.x  &&  pos.y <= f.pos_se.y) {
					// check height
					if (f.hmin <= pos.z  &&  pos.z <= f.hmax) {
						const char* err = f.error.c_str();
						if (err == NULL) {
							err = "";
						}
						return err;
					}
				}
			}
			// logic to test all possible four cases
			sim::swap<sint16>( wt, test.waytype );
			if (test.waytype == invalid_wt) {
				sim::swap<uint8>( player_nr, test.player_nr );
				if (test.player_nr == PLAYER_UNOWNED) {
					break;
				}
			}
		}
	}

	// then call the script
	// cannot be done for two_click_tool_t's as they depend on routefinding,
	// which is done per client
	if (what_scenario == SCRIPTED) {
		static plainstring msg;
		const char *err = script->call_function("is_work_allowed_here", msg, (uint8)(sp ? sp->get_player_nr() : PLAYER_UNOWNED), wkz_id, pos);

		return err == NULL ? msg.c_str() : NULL;
	}
	return NULL;
}


const char* scenario_t::is_schedule_allowed(spieler_t* sp, schedule_t* schedule)
{
	// sanity checks
	if (schedule == NULL) {
		return "";
	}
	if (schedule->empty()  ||  umgebung_t::server) {
		// empty schedule, networkgame: all allowed
		return NULL;
	}
	// call script
	if (what_scenario == SCRIPTED) {
		static plainstring msg;
		const char *err = script->call_function("is_schedule_allowed", msg, (uint8)(sp ? sp->get_player_nr() : PLAYER_UNOWNED), schedule);

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
		if (umgebung_t::networkmode  &&  !umgebung_t::server  &&  win_get_magic(magic_scenario_info)) {
			update_scenario_texts();
		}
		return;
	}

	uint16 new_won = 0;
	uint16 new_lost = 0;

	// first check, whether win/loss state of any player changed
	for(uint32 i=0; i<PLAYER_UNOWNED; i++) {
		spieler_t *sp = welt->get_spieler(i);
		uint16 mask = 1 << i;
		// player exists and has not won/lost yet
		if (sp  &&  (((won | lost) & mask)==0)) {
			sint32 percentage = 0;
			script->call_function("is_scenario_completed", percentage, (uint8)(sp ? sp->get_player_nr() : PLAYER_UNOWNED));

			// script might have deleted the player
			sp = welt->get_spieler(i);
			if (sp == NULL) {
				continue;
			}

			sp->set_scenario_completion(percentage);
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

	// server sends the new state to the clients
	if (umgebung_t::server  &&  (new_won | new_lost)) {
		nwc_scenario_t *nwc = new nwc_scenario_t();
		nwc->won = new_won;
		nwc->lost = new_lost;
		nwc->what = nwc_scenario_t::UPDATE_WON_LOST;
		network_send_all(nwc, true);
	}

	// update texts
	if (win_get_magic(magic_scenario_info) ) {
		update_scenario_texts();
	}
}


void scenario_t::update_won_lost(uint16 new_won, uint16 new_lost)
{
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
	spieler_t *sp = welt->get_active_player();
	info_text.update(script, sp);
	goal_text.update(script, sp);
	rule_text.update(script, sp);
	result_text.update(script, sp);
	about_text.update(script, sp);
	description_text.update(script, sp);
}


plainstring scenario_t::load_language_file(const char* filename)
{
	std::string path = scenario_path.c_str();
	// try user language
	std::string wanted_file = path + translator::get_lang()->iso + "/" + filename;

	const plainstring& cached = cached_text_files.get(wanted_file.c_str());
	if (cached != NULL) {
		// file already cached
		return cached;
	}
	// not cached: try to read file
	FILE* file = fopen(wanted_file.c_str(), "rb");
	if (file == NULL) {
		// try English
		file = fopen((path + "en/" + filename).c_str(), "rb");
	}
	if (file == NULL) {
		// try scenario directory
		file = fopen((path + filename).c_str(), "rb");
	}

	plainstring text = "";
	if (file) {
		fseek(file,0,SEEK_END);
		long len = ftell(file);
		if(len>0) {
			char* const buf = MALLOCN(char, len + 1);
			fseek(file,0,SEEK_SET);
			fread(buf, 1, len, file);
			buf[len] = '\0';
			text = buf;
			free(buf);
		}
		fclose(file);
	}
	// store text to cache
	cached_text_files.put(wanted_file.c_str(), text);

	return text;
}

bool scenario_t::open_info_win() const
{
	// pop up for the win
	scenario_info_t *si = (scenario_info_t*)win_get_magic(magic_scenario_info);
	if (si == NULL) {
		si = new scenario_info_t(welt);
		create_win(si, w_info, magic_scenario_info);
	}
	si->open_result_tab();
	return true; // dummy return value
}


void scenario_t::rdwr(loadsave_t *file)
{
	file->rdwr_short(what_scenario);
	if (file->get_version() <= 111004) {
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

	file->rdwr_byte(rotation);
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
			if (umgebung_t::networkmode   &&  !umgebung_t::server) {
				// client playing network scenario game:
				// script files are not available
				what_scenario = SCRIPTED_NETWORK;
				script = NULL;
			}
			else {
				// load script
				cbuffer_t script_filename;

				// try addon directory first
				scenario_path = ( std::string("addons/") + umgebung_t::objfilename + "scenario/" + scenario_name.c_str() + "/").c_str();
				script_filename.printf("%sscenario.nut", scenario_path.c_str());
				rdwr_error = !load_script(script_filename);

				// failed, try scenario from pakset directory
				if (rdwr_error) {
					scenario_path = (umgebung_t::program_dir + umgebung_t::objfilename + "scenario/" + scenario_name.c_str() + "/").c_str();
					script_filename.clear();
					script_filename.printf("%sscenario.nut", scenario_path.c_str());
					rdwr_error = !load_script(script_filename);
				}

				if (!rdwr_error) {
					// restore persistent data
					const char* err = script->eval_string(str);
					if (err) {
						dbg->warning("scenario_t::rdwr", "error [%s] evaluating persistent scenario data", err);
						rdwr_error = true;
					}
					// load translations
					translator::load_files_from_folder( scenario_path.c_str(), "scenario" );
				}
				else {
					dbg->warning("scenario_t::rdwr", "could not load script file %s", (const char*)script_filename);
				}
			}
		}
		else {
			plainstring str;
			script->call_function("save", str);
			dbg->warning("scenario_t::rdwr", "write persistent scenario data: %s", str.c_str());
			file->rdwr_str(str);
		}
	}

	// load forbidden tools
	if (file->is_loading()) {
		clear_ptr_vector(forbidden_tools);
	}
	uint32 count = forbidden_tools.get_count();
	file->rdwr_long(count);

	for(uint32 i=0; i<count; i++) {
		if (file->is_loading()) {
			forbidden_tools.append(new forbidden_t());
		}
		forbidden_tools[i]->rdwr(file);
	}

	if (what_scenario == SCRIPTED  &&  file->is_loading()  &&  !rdwr_error) {
		const char* err = script->call_function("resume_game");
		if (err) {
			dbg->warning("scenario_t::rdwr", "error [%s] calling resume_game", err);
			rdwr_error = true;
		}
	}
}

void scenario_t::rotate90(const sint16 y_size)
{
	rotation ++;
	rotation %= 4;

	for(uint32 i=0; i<forbidden_tools.get_count(); i++) {
		forbidden_tools[i]->rotate90(y_size);
	}
}


// return percentage completed
int scenario_t::get_completion(int player_nr)
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
	spieler_t *sp = welt->get_spieler(player_nr);

	if ( what_scenario == SCRIPTED ) {
		// take cached value
		if (sp) {
			percentage = sp->get_scenario_completion();
		}
	}
	else if ( what_scenario == SCRIPTED_NETWORK ) {
		cbuffer_t buf;
		buf.printf("is_scenario_completed(%d)", pl);
		const char *ret = dynamic_string::fetch_result((const char*)buf, NULL, NULL);
		percentage = ret ? atoi(ret) : 0;
		// cache value
		if (sp) {
			sp->set_scenario_completion(percentage);
		}
	}
	return min( 100, percentage);
}
