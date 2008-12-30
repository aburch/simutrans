/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Dialog zur Sprachauswahl
 * Hj. Malthaner, 2000
 */

#include <stdio.h>
#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "../simdebug.h"
#include "../pathes.h"
#include "../simimg.h"
#include "../simskin.h"
#include "../besch/skin_besch.h"
#include "sprachen.h"

#include "../font.h"

#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../utils/simstring.h"


/**
 * Causes the required fonts for currently selected
 * language to be loaded
 * @author Hj. Malthaner
 */
void sprachengui_t::init_font_from_lang()
{
	chdir(umgebung_t::program_dir);
	static const char *default_name = "PROP_FONT_FILE";
	const char * prop_font_file = translator::translate(default_name);

	// Hajo: fallback if entry is missing
	// -> use latin-1 font
	if(prop_font_file == default_name) {
		prop_font_file = "prop.fnt";
	}

	// load large font
	char prop_font_file_name [1024];
	sprintf(prop_font_file_name, "%s%s", FONT_PATH_X, prop_font_file);
	display_load_font(prop_font_file_name);

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


sprachengui_t::sprachengui_t() :
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

	const translator::lang_info* lang = translator::get_langs();
	for (int i = 0; i < translator::get_language_count(); ++i, ++lang) {
		button_t& b = buttons[i];

		b.setze_pos(koord(10 + (i % 2) * 100 , 44 + 14 * (i / 2)));
		b.setze_typ(button_t::square_state);
		b.setze_text(lang->name);
		b.set_no_translate(true);

		// check, if font exists
		chdir(umgebung_t::program_dir);
		const char* fontname = lang->translate("PROP_FONT_FILE");
		char prop_font_file_name [1024];
		sprintf(prop_font_file_name, "%s%s", FONT_PATH_X, fontname);
		bool ok = true;
		font_type fnt;
		fnt.screen_width = NULL;
		fnt.char_data = NULL;
		if(  *fontname==0  ||  !load_font(&fnt,prop_font_file_name)  ) {
			ok = false;
		}
		else {
			if(  lang->utf_encoded  &&  fnt.num_chars<=255  ) {
				dbg->warning( "sprachengui_t::sprachengui_t()", "Unicode language %s need BDF fonts with most likely more than 256 characters!", lang->name );
			}
			free(fnt.screen_width);
			free(fnt.char_data);
		}
		// now we know this combination is working
		if(ok) {
			// only listener for working languages ...
			b.add_listener(this);
		} else {
			dbg->warning("sprachengui_t::sprachengui_t()", "no font found for %s", lang->name);
			b.disable();
		}
		add_komponente(&b);
	}
	chdir(umgebung_t::user_dir);

	buttons[translator::get_language()].pressed = true;
	setze_fenstergroesse( koord(220, 74+(translator::get_language_count()/2)*14) );
}



bool
sprachengui_t::action_triggered( gui_action_creator_t *komp, value_t)
{
	for(int i=0; i<translator::get_language_count(); i++) {
		button_t& b = buttons[i];
		if (&b == komp) {
			buttons[translator::get_language()].pressed = false;
			b.pressed = true;
			translator::set_language(i);
			init_font_from_lang();
		}
	}
	return true;
}
