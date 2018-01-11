#include "ai_scripted.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "../gui/simwin.h"
#include "../gui/player_frame_t.h"

// scripting
#include "../script/script.h"
#include "../script/export_objs.h"
#include "../script/api/api.h"

// TODO ai debug window

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
		if (strcmp(err, "suspended")) {
			dbg->warning("ai_scripted_t::init", "error [%s] calling start", err);
		}
	}

	// update player window
	if (ki_kontroll_t *frame = dynamic_cast<ki_kontroll_t *>( win_get_magic(magic_ki_kontroll_t) ) ) {
		frame->update_data();
	}

	return NULL;
}

bool load_base_script(script_vm_t *script, const char* base); // scenario.cc

bool ai_scripted_t::load_script(const char* filename)
{
	cbuffer_t buf;
	buf.printf("script-ai-%d.log", player_nr);

	script = new script_vm_t(ai_path.c_str(), buf);
	// load global stuff
	// constants must be known compile time
	export_global_constants(script->get_vm());

	// load scripting base definitions
	if (!load_base_script(script, "script_base.nut")) {
		return false;
	}
	// load ai base definitions
	if (!load_base_script(script, "ai_base.nut")) {
		return false;
	}

	// register api functions
	register_export_function(script->get_vm(), false);
	if (script->get_error()) {
		dbg->error("ai_scripted_t::load_script", "error [%s] calling register_export_function", script->get_error());
		return false;
	}
	// set my player number
	script->set_my_player(get_player_nr());
	// load ai definition
	if (const char* err = script->call_script(filename)) {
		if (strcmp(err, "suspended")) {
			dbg->error("ai_scripted_t::load_script", "error [%s] calling %s", err, filename);
		}
		return false;
	}

	return true;
}


const char* ai_scripted_t::reload_script()
{
	// save the script to string
	plainstring str;
	script->call_function(script_vm_t::FORCEX, "save", str);
	// save old vm instance
	script_vm_t *old_script = script;
	script = NULL;

	// ai script file
	cbuffer_t buf;
	buf.printf("%sai.nut", ai_path.c_str());
	if (!load_script( buf )) {
		dbg->warning("ai_scripted_t::reload_script", "could not reload script file %s", (const char*)buf);
		delete script;
		script = old_script;
		return "Reloading ai script failed";
	}
	// restore persistent data
	const char* err = script->eval_string(str);
	if (err) {
		dbg->warning("ai_scripted_t::reload_script", "error [%s] evaluating persistent ai data", err);
		return "Reloading ai script data failed";
	}
	// resume
	uint8 dummy = 0;
	script->call_function(script_vm_t::QUEUE, "resume_game", dummy, get_player_nr());
	return NULL;
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


void ai_scripted_t::rdwr(loadsave_t *file)
{
	ai_t::rdwr(file);

	script_api::coordinate_transform_t::rdwr(file);

	file->rdwr_str(ai_name);

	if (file->is_loading()) {
		// ai saved, but no script was assigned
		if (ai_name == NULL  ||  *ai_name == 0) {
			script = NULL;
			return;
		}
		// load persistent data
		plainstring str;
		file->rdwr_str(str);
		dbg->warning("ai_scripted_t::rdwr", "loaded persistent ai data: %s", str.c_str());

		if (env_t::networkmode  &&  !env_t::server) {
			// scripted players run on server only, for now at least
			delete script;
			script = NULL;
			return;
		}
		// DO NOT READ ANYTHING FROM file AFTER THIS POINT

		// load script
		cbuffer_t script_filename;

		// try addon directory first
		ai_path = ( std::string("addons/ai/") + ai_name.c_str() + "/").c_str();
		script_filename.printf("%sai.nut", ai_path.c_str());
		bool rdwr_error = !load_script(script_filename);

		// failed, try ai from program directory
		if (rdwr_error) {
			ai_path = ( std::string(env_t::program_dir) + "/ai/" + ai_name.c_str() + "/").c_str();
			script_filename.clear();
			script_filename.printf("%sai.nut", ai_path.c_str());
			rdwr_error = !load_script(script_filename);
		}

		if (!rdwr_error) {
			// restore persistent data
			const char* err = script->eval_string(str);
			if (err) {
				dbg->warning("ai_scripted_t::rdwr", "error [%s] evaluating persistent ai data", err);
				rdwr_error = true;
			}
		}
		else {
			dbg->warning("ai_scripted_t::rdwr", "could not load script file %s", (const char*)script_filename);
		}

		if (rdwr_error) {
			delete script;
			script = NULL;
		}
	}
	else {
		plainstring str("");
		if (script) {
			script->call_function(script_vm_t::FORCEX, "save", str);
			dbg->warning("ai_scripted_t::rdwr", "write persistent ai data: %s", str.c_str());
		}
		if (ai_name  &&  *ai_name) { // valid name, save even if script is NULL
			file->rdwr_str(str);
		}
	}
}


void ai_scripted_t::finish_rd()
{
	if (script) {
		uint8 dummy = 0;
		if (const char* err = script->call_function(script_vm_t::QUEUE, "resume_game", dummy, get_player_nr())) {
			if (strcmp(err, "suspended")) {
				dbg->warning("ai_scripted_t::finish_rd", "error [%s] calling resume_game", err);
			}
		}
	}
}
