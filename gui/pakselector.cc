
#include <string.h>
#include <stdio.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#else
#include <io.h>
#include <direct.h>
#endif
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include "../simdebug.h"
#include "pakselector.h"
#include "../dataobj/umgebung.h"


void pakselector_t::init(const char * /*suffix*/, const char * /*path*/)
{
	file_table.set_owns_columns(false);
	addon_column.set_width(150);
	addon_column.set_text("With user addons");
	file_table.add_column(&addon_column);
	action_column.set_width(150);
	file_table.add_column(&action_column);
	set_min_windowsize(get_fenstergroesse());
	set_resizemode(diagonal_resize);

	// remove unneccessary buttons
	remove_komponente( &input );
	remove_komponente( &savebutton );
	remove_komponente( &cancelbutton );
	remove_komponente( &divider1 );

	fnlabel.set_text( "Choose one graphics set for playing:" );
}


/**
 * what to do after loading
 */
void pakselector_t::action(const char *filename)
{
	umgebung_t::objfilename = (cstring_t)filename + "/";
	umgebung_t::default_einstellungen.set_with_private_paks( false );
}


bool pakselector_t::del_action(const char *filename)
{
	// cannot delete set => use this for selection
	umgebung_t::objfilename = (cstring_t)filename + "/";
	umgebung_t::default_einstellungen.set_with_private_paks( true );
	return true;
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
		return true;
	}
	return false;
}


pakselector_t::pakselector_t() : savegame_frame_t( NULL, umgebung_t::program_dir, true )
{
	//at_least_one_add = false;
	init(NULL, umgebung_t::program_dir);
}


void pakselector_t::fill_list()
{
	// do the search ...
	savegame_frame_t::fill_list();
	if (use_table) {
		if (file_table.get_size().get_y() == 1) {
			gui_file_table_row_t *file_row = (gui_file_table_row_t *) file_table.get_row(0);
			if (!file_row->get_delete_enabled()) {
				// only single entry and no addons => no need to question further ...
				umgebung_t::objfilename = (cstring_t)file_row->get_name() + "/";
			}
		}
	}
	else {
		bool disable = umgebung_t::program_dir==umgebung_t::user_dir;

		for(  slist_tpl<entry>::iterator iter = entries.begin(), end = entries.end();  iter != end;  ++iter  ) {
			char path[1024];
			sprintf(path,"%s%s", umgebung_t::user_dir, iter->button->get_text() );
			iter->del->groesse.x += 150;
			iter->del->set_text( "Load with addons" );
			iter->button->set_pos( koord(150,0)+iter->button->get_pos() );
			if(  disable  ||  chdir( path )!=0  ) {
				// no addons for this
				iter->del->set_visible( false );
				iter->del->disable();
				if(entries.get_count()==1) {
					// only single entry and no addons => no need to question further ...
					umgebung_t::objfilename = (cstring_t)iter->button->get_text() + "/";
				}
			}
		}
		chdir( umgebung_t::program_dir );
	}
}


void pakselector_t::add_file(const char *filename, const bool not_cutting_suffix)
{
	char pathname[1024];
	const char *path = get_path();
	sprintf( pathname, "%s%s", path ? path : "", filename );

	char buttontext[1024];
	strcpy( buttontext, filename );
	if ( !not_cutting_suffix ) {
		buttontext[strlen(buttontext)-4] = '\0';
	}
	bool has_addon_dir = umgebung_t::program_dir!=umgebung_t::user_dir;
	if (has_addon_dir) {
		char path[1024];
		sprintf(path,"%s%s", umgebung_t::user_dir, filename);
		has_addon_dir = chdir( path ) == 0;
		chdir( umgebung_t::program_dir );
	}
	file_table.add_row( new gui_file_table_row_t( pathname, buttontext, has_addon_dir ));
}
