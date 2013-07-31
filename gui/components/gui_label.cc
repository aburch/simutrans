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
#include "../../simwin.h"




gui_label_t::gui_label_t(const char* text, COLOR_VAL color_, align_t align_) :
	tooltip(NULL)
{
	set_groesse( koord( D_BUTTON_WIDTH, LINESPACE ) );
	init( text, koord (0,0), color_, align_);
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
		set_groesse( koord( display_calc_proportional_string_len_width(text,strlen(text)),LINESPACE ) );
	}
}


void gui_label_t::zeichnen(koord offset)
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
				display_proportional_clip(pos.x+offset.x, pos.y+offset.y, seperator, DT_DIRTY|ALIGN_LEFT, color, true);
				if(  seperator!=text  ) {
					display_text_proportional_len_clip(pos.x+offset.x, pos.y+offset.y, text, DT_DIRTY|ALIGN_RIGHT, color, seperator-text );
				}
			}
			else {
				display_proportional_clip(pos.x+offset.x, pos.y+offset.y, text, ALIGN_RIGHT, color, true);
			}
		}
	}

	else if(text) {
		int al;
		KOORD_VAL align_offset_x=0;

		switch(align) {
			case left:
				al = ALIGN_LEFT;
				break;
			case centered:
				al = ALIGN_CENTER_H;
				align_offset_x = (groesse.x>>1);
				break;
			case right:
				al = ALIGN_RIGHT;
				align_offset_x = groesse.x;
				break;
			default:
				al = ALIGN_LEFT;
		}

		size_t idx = display_fit_proportional( text, groesse.x+1, translator::get_lang()->eclipse_width );
		if(  text[idx]==0  ) {
			display_proportional_clip(pos.x+offset.x+align_offset_x, pos.y+offset.y, text, al, color, true);
		}
		else {
			scr_coord_val w = display_text_proportional_len_clip( pos.x+offset.x+align_offset_x, pos.y+offset.y, text, al | DT_DIRTY | DT_CLIP, color, idx );
			display_proportional_clip( pos.x+offset.x+align_offset_x+w, pos.y+offset.y, translator::translate("..."), al | DT_DIRTY | DT_CLIP, color, false );
		}

	}

	if ( tooltip  &&  getroffen(get_maus_x()-offset.x, get_maus_y()-offset.y) ) {
		const KOORD_VAL by = offset.y + pos.y;
		const KOORD_VAL bh = groesse.y;

		win_set_tooltip(get_maus_x() + TOOLTIP_MOUSE_OFFSET_X, by + bh + TOOLTIP_MOUSE_OFFSET_Y, tooltip, this);
	}
}

void gui_label_t::set_tooltip(const char * t)
{
	tooltip = t;
}
