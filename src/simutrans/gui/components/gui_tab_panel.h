/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_TAB_PANEL_H
#define GUI_COMPONENTS_GUI_TAB_PANEL_H


#include "../../display/simimg.h"

#include "../../descriptor/skin_desc.h"

#include "gui_action_creator.h"
#include "gui_component.h"
#include "gui_button.h"

class image_t;
class loadsave_t;

/**
 * A class for distribution of tabs through the gui_component_t component.
 */
class gui_tab_panel_t :
	public gui_action_creator_t,
	public action_listener_t,
	public gui_component_t
{
private:
	struct tab
	{
		tab(gui_component_t* c, const char *name, const image_t *b, const char *tool) : component(c), title(name), img(b), tooltip(tool), x_offset(4) {}

		gui_component_t* component;
		const char *title;
		const image_t *img;
		const char *tooltip;
		sint16 x_offset;
		sint16 width;
	};

	slist_tpl<tab> tabs;
	int active_tab;

	scr_size required_size;
	scr_coord_val tab_offset_x;
	button_t left, right;

	bool is_dragging;

	bool tab_getroffen(scr_coord_val x, scr_coord_val y);

public:
	static scr_coord_val header_vsize;

	gui_tab_panel_t();

	/**
	 * Add new tab to tab bar
	 * @param c is tab component
	 * @param name is name for tab component
	 */
	void add_tab(gui_component_t *c, const char *name, const skin_desc_t *b=NULL, const char *tooltip=NULL );

	/**
	 * Get the active component/active tab
	 */
	gui_component_t* get_aktives_tab() const { return get_tab(active_tab); }

	gui_component_t* get_tab( uint8 i ) const { return i < tabs.get_count() ? tabs.at(i).component : NULL; }

	int get_active_tab_index() const { return min((int)tabs.get_count()-1,active_tab); }
	void set_active_tab_index( int i ) { active_tab = min((int)tabs.get_count()-1,i); }

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw tabs
	 */
	void draw(scr_coord offset) OVERRIDE;

	/**
	 * Resizing must be propagated!
	 */
	void set_size(scr_size size) OVERRIDE;

	/**
	 * Remove all tabs.
	 */
	void clear();

	/**
	 * How many tabs we have?
	 */
	uint32 get_count () const { return tabs.get_count(); }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * Returns true if the hosted component of the active tab is focusable
	 */
	bool is_focusable() OVERRIDE { return get_aktives_tab()->is_focusable(); }

	gui_component_t *get_focus() OVERRIDE { return get_aktives_tab()->get_focus(); }

	/**
	 * Get the relative position of the focused component.
	 * Used for auto-scrolling inside a scroll pane.
	 */
	scr_coord get_focus_pos() OVERRIDE { return pos + get_aktives_tab()->get_focus_pos(); }


	scr_size get_min_size() const OVERRIDE;

	// size of tab header
	scr_size get_required_size() const { return required_size; }

	bool is_marginless() const OVERRIDE { return true; }

	/**
	 * Take tabs from other tab.
	 */
	void take_tabs(gui_tab_panel_t* other);

	/// save active tab
	void rdwr( loadsave_t *file );
};

#endif
