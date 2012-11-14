/*
 * just displays a text, will be auto-translated
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include <stdio.h>
#include <string.h>

#include "../../simdebug.h"
#include "gui_label.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../dataobj/translator.h"
#include "../../utils/simstring.h"


gui_label_t::gui_label_t(const char* text, COLOR_VAL color_, align_t align_) :
	align(align_),
	color(color_)
{
	set_text( text );
}

/**
 * setzt den Text des Labels
 * @author Hansjörg Malthaner
 */
void gui_label_t::set_text(const char *text)
{
	this->text = translator::translate(text);
}



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_label_t::zeichnen(koord offset)
{
	if(align == money) {
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
		display_proportional_clip(pos.x+offset.x, pos.y+offset.y, text, al, color, true);
	}
}
