/*
 * gui_label.cc
 *
 * just displays a text, will be auto-translated
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#include <string.h>

#include "../../simdebug.h"
#include "gui_label.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../dataobj/translator.h"
#include "../../utils/simstring.h"


/**
 * Konstruktor
 * @author Hansjörg Malthaner
 */
gui_label_t::gui_label_t(const char *text)
{
    setze_text( text );
    this->color = COL_BLACK;
    this->align = left;
}


/**
 * Konstruktor
 * @author Hansjörg Malthaner
 */
gui_label_t::gui_label_t(const char *text, int color)
{
    setze_text( text );
    this->color = color;
    this->align = left;
}

/**
 * Konstruktor
 * @author Volker Meyer
 */
gui_label_t::gui_label_t(const char *text, int color, align_t align)
{
    setze_text( text );
    this->color = color;
    this->align = align;
}

/**
 * setzt den Text des Labels
 * @author Hansjörg Malthaner
 */
void gui_label_t::setze_text(const char *text)
{
    this->text = translator::translate(text);
}

/**
 * Sets the colour of the label
 * @author Owen Rudge
 */
void gui_label_t::set_color(int colour)
{
    this->color = colour;
}

/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void gui_label_t::infowin_event(const event_t *)
{

}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_label_t::zeichnen(koord offset) const
{
    if(align == money) {
	if(text) {
	    const char *separator = strrchr(text, get_fraction_sep());

	    if(separator) {
		display_proportional_clip(pos.x+offset.x, pos.y+offset.y,
				     translator::translate(separator),
				     ALIGN_LEFT, color, true);

		*const_cast<char *>(separator) = '\0';
		display_proportional_clip(pos.x+offset.x, pos.y+offset.y,
				     translator::translate(text),
				     ALIGN_RIGHT, color, true);
		*const_cast<char *>(separator) = ',';
	    } else {
		display_proportional_clip(pos.x+offset.x, pos.y+offset.y,
				     text,
				     ALIGN_RIGHT, color, true);
	    }
	}
    }
    else if(text) {
	int al;

	switch(align) {
	case left:
	    al = ALIGN_LEFT;
	    break;
	case centered:
	    al = ALIGN_MIDDLE;
	    break;
	case right:
	    al = ALIGN_RIGHT;
	    break;
	default:
	    al = ALIGN_LEFT;
	}
	display_proportional_clip(pos.x+offset.x, pos.y+offset.y,
                             text,
			     al, color, true);
    }
}
