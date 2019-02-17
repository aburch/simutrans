/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * A text display component
 *
 * @author Hj. Malthaner
 */

#include <string.h>

#include "gui_textarea.h"
#include "../../display/simgraph.h"
#include "../../simdebug.h"
#include "../../simcolor.h"
#include "../../simskin.h"
#include "../gui_theme.h"
#include "../../utils/cbuffer_t.h"

#define L_PADDING_RIGHT (10)

gui_textarea_t::gui_textarea_t(cbuffer_t* buf_)
{
	set_buf(buf_);
}


void gui_textarea_t::set_buf( cbuffer_t* buf_ )
{
	buf = buf_;
	recalc_size();
}


scr_size gui_textarea_t::get_min_size() const
{
	return calc_size();
}


scr_size gui_textarea_t::get_max_size() const
{
	return get_min_size();
}


void gui_textarea_t::recalc_size()
{
	set_size(calc_size());
}


scr_size gui_textarea_t::calc_size() const
{
	const char *text(*buf);

	// we cannot use: display_multiline_text(pos.x+offset.x, pos.y+offset.y+10, text, color_idx_to_rgb(COL_BLACK));
	// since we also want to dynamically change the size of the component
	int new_lines=0;
	int x_size = 1;

	if (  (text != NULL)  &&  (*text != '\0')  ) {
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
	return scr_size( x_size + L_PADDING_RIGHT, new_lines );
}


/**
 * Draw the component
 * @author Hj. Malthaner
 */
void gui_textarea_t::draw(scr_coord offset)
{
	const char *text(*buf);

	// we cannot use: display_multiline_text(pos.x+offset.x, pos.y+offset.y+10, text, color_idx_to_rgb(COL_BLACK));
	// since we also want to dynamically change the size of the component
	int new_lines=0;

	// keep previous maximum width
	int x_size = get_size().w - L_PADDING_RIGHT;
	if (  (text != NULL)  &&  (*text != '\0')  ) {
		const char *buf=text;
		const char *next;
		const sint16 x = pos.x+offset.x;
		sint16 y = pos.y+offset.y;

		do {
			next = strchr(buf, '\n');

			const size_t len = next != NULL ? next - buf : -1;
			int px_len = display_text_proportional_len_clip_rgb(x, y + new_lines, buf, ALIGN_LEFT | DT_CLIP, SYSCOL_TEXT, true, len);
			if(px_len>x_size) {
				x_size = px_len;
			}

			buf = next + 1;
			new_lines += LINESPACE;
		} while(  next != NULL  &&  *buf!=0  );
	}
	scr_size size( max( x_size + L_PADDING_RIGHT, get_size().w ), new_lines );
	if(  size!=get_size()  ) {
		set_size(size);
	}
}
