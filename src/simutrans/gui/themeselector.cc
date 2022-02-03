/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>

#include "themeselector.h"
#include "simwin.h"
#include "../sys/simsys.h"
#include "../simevent.h"
#include "gui_theme.h"
#include "../utils/simstring.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/tabfile.h"


std::string themeselector_t::undo = "";

themeselector_t::themeselector_t() :
	savegame_frame_t( ".tab", false, NULL, false )
{
	// remove unnecessary buttons
	top_frame.remove_component( &input );
	delete_enabled = false;
	label_enabled  = false;

	set_name( translator::translate( "Theme selector" ) );
	fnlabel.set_text_pointer( translator::translate( "Select a theme for display" ) );
	if( undo.empty() ) {
		undo = env_t::default_theme;
		set_windowsize(get_min_windowsize());
	}
}

bool themeselector_t::check_file(const char *filename, const char *suffix)
{
	return savegame_frame_t::check_file(filename,suffix);
}



// A theme button was pressed
bool themeselector_t::item_action(const char *fullpath)
{
	gui_theme_t::themes_init(fullpath,true,true);

	event_t *ev = new event_t();
	ev->ev_class = EVENT_SYSTEM;
	ev->ev_code = SYSTEM_RELOAD_WINDOWS;
	queue_event( ev );

	return false;
}



// Ok button was pressed
bool themeselector_t::ok_action(const char *)
{
	undo = "";
	return true;
}



// Cancel button was pressed
bool themeselector_t::cancel_action(const char *)
{
	item_action(undo.c_str());
	undo = "";
	return true;
}


// returns the additional name of the file
const char *themeselector_t::get_info(const char *fn )
{
	const char *info = "";
	tabfile_t themesconf;

	if(  themesconf.open(fn)  ) {
		tabfileobj_t contents;

		// get trimmed theme name
		themesconf.read(contents);
		std::string name( contents.get( "name" ) );
		info = strdup( trim( name ).c_str() );

	}
	themesconf.close();
	return info;
}


void themeselector_t::fill_list()
{
	add_path( ((std::string)env_t::data_dir+"themes/").c_str() );
	if(  env_t::user_dir != env_t::data_dir  ) {
		// not signle user
		add_path( ((std::string)env_t::user_dir+"themes/").c_str() );
	}

	// do the search ...
	savegame_frame_t::fill_list();

	for(dir_entry_t const& i : entries) {

		if (i.type == LI_HEADER) {
			continue;
		}

		delete[] i.button->get_text(); // free up default allocation.
		i.button->set_typ(button_t::roundbox_state | button_t::flexible);
		i.button->set_text(i.label->get_text_pointer());
		i.button->pressed = !strcmp( env_t::default_theme.c_str(), i.info ); // mark current theme
		i.label->set_text_pointer( NULL ); // remove reference to prevent conflicts at delete[]

		// Get a new label buffer since the original is now owned by i.button.
		// i.label->set_text_pointer( strdup( get_filename(i.info).c_str() ) );

	}

	if(entries.get_count() <= this->num_sections+1) {
		// less than two themes exist => we coudl close now ...
	}

	// force new resize after we have rearranged the gui
	resize(scr_coord(0,0));
}


void themeselector_t::rdwr( loadsave_t *file )
{
	scr_size size = get_windowsize();
	size.rdwr( file );
	if(  file->is_loading()  ) {
		set_windowsize( size );
		resize( scr_coord(0,0) );
	}
}
