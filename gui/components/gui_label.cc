/*
 * just displays a text, will be auto-translated
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include "gui_label.h"
#include "../gui_frame.h"
#include "../../dataobj/translator.h"
#include "../../utils/simstring.h"
#include "../../gui/simwin.h"


static scr_coord_val separator_width = 0;

gui_label_t::gui_label_t(const char* text, PIXVAL color_, align_t align_) :
	tooltip(NULL)
{
	if(  align_==money  ) {
		separator_width = proportional_string_width( ",00$" );
	}
	set_size( scr_size( D_BUTTON_WIDTH, D_LABEL_HEIGHT ) );
	init( text, scr_coord (0,0), color_, align_);
}


void gui_label_t::set_text(const char *text, bool autosize)
{
	if (text != NULL) {
		set_text_pointer(translator::translate(text), autosize);
	}
	else {
		set_text_pointer(NULL, false);
	}
}


void gui_label_t::set_text_pointer(const char *text_par, bool autosize)
{
	text = text_par;

	if (autosize && text && *text != '\0') {
		set_size( scr_size( display_calc_proportional_string_len_width(text,strlen(text)),size.h ) );
	}
}


void gui_label_t::draw(scr_coord offset)
{
	if(  align == money  ) {
		if(text) {
			const char *seperator = NULL;
			const bool not_a_number = atol(text)==0  &&  !isdigit(*text);

			if(  !not_a_number  ) {
				seperator = strrchr(text, get_fraction_sep());
				if(seperator==NULL  &&  get_large_money_string()!=NULL) {
					seperator = strrchr(text, *(get_large_money_string()) );
				}
			}

			if(seperator) {
				display_proportional_clip_rgb(pos.x+offset.x, pos.y+offset.y, seperator, ALIGN_LEFT, color, true);
				if(  seperator!=text  ) {
					display_text_proportional_len_clip_rgb(pos.x+offset.x, pos.y+offset.y, text, ALIGN_RIGHT | DT_CLIP, color, true, seperator-text );
				}
			}
			else if(  not_a_number  ) {
				// normal text, correct for money decimals
				display_proportional_clip_rgb(pos.x+offset.x+separator_width, pos.y+offset.y, text, ALIGN_RIGHT, color, true);
			}
			else {
				// integer numbers without deciamals, aling at decimal separator
				display_proportional_clip_rgb(pos.x+offset.x, pos.y+offset.y, text, ALIGN_RIGHT, color, true);
			}
		}
	}

	else if(text) {
		const scr_rect area( offset+pos, size );
		display_proportional_ellipsis_rgb( area, text, align  | DT_CLIP, color, true );
	}

	if ( tooltip  &&  getroffen(get_mouse_x()-offset.x, get_mouse_y()-offset.y) ) {
		const scr_coord_val by = offset.y + pos.y;
		const scr_coord_val bh = size.h;

		win_set_tooltip(get_mouse_x() + TOOLTIP_MOUSE_OFFSET_X, by + bh + TOOLTIP_MOUSE_OFFSET_Y, tooltip, this);
	}

	// DEBUG
	//display_ddd_box_clip_rgb(offset.x+pos.x,offset.y+pos.y,size.w,size.h,SYSCOL_HIGHLIGHT,SYSCOL_HIGHLIGHT);
}

void gui_label_t::set_tooltip(const char * t)
{
	tooltip = t;
}
