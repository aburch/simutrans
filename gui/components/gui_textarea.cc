/*
 * gui_textarea.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "gui_textarea.h"
#include "../../simgraph.h"
#include "../../simcolor.h"

gui_textarea_t::gui_textarea_t(const char *text)
{
    setze_text(text);
}


void
gui_textarea_t::setze_text(const char *text)
{
    if(text) {
	this->text = text;
    } else {
	text = "";
    }

    int max = 0;
    int lines = 1;

    const char *p = text;
    const char *line_start = p;

    while(*p) {
	if(*p == '\n') {
	    lines ++;
	    if(p-line_start+1 > max) {
		max = p-line_start+1;
		line_start = p+1;
	    }
	}
	p++;
    }

    koord gr (max * 8, lines * LINESPACE);

    setze_groesse(gr);
}

/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_textarea_t::zeichnen(koord offset) const
{
    display_multiline_text(pos.x+offset.x, pos.y+offset.y+10, text, COL_BLACK);
}
