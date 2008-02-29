/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "gui_textarea.h"
#include "../../simgraph.h"
#include "../../simdebug.h"
#include "../../simcolor.h"

gui_textarea_t::gui_textarea_t(const char *text)
{
 	setze_text(text);
 	recalc_size();
}



void
gui_textarea_t::setze_text(const char *text)
{
	if(text) {
		this->text = text;
	} else {
		text = "";
	}
}



// recalcs the current size;
// usually not needed to be called explicitely
void gui_textarea_t::recalc_size()
{
	// we cannot use: display_multiline_text(pos.x+offset.x, pos.y+offset.y+10, text, COL_BLACK);
	// since we also want to dynamically change the size of the component
	int new_lines=0;
	int x_size = 1;
	if (text!=NULL   &&   *text!= '\0') {
		const char *buf=text;
		const char *next;

		do {
			next = strchr(buf, '\n');
			const long len = next != NULL ? next - buf : -1;
			int px_len = display_calc_proportional_string_len_width(buf, len);
			if(px_len>x_size) {
				x_size = px_len;
			}
			buf = next + 1;
			new_lines += LINESPACE;
		} while (next != NULL);
	}
DBG_MESSAGE("gui_textarea_t::recalc_size()","reset size to %i,%i",x_size+10,new_lines);
	setze_groesse(koord(x_size+10,new_lines));
}



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_textarea_t::zeichnen(koord offset)
{
	// we cannot use: display_multiline_text(pos.x+offset.x, pos.y+offset.y+10, text, COL_BLACK);
	// since we also want to dynamically change the size of the component
	int new_lines=0;
	// keep previous maximum width
	int x_size = gib_groesse().x-10;
	if (text!=NULL   &&   *text!= '\0') {
		const char *buf=text;
		const char *next;
		const sint16 x = pos.x+offset.x;
		sint16 y = pos.y+offset.y+10;

		do {
			next = strchr(buf, '\n');
			if(pos.y+new_lines>=0) {
				const long len = next != NULL ? next - buf : -1;
				int px_len = display_text_proportional_len_clip(x, y + new_lines, buf, ALIGN_LEFT | DT_DIRTY | DT_CLIP, COL_BLACK, len);
				if(px_len>x_size) {
					x_size = px_len;
				}
			}
			buf = next + 1;
			new_lines += LINESPACE;
		} while (next != NULL);
	}
	koord gr(max(x_size+10,gib_groesse().x),new_lines);
	if(gr!=gib_groesse()) {
		setze_groesse(gr);
	}
}
