/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <string>

#include "../simmem.h"
#include "../simwin.h"
#include "../simmenu.h"
#include "../simsys.h"
#include "../simworld.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../player/simplay.h"

#include "help_frame.h"

// just loads a whole help file as one chunk
const char *help_frame_t::load_text(char const* const filename )
{
	std::string file_prefix("text/");
	std::string fullname = file_prefix + translator::get_lang()->iso + "/" + filename;
	chdir(umgebung_t::program_dir);

	FILE* file = fopen(fullname.c_str(), "rb");
	if (!file) {
		//Check for the 'base' language(ie en from en_gb)
		file = fopen((file_prefix + translator::get_lang()->iso_base + "/" + filename).c_str(), "rb");
	}
	if (!file) {
		// Hajo: check fallback english
		file = fopen((file_prefix + "/en/" + filename).c_str(), "rb");
	}
	// go back to load/save dir
	chdir( umgebung_t::user_dir );

	bool success=false;
	if(file) {
		fseek(file,0,SEEK_END);
		long len = ftell(file);
		if(  len>0  ) {
			char* const buf = MALLOCN(char, len + 1);
			fseek( file, 0, SEEK_SET );
			fread(  buf, 1, len, file);
			buf[len] = '\0';
			fclose( file );
			return buf;
		}
	}

	return NULL;
}


void help_frame_t::set_text(const char * buf)
{
	if(  scrolly_generaltext.is_visible()  ) {
		generaltext.set_pos( koord(D_MARGIN_LEFT, D_MARGIN_TOP) );
		int generalwidth = generaltext.get_preferred_size().x;
		if(  generalwidth > display_get_width()/3  ) {
			generalwidth = display_get_width()/3;
		}
		generaltext.set_groesse( koord( generalwidth, 0 ) );
		generaltext.set_groesse( generaltext.get_text_size() );
	}
	else {
		generaltext.set_groesse( koord( 0, 0 ) );
	}

	helptext.set_text(buf);
	helptext.set_pos( koord(D_MARGIN_LEFT, D_MARGIN_TOP) );
	helptext.set_groesse(koord(220, 0));

	// try to get the following sizes
	// y<400 or, if not possible, x<620
	int last_y = 0;
	koord curr=helptext.get_preferred_size();
	for( int i=0;  i<10  &&  curr.y>400  &&  curr.y!=last_y;  i++  ) {
		helptext.set_groesse(koord(260+i*40, 0));
		last_y = curr.y;
		curr = helptext.get_preferred_size();
	}

	// the second line isn't redundant!!!
	helptext.set_groesse(helptext.get_preferred_size());
	helptext.set_groesse(helptext.get_preferred_size());

	set_name(helptext.get_title());

	// calculate sizes (might not a help but info window, which do not have general text)
	KOORD_VAL size_x = helptext.get_groesse().x + D_MARGIN_LEFT + D_MARGIN_RIGHT + scrollbar_t::BAR_SIZE;
	KOORD_VAL size_y = max( helptext.get_groesse().y, generaltext.get_groesse().y );
	if(  scrolly_generaltext.is_visible()  ) {
		size_x += generaltext.get_groesse().x + D_MARGIN_LEFT + D_MARGIN_RIGHT + scrollbar_t::BAR_SIZE;
	}
	// set window size
	if(  size_x > display_get_width()-32  ) {
		size_x = display_get_width()-32;
	}

	if(  size_y>display_get_height()-64) {
		size_y = display_get_height()-64;
	}
	set_fenstergroesse( koord( size_x, size_y ) );

	resize( koord(0,0) );
	set_dirty();
}


help_frame_t::help_frame_t() :
	gui_frame_t( translator::translate("Help") ),
	scrolly_helptext(&helptext),
	scrolly_generaltext(&generaltext)
{
	set_text("<title>Unnamed</title><p>No text set</p>");
	helptext.add_listener(this);
	set_resizemode(diagonal_resize);
	scrolly_helptext.set_show_scroll_x(true);
	add_komponente(&scrolly_helptext);
	// info windows do not show general text
	scrolly_generaltext.set_visible( false );
	set_min_windowsize(koord(70, 30));
}


help_frame_t::help_frame_t(char const* const filename) :
	gui_frame_t( translator::translate("Help") ),
	scrolly_helptext(&helptext),
	scrolly_generaltext(&generaltext)
{
	// load the content list
	if(  const char *buf = load_text( "general.txt" )  ) {
		generaltext.set_text( buf );
		guarded_free( (void *)buf );
	}

	// the key help texts are built automagically
	if (strcmp(filename, "keys.txt") == 0) {
		cbuffer_t buf;
		buf.append( translator::translate( "<title>Keyboard Help</title>\n<h1><strong>Keyboard Help</strong></h1><p>\n" ) );
		spieler_t *sp = spieler_t::get_welt()->get_active_player();
		const char *trad_str = translator::translate( "<em>%s</em> - %s<br>\n" );
		FOR(vector_tpl<werkzeug_t*>, const i, werkzeug_t::char_to_tool) {
			char const* c = NULL;
			char str[16];
			switch (uint16 const key = i->command_key) {
				case '<': c = "&lt;"; break;
				case '>': c = "&gt;"; break;
				case 27:  c = "ESC"; break;
				case SIM_KEY_HOME:	c=translator::translate( "[HOME]" ); break;
				case SIM_KEY_END:	c=translator::translate( "[END]" ); break;
				default:
					if (key < 32) {
						sprintf(str, "%s + %c", translator::translate("[CTRL]"), '@' + key);
					} else if (key < 256) {
						sprintf(str, "%c", key);
					} else if (key < SIM_KEY_F15) {
						sprintf(str, "F%i", key - SIM_KEY_F1 + 1);
					}
					else {
						// try unicode
						str[utf16_to_utf8(key, (utf8*)str)] = '\0';
					}
					c = str;
					break;
			}
			buf.printf(trad_str, c, i->get_tooltip(sp));
		}
		set_text(buf);
	}
	else if(  strcmp( filename, "general.txt" )!=0  ) {
		// and the actual help text (if not identical)
		if(  const char *buf = load_text( filename )  ) {
			set_text( buf );
			guarded_free( (void *)buf );
		}
		else {
			set_text("<title>Error</title>Help text not found");
		}
	}
	else {
		set_text( "" );
	}

	set_resizemode(diagonal_resize);
	scrolly_helptext.set_show_scroll_x(true);
	add_komponente(&scrolly_helptext);
	helptext.add_listener(this);
	scrolly_generaltext.set_show_scroll_x(true);
	add_komponente(&scrolly_generaltext);
	scrolly_generaltext.set_visible( true );
	generaltext.add_listener(this);
	set_min_windowsize(koord(200, 30));
}


/**
 * Called upon link activation
 * @param the hyper ref of the link
 * @author Hj. Malthaner
 */
bool help_frame_t::action_triggered( gui_action_creator_t *, value_t extra)
{
	const char *filename = (const char *)(extra.p);
	if(  gui_frame_t *gui = win_get_magic( magic_mainhelp )  ) {
		top_win( gui );
		if(  strcmp( filename, "general.txt" )!=0  ) {
			// and the actual help text (if not identical)
			if(  const char *buf = load_text( filename )  ) {
				((help_frame_t *)gui)->set_text( buf );
				guarded_free( (void *)buf );
			}
			else {
				((help_frame_t *)gui)->set_text("<title>Error</title>Help text not found");
			}
		}
		else {
			((help_frame_t *)gui)->set_text( "" );
		}

	}
	else {
		assert( false );
		// should actually not happen
		create_win(new help_frame_t((const char *)(extra.p)), w_info, magic_mainhelp );
	}
	return true;
}



/**
 * Resize the contents of the window
 * @author Markus Weber
 */
void help_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	KOORD_VAL generalwidth = 0;
	if(  scrolly_generaltext.is_visible()  ) {
		// do not use more than 1/3 for the general infomations
		generalwidth = generaltext.get_preferred_size().x+D_MARGIN_LEFT+D_MARGIN_RIGHT+scrollbar_t::BAR_SIZE;
		if(  generalwidth > get_client_windowsize().x/3  ) {
			//generalwidth = get_client_windowsize().x/3;
		}
		scrolly_generaltext.set_pos( koord( 0, 0) );
		scrolly_generaltext.set_groesse( koord( generalwidth, get_client_windowsize().y ) );
		koord general_gr = scrolly_generaltext.get_groesse() - helptext.get_pos() - koord(scrollbar_t::BAR_SIZE+D_MARGIN_RIGHT+D_MARGIN_LEFT, scrollbar_t::BAR_SIZE);
		generaltext.set_groesse( general_gr );
		generaltext.set_groesse( generaltext.get_text_size());
	}

	scrolly_helptext.set_pos( koord( generalwidth, 0) );
	scrolly_helptext.set_groesse( get_client_windowsize()-koord(generalwidth,0) );
	koord helptext_gr = scrolly_helptext.get_groesse() - helptext.get_pos() - koord(scrollbar_t::BAR_SIZE+D_MARGIN_RIGHT+D_MARGIN_LEFT, scrollbar_t::BAR_SIZE );
	helptext.set_groesse( helptext_gr );
	helptext.set_groesse( helptext.get_text_size());
}
