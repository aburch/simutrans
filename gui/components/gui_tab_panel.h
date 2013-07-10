/*
 * A class for distribution of tabs through the gui_komponente_t component.
 * @author Hj. Malthaner
 */

#ifndef gui_tab_panel_h
#define gui_tab_panel_h

#include "../../simimg.h"

#include "../../besch/skin_besch.h"

#include "gui_action_creator.h"
#include "gui_komponente.h"
#include "gui_button.h"

class bild_besch_t;

class gui_tab_panel_t :
	public gui_action_creator_t,
	public action_listener_t,
	public gui_komponente_t
{
private:
	struct tab
	{
		tab(gui_komponente_t* c, const char *name, const bild_besch_t *b, const char *tool) : component(c), title(name), img(b), tooltip(tool), x_offset(4) {}

		gui_komponente_t* component;
		const char *title;
		const bild_besch_t *img;
		const char *tooltip;
		sint16 x_offset;
		sint16 width;
	};

	slist_tpl<tab> tabs;
	int active_tab, offset_tab;

	koord required_groesse;
	button_t left, right;

public:
	enum { HEADER_VSIZE = 18};

	gui_tab_panel_t();

	/**
	 * Add new tab to tab bar
	 * @param c is tab component
	 * @param name is name for tab component
	 * @author Hj. Malthaner
	 */
	void add_tab(gui_komponente_t *c, const char *name, const skin_besch_t *b=NULL, const char *tooltip=NULL );

	/**
	 * Get the active component/active tab
	 * @author Hj. Malthaner
	 */
	gui_komponente_t* get_aktives_tab() const { return get_tab(active_tab); }

	gui_komponente_t* get_tab( uint8 i ) const { return i < tabs.get_count() ? tabs.at(i).component : NULL; }

	int get_active_tab_index() const { return min((int)tabs.get_count()-1,active_tab); }
	void set_active_tab_index( int i ) { active_tab = min((int)tabs.get_count()-1,i); }

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw tabs
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord offset);

	/**
	 * Resizing must be propagated!
	 * @author Volker Meyer
	 * @date  18.06.2003
	 */
	void set_groesse(koord groesse) OVERRIDE;

	/*
	 * Remove all tabs.
	 * @author Gerd Wachsmuth
	 * @date  08.05.2009
	 */
	void clear();

	/*
	 * How many tabs we have?
	 * @author Gerd Wachsmuth
	 * @date  08.05.2009
	 */
	uint32 get_count () const { return tabs.get_count(); }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * Returns true if the hosted component of the active tab is focusable
	 * @author Knightly
	 */
	virtual bool is_focusable() { return get_aktives_tab()->is_focusable(); }

	gui_komponente_t *get_focus() { return get_aktives_tab()->get_focus(); }

	/**
	 * Get the relative position of the focused component.
	 * Used for auto-scrolling inside a scroll pane.
	 * @author Knightly
	 */
	virtual koord get_focus_pos() { return pos + get_aktives_tab()->get_focus_pos(); }
};

#endif
