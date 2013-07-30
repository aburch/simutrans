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
#include "../simsys.h"
#include "../simworld.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../player/simplay.h"
#include "werkzeug_waehler.h"

#include "help_frame.h"

// Padding between text and scroll client area.
#define DIALOG_MIN_WIDTH (100)

help_frame_t::help_frame_t() :
	gui_frame_t( translator::translate("Help") ),
	scrolly_generaltext(&generaltext),
	scrolly_helptext(&helptext)
{
	// info windows do not show general text
	scrolly_generaltext.set_visible( false );

	set_text("<title>Unnamed</title><p>No text set</p>");

	helptext.set_pos( koord(D_MARGIN_LEFT,D_MARGIN_TOP) );
	helptext.add_listener(this);

	scrolly_helptext.set_show_scroll_x(true);
	add_komponente(&scrolly_helptext);

	set_resizemode(diagonal_resize);
	set_min_windowsize( koord( D_MARGIN_LEFT + (button_t::gui_scrollbar_size.x<<1) + DIALOG_MIN_WIDTH + D_MARGIN_RIGHT, D_TITLEBAR_HEIGHT + D_MARGIN_TOP + (button_t::gui_scrollbar_size.y<<1) + D_MARGIN_BOTTOM) );
}


help_frame_t::help_frame_t(char const* const filename) :
	gui_frame_t( translator::translate("Help") ),
	scrolly_generaltext(&generaltext),
	scrolly_helptext(&helptext)
{
	// we now exclusive build out index on the fly
	slist_tpl<plainstring> already_there;
	cbuffer_t index_txt;
	cbuffer_t introduction;
	cbuffer_t usage;
	cbuffer_t toolbars;
	cbuffer_t game_start;
	cbuffer_t how_to_play;
	cbuffer_t others;

	helptext.set_pos( koord(D_MARGIN_LEFT,D_MARGIN_TOP) );
	scrolly_helptext.set_show_scroll_x(true);
	add_komponente(&scrolly_helptext);
	helptext.add_listener(this);

	generaltext.set_pos( koord(D_MARGIN_LEFT,D_MARGIN_TOP) );
	scrolly_generaltext.set_pos( koord( 0, 0) );
	scrolly_generaltext.set_show_scroll_x(true);
	scrolly_generaltext.set_visible( true );
	add_komponente(&scrolly_generaltext);
	generaltext.add_listener(this);

	add_helpfile( introduction, NULL, "simutrans.txt", true, 0 );

	// main usage section
	{
		// get title of keyboard help ...
		std::string kbtitle = extract_title( translator::translate( "<title>Keyboard Help</title>\n<h1><strong>Keyboard Help</strong></h1><p>\n" ) );
		assert( !kbtitle.empty() );
		usage.printf( "<a href=\"keys.txt\">%s</a><br>\n", kbtitle.c_str() );
	}
	add_helpfile( usage, NULL, "mouse.txt", true, 0 );
	add_helpfile( usage, NULL, "window.txt", true, 0 );

	// enumerate toolbars
	bool special = false;
	add_helpfile( toolbars, NULL, "mainmenu.txt", false, 0 );
	FOR( vector_tpl<toolbar_t *>, iter, werkzeug_t::toolbar_tool ) {
		if(  strstart(iter->get_werkzeug_waehler()->get_hilfe_datei(),"list.txt" )  ) {
			continue;
		}
		add_helpfile( toolbars, iter->get_werkzeug_waehler()->get_name(), iter->get_werkzeug_waehler()->get_hilfe_datei(), false, 0 );
		if(  strstart(iter->get_werkzeug_waehler()->get_hilfe_datei(),"railtools.txt" )  ) {
			add_helpfile( toolbars, NULL, "bridges.txt", true, 1 );
			add_helpfile( toolbars, NULL, "signals.txt", true, 1 );
			add_helpfile( toolbars, "set signal spacing", "signal_spacing.txt", false, 1 );
		}
		if(  strstart(iter->get_werkzeug_waehler()->get_hilfe_datei(),"roadtools.txt" )  ) {
			add_helpfile( toolbars, NULL, "privatesign_info.txt", false, 1 );
			add_helpfile( toolbars, NULL, "trafficlight_info.txt", false, 1 );
		}
		if(  !special  &&  (  strstart(iter->get_werkzeug_waehler()->get_hilfe_datei(),"special.txt" )
							||  strstart(iter->get_werkzeug_waehler()->get_hilfe_datei(),"edittools.txt" )  )
			) {
			special = true;
			add_helpfile( toolbars, "baum builder", "baum_build.txt", false, 1 );
			add_helpfile( toolbars, "citybuilding builder", "citybuilding_build.txt", false, 1 );
			add_helpfile( toolbars, "curiosity builder", "curiosity_build.txt", false, 1 );
			add_helpfile( toolbars, "factorybuilder", "factory_build.txt", false, 1 );
		}
	}
	add_helpfile( toolbars, NULL, "inspection_tool.txt", true, 0 );
	add_helpfile( toolbars, NULL, "removal_tool.txt", true, 0 );
	add_helpfile( toolbars, "LISTTOOLS", "list.txt", false, 0 );
	add_helpfile( toolbars, NULL, "citylist_filter.txt", false, 1 );
	add_helpfile( toolbars, NULL, "convoi.txt", false, 1 );
	add_helpfile( toolbars, NULL, "convoi_filter.txt", false, 2 );
	add_helpfile( toolbars, NULL, "curiositylist_filter.txt", false, 1 );
	add_helpfile( toolbars, NULL, "factorylist_filter.txt", false, 1 );
	add_helpfile( toolbars, NULL, "goods_filter.txt", false, 1 );
	add_helpfile( toolbars, NULL, "haltlist.txt", false, 1 );
	add_helpfile( toolbars, NULL, "haltlist_filter.txt", false, 2 );
	add_helpfile( toolbars, NULL, "labellist_filter.txt", false, 1 );

	add_helpfile( game_start, "Neue Welt", "new_world.txt", false, 0 );
	add_helpfile( game_start, "Lade Relief", "load_relief.txt", false, 1 );
	add_helpfile( game_start, "Climate Control", "climates.txt", false, 1 );
	add_helpfile( game_start, "Setting", "settings.txt", false, 1 );
	add_helpfile( game_start, "Load game", "load.txt", false, 0 );
	add_helpfile( game_start, "Load scenario", "scenario.txt", false, 0 );
	add_helpfile( game_start, "join game", "server.txt", false, 0 );
	add_helpfile( game_start, "Speichern", "save.txt", false, 0 );

	add_helpfile( how_to_play, "Reliefkarte", "map.txt", false, 0 );
	add_helpfile( how_to_play, "enlarge map", "enlarge_map.txt", false, 1 );
	add_helpfile( how_to_play, NULL, "underground.txt", true, 0 );
	add_helpfile( how_to_play, NULL, "citywindow.txt", true, 0 );
	add_helpfile( how_to_play, NULL, "depot.txt", false, 0 );
	add_helpfile( how_to_play, NULL, "convoiinfo.txt", false, 0 );
	add_helpfile( how_to_play, NULL, "convoidetail.txt", false, 1 );
	add_helpfile( how_to_play, "Line Management", "linemanagement.txt", false, 0 );
	add_helpfile( how_to_play, "Fahrplan", "schedule.txt", false, 1 );
	add_helpfile( how_to_play, NULL, "station.txt", false, 0 );
	add_helpfile( how_to_play, NULL, "station_details.txt", false, 1 );
	add_helpfile( how_to_play, NULL, "industry_info.txt", false, 0 );
	add_helpfile( how_to_play, "Spielerliste", "players.txt", false, 0 );
	add_helpfile( how_to_play, "Finanzen", "finances.txt", false, 1 );
	add_helpfile( how_to_play, "Farbe", "color.txt", false, 1 );
//		add_helpfile( how_to_play, "Scenario", "scenario.txt", false, 1 );
	add_helpfile( how_to_play, "Enter Password", "password.txt", false, 1 );

	add_helpfile( others, "Einstellungen aendern", "options.txt", false, 0 );
	add_helpfile( others, "Helligk. u. Farben", "display.txt", false, 0 );
	add_helpfile( others, "Mailbox", "mailbox.txt", false, 0 );
	add_helpfile( others, "Sound settings", "sound.txt", false, 0 );
	add_helpfile( others, "Sprachen", "language.txt", false, 0 );

	index_txt.printf( translator::translate("<h1>Index</h1><p>*: only english</p><p>General</p>%s<p>Usage</p>%s<p>Tools</p>%s<p>Start</p>%s<p>How to play</p>%s<p>Others:</p>%s"),
		(const char *)introduction, (const char *)usage, (const char *)toolbars, (const char *)game_start, (const char *)how_to_play, (const char *)others );
	generaltext.set_text( index_txt );

	set_helpfile( filename, true );

	//scrolly_helptext.set_show_scroll_x(true);
	//add_komponente(&scrolly_helptext);

	set_resizemode(diagonal_resize);
	set_min_windowsize(koord(200, D_TITLEBAR_HEIGHT + D_MARGIN_TOP + (button_t::gui_scrollbar_size.y) + D_MARGIN_BOTTOM));
}


// opens or tops main helpfile
void help_frame_t::open_help_on( const char *helpfilename )
{
	if(  help_frame_t *gui = (help_frame_t *)win_get_magic( magic_mainhelp )  ) {
		top_win( gui );
		gui->set_helpfile( helpfilename, false );
	}
	else {
		create_win( new help_frame_t( helpfilename ), w_info, magic_mainhelp );
	}
}


// just loads a whole help file as one chunk
static const char *load_text(char const* const filename )
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


void help_frame_t::set_text(const char * buf, bool resize_frame )
{
	helptext.set_text(buf);
	helptext.set_pos( koord(D_MARGIN_LEFT,D_MARGIN_TOP) );

	if(  resize_frame  ) {

		// try to get the following sizes
		// y<400 or, if not possible, x<620
		helptext.set_groesse(koord(220, 0));
		int last_y = 0;
		koord curr = helptext.get_preferred_size();
		for(  int i = 0;  i<10  &&  curr.y>400  &&  curr.y!=last_y;  i++  ) {
			helptext.set_groesse(koord(260+i*40, 0));
			last_y = curr.y;
			curr = helptext.get_preferred_size();
		}

		// the second line isn't redundant!!!
		helptext.set_groesse(helptext.get_preferred_size());
		helptext.set_groesse(helptext.get_preferred_size());

		if(  scrolly_generaltext.is_visible()  ) {
			generaltext.set_pos( koord(D_MARGIN_LEFT, D_MARGIN_TOP) );
			generaltext.set_groesse( koord( min(180,display_get_width()/3), 0 ) );
			int generalwidth = min( display_get_width()/3, generaltext.get_preferred_size().x );
			generaltext.set_groesse( koord( generalwidth, helptext.get_groesse().y ) );
			generaltext.set_groesse( generaltext.get_preferred_size() );
			generaltext.set_groesse( generaltext.get_preferred_size() );
			generaltext.set_groesse( generaltext.get_text_size() );
		}
		else {
			generaltext.set_groesse( koord(D_MARGIN_LEFT, D_MARGIN_TOP) );
		}

		// calculate sizes (might not a help but info window, which do not have general text)
		KOORD_VAL size_x = helptext.get_groesse().x + D_MARGIN_LEFT + D_MARGIN_RIGHT + button_t::gui_scrollbar_size.x;
		KOORD_VAL size_y = helptext.get_groesse().y + D_TITLEBAR_HEIGHT + D_MARGIN_TOP  + D_MARGIN_BOTTOM + button_t::gui_scrollbar_size.y;
		if(  scrolly_generaltext.is_visible()  ) {
			size_x += generaltext.get_groesse().x + button_t::gui_scrollbar_size.x + D_H_SPACE;
		}
		// set window size
		if(  size_x > display_get_width()-32  ) {
			size_x = display_get_width()-32;
		}

		if(  size_y>display_get_height()-64) {
			size_y = display_get_height()-64;
		}
		set_fenstergroesse( koord( size_x, size_y ) );
	}

	// generate title
	title = "";
	if(  scrolly_generaltext.is_visible()  ) {
		title = translator::translate( "Help" );
		title += " - ";
	}
	title += helptext.get_title();
	set_name( title.c_str() );

	resize( koord(0,0) );
	set_dirty();
}


// show the help to one topic
void help_frame_t::set_helpfile(const char *filename, bool resize_frame )
{
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
					}
					else if (key < 256) {
						sprintf(str, "%c", key);
					}
					else if (key < SIM_KEY_F15) {
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
		set_text( buf, resize_frame );
	}
	else if(  strcmp( filename, "general.txt" )!=0  ) {
		// and the actual help text (if not identical)
		if(  const char *buf = load_text( filename )  ) {
			set_text( buf, resize_frame );
			guarded_free(const_cast<char *>(buf));
		}
		else {
			set_text( "<title>Error</title>Help text not found", resize_frame );
		}
	}
	else {
		// default text when opening general help
		if(  const char *buf = load_text( "about.txt" )  ) {
			set_text( buf, resize_frame );
			guarded_free(const_cast<char *>(buf));
		}
		else if(  const char *buf = load_text( "simutrans.txt" )  ) {
			set_text( buf, resize_frame );
			guarded_free(const_cast<char *>(buf));
		}
		else {
			set_text( "", resize_frame );
		}
	}
}


FILE *help_frame_t::has_helpfile( char const* const filename, int &mode )
{
	mode = native;
	std::string file_prefix("text/");
	std::string fullname = file_prefix + translator::get_lang()->iso + "/" + filename;
	chdir(umgebung_t::program_dir);

	FILE* file = fopen(fullname.c_str(), "rb");
	if(  !file  &&  strcmp(translator::get_lang()->iso,translator::get_lang()->iso_base)  ) {
		//Check for the 'base' language(ie en from en_gb)
		file = fopen(  (file_prefix + translator::get_lang()->iso_base + "/" + filename).c_str(), "rb"  );
	}
	if(  !file  ) {
		// Hajo: check fallback english
		file = fopen((file_prefix + "en/" + filename).c_str(), "rb");
		mode = english;
	}
	// go back to load/save dir
	chdir( umgebung_t::user_dir );
	// success?
	if(  !file  ) {
		mode = missing;
	}
	return file;
}


// extracts the title and ASCII from a string
std::string help_frame_t::extract_title( const char *htmllines )
{
	const uint8 *start = (const uint8 *)strstr( htmllines, "<title>" );
	const uint8 *end = (const uint8 *)strstr( htmllines, "</title>" );
	uint8 title_form_html[1024];
	if(  start  &&  end  &&  (size_t)(end-start)<lengthof(title_form_html)  ) {
		uint8 *dest = title_form_html;
		const uint8 *c = start;
		while(  c < end  ) {
			if(  *c == '<'  ) {
				while(  *c  &&  *c++!='>'  ) {
				}
				continue;
			}
			// skip tabs and newlines
			if(  *c>=32  ) {
				*dest++ = *c++;
			}
			else {
				// avoid double spaces
				if(  dest!=title_form_html  &&  dest[-1]!=' '  ) {
					*dest++ = ' ';
				}
				c++;
			}
		}
		*dest = 0;
		return (const char *)title_form_html;
	}
	return "";
}


void help_frame_t::add_helpfile( cbuffer_t &section, const char *titlename, const char *filename, bool only_native, int indent_level )
{
	if(  strempty(filename)  ) {
		return;
	}
	int mode;
	FILE *file = has_helpfile( filename, mode );
	if(  (  only_native  &&  mode!=native  )  ||  mode==missing  ) {
		return;
	}
	std::string filetitle;	// just in case as temporary storage ...
	if(  titlename == NULL  &&  file  ) {
		// get the title from the helpfile
		char htmlline[1024];
		fread( htmlline, lengthof(htmlline)-1, 1, file );
		filetitle = extract_title( htmlline );
		if(  filetitle.empty()  ) {
			// no idea how to generate the right name ...
			titlename = filename;
		}
		else {
			titlename = filetitle.c_str();
		}
	}
	else {
		titlename = translator::translate( titlename );
	}
	// now build the entry
	if(  mode != missing  ) {
		while(  indent_level-- > 0  ) {
			section.append( "+" );
		}
		if(  mode == native  ) {
			section.printf( "<a href=\"%s\">%s</a><br>\n", filename, titlename );
		}
		else if(  mode == english  ) {
			section.printf( "<a href=\"%s\">%s%s</a><br>\n", filename, "*", titlename );
		}
		fclose( file );
	}
}


/**
 * Called upon link activation
 * @param the hyper ref of the link
 * @author Hj. Malthaner
 */
bool help_frame_t::action_triggered( gui_action_creator_t *, value_t extra)
{
	top_win( this );
	set_helpfile( (const char *)(extra.p), false );
	return true;
}


void help_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	scr_coord_val generalwidth = 0;
	if(  scrolly_generaltext.is_visible()  ) {
		// do not use more than 1/3 for the general infomations
		generalwidth = min( display_get_width()/3, generaltext.get_preferred_size().x ) + button_t::gui_scrollbar_size.x + D_MARGIN_LEFT;
		scrolly_generaltext.set_groesse( koord( generalwidth, get_fenstergroesse().y-D_TITLEBAR_HEIGHT ) );

		koord general_gr = scrolly_generaltext.get_groesse() - button_t::gui_scrollbar_size - koord(D_MARGIN_LEFT,D_MARGIN_TOP);
		generaltext.set_groesse( general_gr );
		generaltext.set_groesse( generaltext.get_text_size() );

		generalwidth += D_H_SPACE;
		scrolly_helptext.set_pos( koord( generalwidth, 0 ) );
	}

	scrolly_helptext.set_groesse( get_fenstergroesse() - koord( generalwidth, D_TITLEBAR_HEIGHT ) );

	koord helptext_gr =  scrolly_helptext.get_groesse() - helptext.get_pos() - button_t::gui_scrollbar_size - koord(D_MARGIN_LEFT,D_MARGIN_TOP);

	helptext.set_groesse( helptext_gr );
	helptext.set_groesse( helptext.get_text_size() );
}
