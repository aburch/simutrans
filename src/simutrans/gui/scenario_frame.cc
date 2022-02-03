/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"
#include "../sys/simsys.h"

#include "scenario_frame.h"
#include "scenario_info.h"
#include "messagebox.h"

#include "simwin.h"
#include "../world/simworld.h"
#include "../tool/simmenu.h"

#include "../dataobj/environment.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"

#include "../network/network.h"
#include "../network/network_cmd.h"

#include "../utils/cbuffer.h"

scenario_frame_t::scenario_frame_t() : savegame_frame_t(NULL, true, NULL, false)
{
	static cbuffer_t pakset_scenario;
	static cbuffer_t addons_scenario;

	pakset_scenario.clear();
	pakset_scenario.printf("%s%sscenario/", env_t::data_dir, env_t::objfilename.c_str());

	addons_scenario.clear();
	addons_scenario.printf("addons/%sscenario/", env_t::objfilename.c_str());

	if (env_t::default_settings.get_with_private_paks()) {
		this->add_path(addons_scenario);
	}
	this->add_path(pakset_scenario);

	easy_server.init( button_t::square_automatic, "Start this as a server");
	bottom_left_frame.add_component(&easy_server);

	set_name(translator::translate("Load scenario"));
	set_focus(NULL);
}


/**
 * Action, started after button pressing.
 */
bool scenario_frame_t::item_action(const char *fullpath)
{
	// since loading a scenario may not init the world
	welt->switch_server( easy_server.pressed, true );

	scenario_t *scn = new scenario_t(welt);

	const char* err = scn->init(this->get_basename(fullpath).c_str(), this->get_filename(fullpath).c_str(), welt );
	if (err == NULL) {
		// start the game
		welt->set_pause(false);
		// open scenario info window
		destroy_win(magic_scenario_info);
		create_win(new scenario_info_t(), w_info, magic_scenario_info);
		tool_t::update_toolbars();
		if(  env_t::server  ) {
			welt->announce_server(karte_t::SERVER_ANNOUNCE_HELLO);
		}
	}
	else {
		if(  env_t::server  ) {
			welt->switch_server( false, true );
		}
		create_win(new news_img(err), w_info, magic_none);
		delete scn;
	}

	return true;
}


const char *scenario_frame_t::get_info(const char *filename)
{
	static char info[PATH_MAX];

	sprintf(info,"%s",this->get_filename(filename, false).c_str());

	return info;
}


bool scenario_frame_t::check_file( const char *filename, const char * )
{
	char buf[PATH_MAX];
	sprintf( buf, "%s/scenario.nut", filename );
	if (FILE* const f = dr_fopen(buf, "r")) {
		fclose(f);
		return true;
	}
	return false;
}
