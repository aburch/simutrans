/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"

#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include "loadfont_frame.h"

#include "../sys/simsys.h"
#include "../world/simworld.h"
#include "../simversion.h"
#include "../pathes.h"
#include "../utils/unicode.h"
#include "../utils/searchfolder.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "gui_theme.h"

#include "../utils/simstring.h"

// static, since we keep them over reloading
std::string loadfont_frame_t::old_fontname;
uint8 loadfont_frame_t::old_linespace;

bool loadfont_frame_t::use_unicode=true;

bool loadfont_frame_t::is_resizable_font (const char *fontname) {
	const char *start_extension = strrchr(fontname, '.' );
	return start_extension  &&  STRICMP( start_extension, ".fnt" ) && STRICMP( start_extension, ".bdf" );
}

/**
 * Action that's started with a button click
 */
bool loadfont_frame_t::item_action(const char *filename)
{
	fontsize.enable( is_resizable_font(filename) );
	win_load_font(filename, env_t::fontsize);
	return false;
}



bool loadfont_frame_t::ok_action(const char *filename)
{
	item_action(filename);
	old_fontname.clear();
	return true;
}



bool loadfont_frame_t::cancel_action(const char *)
{
	win_load_font(old_fontname.c_str(), old_linespace);
	old_fontname.clear();
	return true;
}

loadfont_frame_t::loadfont_frame_t() : savegame_frame_t(NULL,false,NULL,false)
{
	set_name(translator::translate("Select display font"));
	use_unicode = true; // only try matching fonts

	top_frame.remove_component(&input);
	fontsize.init( env_t::fontsize, 6, 40, gui_numberinput_t::AUTOLINEAR, false );
	fontsize.add_listener(this);
	fontsize.enable( is_resizable_font(env_t::fontname.c_str()) );

#ifdef USE_FREETYPE
	fnlabel.set_text( "font size" );
	top_frame.add_component(&fontsize);
#else
	top_frame.remove_component(&fnlabel);
#endif

	unicode_only.init( button_t::square_automatic, "Only full Unicode fonts");
	unicode_only.pressed = use_unicode;
	unicode_only.add_listener(this);
	top_frame.add_component(&unicode_only, 2);
}


const char *loadfont_frame_t::get_info(const char *fname)
{
	return fname;
}


bool loadfont_frame_t::compare_items ( const dir_entry_t & entry, const char *info, const char *)
{
	return (STRICMP(entry.info, info) > 0);
}


/**
 * CHECK FILE
 * Check if a file name qualifies to be added to the item list.
 */
bool loadfont_frame_t::check_file(const char *filename, const char *)
{
	FILE *test = fopen( filename, "r" );
	if(  test == NULL  ) {
		return false;
	}
	fclose(test);

	// just match textension for buildin fonts
	const char *start_extension = strrchr(filename, '.' );

	// we only show matching fonts for this language
	const utf8 *new_world = (const utf8 *)translator::translate("Beenden");
	size_t len;
	utf16 testfor_this_character = utf8_decoder_t::decode(new_world, len);

	// no support for windows fon files, so we skip them to speed things up
	if(  start_extension  &&  !STRICMP( start_extension, ".fon" )  ) {
		return false;
	}
#ifdef USE_FREETYPE
	if(  ft_library  ) {
		// if we can open this font, it is probably ok ...
		FT_Face face;
		if(  !FT_New_Face( ft_library, filename, 0, &face )  ) {
			// can load (no error returned)
			bool ok = false;
			if(  FT_Get_Char_Index( face, '}' )!=0  &&  (STRICMP(face->style_name,"Regular")==0  ||  STRICMP(face->style_name,"Bold")==0) ) {
				// ok, we have at least charecter 126, and it is a regular font, so it is probably a valid font)
				ok = !use_unicode;
				if(  FT_Get_Char_Index(face, testfor_this_character)!=0  ) {
					// the char NAMA does exist
					ok = true; // in pricipal we must also check if it can be rendered ...
				}
			}
			FT_Done_Face(face);
			return ok;
		}
		// next check for extension, might be still a valid font
	}
#endif
	return false;
}


// parses the directory, using freetype lib, in installed
void loadfont_frame_t::fill_list()
{
	add_path( ((std::string)env_t::base_dir+"font/").c_str() );
#ifdef USE_FREETYPE
	// ok, we can handle TTF fonts
	ft_library = NULL;
	if(  FT_Init_FreeType(&ft_library) != FT_Err_Ok  ) {
		ft_library = NULL;
	}
	else {
		const char *addpath;
		searchfolder_t subfolders;
		for(  int i=0;  ( addpath = dr_query_fontpath(i) );  i++  ) {
			add_path( addpath );
			subfolders.search(addpath, "", searchfolder_t::SF_ONLYDIRS | searchfolder_t::SF_PREPEND_PATH, 4);
			for( const char * folder : subfolders ){
				add_path( ((std::string) folder + PATH_SEPARATOR).c_str() );
			}
		}
	}
#endif

	if(  old_fontname.empty()  ) {
		old_fontname = env_t::fontname;
		old_linespace = env_t::fontsize;
	}

	// do the search ...
	savegame_frame_t::fill_list();

	// mark current fonts
	for(dir_entry_t const& i : entries) {
		if (i.type == LI_HEADER) {
			continue;
		}
		i.button->set_typ(button_t::roundbox_state | button_t::flexible);
#ifndef USE_FREETYPE
	}
#else
		// Use internal name instead the cutted file name
		if(  ft_library  ) {
			FT_Face face;

			if(  FT_New_Face( ft_library, i.info, 0, &face )==FT_Err_Ok  ) {
				if (  face->family_name  ) {
					const size_t len = strlen(face->family_name) + 1 + (face->style_name ? strlen(face->style_name) + 1 : 0);
					char *name = new char[len];

					strcpy( name, face->family_name );

					if (face->style_name) {
						strcat( name, " ");
						strcat( name, face->style_name );
					}

					delete [] const_cast<char*>(i.button->get_text());
					i.button->set_text(name);
					i.button->pressed = strstr( env_t::fontname.c_str(), i.info ) != NULL;
				}

				FT_Done_Face( face );
			}
		}
	}

	FT_Done_FreeType( ft_library );
	ft_library = NULL;
#endif

	// force new resize after we have rearranged the gui
	resize(scr_coord(0,0));
}



void loadfont_frame_t::draw(scr_coord pos, scr_size size)
{
	// mark current fonts
	for(dir_entry_t const& i : entries) {
		if (i.type == LI_HEADER) {
			continue;
		}
		i.button->pressed = strstr( env_t::fontname.c_str(), i.info );
	}
	savegame_frame_t::draw(pos, size);
}



void loadfont_frame_t::rdwr( loadsave_t *file )
{
	file->rdwr_bool( unicode_only.pressed );
	scr_size size = get_windowsize();
	size.rdwr( file );
	if(  file->is_loading()  ) {
		set_windowsize( size );
		resize( scr_coord(0,0) );
	}
}


bool loadfont_frame_t::action_triggered(gui_action_creator_t *component, value_t v)
{
	if(  &unicode_only==component  ) {
		// send event, this will reload window
		event_t *ev = new event_t();
		ev->ev_class = EVENT_SYSTEM;
		ev->ev_code = SYSTEM_RELOAD_WINDOWS;
		queue_event( ev );

		use_unicode = unicode_only.pressed;
		return false;
	}

	if(  &fontsize==component  ) {
		win_load_font(env_t::fontname.c_str(), fontsize.get_value());
		fontsize.set_limits(6, 40);
		return false;
	}

	return savegame_frame_t::action_triggered(component,v);
}
