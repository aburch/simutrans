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

#include "../simdebug.h"
#include "../pathes.h"
#include "../simimg.h"
#include "../simskin.h"
#include "../besch/skin_besch.h"
#include "sprachen.h"

#include "../dataobj/translator.h"
#include "../utils/simstring.h"


/**
 * Causes the required fonts for currently selected
 * language to be loaded
 * @author Hj. Malthaner
 */
void sprachengui_t::init_font_from_lang()
{
	const char * prop_font_file = translator::translate("PROP_FONT_FILE");

	// Hajo: fallback if entry is missing
	// -> use latin-1 font
	if(*prop_font_file == 'P') {
		prop_font_file = "prop.fnt";
	}

	// load large font
	char prop_font_file_name [1024];
	sprintf(prop_font_file_name, "%s%s", FONT_PATH_X, prop_font_file);
	display_load_font(prop_font_file_name,true);

#ifdef USE_SMALL_FONT
	const char * hex_font_file = translator::translate("4x7_FONT_FILE");

	if(hex_font_file[4] == 'F') {
		hex_font_file = "4x7.hex";
	}

	// load small font
	char hex_font_file_name [1024];
	sprintf(hex_font_file_name, "%s%s", FONT_PATH_X, hex_font_file);
	display_load_font(hex_font_file_name,false);

	// if missing, substitute
	display_check_fonts();
#endif

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



sprachengui_t::sprachengui_t(karte_t */* */) :
	gui_frame_t("Sprachen"),
	text_label(translator::translate("LANG_CHOOSE\n")),
	flags(skinverwaltung_t::flaggensymbol->gib_bild_nr(0)),
	buttons(translator::get_language_count())
{
	text_label.setze_pos( koord(10,0) );
	add_komponente( &text_label );

	flags.setze_pos( koord(220-60,-2) );
	add_komponente( &flags);

	seperator.setze_pos( koord(10, 37) );
	seperator.setze_groesse( koord(155,0) );
	add_komponente( &seperator );

	for(int i=0; i<translator::get_language_count(); i++) {
		buttons.at(i).setze_pos(koord(10 + (i%2) * 100 , 44+14*(i/2)));
		buttons.at(i).setze_typ(button_t::square_state);
		buttons.at(i).text = translator::get_language_name(i);

		// check, if font exists
		const char *fontname=translator::translate_from_lang(i,"PROP_FONT_FILE");
		char prop_font_file_name [1024];
		sprintf(prop_font_file_name, "%s%s", FONT_PATH_X, fontname);
		FILE *f=fopen(prop_font_file_name,"r");
		if(f) {
			fclose(f);
			// only listener for working languages ...
			buttons.at(i).add_listener(this);
		}
		else {
			dbg->warning("sprachengui_t::sprachengui_t()","no font found for %s",translator::get_language_name(i) );
			buttons.at(i).disable();
		}
		add_komponente( &(buttons.at(i)) );
	}

	buttons.at(translator::get_language()).pressed = true;
	setze_opaque(true);
	setze_fenstergroesse( koord(220, 74+(translator::get_language_count()/2)*14) );
}



bool
sprachengui_t::action_triggered(gui_komponente_t *komp, value_t)
{
	for(int i=0; i<translator::get_language_count(); i++) {
		if(&(buttons.at(i))==komp) {
			buttons.at(translator::get_language()).pressed = false;
			buttons.at(i).pressed = true;
			translator::set_language(i);
			init_font_from_lang();
		}
	}
	return true;
}
