/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../pathes.h"
#include "../display/simimg.h"
#include "../simskin.h"
#include "../tool/simmenu.h"
#include "../descriptor/skin_desc.h"
#include "sprachen.h"
#include "loadfont_frame.h"
#include "simwin.h"
#include "components/gui_image.h"
#include "components/gui_divider.h"

#include "../display/font.h"

#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../sys/simsys.h"
#include "../utils/simstring.h"
#include "../world/simworld.h"


int sprachengui_t::cmp_language_button(sprachengui_t::language_button_t a, sprachengui_t::language_button_t b)
{
	return strcmp( a.button->get_text(), b.button->get_text() )<0;
}

/**
 * Causes the required fonts for currently selected
 * language to be loaded
 */
void sprachengui_t::init_font_from_lang()
{
	bool reload_font = !has_character( translator::get_lang()->highest_character );

	// the real fonts for the current language
	std::string old_font = env_t::fontname;

	static const char *default_name = "PROP_FONT_FILE";
	const char *prop_font_file = translator::translate(default_name);

	// fallback if entry is missing -> use latin-1 font
	if(  prop_font_file == default_name  ) {
		prop_font_file = "cyr.bdf";
	}

	if(  reload_font  ) {
		// load large font
		dr_chdir( env_t::base_dir );
		bool ok = false;
		char prop_font_file_name[4096];
		tstrncpy( prop_font_file_name, prop_font_file, lengthof(prop_font_file_name) );
		char *f = strtok( prop_font_file_name, ";" );
		do {
			std::string fname = FONT_PATH_X;
			fname += prop_font_file_name;
			ok = display_load_font(fname.c_str());
			f = strtok( NULL, ";" );
		}
		while(  !ok  &&  f  );
		dr_chdir( env_t::user_dir );
	}

	// currency symbol
	{
		const char* org = "CURRENCY";
		const char* trans = translator::translate(org);
		if (trans != org  && env_t::currency_symbol[0] == 0) {
			// no special symbol selected
			set_currency_string(trans,env_t::currency_left);
			env_t::currency_symbol[0] = 0;
		}
	}

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
	set_large_amount(p,v);
}


sprachengui_t::sprachengui_t() :
	gui_frame_t( translator::translate("Sprachen") ),
	text_label(&buf),
	buttons(translator::get_language_count())
{
	set_table_layout(2,0);

	buf.clear();
	buf.append(translator::translate("LANG_CHOOSE\n"));
	text_label.set_buf(&buf); // force recalculation of size
	add_component( &text_label );

	if (skinverwaltung_t::flaggensymbol) {
		gui_image_t *flags = new_component<gui_image_t>(skinverwaltung_t::flaggensymbol->get_image_id(0));
		flags->enable_offset_removal(true);
	}
	else {
		new_component<gui_empty_t>();
	}

	new_component_span<gui_divider_t>(2);

	const translator::lang_info* lang = translator::get_langs();
	dr_chdir( env_t::base_dir );

	for (int i = 0; i < translator::get_language_count(); ++i, ++lang) {
		button_t* b = new button_t();

		b->set_typ(button_t::square_state);
		b->set_text(lang->name);
		b->set_no_translate(true);

		// we do not check for a working font anymore
		b->add_listener(this);

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
	dr_chdir(env_t::user_dir);

	// insert buttons such that language appears columnswise
	const uint32 count = buttons.get_count();
	const uint32 half = (count+1)/2;
	for(uint32 i=0; i < half; i++) {
		add_component(buttons[i].button);
		if (i+ half < count) {
			add_component(buttons[i+half].button);
		}
	}

	reset_min_windowsize();
	set_windowsize(get_min_windowsize() );
}


bool sprachengui_t::action_triggered( gui_action_creator_t *comp, value_t)
{
	for(int i=0; i<translator::get_language_count(); i++) {
		button_t *b = buttons[i].button;

		if(b == comp) {
			b->pressed = true;

			translator::set_language( buttons[i].id );
			init_font_from_lang();

			event_t* ev = new event_t();
			ev->ev_class = EVENT_SYSTEM;
			ev->ev_code = SYSTEM_RELOAD_WINDOWS;
			queue_event( ev );

			// check if we need another font ...
			{
				// we only show matching fonts for this language
				const utf8 *new_world = (const utf8 *)translator::translate("Beenden");
				size_t len;
				if (!has_character(utf8_decoder_t::decode(new_world, len))) {
					// load a matching font ...
					create_win(new loadfont_frame_t(), w_info, magic_font);
				}
			}

			if (world()) {
				// must not update non-existent toolbars
				tool_t::update_toolbars();
			}
		}
		else {
			b->pressed = false;
		}

	}
	return true;
}
