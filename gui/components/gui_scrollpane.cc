/*
 * gui_scrollpane.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "../../simdebug.h"

#include "gui_scrollpane.h"
#include "gui_scrollbar.h"

#include "../../simgraph.h"
#include "../../simcolor.h"



/**
 * Basic constructor
 * @param komp Die zu scrollende Komponente
 * @author Hj. Malthaner
 */
gui_scrollpane_t::gui_scrollpane_t(gui_komponente_t *komp) :
    scroll_x(scrollbar_t::horizontal),
    scroll_y(scrollbar_t::vertical)
{
    this->komp = komp;

    scroll_x.setze_opaque(true);
    scroll_y.setze_opaque(true);

    b_show_scroll_x = true;
    b_show_scroll_y = true;
    b_has_size_corner = true;
}



/**
 * Bei Scrollpanes _muss_ diese Methode zum setzen der Groesse
 * benutzt werden.
 * @author Hj. Malthaner
 */
void gui_scrollpane_t::setze_groesse(koord groesse)
{
	gui_komponente_t::setze_groesse(groesse);

	scroll_x.setze_knob(groesse.x-12, komp->gib_groesse().x);	// set client/komp area
	scroll_x.setze_pos(koord(0, groesse.y-11));
	scroll_x.setze_groesse(groesse-koord(12,12));

	if(b_has_size_corner  ||  b_show_scroll_x) {
		scroll_y.setze_knob(groesse.x-12, komp->gib_groesse().y);
		scroll_y.setze_pos(koord(groesse.x-11, 0));
		scroll_y.setze_groesse(groesse-koord(12,12));
	}
	else {
		scroll_y.setze_pos(koord(groesse.x-11, 0));
		scroll_y.setze_groesse(groesse);
		scroll_y.setze_knob(groesse.y, komp->gib_groesse().y);
	}
}



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void gui_scrollpane_t::infowin_event(const event_t *ev)
{
	if(b_show_scroll_y  &&  (scroll_y.getroffen(ev->mx, ev->my) || scroll_y.getroffen(ev->cx, ev->cy)) ) {
		event_t ev2 = *ev;
		translate_event(&ev2, -scroll_y.gib_pos().x, -scroll_y.gib_pos().y);
		scroll_y.infowin_event(&ev2);
	}
	else if(b_show_scroll_x  &&  (scroll_x.getroffen(ev->mx, ev->my) || scroll_x.getroffen(ev->cx, ev->cy))) {
		event_t ev2 = *ev;
		translate_event(&ev2, -scroll_x.gib_pos().x, -scroll_x.gib_pos().y);
		scroll_x.infowin_event(&ev2);
	}
	else if(b_show_scroll_y  &&  (IS_WHEELUP(ev)  ||  IS_WHEELDOWN(ev))) {
		// otherwise these events are only registered where directly over the scroll region
		// (and sometime even not then ... )
		scroll_y.infowin_event(ev);
	}
	else {
		// translate according to scrolled position
		event_t ev2 = *ev;
		translate_event(&ev2, scroll_x.gib_knob_offset(), scroll_y.gib_knob_offset());

		// hand event to component
		komp->infowin_event(&ev2);

		// Hajo: hack: component could have changed size
		// this recalculates the scrollbars
		setze_groesse(gib_groesse());
	}
}



/**
 * Setzt Positionen der Scrollbars
 * @author Hj. Malthaner
 */
void gui_scrollpane_t::setze_scroll_position(int x, int y)
{
	scroll_x.setze_knob_offset(x);
	scroll_y.setze_knob_offset(y);
}



int gui_scrollpane_t::get_scroll_x() const
{
	return scroll_x.gib_knob_offset();
}



int gui_scrollpane_t::get_scroll_y() const
{
	return scroll_y.gib_knob_offset();
}



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_scrollpane_t::zeichnen(koord pos) const
{
	pos += this->pos;

	// sliding bar background color is now handled by scrollbar!
	if (b_show_scroll_x) {
		scroll_x.zeichnen(pos);
	}

	if (b_show_scroll_y) {
		scroll_y.zeichnen(pos);
	}

	PUSH_CLIP(pos.x, pos.y, groesse.x-12*b_show_scroll_y, groesse.y-11*b_show_scroll_x );
	komp->zeichnen(pos - koord(scroll_x.gib_knob_offset(), scroll_y.gib_knob_offset()));
	POP_CLIP();
}
