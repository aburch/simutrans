/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_TOOL_SELECTOR_H
#define GUI_TOOL_SELECTOR_H


#include "gui_frame.h"
#include "../tpl/vector_tpl.h"
#include "simwin.h"
#include "../dataobj/environment.h"

class tool_t;


/**
 * This class defines all toolbar dialogues, floating bar of tools, i.e. the part the user will see
 */
class tool_selector_t : public gui_frame_t
{
private:
	struct tool_data_t {
		tool_data_t(tool_t* t=NULL) : tool(t), selected(false) {}
		tool_t* tool; ///< pointer to associated tool
		bool selected;    ///< store whether tool was active during last call to tool_selector_t::draw
	};
	/// tool definitions
	vector_tpl<tool_data_t> tools;

	// get current toolbar number for saving
	uint32 toolbar_id;

	/**
	 * window width in toolboxes
	 */
	uint16 tool_icon_width;
	uint16 tool_icon_height;

	scr_coord offset, old_offset;

	uint16 tool_icon_disp_start;
	uint16 tool_icon_disp_end;

	bool has_prev_next, is_dragging;

	/**
	 * Window title
	 */
	const char *title;

	/**
	 * Name of the help file
	 */
	const char *help_file;

	// needs dirty redraw (only when changed)
	bool dirty;

	bool allow_break;

public:
	tool_selector_t(const char *title, const char *help_file, uint32 toolbar_id, bool allow_break=true );

	/**
	 * Add a new tool with values and tooltip text.
	 */
	void add_tool_selector(tool_t *tool_in);

	// purges toolbar
	void reset_tools();

	// untranslated title
	const char *get_internal_name() const {return title;}

	bool has_title() const OVERRIDE { return toolbar_id!=0; }

	const char *get_help_filename() const OVERRIDE {return help_file;}

	FLAGGED_PIXVAL get_titlecolor() const OVERRIDE { return env_t::default_window_title_color; }

	bool is_hit(int x, int y) OVERRIDE;

	/**
	 * Does this window need a next button in the title bar?
	 * @return true if such a button is needed
	 */
	bool has_next() const OVERRIDE {return has_prev_next;}

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	// since no information are needed to be saved to restore this, returning magic is enough
	uint32 get_rdwr_id() OVERRIDE { return magic_toolbar+toolbar_id; }

	bool empty(player_t *player) const;
};

#endif
