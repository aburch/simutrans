/*
 * this is a scrolling area in which subcomponents are drawn
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_scrollpane_h
#define gui_scrollpane_h


#include "gui_komponente.h"
#include "gui_scrollbar.h"

class gui_scrollpane_t : public gui_komponente_t
{
private:
	/**
	 * Die zu scrollende Komponente
	 * @author Hj. Malthaner
	 */
	gui_komponente_t *komp;
	koord old_komp_groesse;

	/**
	 * Scrollbar X/Y
	 * @author Hj. Malthaner
	 */
	scrollbar_t scroll_x, scroll_y;

	bool b_show_scroll_x:1;
	bool b_show_scroll_y:1;
	bool b_has_size_corner:1;

	void recalc_sliders(koord groesse);

public:
	/**
	 * @param komp Die zu scrollende Komponente
	 * @author Hj. Malthaner
	 */
	gui_scrollpane_t(gui_komponente_t *komp);

	/**
	 * Bei Scrollpanes _muss_ diese Methode zum setzen der Groesse
	 * benutzt werden.
	 * @author Hj. Malthaner
	 */
	void set_groesse(koord groesse);

	/**
	 * Setzt Positionen der Scrollbars
	 * @author Hj. Malthaner
	 */
	void set_scroll_position(int x, int y);

	int get_scroll_x() const;
	int get_scroll_y() const;

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author Hj. Malthaner
	 */
	bool infowin_event(const event_t *ev);

	/**
	 * Zeichnet die Komponente
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord offset);

	void set_show_scroll_x(bool yesno) { b_show_scroll_x = yesno; }

	void set_show_scroll_y(bool yesno) { b_show_scroll_y = yesno; }

	void set_size_corner(bool yesno) { b_has_size_corner = yesno; }

	/**
	 * Returns true if the hosted component is focusable
	 * @author Knightly
	 */
	virtual bool is_focusable() { return komp->is_focusable(); }

	/**
	 * returns element that has the focus
	 */
	gui_komponente_t *get_focus() { return komp->get_focus(); }

	/**
	 * Get the relative position of the focused component.
	 * Used for auto-scrolling inside a scroll pane.
	 * @author Knightly
	 */
	virtual koord get_focus_pos() { return pos + ( komp->get_focus_pos() - koord( scroll_x.get_knob_offset(), scroll_y.get_knob_offset() ) ); }
};

#endif
