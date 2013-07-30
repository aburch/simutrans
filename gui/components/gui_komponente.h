/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef ifc_gui_komponente_h
#define ifc_gui_komponente_h

#include "../../dataobj/koord.h"
#include "../../simevent.h"
#include "../../simgraph.h"

struct event_t;

/**
 * Base class for all GUI components.
 *
 * @autor Hj. Malthaner
 */
class gui_komponente_t
{
private:
	/**
	* allow component to show/hide itself
	* @author hsiegeln
	*/
	bool visible:1;

	/**
	* some components might not be allowed to gain focus
	* for example: gui_textarea_t
	* this flag can be set to true to deny focus request for a gui_component always
	* @author hsiegeln
	*/
	bool focusable:1;

protected:
	/**
	 * Component's bounding box position.
	 * @author Hj. Malthaner
	 */
	koord pos;

	/**
	* Component's bounding box size.
	* @author Hj. Malthaner
	*/
	koord groesse;

public:
	/**
	* Basic contructor, initialises member variables
	* @author Hj. Malthaner
	*/
	gui_komponente_t(bool _focusable = false) : visible(true), focusable(_focusable) {}

	/**
	* Virtual destructor so all decendent classes are destructed right
	* @author Hj. Malthaner
	*/
	virtual ~gui_komponente_t() {}

	/**
	* Initialises the component's position and size.
	* @author Max Kielland
	*/
	virtual void init(koord pos_par, koord size_par=koord(0,0)) {
		set_pos(pos_par);
		set_groesse(size_par);
	}

	/**
	* Set focus ability for this component.
	* @author Unknown
	*/
	virtual void set_focusable(bool yesno) {
		focusable = yesno;
	}

	// Knightly : a component can only be focusable when it is visible
	virtual bool is_focusable() {
		return ( visible && focusable );
	}

	/**
	* Sets component to be shown/hidden
	* @author Hj. Malthaner
	*/
	virtual void set_visible(bool yesno) {
		visible = yesno;
	}

	/**
	* Checks if component should be displayed.
	* @author Hj. Malthaner
	*/
	virtual bool is_visible() const {
		return visible;
	}

	/**
	* Set this component's position.
	* @author Hj. Malthaner
	*/
	virtual void set_pos(koord pos_par) {
		pos = pos_par;
	}

	/**
	* Get this component's bounding box position.
	* @author Hj. Malthaner
	*/
	virtual koord get_pos() const {
		return pos;
	}

	/**
	* Set this component's bounding box size.
	* @author Hj. Malthaner
	*/
	virtual void set_groesse(koord size_par) {
		groesse = size_par;
	}

	/**
	* Get this component's bounding box size.
	* @author Hj. Malthaner
	*/
	virtual koord get_groesse() const {
		return groesse;
	}

	/**
	* Set this component's width.
	* @author Max Kielland
	*/
	virtual void set_width(scr_coord_val width_par) {
		set_groesse(koord(width_par,groesse.y));
	}

	/**
	* Set this component's height.
	* @author Max Kielland
	*/
	virtual void set_height(scr_coord_val height_par) {
		set_groesse(koord(groesse.x,height_par));
	}

	/**
	* Checks if the given position is inside the component area.
	* @return true if the coordinates are inside this component area.
	* @author Hj. Malthaner
	*/
	virtual bool getroffen(int x, int y) {
		return ( pos.x <= x && pos.y <= y && (pos.x+groesse.x) > x && (pos.y+groesse.y) > y );
	}

	/**
	* deliver event to a component if
	* - component has focus
	* - mouse is over this component
	* - event for all components
	* @return: true for swalloing this event
	* @author Hj. Malthaner
	* prissi: default -> do nothing
	*/
	virtual bool infowin_event(const event_t *) {
		return false;
	}

	/**
	* Pure virtual paint method
	* @author Hj. Malthaner
	*/
	virtual void zeichnen(koord offset) = 0;

	/**
	 * returns the element that has focus
	 * other derivates like scrolld list of tabs will
	 * return a child component.
	 */
	virtual gui_komponente_t *get_focus() {
		return is_focusable() ? this : 0;
	}

	/**
	 * Get the relative position of the focused component.
	 * Used for auto-scrolling inside a scroll pane.
	 * @author Knightly
	 */
	virtual koord get_focus_pos() {
		return pos;
	}

	/**
	 * Align this component against a target component
	 * @param component_par the component to align against
	 * @param alignment_par the requested alignmnent
	 * @param offset_par Offset added to final alignment
	 * @author Max Kielland
	 */
	void align_to(gui_komponente_t* component_par, control_alignment_t alignment_par, koord offset_par = koord(0,0) );

};

#endif
