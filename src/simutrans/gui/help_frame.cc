/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../simmem.h"
#include "simwin.h"
#include "../tool/simmenu.h"
#include "../sys/simsys.h"
#include "../world/simworld.h"
#include "../simticker.h" // TICKER_HEIGHT

#include "../utils/cbuffer.h"
#include "../utils/simstring.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../utils/unicode.h"
#include "../player/simplay.h"
#include "tool_selector.h"

#include "help_frame.h"

#define DIALOG_MIN_WIDTH (100)


help_frame_t::help_frame_t() :
	gui_frame_t( translator::translate("Help") )
{
	// info windows do not show general text
	generaltext.set_visible( false );

	set_text("<title>Unnamed</title><p>No text set</p>");

	set_table_layout(1,0);

	helptext.add_listener(this);
	add_component(&helptext);

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
	set_windowsize( scr_size( D_MARGIN_LEFT + (D_SCROLLBAR_WIDTH<<1) + DIALOG_MIN_WIDTH + D_MARGIN_RIGHT, D_TITLEBAR_HEIGHT + D_MARGIN_TOP + (D_SCROLLBAR_HEIGHT<<1) + D_MARGIN_BOTTOM) );
}


help_frame_t::help_frame_t(char const* const filename) :
	gui_frame_t( translator::translate("Help") )
{
	set_table_layout(2,0);
	set_alignment(ALIGN_TOP | ALIGN_LEFT);
	set_margin(scr_size(0,D_MARGIN_TOP), scr_size(0,0));

	add_component(&generaltext);
	generaltext.add_listener(this);

	add_component(&helptext);
	helptext.add_listener(this);

	// we now exclusive build out index on the fly
	slist_tpl<plainstring> already_there;
	cbuffer_t index_txt;
	cbuffer_t introduction;
	cbuffer_t usage;
	cbuffer_t toolbars;
	cbuffer_t game_start;
	cbuffer_t how_to_play;
	cbuffer_t others;

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
	for(toolbar_t * iter : tool_t::toolbar_tool ) {
		if(  strstart(iter->get_tool_selector()->get_help_filename(),"list.txt" )  ) {
			continue;
		}
		add_helpfile( toolbars, translator::translate(iter->get_tool_selector()->get_internal_name()), iter->get_tool_selector()->get_help_filename(), false, 0 );
		if(  strstart(iter->get_tool_selector()->get_help_filename(),"railtools.txt" )  ) {
			add_helpfile( toolbars, NULL, "bridges.txt", true, 1 );
			add_helpfile( toolbars, NULL, "signals.txt", true, 1 );
			add_helpfile( toolbars, "set signal spacing", "signal_spacing.txt", false, 1 );
		}
		if(  strstart(iter->get_tool_selector()->get_help_filename(),"roadtools.txt" )  ) {
			add_helpfile( toolbars, NULL, "privatesign_info.txt", false, 1 );
			add_helpfile( toolbars, NULL, "trafficlight_info.txt", false, 1 );
		}
		if(  !special  &&  (  strstart(iter->get_tool_selector()->get_help_filename(),"special.txt" )
							||  strstart(iter->get_tool_selector()->get_help_filename(),"edittools.txt" )  )
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
//	add_helpfile( how_to_play, "Scenario", "scenario.txt", false, 1 );
	add_helpfile( how_to_play, "Enter Password", "password.txt", false, 1 );

	add_helpfile( others, "Einstellungen aendern", "options.txt", false, 0 );
	add_helpfile( others, "Display settings", "display.txt", false, 0 );
	add_helpfile( others, "Mailbox", "mailbox.txt", false, 0 );
	add_helpfile( others, "Sound settings", "sound.txt", false, 0 );
	add_helpfile( others, "Sprachen", "language.txt", false, 0 );

	index_txt.printf( translator::translate("<h1>Index</h1><p>*: only english</p><p>General</p>%s<p>Usage</p>%s<p>Tools</p>%s<p>Start</p>%s<p>How to play</p>%s<p>Others:</p>%s"),
		(const char *)introduction, (const char *)usage, (const char *)toolbars, (const char *)game_start, (const char *)how_to_play, (const char *)others );
	generaltext.set_text( index_txt );

	set_helpfile( filename, true );

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
	set_windowsize(scr_size(200, D_DEFAULT_HEIGHT));
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
static char *load_text(char const* const filename )
{
	std::string file_prefix= std::string("text") + PATH_SEPARATOR;
	std::string fullname = file_prefix + translator::get_lang()->iso + PATH_SEPARATOR + filename;
	dr_chdir(env_t::base_dir);

	FILE* file = dr_fopen(fullname.c_str(), "rb");
	if (!file) {
		//Check for the 'base' language(ie en from en_gb)
		file = dr_fopen((file_prefix + translator::get_lang()->iso_base + PATH_SEPARATOR + filename).c_str(), "rb");
	}
	if (!file) {
		// check fallback english
		file = dr_fopen((file_prefix + PATH_SEPARATOR + "en" + PATH_SEPARATOR + filename).c_str(), "rb");
	}
	// go back to load/save dir
	dr_chdir( env_t::user_dir );

	if(file) {
		fseek(file,0,SEEK_END);
		long len = ftell(file);
		char *buf = NULL;
		if(  len>0  ) {
			buf = MALLOCN(char, len + 1);
			fseek( file, 0, SEEK_SET );
			len = fread(  buf, 1, len, file);
			buf[len] = '\0';
			fclose( file );
		}
		// now we may need to translate the text ...
		if(  len>0  ) {
			bool is_latin = strchr( buf, 0xF6 )!=NULL; // "o-umlaut, is forbidden for unicode
			if(  !is_latin  &&  translator::get_lang()->is_latin2_based  ) {
				is_latin |= strchr( buf, 0xF8 )!=NULL; // "o-umlaut, is forbidden for unicode
			}
			if(  !is_latin  &&  translator::get_lang()->is_latin2_based  ) {
				is_latin |= strchr( buf, 0xFE )!=NULL; // "o-umlaut, is forbidden for unicode
			}
			if(  !is_latin  &&  translator::get_lang()->is_latin2_based  ) {
				is_latin |= strchr( buf, 0xFA )!=NULL; // "o-umlaut, is forbidden for unicode
			}
			if(  !is_latin  &&  translator::get_lang()->is_latin2_based  ) {
				is_latin |= strchr( buf, 0xF3 )!=NULL; // "o-umlaut, is forbidden for unicode
			}
			if(  !is_latin  &&  translator::get_lang()->is_latin2_based  ) {
				is_latin |= strchr( buf, 0xF1 )!=NULL; // "o-umlaut, is forbidden for unicode
			}
			if(  is_latin  ) {
				// we need to translate charwise ...
				utf8 *buf2 = MALLOCN(utf8, len*2 + 1); //assume the worst
				utf8 *src = (utf8 *)buf, *dest = buf2;
				if(  translator::get_lang()->is_latin2_based  ) {
					do {
						dest += utf16_to_utf8( latin2_to_unicode(*src), dest );
					} while(  *src++  );
					*dest = 0;
				}
				else {
					do {
						dest += utf16_to_utf8( *src, dest );
					} while(  *src++  );
					*dest = 0;
				}
				free( buf );
				buf = (char *)buf2;
			}
		}
		return buf;
	}

	return NULL;
}


void help_frame_t::set_text(const char * buf, bool resize_frame )
{
	helptext.set_text(buf);

	if(  resize_frame  ) {

		// try to get the following sizes
		// y<400 or, if not possible, x<620
		helptext.set_size(scr_size(220, 0));
		int last_y = 0;
		scr_coord curr = helptext.get_preferred_size();
		for(  int i = 0;  i<10  &&  curr.y>400  &&  curr.y!=last_y;  i++  ) {
			helptext.set_size(scr_size(260+i*40, 0));
			last_y = curr.y;
			curr = helptext.get_preferred_size();
		}

		helptext.set_size(helptext.get_preferred_size());

		if(  generaltext.is_visible()  ) {
			generaltext.set_pos( scr_coord(D_MARGIN_LEFT, D_MARGIN_TOP) );
			generaltext.set_size( scr_size( min(180,display_get_width()/3), 0 ) );
			int generalwidth = min( display_get_width()/3, generaltext.get_preferred_size().w );
			generaltext.set_size( scr_size( generalwidth, helptext.get_size().h ) );
			generaltext.set_size( generaltext.get_preferred_size() );
		}
		else {
			generaltext.set_size( scr_size(D_MARGIN_LEFT, D_MARGIN_TOP) );
		}

		// calculate sizes (might not a help but info window, which do not have general text)
		scr_coord_val size_x = helptext.get_size().w + D_MARGIN_LEFT + D_MARGIN_RIGHT + D_SCROLLBAR_WIDTH;
		scr_coord_val size_y = helptext.get_size().h + D_TITLEBAR_HEIGHT + D_MARGIN_TOP  + D_MARGIN_BOTTOM + D_SCROLLBAR_HEIGHT;
		if(  generaltext.is_visible()  ) {
			size_x += generaltext.get_size().w + D_SCROLLBAR_WIDTH + D_H_SPACE;
		}
		// set window size
		if(  size_x > display_get_width()-32  ) {
			size_x = display_get_width()-32;
		}

		const scr_coord_val h = display_get_height() - D_TITLEBAR_HEIGHT - win_get_statusbar_height() - TICKER_HEIGHT;
		if(  size_y>h) {
			size_y = h;
		}
		set_windowsize( scr_size( size_x, size_y ) );
	}

	// generate title
	title = "";
	if(  generaltext.is_visible()  ) {
		title = translator::translate( "Help" );
		title += " - ";
	}
	title += helptext.get_title();
	set_name( title.c_str() );

	resize( scr_coord(0,0) );
	set_dirty();
}


// show the help to one topic
void help_frame_t::set_helpfile(const char *filename, bool resize_frame )
{
	// the key help texts are built automagically
	if (strcmp(filename, "keys.txt") == 0) {
		cbuffer_t buf;
		buf.append( translator::translate( "<title>Keyboard Help</title>\n<h1><strong>Keyboard Help</strong></h1><p>\n" ) );
		player_t *player = welt->get_active_player();
		const char *trad_str = translator::translate( "<em>%s</em> - %s<br>\n" );
		for(tool_t* const i : tool_t::char_to_tool) {
			cbuffer_t c;
			char str[16];
			if(  i->command_flags & SIM_MOD_CTRL  ) {
				c.append( translator::translate( "[CTRL]" ) );
				c.append( " + " );
			}
			if(  i->command_flags & SIM_MOD_SHIFT  ) {
				c.append( translator::translate( "[SHIFT]" ) );
				c.append( " + " );
			}
			switch (uint16 const key = i->command_key) {
				case '<': c.append( "&lt;" ); break;
				case '>': c.append( "&gt;" ); break;
				case 27:  c.append( translator::translate( "[ESCAPE]" ) ); break;
				case 127: c.append( translator::translate( "[DELETE]" ) ); break;
				case SIM_KEY_HOME: c.append( translator::translate( "[HOME]" ) ); break;
				case SIM_KEY_END:  c.append( translator::translate( "[END]" ) ); break;
				case SIM_KEY_SCROLLLOCK: c.append( translator::translate( "[SCROLLLOCK]" ) ); break;
				default:
					if (key <= 26) {
						c.printf("%c", '@' + key);
					}
					else if (key < 256) {
						c.printf("%c", key);
					}
					else if (key < SIM_KEY_F15) {
						c.printf("F%i", key - SIM_KEY_F1 + 1);
					}
					else {
						// try unicode
						str[utf16_to_utf8(key, (utf8*)str)] = '\0';
						c.append( str );
					}
					break;
			}
			buf.printf(trad_str, c.get_str(), i->get_tooltip(player));
		}
		set_text( buf, resize_frame );
	}
	else if(  strcmp( filename, "general.txt" )!=0  ) {
		// and the actual help text (if not identical)
		if(  char *buf = load_text( filename )  ) {
			set_text( buf, resize_frame );
			free(buf);
		}
		else {{
			cbuffer_t buf;
			buf.printf("<title>%s</title>%s", translator::translate("Error"), translator::translate("Help text not found"));
			set_text(buf, resize_frame );
		}}
	}
	else {
		// default text when opening general help
		if(  char *buf = load_text( "about.txt" )  ) {
			set_text( buf, resize_frame );
			free(buf);
		}
		else if(  char *buf = load_text( "simutrans.txt" )  ) {
			set_text( buf, resize_frame );
			free(buf);
		}
		else {
			set_text( "", resize_frame );
		}
	}
}


FILE *help_frame_t::has_helpfile( char const* const filename, int &mode )
{
	mode = native;
	std::string file_prefix = std::string("text") + PATH_SEPARATOR;
	std::string fullname = file_prefix + translator::get_lang()->iso + PATH_SEPARATOR + filename;
	dr_chdir(env_t::base_dir);

	FILE* file = dr_fopen(fullname.c_str(), "rb");
	if(  !file  &&  strcmp(translator::get_lang()->iso,translator::get_lang()->iso_base)  ) {
		//Check for the 'base' language(ie en from en_gb)
		file = dr_fopen(  (file_prefix + translator::get_lang()->iso_base + PATH_SEPARATOR + filename).c_str(), "rb"  );
	}
	if(  !file  ) {
		// check fallback english
		file = dr_fopen((file_prefix + "en/" + filename).c_str(), "rb");
		mode = english;
	}
	// go back to load/save dir
	dr_chdir( env_t::user_dir );
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
	bool convert_to_utf = false;
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
				// convert to UTF if needed
				if(  !convert_to_utf  &&  *c >= 0x80  ) {
					size_t len;
					utf8_decoder_t::decode(c, len);
					convert_to_utf = (len <= 1);
				}
				if (convert_to_utf) {
					if (translator::get_lang()->is_latin2_based) {
						dest += utf16_to_utf8(latin2_to_unicode(*c), dest);
					}
					else {
						dest += utf16_to_utf8(*c, dest);
					}
					c++;
				}
				else {
					size_t len;
					utf32 cc = utf8_decoder_t::decode(c, len);
					dest += utf16_to_utf8(cc, dest);
					c += len;
				}
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

	std::string filetitle; // just in case as temporary storage ...
	if(  titlename == NULL  &&  file  ) {
		// get the title from the helpfile
		char htmlline[1024];
		if (fread( htmlline, lengthof(htmlline)-1, 1, file ) == 1) {
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
			titlename = filename;
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
 * @param extra the name of the help file
 */
bool help_frame_t::action_triggered( gui_action_creator_t *, value_t extra)
{
	top_win( this );
	set_helpfile( (const char *)(extra.p), false );
	return true;
}


void help_frame_t::resize(const scr_coord delta)
{
	gui_frame_t::resize(delta);

	scr_coord_val generalwidth = 0;
	if(  generaltext.is_visible()  ) {
		// do not use more than 1/3 for the general infomations
		generalwidth = min( get_windowsize().w/3, generaltext.get_preferred_size().w ) + D_SCROLLBAR_WIDTH;
		generaltext.set_size( scr_size( generalwidth, get_client_windowsize().h  - D_MARGIN_BOTTOM) );

		generalwidth += D_H_SPACE;
		helptext.set_pos( generaltext.get_pos() + scr_size( generalwidth, 0 ) );
	}

	helptext.set_size( get_client_windowsize() - scr_size( generalwidth, 0) -scr_size(0,D_MARGIN_BOTTOM) );
}
