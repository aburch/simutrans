/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "script_generator_frame.h"

#include "../tool/simtool.h"


script_generator_frame_t::script_generator_frame_t(tool_generate_script_t* tl, const char *_p, cbuffer_t &cmd)
	: savegame_frame_t("", false, _p, false)
{
	this->tool = tl;
	command = cmd;
	set_name(translator::translate("Save generated script"));
	set_focus(NULL);
}


/**
 * Action, started after button pressing.
 */
bool script_generator_frame_t::item_action(const char *fullpath)
{
	tool->save_script(fullpath,command);
	return true;
}


bool script_generator_frame_t::ok_action(const char *fullpath)
{
	tool->save_script(fullpath,command);
	return true;
}


const char *script_generator_frame_t::get_info(const char *)
{
	return "";
}


bool script_generator_frame_t::check_file( const char *, const char * )
{
	return true;
}
