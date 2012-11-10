#include <string>

#include "pakselector.h"
#include "../dataobj/umgebung.h"
#include "../utils/cbuffer_t.h"
#include "../simsys.h"

/**
 * what to do after loading
 */
void pakselector_t::action(const char *fullpath)
{
	umgebung_t::objfilename = this->get_filename(fullpath)+"/";
	umgebung_t::default_einstellungen.set_with_private_paks( false );
}


bool pakselector_t::del_action(const char *fullpath)
{
	// cannot delete set => use this for selection
	umgebung_t::objfilename = this->get_filename(fullpath)+"/";
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
bool pakselector_t::action_triggered(gui_action_creator_t *komp, value_t v)
{
	if(komp == &savebutton) {
		savebutton.pressed ^= 1;
		return true;
	}
	else if(komp != &input) {
		return savegame_frame_t::action_triggered(komp, v );
	}
	return false;
}


void pakselector_t::zeichnen(koord p, koord gr)
{
	gui_frame_t::zeichnen(p, gr );

	display_multiline_text( p.x+10, p.y+gr.y-3*LINESPACE-4,
		"To avoid seeing this dialogue define a path by:\n"
		" - adding 'pak_file_path = pak/' to your simuconf.tab\n"
		" - using '-objects pakxyz/' on the command line", COL_BLACK );
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


pakselector_t::pakselector_t() : savegame_frame_t( NULL, true, umgebung_t::program_dir)
{
	// remove unneccessary buttons
	remove_komponente( &input );
	remove_komponente( &savebutton );
	remove_komponente( &cancelbutton );
	remove_komponente( &divider1 );

	fnlabel.set_text( "Choose one graphics set for playing:" );

	resize(koord(360-488,0));
}


void pakselector_t::fill_list()
{
	// do the search ...
	savegame_frame_t::fill_list();

	int y = 0;
	FOR(slist_tpl<dir_entry_t>, const& i, entries) {

		if (i.type == LI_HEADER && this->num_sections > 1) {
			y += D_BUTTON_HEIGHT;
			continue;
		}
		if (i.type == LI_HEADER && this->num_sections <= 1) {
			continue;
		}

		cbuffer_t path;
		path.printf("%saddons/%s", umgebung_t::user_dir, i.button->get_text());
		i.del->groesse.x += 150;
		i.del->set_text("Load with addons");
		i.button->set_pos(koord(150,0) + i.button->get_pos());
		if(  chdir( path )!=0  ) {
			// no addons for this
			i.del->set_visible(false);
			i.del->disable();

			// if list contains only one header, one pakset entry without addons
			// store path to pakset temporary, reset later if more choices available
			umgebung_t::objfilename = (std::string)i.button->get_text() + "/";
			// if umgebung_t::objfilename is non-empty then simmain.cc will close the window immediately
		}
		y += D_BUTTON_HEIGHT;
	}
	chdir( umgebung_t::program_dir );

	if(entries.get_count() > this->num_sections+1) {
 		// empty path as more than one pakset is present, user has to choose
		umgebung_t::objfilename = "";
	}

	button_frame.set_groesse(koord(get_fenstergroesse().x-1, y ));
	set_fenstergroesse(koord(get_fenstergroesse().x, D_TITLEBAR_HEIGHT+30+y+3*LINESPACE+4+1));
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
	FOR(slist_tpl<dir_entry_t>, const& i, entries) {
		// resize all but delete button

		if (i.type == LI_HEADER && this->num_sections > 1) {
			y += D_BUTTON_HEIGHT;
			continue;
		}
		if (i.type == LI_HEADER && this->num_sections <= 1) {
			continue;
		}

		if (i.button->is_visible()) {
			button_t* const button1 = i.del;
			button1->set_pos(koord(button1->get_pos().x, y));
			button_t* const button2 = i.button;
			button2->set_pos(koord(button2->get_pos().x, y));
			button2->set_groesse(koord(groesse.x/2-40, D_BUTTON_HEIGHT));
			i.label->set_pos(koord(groesse.x/2-40+30, y+2));
			y += D_BUTTON_HEIGHT;
		}
	}

	button_frame.set_groesse(koord(groesse.x,y));
	scrolly.set_groesse(koord(groesse.x,groesse.y-D_TITLEBAR_HEIGHT-30-3*LINESPACE-4-1));
}
