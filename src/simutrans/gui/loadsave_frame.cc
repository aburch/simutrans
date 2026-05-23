/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"

#include <sys/stat.h>

#include "loadsave_frame.h"

#include "../sys/simsys.h"
#include "../world/simworld.h"
#include "../simversion.h"
#include "../pathes.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/sve_cache.h"

#include "../network/network.h"
#include "../network/network_cmd.h"
#include "../network/network_cmd_ingame.h"
#include "../network/network_socket_list.h"

#include "../tool/simtool.h"
#include "../utils/simstring.h"



/**
 * Action that's started with a button click
 */
bool loadsave_frame_t::item_action(const char *filename)
{
	if(do_load) {
		cbuffer_t param;
		param.printf("l%c%s", easy_server.pressed+'0', filename);
		tool_t::simple_tool[TOOL_WORK_WORLD]->set_default_param(param);
		welt->set_tool(tool_t::simple_tool[TOOL_WORK_WORLD], NULL);
	}
	else {
		cbuffer_t param;
		param.printf("s%s", filename);
		tool_t::simple_tool[TOOL_WORK_WORLD]->set_default_param(param);
		welt->set_tool(tool_t::simple_tool[TOOL_WORK_WORLD], NULL);
	}
	return true;
}



bool loadsave_frame_t::ok_action(const char *filename)
{
	return item_action(filename);
}



loadsave_frame_t::loadsave_frame_t(bool do_load, bool back_to_menu) : savegame_frame_t(".sve",false,"save/",true, back_to_menu)
{
	this->do_load = do_load;

	if(do_load) {
		set_name(translator::translate("Laden"));
		easy_server.init( button_t::square_automatic, "Start this as a server");
		bottom_left_frame.add_component(&easy_server);
	}
	else {
		set_filename(welt->get_settings().get_filename());
		set_name(translator::translate("Save"));
	}

	sve_cache_t::load_cache();
}


const char *loadsave_frame_t::get_info(const char *fname)
{
	return sve_cache_t::get_info(fname);
}


/**
 * Set the window associated helptext
 * @return the filename for the helptext, or NULL
 */
const char *loadsave_frame_t::get_help_filename() const
{
	return do_load ? "load.txt" : "save.txt";
}


loadsave_frame_t::~loadsave_frame_t()
{
	// save hashtable
	sve_cache_t::write_cache();
}


bool loadsave_frame_t::compare_items ( const dir_entry_t & entry, const char *info, const char *)
{
	return (strcmp(entry.label->get_text_pointer(), info) < 0);
}
