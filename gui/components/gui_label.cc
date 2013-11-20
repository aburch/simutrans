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




gui_label_t::gui_label_t(const char* text, COLOR_VAL color_, align_t align_) :
	tooltip(NULL)
{
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

			if(  strrchr(text, '$')!=NULL  ) {
				seperator = strrchr(text, get_fraction_sep());
				if(seperator==NULL  &&  get_large_money_string()!=NULL) {
					seperator = strrchr(text, *(get_large_money_string()) );
				}
			}

			if(seperator) {
				display_proportional_clip(pos.x+offset.x, pos.y+offset.y, seperator, ALIGN_LEFT, color, true);
				if(  seperator!=text  ) {
					display_text_proportional_len_clip(pos.x+offset.x, pos.y+offset.y, text, ALIGN_RIGHT, color, true, seperator-text );
				}
			}
			else {
				display_proportional_clip(pos.x+offset.x, pos.y+offset.y, text, ALIGN_RIGHT, color, true);
			}
		}
	}

	else if(text) {
		int al;
		scr_coord_val align_offset_x = 0;
		scr_coord_val align_offset_y = D_GET_CENTER_ALIGN_OFFSET( LINESPACE, size.h );

		switch(align) {
			case left:
				al = ALIGN_LEFT;
				break;
			case centered:
				al = ALIGN_CENTER_H;
				align_offset_x = (size.w>>1);
				break;
			case right:
				al = ALIGN_RIGHT;
				align_offset_x = size.w;
				break;
			default:
				al = ALIGN_LEFT;
		}

		size_t idx = display_fit_proportional( text, size.w+1, -1 );
		if(  text[idx]==0  ) {
			display_proportional_clip(pos.x + offset.x + align_offset_x, pos.y + offset.y + align_offset_y, text, al, color, true);
		}
		else {
			scr_coord_val w = display_text_proportional_len_clip( pos.x+offset.x+align_offset_x, pos.y+offset.y, text, al | DT_CLIP, color, true, idx );
			display_proportional_clip( pos.x + offset.x + align_offset_x + w, pos.y + offset.y + align_offset_y, translator::translate("..."), al | DT_CLIP, color, true );
		}

	}

	if ( tooltip  &&  getroffen(get_maus_x()-offset.x, get_maus_y()-offset.y) ) {
		const scr_coord_val by = offset.y + pos.y;
		const scr_coord_val bh = size.h;

		win_set_tooltip(get_maus_x() + TOOLTIP_MOUSE_OFFSET_X, by + bh + TOOLTIP_MOUSE_OFFSET_Y, tooltip, this);
	}

	// DEBUG
	//display_ddd_box_clip(offset.x+pos.x,offset.y+pos.y,size.w,size.h,SYSCOL_HIGHLIGHT,SYSCOL_HIGHLIGHT);
}

void gui_label_t::set_tooltip(const char * t)
{
	tooltip = t;
}
