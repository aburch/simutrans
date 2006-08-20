/*
 * optionen.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/* optionen.cc
 *
 * Dialog fuer Spieloptionen
 * Niels Roest, Hj. Malthaner, 2000
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simimg.h"
#include "../simwin.h"
#include "../simplay.h"
#include "../simgraph.h"
#include "../simdisplay.h"
#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"

#include "optionen.h"
#include "colors.h"
#include "sprachen.h"
#include "player_frame_t.h"
#include "welt.h"
#include "kennfarbe.h"
#include "sound_frame.h"
#include "loadsave_frame.h"
#ifdef spaeter
#include "autosave_gui.h"
#endif
#include "../simcolor.h"

optionen_gui_t::optionen_gui_t(karte_t *welt)
 : infowin_t(welt), buttons(10)
{
    // init buttons
    button_t button_def;

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(11,65) );
    button_def.setze_text("Sprache");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(11,81) );
    button_def.setze_text("Farbe");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(11,97) );
    button_def.setze_text("Helligk.");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(11,113) );
    button_def.setze_text("Sound");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(11,129) );
    button_def.setze_text("Spieler(mz)");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(112,65) );
    button_def.setze_text("Laden");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(112,81) );
    button_def.setze_text("Speichern");
    buttons.append(button_def);

    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(112,97) );
    button_def.setze_text("Neue Karte");
    buttons.append(button_def);

    // 01-Nov-2001      Markus Weber    Added
    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(112,129) );
    button_def.setze_text("Beenden");
    buttons.append(button_def);
#ifdef spaeter
    button_def.setze_groesse( koord(90, 14) );
    button_def.setze_typ(button_t::roundbox);
    button_def.setze_pos( koord(112,113) );
    button_def.setze_text("Autosave");
    buttons.append(button_def);
#endif
}

const char *
optionen_gui_t::gib_name() const
{
    return "Einstellungen";
}

/**
 * Jedes Objekt braucht ein Bild.
 *
 * @author Hj. Malthaner
 * @return Die Nummer des aktuellen Bildes für das Objekt.
 */
int optionen_gui_t::gib_bild() const
{
  return IMG_LEER;
}


void optionen_gui_t::info(cbuffer_t & buf) const
{
  buf.append(translator::translate("Einstellungen aendern"));
}


koord optionen_gui_t::gib_fenstergroesse() const
{
    return koord(213, 160);
}

void optionen_gui_t::infowin_event(const event_t *ev)
{
    infowin_t::infowin_event(ev);

    if(IS_LEFTCLICK(ev)) {
	if(buttons.at(0).getroffen(ev->cx, ev->cy)) {
	} else if(buttons.at(1).getroffen(ev->cx, ev->cy)) {
	} else if(buttons.at(2).getroffen(ev->cx, ev->cy)) {
	} else if(buttons.at(3).getroffen(ev->cx, ev->cy)) {
	} else if(buttons.at(4).getroffen(ev->cx, ev->cy)) {
	}  else if(buttons.at(5).getroffen(ev->cx, ev->cy)) {
	}  else if(buttons.at(6).getroffen(ev->cx, ev->cy)) {
	}  else if(buttons.at(7).getroffen(ev->cx, ev->cy)) {
	}  else if(buttons.at(8).getroffen(ev->cx, ev->cy)) {    //01-Nov-2001   Markus Weber    Added
	}
    }



    if(IS_LEFTRELEASE(ev)) {
	if(buttons.at(0).getroffen(ev->cx, ev->cy)) {
	    buttons.at(0).pressed = false;
	    create_win(new sprachengui_t(welt), w_info, magic_sprachengui_t);

	} else if(buttons.at(1).getroffen(ev->cx, ev->cy)) {
	    buttons.at(1).pressed = false;
	    create_win(new farbengui_t(welt), w_info, magic_farbengui_t);

	} else if(buttons.at(2).getroffen(ev->cx, ev->cy)) {
	    buttons.at(2).pressed = false;
	    create_win(new color_gui_t(welt), w_info, magic_color_gui_t);

	} else if(buttons.at(3).getroffen(ev->cx, ev->cy)) {
	    buttons.at(3).pressed = false;
	    create_win(new sound_frame_t(), w_info, magic_sound_kontroll_t);

	} else if(buttons.at(4).getroffen(ev->cx, ev->cy)) {
	    buttons.at(4).pressed = false;
	    create_win(new ki_kontroll_t(welt), w_info, magic_ki_kontroll_t);

	}  else if(buttons.at(5).getroffen(ev->cx, ev->cy)) {
	    buttons.at(5).pressed = false;

	    destroy_win(this);

	    create_win(new loadsave_frame_t(welt, true), w_info, magic_load_t);

	}  else if(buttons.at(6).getroffen(ev->cx, ev->cy)) {
	    buttons.at(6).pressed = false;

	    destroy_win(this);

	    create_win(new loadsave_frame_t(welt, false), w_info, magic_save_t);


            // New world-Button
            //-----------------------
        }  else if(buttons.at(7).getroffen(ev->cx, ev->cy)) {
	    buttons.at(7).pressed = false;
	    destroy_all_win();
	    //welt->beenden();                  //02-Nov-2001   Markus Weber    modified
            welt->beenden(false);


            // Quit-Button                      // 01-Nov-2001      Markus Weber    Added
            //-----------------------
	}  else if(buttons.at(8).getroffen(ev->cx, ev->cy)) {
            // quit the game
	    welt->beenden(true);

#ifdef spaeter
	}  else if(buttons.at(9).getroffen(ev->cx, ev->cy)) {
	    buttons.at(9).pressed = false;
	    create_win(new autosave_gui_t(welt), w_info, magic_autosave_t);
#endif
	}
    }
}


vector_tpl<button_t>*
optionen_gui_t::gib_fensterbuttons()
{
  // variable teile aktualsieren

  buttons.at(0).setze_text("Sprache");
  buttons.at(1).setze_text("Farbe");
  buttons.at(2).setze_text("Helligk.");
  buttons.at(3).setze_text("Sound");
  buttons.at(4).setze_text("Spieler(mz)");
  buttons.at(5).setze_text("Laden");
  buttons.at(6).setze_text("Speichern");
  buttons.at(7).setze_text("Neue Karte");
  buttons.at(8).setze_text("Beenden");     // 01-Nov-2001  Markus Weber    Added
#ifdef spaeter
  buttons.at(9).setze_text("Autosave");
#endif
  return &buttons;
}


void optionen_gui_t::zeichnen(koord pos, koord gr)
{
  infowin_t::zeichnen(pos, gr);

  display_ddd_box_clip(pos.x+10, pos.y+55, 191, 0, MN_GREY0, MN_GREY4);
}
