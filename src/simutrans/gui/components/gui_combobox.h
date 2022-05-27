/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_COMBOBOX_H
#define GUI_COMPONENTS_GUI_COMBOBOX_H


#include "../../simcolor.h"
#include "gui_action_creator.h"
#include "gui_scrolled_list.h"
#include "gui_textinput.h"
#include "gui_button.h"


class loadsave_t;

/*
 * Defines a drop-down list with left/right arrows
 */
class gui_combobox_t :
	public gui_action_creator_t,
	public gui_component_t,
	public action_listener_t
{
private:
	char editstr[128],old_editstr[128];

	// buttons for setting selection manually
	gui_textinput_t textinp;
	button_t bt_prev;
	button_t bt_next;

	scr_size closed_size;

	/**
	 * the drop box list
	 */
	gui_scrolled_list_t droplist;
	bool opened_above:1;

	// flag for first call
	bool first_call:1;

	// flag for finish selection
	bool finish:1;

	// true to allow buttons to wrap around selection
	bool wrapping:1;

	// offset of last draw call, needed to decide, where to open droplist
	scr_coord last_draw_offset;

	/**
	 * the max size this component can have
	 */
	scr_size max_size;

	/**
	 * renames the selected item if necessary
	 */
	void rename_selected_item();

	/**
	 * resets the input field to the name of the item
	 */
	void reset_selected_item_name();

public:
	gui_combobox_t(gui_scrolled_list_t::item_compare_func cmp = 0);

	bool infowin_event(event_t const*) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void sort( int offset ) { droplist.sort( offset ); }

	/**
	 * returns the element that has focus.
	 * child classes like scrolled list of tabs should
	 * return a child component.
	 */
	gui_component_t *get_focus() OVERRIDE { return this; }

	/**
	 * Draw the component
	 */
	void draw(scr_coord offset) OVERRIDE;

	/**
	 * add element to droplist
	 */
	template<class C, class... As>
	void new_component(const As &... as)
	{
		droplist.new_component<C>(as...);
	}

	/**
	 * remove all elements from droplist
	 */
	void clear_elements() { droplist.clear_elements(); }

	/**
	 * return number of elements in droplist
	 */
	int count_elements() const { return droplist.get_count(); }

	/**
	 * return element at index from droplist
	 */
	gui_scrolled_list_t::scrollitem_t *get_element(sint32 idx) const { return droplist.get_element(idx); }

	/**
	 * set maximum size for control
	 */
	void set_max_size(scr_size max);

	/**
	 * returns the selection id
	 */
	int get_selection() { return droplist.get_selection(); }

	gui_scrolled_list_t::scrollitem_t* get_selected_item() const { return droplist.get_selected_item(); }
	/**
	 * sets the selection
	 */
	void set_selection(int s);

	/**
	* Set this component's position.
	*/
	void set_pos(scr_coord pos_par) OVERRIDE;

	void set_size(scr_size size) OVERRIDE;

	/**
	 * called when the focus should be released
	 * does some cleanup before releasing
	 */
	void close_box();

	void set_wrapping(const bool wrap) { wrapping = wrap; }

	bool is_dropped() const { return droplist.is_visible(); }

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;

	void enable();
	void disable();
	void enable( bool yesno ) {
		if( yesno && !is_focusable() ) {
			enable();
		}
		else if( !yesno  &&  is_focusable() ) {
			disable();
		}
	}

	// default comoboboxes are sorted!
	void set_unsorted() { droplist.set_cmp(NULL); }

	// save selection
	void rdwr( loadsave_t *file );
};

#endif
