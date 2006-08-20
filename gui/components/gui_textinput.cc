/*
 * gui_textinput.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <string.h>

#include "../../simdebug.h"
#include "gui_textinput.h"
#include "../../simevent.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"

gui_komponente_t * gui_komponente_t::focus;

/**
 * Konstruktor
 *
 * @author Hj. Malthaner
 */
gui_textinput_t::gui_textinput_t()
{
    max = 0;
    text = NULL;
    cursor_pos = 0;
    set_read_only(false);
}

/**
 * Destruktor
 *
 * @author Hj. Malthaner
 */
gui_textinput_t::~gui_textinput_t()
{
    if(has_focus()) {
		release_focus();
    }

    text = NULL;
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void gui_textinput_t::infowin_event(const event_t *ev)
{
    if(ev->ev_class == EVENT_KEYBOARD) {
		if((text != NULL) && has_focus()) {
		    const int len = strlen(text);

		    switch(ev->ev_code) {
		    case 274: // down arrow
		    	// not used currently
			break;
		    case 276: // left arrow
		    	if (cursor_pos > 0)
		    		cursor_pos--;
			break;
		    case 275: // right arrow
		    	if (cursor_pos < len)
		    		cursor_pos++;
			break;
		    case 273: // rightarrow
		    	// not used currently
			break;
			case 278: // home
				cursor_pos = 0;
			break;
			case 279: // end
				cursor_pos = strlen(text);
			break;
		    case 8:
			// backspace
			if(cursor_pos > 0) {
				if (cursor_pos < len) {
					for (int pos = cursor_pos-1; pos < len; pos++) {
						text[pos] = text[pos+1];
					}
				} else {
				    text[len-1] = 0;
				}
				cursor_pos > 0 ? cursor_pos-- : cursor_pos = 0;
			}
			break;
			case 127:
			// delete
			if (cursor_pos < len) {
				for (int pos = cursor_pos; pos < len; pos++) {
					text[pos] = text[pos+1];
				}
			}
			break;
		    case 13:
			if(has_focus()) {
			    this->release_focus();
			    ::release_focus();
			    call_listeners();
			}
			break;
			case 0:
			// ignore key
			break;
		    default:
			// Buchstaben, Ziffern und Sonderzeichen einfügen:
			if (ev->ev_code <= 255) {
				if((len+1 < max) && (ev->ev_code != 0)) {
					// insert into text?
					if (cursor_pos < len) {
						for (int pos = len+1; pos >= cursor_pos; pos--) {
							text[pos] = text[pos-1];
						}
						text[cursor_pos] = ev->ev_code;
					} else {
						// append to text
					    text[len] = ev->ev_code;
					    text[len+1] = 0;
					}
				    cursor_pos++;
				}
			}
		    }
		} else {
		    printf("Warning: gui_textinput_t::infowin_event() called but text is NULL\n");
		}

    } else if(IS_LEFTRELEASE(ev) || IS_LEFTCLICK(ev)) {
        this->request_focus();
        cursor_pos = strlen(text);
    } else if(ev->ev_class == INFOWIN &&
	      ev->ev_code == WIN_CLOSE &&
	      has_focus()) {
        this->release_focus();
    }
}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_textinput_t::zeichnen(koord offset) const
{
	PUSH_CLIP(pos.x+offset.x+1, pos.y+offset.y+1,
	                       groesse.x-2, groesse.y-2);
    display_fillbox_wh_clip(pos.x+offset.x+1, pos.y+offset.y+1,
                       groesse.x-2, groesse.y-2,
		       MN_GREY1, true);

    display_ddd_box(pos.x+offset.x, pos.y+offset.y,
                       groesse.x, groesse.y,
		       MN_GREY0, MN_GREY4);

    if(text) {
    	int cursor_offset = proportional_string_len_width(text, cursor_pos);
    	if (cursor_offset > groesse.x - 2) {
    		cursor_offset -= (groesse.x - 3);
    	} else {
    		cursor_offset = 0;
    	}

		display_proportional_clip(pos.x+offset.x+2-cursor_offset, pos.y+offset.y+2,
	                             text, ALIGN_LEFT, SCHWARZ, true);

    	// cursor must been shown, if textinput has focus!
		if(has_focus()) {
		    display_fillbox_wh_clip(pos.x+offset.x+1+proportional_string_len_width(text, cursor_pos)-cursor_offset, pos.y+offset.y+1, 1, 11, WHITE, true);
		}
    }
	POP_CLIP();
}


/**
 * Inform all listeners that an action was triggered.
 * @author Hj. Malthaner
 */
void gui_textinput_t::call_listeners()
{
    // printf("Message: button_t::call_listeners()\n");

    slist_iterator_tpl<action_listener_t *> iter (listeners);

    while(iter.next() && !iter.get_current()->action_triggered(this));
}

void
gui_textinput_t::setze_text(char *text, int max)
{
	this->text = text;
	this->max=max;
	cursor_pos = strlen(text);
}
