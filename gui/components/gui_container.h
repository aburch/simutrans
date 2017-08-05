/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/**
 * A container for other gui_components. Is itself
 * a gui_component, and can therefore be nested.
 *
 * @author Hj. Malthaner
 * @date 03-Mar-01
 */

#ifndef gui_container_h
#define gui_container_h


#include "../../simdebug.h"
#include "../../simevent.h"
#include "../../tpl/slist_tpl.h"
#include "gui_komponente.h"

class gui_container_t : public gui_component_t
{
private:
	slist_tpl <gui_component_t *> components;

	// holds the GUI component that has the focus in this window
	// focused component of this container can only be one of its immediate children
	gui_component_t *comp_focus;

	bool list_dirty:1;

	// true, while infowin_event is processed
	bool inside_infowin_event:1;

public:
	gui_container_t();

	// needed for WIN_OPEN events
	void clear_dirty() { list_dirty=false; }

	/**
	 * Returns the minimum rectangle which encloses all children
	 * @author Max Kielland
	 */
	scr_rect get_min_boundaries() const;

	/**
	* Adds a Component to the Container.
	* @author Hj. Malthaner
	*/
	void add_component(gui_component_t *comp);

	/**
	* Removes a Component in the Container.
	* @author Hj. Malthaner
	*/
	void remove_component(gui_component_t *comp);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Draw the component
	* @author Hj. Malthaner
	*/
	virtual void draw(scr_coord offset);

	/**
	* Removes all Components in the Container.
	* @author Markus Weber
	*/
	void remove_all();

	/**
	 * Returns true if any child component is focusable
	 * @author Knightly
	 */
	virtual bool is_focusable();

	/**
	 * Activates this element.
	 *
	 * @warning Calling this method from anything called from
	 * gui_container_t::infowin_event (e.g. in action_triggered)
	 * will have NO effect.
	 */
	void set_focus( gui_component_t *comp_focus );

	/**
	 * returns element that has the focus
	 * that is: go down the hierarchy as much as possible
	 */
	virtual gui_component_t *get_focus();

	/**
	 * Get the relative position of the focused component.
	 * Used for auto-scrolling inside a scroll pane.
	 * @author Knightly
	 */
	virtual scr_coord get_focus_pos() { return comp_focus ? pos+comp_focus->get_focus_pos() : scr_coord::invalid; }
};

#endif
