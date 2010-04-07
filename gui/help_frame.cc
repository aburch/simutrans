/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simmem.h"
#include "../simwin.h"
#include "../simmenu.h"
#include "../simworld.h"

#include "../utils/cbuffer_t.h"
#include "../utils/cstring_t.h"
#include "../utils/simstring.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"

#include "help_frame.h"

// for chdir
#ifdef WIN32
#include <direct.h>
#ifdef __MINGW32CE__
#define _chdir(i) chdir(i)
#endif
#else
#include <unistd.h>
#endif


void help_frame_t::set_text(const char * buf)
{
	flow.set_text(buf);

	flow.set_pos(koord(10, 10));
	flow.set_groesse(koord(220, 0));

	// try to get the following sizes
	// y<400 or, if not possible, x<620
	int last_y = 0;
	koord curr=flow.get_preferred_size();
	for( int i=0;  i<10  &&  curr.y>400  &&  curr.y!=last_y;  i++  )
	{
		flow.set_groesse(koord(260+i*40, 0));
		last_y = curr.y;
		curr = flow.get_preferred_size();
	}

	// the second line isn't redundant!!!
	flow.set_groesse(flow.get_preferred_size());
	flow.set_groesse(flow.get_preferred_size());

	set_name(flow.get_title());

	// set window size
	curr = flow.get_groesse()+koord(20, 36);
	if(curr.y>display_get_height()-64) {
		curr.y = display_get_height()-64;
	}
	set_fenstergroesse( curr );
	resize( koord(0,0) );
}


help_frame_t::help_frame_t() :
	gui_frame_t("Help"),
	scrolly(&flow)
{
	set_text("<title>Unnamed</title><p>No text set</p>");
	add_komponente(&flow);
	flow.add_listener(this);
}


help_frame_t::help_frame_t(cstring_t filename) :
	gui_frame_t("Help"),
	scrolly(&flow)
{
	// the key help texts are built automagically
	if(filename=="keys.txt") {
		cbuffer_t buf(16000);
		buf.append( translator::translate( "<title>Keyboard Help</title>\n<h1><strong>Keyboard Help</strong></h1><p>\n" ) );
		spieler_t *sp = spieler_t::get_welt()->get_active_player();
		const char *trad_str = translator::translate( "<em>%s</em> - %s<br>\n" );
		for (vector_tpl<werkzeug_t *>::const_iterator iter = werkzeug_t::char_to_tool.begin(), end = werkzeug_t::char_to_tool.end(); iter != end; ++iter) {
			char const* c = NULL;
			char str[16];
			switch(  (*iter)->command_key  ) {
				case '<': c = "&lt;"; break;
				case '>': c = "&gt;"; break;
				case 27:  c = "ESC"; break;
				case SIM_KEY_HOME:	c=translator::translate( "[HOME]" ); break;
				case SIM_KEY_END:	c=translator::translate( "[END]" ); break;
				default:
					if((*iter)->command_key<32) {
						sprintf( str, "%s + %C", translator::translate( "[CTRL]" ),  (*iter)->command_key+64 );
					}
					else if((*iter)->command_key<256) {
						sprintf( str, "%C", (*iter)->command_key );
					}
					else if((*iter)->command_key<SIM_KEY_F15) {
						sprintf( str, "F%i", (*iter)->command_key-255 );
					}
					else {
						// try unicode
						str[utf16_to_utf8( (*iter)->command_key, (utf8 *)str )] = 0;
					}
					c = str;
					break;
			}
			buf.printf( trad_str, c, (*iter)->get_tooltip(sp) );
		}
		set_text(buf);
	}
	else {
		cstring_t file_prefix("text/");
		cstring_t fullname = file_prefix + translator::get_lang()->iso + "/" + filename;
		chdir( umgebung_t::program_dir );

		FILE * file = fopen(fullname, "rb");
		if(!file) {
			//Check for the 'base' language(ie en from en_gb)
			file = fopen(file_prefix + translator::get_lang()->iso_base + "/" + filename, "rb");
	  }
		if(!file) {
			// Hajo: check fallback english
			file = fopen(file_prefix+"/en/"+filename,"rb");
		}
		// go back to load/save dir
		chdir( umgebung_t::user_dir );

		bool success=false;
		if(file) {
			fseek(file,0,SEEK_END);
			long len = ftell(file);
			if(len>0) {
				char* const buf = MALLOCN(char, len + 1);
				fseek(file,0,SEEK_SET);
				fread(buf, 1, len, file);
				buf[len] = '\0';
				fclose(file);
				success = true;
				set_text(buf);
				free(buf);
			}
		}

		if(!success) {
			set_text("<title>Error</title>Help text not found");
		}
	}

	set_resizemode(diagonal_resize);
	add_komponente(&scrolly);
	flow.add_listener(this);
}


/**
 * Called upon link activation
 * @param the hyper ref of the link
 * @author Hj. Malthaner
 */
bool
help_frame_t::action_triggered( gui_action_creator_t *, value_t extra)
{
	const char *str = (const char *)(extra.p);
	uint32 magic = 0;
	while(*str) {
		magic += *str++;
	}
	magic = (magic % 842) + magic_info_pointer;
	create_win(new help_frame_t((const char *)(extra.p)), w_info, magic );
	return true;
}



/**
 * Resize the contents of the window
 * @author Markus Weber
 */
void help_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	scrolly.set_groesse(get_client_windowsize());
}
