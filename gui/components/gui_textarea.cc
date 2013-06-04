/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * A text display component
 *
 * @autor Hj. Malthaner
 */

#include <string.h>

#include "gui_textarea.h"
#include "../../simgraph.h"
#include "../../simdebug.h"
#include "../../simcolor.h"
#include "../../utils/cbuffer_t.h"

gui_textarea_t::gui_textarea_t(cbuffer_t* buf_)
{
	buf = buf_;
	recalc_size();
}



// recalcs the current size;
// usually not needed to be called explicitly
void gui_textarea_t::recalc_size()
{
	const char *text(*buf);
	// we cannot use: display_multiline_text(pos.x+offset.x, pos.y+offset.y+10, text, COL_BLACK);
	// since we also want to dynamically change the size of the component
	int new_lines=0;
	int x_size = 1;
	if (text!=NULL   &&   *text!= '\0') {
		const char *buf=text;
		const char *next;

		do {
			next = strchr(buf, '\n');
			const size_t len = next != NULL ? next - buf : -1;
			int px_len = display_calc_proportional_string_len_width(buf, len);
			if(px_len>x_size) {
				x_size = px_len;
			}
			buf = next + 1;
			new_lines += LINESPACE;
		} while(  next != NULL  &&  *buf!=0  );
	}
DBG_MESSAGE("gui_textarea_t::recalc_size()","reset size to %i,%i",x_size+10,new_lines);
	set_groesse(koord(x_size+10,new_lines));
}



/**
 * Draw the component
 * @author Hj. Malthaner
 */
void gui_textarea_t::zeichnen(koord offset)
{
	const char *text(*buf);
	// we cannot use: display_multiline_text(pos.x+offset.x, pos.y+offset.y+10, text, COL_BLACK);
	// since we also want to dynamically change the size of the component
	int new_lines=0;
	// keep previous maximum width
	int x_size = get_groesse().x-10;
	if (text!=NULL   &&   *text!= '\0') {
		const char *buf=text;
		const char *next;
		const sint16 x = pos.x+offset.x;
		sint16 y = pos.y+offset.y;

		do {
			next = strchr(buf, '\n');
			if(pos.y+new_lines+LINESPACE>=0) {
				const int len = next != NULL ? (long)(size_t)(next - buf) : -1;
				int px_len = display_text_proportional_len_clip(x, y + new_lines, buf, ALIGN_LEFT | DT_DIRTY | DT_CLIP, COL_BLACK, len);
				if(px_len>x_size) {
					x_size = px_len;
				}
			}
			buf = next + 1;
			new_lines += LINESPACE;
		} while(  next != NULL  &&  *buf!=0  );
	}
	koord gr(max(x_size+10,get_groesse().x),new_lines);
	if(gr!=get_groesse()) {
		set_groesse(gr);
	}
}
