/*
 * with a connected edit field
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Defines a drop-down list with left/right arrows
 */

#ifndef gui_components_gui_combobox_h
#define gui_components_gui_combobox_h

#include "../../simcolor.h"
#include "gui_action_creator.h"
#include "gui_scrolled_list.h"
#include "gui_textinput.h"
#include "gui_button.h"


class gui_combobox_t :
	public gui_action_creator_t,
	public gui_komponente_t,
	public action_listener_t
{
private:
	char editstr[128],old_editstr[128];

	// buttons for setting selection manually
	gui_textinput_t textinp;
	button_t bt_prev;
	button_t bt_next;

	/**
	 * the drop box list
	 * @author hsiegeln
	 */
	gui_scrolled_list_t droplist;

	// flag for first call
	bool first_call:1;

	// flag for finish selection
	bool finish:1;

	// true to allow buttons to wrap around selection
	bool wrapping:1;

	/**
	 * the max size this component can have
	 * @author hsiegeln
	 */
	koord max_size;

	/**
	 * renames the selected item if necessary
	 */
	void rename_selected_item();

	/**
	 * resets the input field to the name of the item
	 */
	void reset_selected_item_name();

public:
	gui_combobox_t();

	bool infowin_event(event_t const*) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * Draw the component
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord offset);

	/**
	 * add element to droplist
	 * @author hsiegeln
	 */
	void append_element( gui_scrolled_list_t::scrollitem_t *item ) { droplist.append_element( item ); set_max_size( max_size ); }

	/**
	 * remove all elements from droplist
	 * @author hsiegeln
	 */
	void clear_elements() { droplist.clear_elements(); }

	/**
	 * return number of elements in droplist
	 * @author hsiegeln
	 */
	int count_elements() const { return droplist.get_count(); }

	/**
	 * return element at index from droplist
	 * @author hsiegeln
	 */
	gui_scrolled_list_t::scrollitem_t *get_element(sint32 idx) const { return droplist.get_element(idx); }

	/**
	 * sets the highlight color for the droplist
	 * @author hsiegeln
	 */
	void set_highlight_color(int color) { droplist.set_highlight_color(color); }

	/**
	 * set maximum size for control
	 * @author hsiegeln
	 */
	void set_max_size(koord max);

	/**
	 * returns the selection id
	 * @author hsiegeln
	 */
	int get_selection() { return droplist.get_selection(); }

	/**
	 * sets the selection
	 * @author hsiegeln
	 */
	void set_selection(int s);

	/**
	* Set this component's position.
	* @author Hj. Malthaner
	*/
	virtual void set_pos(koord pos_par);

	void set_groesse(koord groesse) OVERRIDE;

	/**
	 * called when the focus should be released
	 * does some cleanup before releasing
	 * @author hsiegeln
	 */
	void close_box();

	void set_wrapping(const bool wrap) { wrapping = wrap; }

	bool is_dropped() const { return droplist.is_visible(); }
};

#endif
