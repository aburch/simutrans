#include "ai_scripted.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "../gui/simwin.h"
#include "../gui/player_frame_t.h"

// scripting
#include "../script/script.h"
#include "../script/export_objs.h"
#include "../script/api/api.h"

// TODO load/save support
// TODO ai debug window
// TODO restructure script/*.nut files

ai_scripted_t::ai_scripted_t(uint8 nr) : ai_t(nr)
{
	script = NULL;
	set_name("The unknown AI player");
}

ai_scripted_t::~ai_scripted_t()
{
	delete script;
	script = NULL;
}

const char* ai_scripted_t::init( const char *ai_base, const char *ai_name_)
{
	if (script) {
		return "AI already initialized";
	}
	ai_name = ai_name_;

	// path to ai files
	cbuffer_t buf;
	buf.printf("%s%s/", ai_base, ai_name_);
	ai_path = buf;

	// ai script file
	buf.append("ai.nut");
	if (!load_script( buf )) {
		dbg->warning("ai_scripted_t::init", "could not load script file %s", (const char*)buf);
		delete script;
		script = NULL;
		return "Loading ai script failed";
	}

	// set the standard name
	buf.clear();
	buf.printf("player %i", player_nr-1);
	set_name(buf);

	// now call startup function
	uint8 dummy = 0;
	if (const char* err = script->call_function(script_vm_t::QUEUE, "start", dummy, get_player_nr())) {
		dbg->warning("ai_scripted_t::init", "error [%s] calling start", err);
	}

	// update player window
	if (ki_kontroll_t *frame = dynamic_cast<ki_kontroll_t *>( win_get_magic(magic_ki_kontroll_t) ) ) {
		frame->update_data();
	}

	return NULL;
}

bool ai_scripted_t::load_script(const char* filename)
{
	cbuffer_t buf;
	buf.printf("script-ai-%d.log", player_nr);

	script = new script_vm_t(ai_path.c_str(), buf);
	// load global stuff
	// constants must be known compile time
	export_global_constants(script->get_vm());

	// load ai base definitions
	buf.clear();
	buf.printf("%sscript/scenario_base.nut", env_t::program_dir );
	const char* err = script->call_script(buf);
	if (err) { // should not happen ...
		dbg->error("ai_scripted_t::load_script", "error [%s] calling %s", err, (const char*)buf);
		return false;
	}

	// register api functions
	register_export_function(script->get_vm());
	err = script->get_error();
	if (err) {
		dbg->error("ai_scripted_t::load_script", "error [%s] calling register_export_function", err);
		return false;
	}
	// set my player number
	script->set_my_player(get_player_nr());
	// load ai definition
	err = script->call_script(filename);
	if (err) {
		if (strcmp(err, "suspended")) {
			dbg->error("ai_scripted_t::load_script", "error [%s] calling %s", err, filename);
		}
		return false;
	}

	return true;
}

void ai_scripted_t::step()
{
	if(!active) {
		return;
	}

	if (script) {
		script->call_function(script_vm_t::QUEUE, "step");
	}
}

bool ai_scripted_t::new_month()
{
	bool res = ai_t::new_month();
	if (res  &&  script) {
		script->call_function(script_vm_t::QUEUE, "new_month");
	}
	return res;
}

void ai_scripted_t::new_year()
{
	ai_t::new_year();
	if (script) {
		script->call_function(script_vm_t::QUEUE, "new_year");
	}
}
