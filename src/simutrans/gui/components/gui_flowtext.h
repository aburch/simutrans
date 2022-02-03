/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_FLOWTEXT_H
#define GUI_COMPONENTS_GUI_FLOWTEXT_H


#include "action_listener.h"
#include "gui_action_creator.h"
#include "gui_scrollpane.h"

class gui_flowtext_intern_t;

/**
 * A component for floating text wrapped into a scrollpane.
 */
class gui_flowtext_t :
	public gui_action_creator_t,
	public action_listener_t,
	public gui_scrollpane_t
{
	gui_flowtext_intern_t* flowtext;

	using gui_scrollpane_t::set_component;

public:
	gui_flowtext_t();

	~gui_flowtext_t();

	/**
	 * Sets the text to display.
	 */
	void set_text(const char* text);

	const char* get_title() const;

	/**
	 * Updates size and preferred_size.
	 */
	void set_size(scr_size size_par) OVERRIDE;

	/**
	 * Computes and returns preferred size.
	 * Depends on current width.
	 */
	scr_size get_preferred_size();

	bool action_triggered(gui_action_creator_t *comp, value_t extra) OVERRIDE;
};


#endif
