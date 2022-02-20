/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TOOL_SIMTOOL_SCRIPTED_H
#define TOOL_SIMTOOL_SCRIPTED_H


#include "../tool/simmenu.h"
#include "../script/script.h"
#include "../utils/plainstring.h"

class script_vm_t;
class skin_desc_t;


/**
 * Information about scripted tools, extracted from description.tab.
 * - data from description.tab:
 *
 *       plainstring  path
 *       plainstring  title=tool for test 2
 *       bool         type=one_click or two_click
 *       bool         restart=1
 *       plainstring  menu=my_script
 *       skin_desc_t* icon=one_click_test: cursor, icon, marker
 *
 */
struct scripted_tool_info_t {
	plainstring path;        ///< path to files
	plainstring title;       ///< name of tool (used for dialog)
	plainstring tooltip;
	plainstring menu_arg;    ///< menu name, where this tool should appear (used in menuconf.tab)
	const skin_desc_t* desc; ///< skin object used for cursor (0), icon (1), and maybe marker (2)
	bool restart;            ///< true, if script vm has to be restarted after work()
	bool is_one_click;       ///< true, if tool is one-click (otherwise needs two clicks/coordinates to work)
	koord cursor_area;       ///< size of cursor defined in tool_t
	koord cursor_offset;     ///< cursor offset defined in tool_t
	/// sets default values
	scripted_tool_info_t()
	{
		desc = NULL;
		restart = true;
		is_one_click = true;
		cursor_area = koord(1,1);
		cursor_offset = koord(0,0);
	}
};

class exec_script_base_t {
private:
	/// information about tool, take ownership of pointer
	const scripted_tool_info_t *info;

	void load_script(const char* path, player_t* player);
protected:
	/// the vm, will be initialized in init()
	script_vm_t *script;
	/// starts vm, sets our_player, returns true if successful
	bool init_vm(player_t* player);

	template<class R, class... As>
	const char* call_function(script_vm_t::call_type_t ct, const char* function, player_t* player, R& ret, const As &... as);
public:
	exec_script_base_t(const scripted_tool_info_t *i) : info(i), script(NULL) {}
	virtual ~exec_script_base_t();

	void set_info(const scripted_tool_info_t *i);
	const scripted_tool_info_t* get_info() const { return info; }

	void init_images(tool_t *tool) const;

	const char* get_menu_arg() const { return info ? info->menu_arg.c_str() : ""; }

	/// has to be called if the tool is active, to resume script if a work-command gets suspended
	void step(player_t* player);

	const char *get_tooltip(const player_t *) const { return info ? info->tooltip.c_str() : ""; }
	koord get_cursor_area() const { return info->cursor_area; }
	koord get_cursor_offset() const { return info->cursor_offset; }
};


class tool_exec_script_t : public tool_t, public exec_script_base_t {
protected:

public:
	tool_exec_script_t(const scripted_tool_info_t *info = NULL);
	/// is network-safe, as calls to work-commands will be properly handled in network mode
	bool is_init_network_safe() const OVERRIDE { return true; }
	bool is_work_network_safe() const OVERRIDE { return true; }

	bool init(player_t* player) OVERRIDE;
	bool exit(player_t* player) OVERRIDE;

	const char *work(player_t* player, koord3d pos) OVERRIDE;

	const char *get_tooltip(const player_t *pl) const OVERRIDE { return exec_script_base_t::get_tooltip(pl); }
};

class tool_exec_two_click_script_t : public two_click_tool_t, public exec_script_base_t {

	image_id marker;
public:
	tool_exec_two_click_script_t(const scripted_tool_info_t *info = NULL);
	/// is network-safe, as calls to work-commands will be properly handled in network mode
	bool is_work_network_safe() const OVERRIDE { return true; }
	bool is_init_network_safe() const OVERRIDE { return true; }

	bool init(player_t* player) OVERRIDE;
	bool exit(player_t* player) OVERRIDE;

	uint8 is_valid_pos(player_t* player, const koord3d &pos, const char *&error, const koord3d &start) OVERRIDE;
	const char *do_work(player_t* player, const koord3d &start, const koord3d &end) OVERRIDE;
	void mark_tiles(player_t* player, const koord3d &start, const koord3d &end) OVERRIDE;

	bool mark_tile(player_t* player, const koord3d &start);
	// two_click_tool_t calls init() after do_work(),
	// because it assumes all work is done.
	// We have to block this.
	bool waiting_for_do_work;
	// instead, init() will be called in step()
	// if necessary
	bool needs_call_to_init;

	void set_marker(image_id m) { marker = m; }

	virtual image_id get_marker_image() const OVERRIDE;

	const char *get_tooltip(const player_t *pl) const OVERRIDE { return exec_script_base_t::get_tooltip(pl); }
};

#endif
