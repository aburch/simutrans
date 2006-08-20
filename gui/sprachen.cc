/*
 * sprachen.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/* sprachen.cc
 *
 * Dialog zur Sprachauswahl
 * Hj. Malthaner, 2000
 */

#include <stdio.h>

#include "../pathes.h"
#include "../simevent.h"
#include "../simimg.h"
#include "../simskin.h"
#include "../besch/skin_besch.h"
#include "sprachen.h"

#include "../simgraph.h"
#include "../simdisplay.h"
#include "../dataobj/translator.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"


/**
 * Causes the required fonts for currently selected
 * language to be loaded
 * @author Hj. Malthaner
 */
void sprachengui_t::init_font_from_lang()
{
  const char * prop_font_file =
    translator::translate("PROP_FONT_FILE");

  const char * hex_font_file =
    translator::translate("4x7_FONT_FILE");

  // Hajo: fallback if entry is missing
  // -> use latin-1 font

  if(*prop_font_file == 'P') {
    prop_font_file = "prop.fnt";
    hex_font_file = "4x7.hex";
  }

	// load large font
  char prop_font_file_name [1024];
	sprintf(prop_font_file_name, "%s%s", FONT_PATH_X, prop_font_file);
	display_load_font(prop_font_file_name,true);

	// load small font
  char hex_font_file_name [1024];
  sprintf(hex_font_file_name, "%s%s", FONT_PATH_X, hex_font_file);
  display_load_font(hex_font_file_name,false);

	// if missing, substitute
  display_check_fonts();

  const char * p = translator::translate("SEP_THOUSAND");
  char c = ',';
  if(*p != 'S') {
    c = *p;
  }

  set_thousand_sep(c);

  p = translator::translate("SEP_FRACTION");
  c = '.';
  if(*p != 'S') {
    c = *p;
  }

  set_fraction_sep(c);
}


sprachengui_t::sprachengui_t(karte_t *welt) : infowin_t(welt)
{
    buttons = new vector_tpl<button_t> (translator::get_language_count());
    button_t button_def;

    int i;
    for(i=0; i<translator::get_language_count(); i++) {
	button_def.setze_pos(koord(11 + (i%2) * 84 , 68+15*(i/2)));
        button_def.setze_typ(button_t::square);
	button_def.text = translator::get_language_name(i);

	buttons->append(button_def);
    }

    buttons->at(translator::get_language()).pressed = true;
}


sprachengui_t::~sprachengui_t()
{
    delete buttons;
}


/**
 * Jedes Objekt braucht ein Bild.
 *
 * @author Hj. Malthaner
 * @return Die Nummer des aktuellen Bildes für das Objekt.
 */
int sprachengui_t::gib_bild() const
{
    return skinverwaltung_t::flaggensymbol->gib_bild_nr(0);
}

koord sprachengui_t::gib_bild_offset() const {  return koord(-9,12); }

koord
sprachengui_t::gib_fenstergroesse() const
{
  return koord(64 + 2*(8*7+4) + 64, 80+(translator::get_language_count()/2)*15);
}

/**
 *
 * @author Hj. Malthaner
 * @return eine NULL-Terminierte Liste von Buttons fuer das
 * Beobachtungsfenster
 */
vector_tpl<button_t>*
sprachengui_t::gib_fensterbuttons()
{
    buttons->at(translator::get_language()).pressed = true;
    return buttons;
}


void sprachengui_t::info(cbuffer_t & buf) const
{
  buf.append(translator::translate("LANG_CHOOSE\n"));
}


void
sprachengui_t::infowin_event(const event_t *ev)
{
    infowin_t::infowin_event(ev);

    if(IS_LEFTCLICK(ev)) {
	for(int i=0; i<translator::get_language_count(); i++) {
	    if(buttons->at(i).getroffen(ev->mx, ev->my)) {
		buttons->at(translator::get_language()).pressed = false;
		buttons->at(i).pressed = true;
                translator::set_language(i);

		init_font_from_lang();
	    }
	}
    }
}


void sprachengui_t::zeichnen(koord pos, koord gr)
{
  infowin_t::zeichnen(pos, gr);
  display_divider(pos.x+10, pos.y+55, 155);
}
