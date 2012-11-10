#include <string>
#include <stdio.h>

#include "../simdebug.h"
#include "pakselector.h"
#include "../dataobj/umgebung.h"
#include "../simsys.h"



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
	umgebung_t::objfilename = (std::string)filename + "/";
	umgebung_t::default_einstellungen.set_with_private_paks( false );
}


bool pakselector_t::del_action(const char *filename)
{
	// cannot delete set => use this for selection
	umgebung_t::objfilename = (std::string)filename + "/";
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
	else if(komp != &input) {
		return savegame_frame_t::action_triggered( komp, v );
	}
	return false;
}


void pakselector_t::zeichnen(koord p, koord gr)
{
	gui_frame_t::zeichnen( p, gr );

	display_multiline_text( p.x+10, p.y+gr.y-3*LINESPACE-4,
		"To avoid seeing this dialogue define a path by:\n"
		" - adding 'pak_file_path = pak/' to your simuconf.tab\n"
		" - using '-objects pakxyz/' on the command line", COL_BLACK );
}


bool pakselector_t::check_file( const char *filename, const char * )
{
	char buf[1024];
	sprintf( buf, "%s/ground.Outside.pak", filename );
	if (FILE* const f = fopen(buf, "r")) {
		fclose(f);
		return true;
	}
	return false;
}


pakselector_t::pakselector_t() : savegame_frame_t( NULL, umgebung_t::program_dir, true, true )
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
				umgebung_t::objfilename = (std::string)file_row->get_name() + "/";
			}
		}
	}
	else {
		bool disable = umgebung_t::program_dir==umgebung_t::user_dir;

		int y = 0;
		FOR(slist_tpl<entry>, const& i, entries) {
			char path[1024];
			sprintf(path,"%saddons/%s", umgebung_t::user_dir, i.button->get_text() );
			i.del->groesse.x += 150;
			i.del->set_text("Load with addons");
			i.button->set_pos(koord(150,0) + i.button->get_pos());
			if(  disable  ||  chdir( path )!=0  ) {
				// no addons for this
				i.del->set_visible( false );
				i.del->disable();
				if(entries.get_count()==1) {
					// only single entry and no addons => no need to question further ...
					umgebung_t::objfilename = (string)i.button->get_text() + "/";
				}
			}
			y += D_BUTTON_HEIGHT;
		}

		chdir( umgebung_t::program_dir );

		y += D_BUTTON_HEIGHT;
		button_frame.set_groesse( koord( get_fenstergroesse().x-1, y ) );
		set_fenstergroesse(koord(get_fenstergroesse().x, D_TITLEBAR_HEIGHT+30+y+3*LINESPACE+4+1));
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


void pakselector_t::set_fenstergroesse(koord groesse)
{
	if(groesse.y>display_get_height()-70) {
		// too large ...
		groesse.y = ((display_get_height()-D_TITLEBAR_HEIGHT-30-3*LINESPACE-4-1)/D_BUTTON_HEIGHT)*D_BUTTON_HEIGHT+D_TITLEBAR_HEIGHT+30+3*LINESPACE+4+1-70;
		// position adjustment will be done automatically ... nice!
	}
	gui_frame_t::set_fenstergroesse(groesse);
	groesse = get_fenstergroesse();

	sint16 y = 0;
	FOR(slist_tpl<entry>, const& i, entries) {
		// resize all but delete button
		if (i.button->is_visible()) {
			button_t* const button1 = i.del;
			button1->set_pos( koord( button1->get_pos().x, y ) );
			button_t* const button2 = i.button;
			button2->set_pos( koord( button2->get_pos().x, y ) );
			button2->set_groesse(koord( groesse.x/2-40, D_BUTTON_HEIGHT));
			i.label->set_pos(koord(groesse.x / 2 - 40 + 30, y + 2));
			y += D_BUTTON_HEIGHT;
		}
	}

	button_frame.set_groesse(koord(groesse.x,y));
	scrolly.set_groesse(koord(groesse.x,groesse.y-D_TITLEBAR_HEIGHT-30-3*LINESPACE-4-1));
}
