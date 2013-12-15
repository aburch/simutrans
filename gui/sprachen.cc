/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Dialog for language change
 * Hj. Malthaner, 2000
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../pathes.h"
#include "../display/simimg.h"
#include "../simskin.h"
#include "../besch/skin_besch.h"
#include "sprachen.h"

#include "../display/font.h"

#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../simsys.h"
#include "../utils/simstring.h"

#define L_DIALOG_WIDTH (220)

int sprachengui_t::cmp_language_button(sprachengui_t::language_button_t a, sprachengui_t::language_button_t b)
{
	return strcmp( a.button->get_text(), b.button->get_text() )<0;
}

/**
 * Causes the required fonts for currently selected
 * language to be loaded
 * @author Hj. Malthaner
 */
void sprachengui_t::init_font_from_lang()
{
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
	chdir(env_t::program_dir);
	display_load_font(prop_font_file_name);
	chdir(env_t::user_dir);

	const char * p = translator::translate("SEP_THOUSAND");
	char c = ',';
	if(*p != 'S') {
		c = *p;
	}
	set_thousand_sep(c);
	set_thousand_sep_exponent(atoi(translator::translate("SEP_THOUSAND_EXPONENT")));

	p = translator::translate("SEP_FRACTION");
	c = '.';
	if(*p != 'S') {
		c = *p;
	}
	set_fraction_sep(c);

	const char *str = "LARGE_NUMBER_STRING";
	p = translator::translate(str);
	double v = atof( translator::translate("LARGE_NUMBER_VALUE") );
	if(p == str  ||  v==0.0) {
		// fallback: will ignore it
		p = "";
		v = 1e99;
	}
	set_large_amout(p,v);
}


sprachengui_t::sprachengui_t() :
	gui_frame_t( translator::translate("Sprachen") ),
	text_label(&buf),
	flags(skinverwaltung_t::flaggensymbol?skinverwaltung_t::flaggensymbol->get_bild_nr(0):IMG_LEER),
	buttons(translator::get_language_count())
{
	// Coordinates are relative to parent (TITLEHEIGHT already subtracted)
	scr_coord cursor = scr_coord(D_MARGIN_LEFT,D_MARGIN_TOP);

	flags.enable_offset_removal(true);
	flags.set_pos( scr_coord(L_DIALOG_WIDTH-D_MARGIN_RIGHT-flags.get_size().w, cursor.y) );
	add_komponente( &flags);

	buf.clear();
	buf.append(translator::translate("LANG_CHOOSE\n"));
	text_label.set_pos( cursor );
	text_label.set_buf(&buf); // force recalculation of size (size)
	add_komponente( &text_label );
	cursor.y += text_label.get_size().h;

	seperator.set_pos( cursor );
	seperator.set_width( L_DIALOG_WIDTH-D_MARGINS_X-D_H_SPACE-flags.get_size().w );
	add_komponente( &seperator );
	cursor.y = max( seperator.get_pos().y + D_DIVIDER_HEIGHT, flags.get_pos().y + flags.get_size().h);

	const translator::lang_info* lang = translator::get_langs();
	for (int i = 0; i < translator::get_language_count(); ++i, ++lang) {
		button_t* b = new button_t();

		b->set_typ(button_t::square_state);
		b->set_text(lang->name);
		b->set_no_translate(true);

		// check, if font exists
		chdir(env_t::program_dir);
		const char* fontname = lang->translate("PROP_FONT_FILE");
		char prop_font_file_name [1024];
		sprintf(prop_font_file_name, "%s%s", FONT_PATH_X, fontname);
		bool ok = true;

		font_type fnt;
		fnt.screen_width = NULL;
		fnt.char_data = NULL;
		if(  *fontname == 0  ||  !load_font(&fnt,prop_font_file_name)  ) {
			ok = false;
		}
		else {
			if(  lang->utf_encoded  &&  fnt.num_chars <= 256  ) {
				dbg->warning( "sprachengui_t::sprachengui_t()", "Unicode language %s needs BDF fonts with most likely more than 256 characters!", lang->name );
			}
			free(fnt.screen_width);
			free(fnt.char_data);
		}
		// now we know this combination is working
		if(ok) {
			// only listener for working languages ...
			b->add_listener(this);
		} else {
			dbg->warning("sprachengui_t::sprachengui_t()", "no font found for %s", lang->name);
			b->disable();
		}

		// press button
		int id = translator::get_language(lang->iso);
		if(  translator::get_language() == id  ) {
			b->pressed = true;
		}

		// insert ordered by language name
		language_button_t lb;
		lb.button = b;
		lb.id = id;
		buttons.insert_ordered(lb, sprachengui_t::cmp_language_button);
	}

	// now set position
	const uint32 count = buttons.get_count();
	const scr_coord_val width = ((L_DIALOG_WIDTH - D_MARGINS_X - D_H_SPACE) >> 1);
	for(uint32 i=0; i<count; i++)
	{
		const bool right = (2*i >= count);
		const scr_coord_val x = cursor.x + (right ? width + D_H_SPACE : 0);
		const scr_coord_val y = cursor.y + (max(D_CHECKBOX_HEIGHT, LINESPACE) + D_V_SPACE) * (right ? i - (count + 1) / 2: i);
		buttons[i].button->set_pos( scr_coord( x, y + D_V_SPACE ) );
		buttons[i].button->set_width( width );
		add_komponente( buttons[i].button );
	}

	chdir(env_t::user_dir);

	set_windowsize( scr_size(L_DIALOG_WIDTH, D_TITLEBAR_HEIGHT + cursor.y + ((count+1)>>1)*(max(D_CHECKBOX_HEIGHT, LINESPACE)+D_V_SPACE) + D_MARGIN_BOTTOM ) );
}


bool sprachengui_t::action_triggered( gui_action_creator_t *komp, value_t)
{
	for(int i=0; i<translator::get_language_count(); i++) {
		button_t *b = buttons[i].button;

		if(b == komp) {
			b->pressed = true;
			translator::set_language(buttons[i].id);
			init_font_from_lang();
		}
		else {
			b->pressed = false;
		}

	}
	return true;
}
