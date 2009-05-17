
#include <string.h>
#include <stdio.h>

#include "../simdebug.h"
#include "pakselector.h"
#include "../dataobj/umgebung.h"


/**
 * what to do after loading
 */
void pakselector_t::action(const char *filename)
{
	umgebung_t::objfilename = (cstring_t)filename + "/";
	umgebung_t::default_einstellungen.set_with_private_paks( false );
}


void pakselector_t::del_action(const char *filename)
{
	// cannot delete set => use this for selection
	umgebung_t::objfilename = (cstring_t)filename + "/";
	umgebung_t::default_einstellungen.set_with_private_paks( true );
}

const char *pakselector_t::get_info(const char *)
{
	return "";
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool pakselector_t::action_triggered( gui_action_creator_t *komp,value_t v)
{
	if(komp == &savebutton) {
		savebutton.pressed ^= 1;
		return true;
	}
	else {
		return savegame_frame_t::action_triggered( komp, v );
	}
}



void pakselector_t::zeichnen(koord p, koord gr)
{
	gui_frame_t::zeichnen( p, gr );

	display_multiline_text( p.x+10, p.y+10,
		"You have multiple pak sets to choose from.\n", COL_BLACK );

	display_multiline_text( p.x+6, p.y+gr.y-38,
		"To avoid seeing this dialogue define a path by:\n"
		" - adding 'pak_file_path = pak/' to your simuconf.tab\n"
		" - using '-objects pakxyz/' on the command line", COL_BLACK );
}


bool pakselector_t::check_file( const char *filename, const char * )
{
	char buf[1024];
	sprintf( buf, "%s/ground.Outside.pak", filename );
	FILE *f=fopen( buf, "r" );
	if(f) {
		fclose(f);
	}
	// found only one?
	if(f!=NULL) {
		if(entries.get_count()==0) {
			umgebung_t::objfilename = (cstring_t)filename + "/";
		}
		else if(  !umgebung_t::objfilename.empty()  ) {
			umgebung_t::objfilename = "";
		}
	}
	return f!=NULL;
}


pakselector_t::pakselector_t() : savegame_frame_t( NULL, umgebung_t::program_dir )
{
	// remove unneccessary buttons
	remove_komponente( &input );
	remove_komponente( &savebutton );
	remove_komponente( &cancelbutton );
	remove_komponente( &divider1 );

	fnlabel.set_text( "Choose one graphics set for playing:" );
}


void pakselector_t::fill_list()
{
	// do the search ...
	savegame_frame_t::fill_list();
	for(  slist_tpl<entry>::iterator iter = entries.begin(), end = entries.end();  iter != end;  ++iter  ) {
		iter->del->groesse.x += 150;
		iter->del->set_text( "Load with addons" );
		iter->button->set_pos( koord(150,0)+iter->button->get_pos() );
	}
}
