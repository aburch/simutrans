/*
 * Copyright (c) 2008 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

/*
 * This class defines all toolbar dialogues, i.e. the part the user will see
 */

#ifndef gui_werkzeug_waehler_h
#define gui_werkzeug_waehler_h

#include "gui_frame.h"
#include "../tpl/vector_tpl.h"
#include "../gui/simwin.h"

class werkzeug_t;


class werkzeug_waehler_t : public gui_frame_t
{
private:
	struct tool_data_t {
		tool_data_t(werkzeug_t* t=NULL) : tool(t), selected(false) {}
		werkzeug_t* tool; ///< pointer to associated tool
		bool selected;    ///< store whether tool was active during last call to werkzeug_waehler_t::draw
	};
	/// tool definitions
	vector_tpl<tool_data_t> tools;

	// get current toolbar number for saving
	uint32 toolbar_id;

	/**
	 * window width in toolboxes
	 * @author Hj. Malthaner
	 */
	uint16 tool_icon_width;
	uint16 tool_icon_height;

	uint16 tool_icon_disp_start;
	uint16 tool_icon_disp_end;

	bool has_prev_next;

	/**
	 * Window title
	 * @author Hj. Malthaner
	 */
	const char *titel;

	/**
	 * Name of the help file
	 * @author Hj. Malthaner
	 */
	const char *hilfe_datei;

	// needs dirty redraw (only when changed)
	bool dirty;

	bool allow_break;

public:
	werkzeug_waehler_t(const char *titel, const char *helpfile, uint32 toolbar_id, bool allow_break=true );

	/**
	 * Add a new tool with values and tooltip text.
	 * @author Hj. Malthaner
	 */
	void add_werkzeug(werkzeug_t *w);

	// purges toolbar
	void reset_tools();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char *get_hilfe_datei() const {return hilfe_datei;}

	PLAYER_COLOR_VAL get_titelcolor() const { return WIN_TITEL; }

	bool getroffen(int x, int y) OVERRIDE;

	/**
	 * Does this window need a next button in the title bar?
	 * @return true if such a button is needed
	 * @author Volker Meyer
	 */
	bool has_next() const {return has_prev_next;}

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord pos, scr_size size);

	// since no information are needed to be saved to restore this, returning magic is enough
	virtual uint32 get_rdwr_id() { return magic_toolbar+toolbar_id; }


	bool empty(spieler_t *sp) const;
};

#endif
