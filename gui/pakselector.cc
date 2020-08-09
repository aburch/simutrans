/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>

#include "pakselector.h"
#include "../dataobj/environment.h"
#include "../utils/cbuffer_t.h"
#include "../sys/simsys.h"



void pakselector_t::init(const char * /*suffix*/, const char * /*path*/)
{
	file_table.set_owns_columns(false);
	addon_column.set_width(150);
	addon_column.set_text("With user addons");
	file_table.add_column(&addon_column);
	action_column.set_width(150);
	file_table.add_column(&action_column);
	set_min_windowsize(get_windowsize());
	set_resizemode(diagonal_resize);

	// remove unnecessary buttons
	remove_component( &input );
	remove_component( &savebutton );
	remove_component( &cancelbutton );
	remove_component( &divider1 );

	fnlabel.set_text( "Choose one graphics set for playing:" );
}


/**
 * what to do after loading
 */
bool pakselector_t::item_action(const char *fullpath)
{
	env_t::objfilename = this->get_filename(fullpath)+"/";
	env_t::default_settings.set_with_private_paks( false );
	return true;
}


bool pakselector_t::del_action(const char *fullpath)
{
	// cannot delete set => use this for selection
	env_t::objfilename = this->get_filename(fullpath)+"/";
	env_t::default_settings.set_with_private_paks( true );
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
bool pakselector_t::action_triggered(gui_action_creator_t *comp, value_t v)
{
	if(comp == &savebutton) {
		savebutton.pressed ^= 1;
		return true;
	}
	else if(comp != &input) {
		return savegame_frame_t::action_triggered(comp, v );
	}
	return false;
}


void pakselector_t::draw(scr_coord p, scr_size gr)
{
	gui_frame_t::draw(p, gr);

	display_multiline_text( p.x+10, p.y+gr.h-3*LINESPACE-4,
		"To avoid seeing this dialogue define a path by:\n"
		" - adding 'pak_file_path = pak/' to your simuconf.tab\n"
		" - using '-objects pakxyz/' on the command line", SYSCOL_TEXT );
}


bool pakselector_t::check_file(const char *filename, const char *)
{
	cbuffer_t buf;
	buf.printf("%s/ground.Outside.pak", filename);
	if (FILE* const f = fopen(buf, "r")) {
		fclose(f);
		return true;
	}
	return false;
}


pakselector_t::pakselector_t() : savegame_frame_t( NULL, false, env_t::program_dir, true, true )
{
	init(NULL, env_t::program_dir);
}


void pakselector_t::fill_list()
{
	static const scr_coord addon_shift(150, 0);
	// do the search ...
	savegame_frame_t::fill_list();
}


// Check if there's only one option, and if there is,
// (a) load the pak, (b) return true.
bool pakselector_t::check_only_one_option() const
{
	if (file_table.get_size().h == 1) {
		gui_file_table_row_t* file_row = (gui_file_table_row_t*) file_table.get_row(0);
		if ( !file_row->get_delete_enabled() ) {
			// This means "no private pak addon options" (overloading of meaning)
			// Invoke the automatic load feature
			env_t::objfilename = get_filename(file_row->get_name()) + "/";
			env_t::default_settings.set_with_private_paks( false );
			return true;
		}
	}
	// More than one option or private pak addon option.
	return false;
}



void pakselector_t::add_file(const char *, const char *filename, const bool not_cutting_suffix)
{
	char buttontext[1024];
	strcpy( buttontext, filename );
	if ( !not_cutting_suffix ) {
		buttontext[strlen(buttontext)-4] = '\0';
	}
	bool has_addon_dir = env_t::program_dir!=env_t::user_dir;
	if (has_addon_dir) {
		char path[1024];
		sprintf(path,"%s%s", env_t::user_dir, filename);
		has_addon_dir = chdir( path ) == 0;
		chdir( env_t::program_dir );
	}
	file_table.add_row( new gui_file_table_row_t( filename, buttontext, has_addon_dir ));
}


void pakselector_t::set_windowsize(scr_size size)
{
	if(size.h>display_get_height()-70) {
		// too large ...
		size.h = ((display_get_height()-D_TITLEBAR_HEIGHT-30-3*LINESPACE-4-1)/D_BUTTON_HEIGHT)*D_BUTTON_HEIGHT+D_TITLEBAR_HEIGHT+30+3*LINESPACE+4+1-70;
		// position adjustment will be done automatically ... nice!
	}
	gui_frame_t::set_windowsize(size);
	size = get_windowsize();

	//scr_coord_val y = 0;
	//FOR(slist_tpl<dir_entry_t>, const& i, entries) {
	//	// resize all but delete button

	//	if (i.type == LI_HEADER && this->num_sections > 1) {
	//		y += D_BUTTON_HEIGHT;
	//		continue;
	//	}
	//	if (i.type == LI_HEADER && this->num_sections <= 1) {
	//		continue;
	//	}

	//	if (i.button->is_visible()) {
	//		button_t* const button1 = i.del;
	//		button1->set_pos(scr_coord(button1->get_pos().x, y));
	//		button_t* const button2 = i.button;
	//		button2->set_pos(scr_coord(button2->get_pos().x, y));
	//		button2->set_size(scr_size(size.w/2-40, D_BUTTON_HEIGHT));
	//		i.label->set_pos(scr_coord(size.w/2-40+30, y+2));
	//		y += D_BUTTON_HEIGHT;
	//	}
	//}
	//
	//button_frame.set_size(scr_size(size.w,y));
	scrolly.set_size(scr_size(size.w,size.h-D_TITLEBAR_HEIGHT-30-3*LINESPACE-4-1));
}
