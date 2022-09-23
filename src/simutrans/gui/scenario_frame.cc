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
	pakset_scenario.printf("%s%sscenario/", env_t::base_dir, env_t::pak_name.c_str());

	addons_scenario.clear();
	addons_scenario.printf("addons/%sscenario/", env_t::pak_name.c_str());

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
	cbuffer_t param;

	param.printf("%i,%s", easy_server.pressed, fullpath);
	tool_t::simple_tool[TOOL_LOAD_SCENARIO]->set_default_param(param);
	welt->set_tool(tool_t::simple_tool[TOOL_LOAD_SCENARIO], NULL);
	tool_t::simple_tool[TOOL_LOAD_SCENARIO]->set_default_param(0);

	return true;
}


const char *scenario_frame_t::get_info(const char *filename)
{
	static char info[PATH_MAX];

	sprintf(info,"%s",str_get_filename(filename, false).c_str());

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
