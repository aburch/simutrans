/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "script_generator_frame.h"

#include "../tool/simtool.h"
#include "../sys/simsys.h"


script_generator_frame_t::script_generator_frame_t(tool_generate_script_t* tl, const char *_p, cbuffer_t &cmd, koord a)
	: savegame_frame_t("", false, _p, true)
{
	this->tool = tl;
	command = cmd;
	area = a;
	set_name(translator::translate("Save generated script"));
}


/**
 * Action, started after button pressing.
 */
bool script_generator_frame_t::item_action(const char *fullpath)
{
	tool->save_script(fullpath,command,area);
	return true;
}


bool script_generator_frame_t::ok_action(const char *fullpath)
{
	tool->save_script(fullpath,command,area);
	return true;
}


bool script_generator_frame_t::del_action(const char* fullpath)
{
	dr_chdir(fullpath);
	dr_remove("description.tab");
	dr_remove("tool.nut");
	dr_remove("script-exec-0.log");
	dr_chdir("..");
	const char* p = strrchr(fullpath, *PATH_SEPARATOR);
	if (!p) {
		p = strrchr(fullpath, '/');
	}
	if (p) {
		return dr_remove(p + 1);
	}
	return false;
}


const char *script_generator_frame_t::get_info(const char *)
{
	return "";
}


bool script_generator_frame_t::check_file( const char *f, const char * )
{
	return *f;
}
