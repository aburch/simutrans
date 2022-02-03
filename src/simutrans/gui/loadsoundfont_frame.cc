/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"

#include <string.h>

#include "loadsoundfont_frame.h"

#include "../pathes.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../music/music.h"
#include "../simsound.h"

#include "gui_theme.h"

#include "../utils/simstring.h"

// static, since we keep them over reloading
std::string loadsoundfont_frame_t::old_soundfontname;

/**
 * Action that's started with a button click
 */
bool loadsoundfont_frame_t::item_action(const char *filename)
{
	if(  dr_load_sf( filename )  &&  midi_get_mute()  ) {
		midi_set_mute( false );
		midi_play( env_t::shuffle_midi ? -1 : 0 );
	}
	return false;
}


bool loadsoundfont_frame_t::ok_action(const char *filename)
{
	item_action( filename );
	old_soundfontname.clear();
	return true;
}


bool loadsoundfont_frame_t::cancel_action(const char *)
{
	dr_load_sf( old_soundfontname.c_str() );
	old_soundfontname.clear();
	return true;
}


loadsoundfont_frame_t::loadsoundfont_frame_t() : savegame_frame_t(NULL,false,NULL,false)
{
	set_name( translator::translate("Select soundfont") );
	fnlabel.set_text("Soundfonts are located in the music directory.");
	top_frame.remove_component( &input );
	delete_enabled = false;
}


const char *loadsoundfont_frame_t::get_info(const char *sfname)
{
	return sfname;
}


bool loadsoundfont_frame_t::compare_items ( const dir_entry_t & entry, const char *info, const char *)
{
	return (STRICMP(entry.info, info) > 0);
}


/**
 * CHECK FILE
 * Check if a file name qualifies to be added to the item list.
 */
bool loadsoundfont_frame_t::check_file(const char *filename, const char *)
{
	FILE *test = fopen( filename, "r" );
	if(  test == NULL  ) {
		return false;
	}
	fclose( test );

	// just match extension for building soundfonts
	const char *start_extension = strrchr(filename, '.' );
	if(  start_extension  &&  ( !STRICMP( start_extension, ".sf2" ) || !STRICMP( start_extension, ".sf3" ) )  ) {
		return true;
	}
	return false;
}


// parses the directory
void loadsoundfont_frame_t::fill_list()
{
	add_path( ((std::string)env_t::data_dir + "music/").c_str() );
	add_path( "/usr/share/soundfonts/" );
	add_path( "/usr/share/sounds/sf2/" );

	if(  old_soundfontname.empty()  ) {
		old_soundfontname = env_t::soundfont_filename;
	}

	// do the search ...
	savegame_frame_t::fill_list();

	// mark current fonts
	for(dir_entry_t const& i : entries ) {
		if(  i.type == LI_HEADER  ) {
			continue;
		}
		i.button->set_typ( button_t::roundbox_state | button_t::flexible );
	}

	// force new resize after we have rearranged the gui
	resize( scr_coord(0,0) );
}


void loadsoundfont_frame_t::draw(scr_coord pos, scr_size size)
{
	// mark current fonts
	for(dir_entry_t const& i : entries)  {
		if(  i.type == LI_HEADER  ) {
			continue;
		}
		i.button->pressed = strstr( env_t::soundfont_filename.c_str(), i.info );
	}
	savegame_frame_t::draw( pos, size );
}


void loadsoundfont_frame_t::rdwr( loadsave_t *file )
{
	scr_size size = get_windowsize();
	size.rdwr( file );
	if(  file->is_loading()  ) {
		set_windowsize( size );
		resize( scr_coord(0,0) );
	}
}


bool loadsoundfont_frame_t::action_triggered(gui_action_creator_t *component, value_t v)
{
	return savegame_frame_t::action_triggered( component, v );
}
