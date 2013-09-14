/*
 * selection of paks at the start time
 */

#include <string>

#include "themeselector.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/tabfile.h"
#include "../simsys.h"
#include "../simevent.h"
#include "simwin.h"

#define L_ADDON_WIDTH (150)


themeselector_t::themeselector_t() :
	savegame_frame_t( ".tab" )//, false, NULL, false )
{
	// remove unnecessary buttons
	remove_komponente( &input );
	remove_komponente( &savebutton );
	remove_komponente( &cancelbutton );
	remove_komponente( &fnlabel );

	set_name( translator::translate( "Select a theme for display" ) );
}


void themeselector_t::action(const char *fullpath)
{
	gui_theme_t::themes_init(fullpath);
}


bool themeselector_t::del_action(const char *fullpath)
{
	gui_theme_t::themes_init(fullpath);
	event_t *ev = new event_t();
	ev->ev_class = EVENT_SYSTEM;
	ev->ev_code = SYSTEM_RELOAD_WINDOWS;
	queue_event( ev );
	return false;
}


// returns the additional name of the file
const char *themeselector_t::get_info(const char *fn )
{
	const char *info = "";
	tabfile_t themesconf;

	if(  themesconf.open(fn)  ) {
		// show the name
		tabfileobj_t contents;
		themesconf.read(contents);
		info = strdup( contents.get( "name" ) );
	}
	themesconf.close();
	return info;
}


void themeselector_t::fill_list()
{
	KOORD_VAL y = 0;
	KOORD_VAL width = 0;

	add_path( ((std::string)umgebung_t::program_dir+"themes/").c_str() );
	if(  strcmp( umgebung_t::program_dir, umgebung_t::user_dir ) != 0  ) {
		add_path( ((std::string)umgebung_t::user_dir+"themes/").c_str() );
	}

	// do the search ...
	savegame_frame_t::fill_list();

	FOR(slist_tpl<dir_entry_t>, const& i, entries) {

		if (i.type == LI_HEADER) {
			continue;
		}

		i.del->set_typ( button_t::roundbox_state );
		i.del->set_visible(true);	// show delete button
		i.del->set_text( i.button->get_text() );
		i.del->set_tooltip( NULL );
		i.del->set_groesse( i.button->get_pos()-i.del->get_pos()+i.button->get_groesse() ); // make it as wide as original button and delete together
		i.del->pressed = !strcmp( gui_theme_t::get_current_theme(), i.label->get_text_pointer() ); // mark current theme
		i.button->set_visible( false );	// hide normal button (to be able to instantaniously apply themes)

		y += D_BUTTON_HEIGHT;
	}

	if(entries.get_count() <= this->num_sections+1) {
		// less than two themes exist => we coudl close now ...
	}
}


void themeselector_t::rdwr( loadsave_t *file )
{
	koord gr = get_fenstergroesse();
	gr.rdwr( file );
	if(  file->is_loading()  ) {
		set_fenstergroesse( gr );
		resize( koord(0,0) );
	}
}
