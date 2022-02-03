/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>

#include "gui_textarea.h"
#include "../../display/simgraph.h"
#include "../../simdebug.h"
#include "../../simcolor.h"
#include "../../simskin.h"
#include "../gui_theme.h"
#include "../../utils/cbuffer.h"

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

	// since we also want to dynamically change the size of the component
	int new_lines=0;
	scr_coord_val x_size = 0;

	if (  (text != NULL)  &&  (*text != '\0')  ) {
		const char *buf=text;
		const char *next;

		do {
			next = strchr(buf, '\n');
			const size_t len = next ? next-buf : 99999;
			const int px_len = display_calc_proportional_string_len_width(buf, len);

			if(  px_len > x_size  ) {
				x_size = px_len;
			}

			new_lines += LINESPACE;
		} while(  next != NULL  &&  ((void)(buf = next+1), *buf!=0)  );
	}
	if (x_size > 0) {
		x_size += L_PADDING_RIGHT;
	}
	return scr_size( x_size, new_lines );
}


/**
 * Draw the component
 */
void gui_textarea_t::draw(scr_coord offset)
{
	const char *text(*buf);

	// we cannot use: display_multiline_text(pos.x+offset.x, pos.y+offset.y+10, text, color_idx_to_rgb(COL_BLACK));
	// since we also want to dynamically change the size of the component
	scr_coord_val new_lines = 0;

	// keep previous maximum width
	scr_coord_val x_size = get_size().w - L_PADDING_RIGHT;
	if (  (text != NULL)  &&  (*text != '\0')  ) {
		const char *buf=text;
		const char *next;
		const scr_coord_val x = pos.x+offset.x;
		scr_coord_val y = pos.y+offset.y;

		do {
			next = strchr(buf, '\n');

			const size_t len = next != NULL ? next - buf : -1;
			scr_coord_val const draw_y = y + new_lines;
			int px_len;
			if (  -LINESPACE <= draw_y  &&  draw_y <= display_get_height() + LINESPACE) {
				// draw when in screen area
				px_len = display_text_proportional_len_clip_rgb(x, draw_y, buf, ALIGN_LEFT | DT_CLIP, SYSCOL_TEXT, true, len);
			}
			else {
				// track required length when out of screen area
				px_len = display_calc_proportional_string_len_width(buf, len);
			}
			if(px_len>x_size) {
				x_size = px_len;
			}

			new_lines += LINESPACE;
		} while(  next != NULL  &&  ((void)(buf = next+1), *buf!=0)  );
	}

	scr_size size( max( x_size + L_PADDING_RIGHT, get_size().w ), new_lines );
	if(  size!=get_size()  ) {
		set_size(size);
	}
}
