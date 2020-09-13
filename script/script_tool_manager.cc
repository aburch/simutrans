/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "script_tool_manager.h"

#include "../dataobj/tabfile.h"

#include "../simdebug.h"
#include "../simtool-scripted.h"
#include "../simskin.h"
#include "../descriptor/skin_desc.h"
#include "../sys/simsys.h"
#include "../gui/tool_selector.h"
#include "../utils/cbuffer_t.h"
#include "../utils/searchfolder.h"


vector_tpl<tool_exec_script_t*> script_tool_manager_t::one_click_script_tools;
vector_tpl<tool_exec_two_click_script_t*> script_tool_manager_t::two_click_script_tools;


bool script_tool_manager_t::check_file( const char *path )
{
	cbuffer_t buf;
	buf.printf("%s/tool.nut", path );
	if (FILE* const f = dr_fopen(buf, "r")) {
		fclose(f);
		return true;
	}
	return false;
}


const scripted_tool_info_t* script_tool_manager_t::get_script_info(const char* path)
{
	scripted_tool_info_t* info = new scripted_tool_info_t();
	info->path = path;

	// read description.tab
	cbuffer_t buf;
	buf.printf("%s/description.tab", path);
	tabfile_t file;

	if (  file.open(buf)  ) {
		tabfileobj_t contents;
		file.read( contents );
		info->title    = contents.get_string("title", path);
		info->menu_arg = contents.get_string("menu", "");
		info->tooltip  = contents.get_string("tooltip", "");

		const char* skin_name = contents.get_string("icon", "");
		info->desc   = skinverwaltung_t::get_extra(skin_name, strlen(skin_name), skinverwaltung_t::cursor);
		info->is_one_click = !( strcmp(contents.get_string("type", "one_click"), "two_click")==0 );
	}
	else {
		// no description.tab, use default values
		info->title = path;
	}
	return info;
}


tool_t* script_tool_manager_t::load_tool(char const* path, tool_t* tool)
{
	if (!check_file(path)) {
		return tool;
	}
	const scripted_tool_info_t* info = get_script_info(path);

	if (tool) {
		// reinitialize existing tool
		exec_script_base_t* esb = dynamic_cast<exec_script_base_t*>(tool);
		assert(esb);
		esb->set_info(info);
	}
	else {
		// create new tool
		if (info->is_one_click) {
			tool = new tool_exec_script_t(info);
		}
		else {
			tool = new tool_exec_two_click_script_t(info);
		}
	}
	return tool;
}


void script_tool_manager_t::load_scripts(char const* path)
{
	searchfolder_t find;
	find.search(path, "", true, false);
	FOR(searchfolder_t, const &name, find) {
		cbuffer_t fullname;
		fullname.printf("%s%s",path,name);

		tool_t *tool = load_tool(fullname);

		if (tool_exec_script_t* ot = dynamic_cast<tool_exec_script_t*>(tool)) {
			one_click_script_tools.append(ot);
		}
		else if (tool_exec_two_click_script_t* tt = dynamic_cast<tool_exec_two_click_script_t*>(tool)) {
			two_click_script_tools.append(tt);
		}
	}
}


void script_tool_manager_t::fill_menu(tool_selector_t* tool_selector, char const* arg, sint16 /*sound_ok*/)
{
	for(uint32 i=0; i<one_click_script_tools.get_count(); i++) {
		tool_exec_script_t* tool = one_click_script_tools[i];
		if(  strcmp(tool->get_menu_arg(), arg)==0  ) {
			tool_selector->add_tool_selector(tool);
		}
	}
	for(uint32 i=0; i<two_click_script_tools.get_count(); i++) {
		tool_exec_two_click_script_t* tool = two_click_script_tools[i];
		if(  strcmp(tool->get_menu_arg(), arg)==0  ) {
			tool_selector->add_tool_selector(tool);
		}
	}
}
