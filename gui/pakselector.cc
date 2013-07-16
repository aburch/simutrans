/*
 * selection of paks at the start time
 */

#include <string>

#include "pakselector.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../simsys.h"

#define L_ADDON_WIDTH (150)

pakselector_t::pakselector_t() :
	savegame_frame_t( NULL, true, umgebung_t::program_dir),
	notice_label(&notice_buffer)
{
	// remove unnecessary buttons
	remove_komponente( &input );
	remove_komponente( &savebutton );
	remove_komponente( &cancelbutton );
	//remove_komponente( &divider1 );

	fnlabel.set_text( "Choose one graphics set for playing:" );
	notice_buffer.printf("%s",
		"To avoid seeing this dialogue define a path by:\n"
		" - adding 'pak_file_path = pak/' to your simuconf.tab\n"
		" - using '-objects pakxyz/' on the command line"
	);
	notice_label.recalc_size();
	add_komponente(&notice_label);
	//add_komponente(&divider);

	addon_button_width = 2*D_H_SPACE + proportional_string_width( translator::translate("Load with addons") );

	//resize(koord(360-488,0));
	resize(koord(0,0));
}


/**
 * what to do after loading
 */
void pakselector_t::action(const char *fullpath)
{
	umgebung_t::objfilename = get_filename(fullpath)+"/";
	umgebung_t::default_einstellungen.set_with_private_paks( false );
}


bool pakselector_t::del_action(const char *fullpath)
{
	// cannot delete set => use this for selection
	umgebung_t::objfilename = get_filename(fullpath)+"/";
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


/**
 * This method returns true if filename is what we want and false if not.
 * A PAK directory is concidered valid if the file ground.outside.pak exists.
*/
bool pakselector_t::check_file(const char *filename, const char *)
{
	cbuffer_t buf;
	buf.printf("%s/ground.Outside.pak", filename);

	// if we can open the file, it is valid.
	if (FILE* const f = fopen(buf, "r")) {
		fclose(f);
		return true;
	}

	// the file was not found or couldn't be opend.
	return false;
}


void pakselector_t::fill_list()
{
	cbuffer_t path;
	KOORD_VAL y = 0;
	KOORD_VAL width = 0;

	// do the search ...
	savegame_frame_t::fill_list();

	FOR(slist_tpl<dir_entry_t>, const& i, entries) {

		/*if (i.type == LI_HEADER && this->num_sections > 1) {
			y += D_BUTTON_HEIGHT;
			continue;
		}*/

		//if (i.type == LI_HEADER && this->num_sections <= 1) {
		if (i.type == LI_HEADER) {
			continue;
		}

		// look for addon directory
		path.clear();
		path.printf("%saddons/%s", umgebung_t::user_dir, i.button->get_text());
		i.del->set_groesse( koord(addon_button_width,D_BUTTON_HEIGHT) );
		i.del->set_text("Load with addons");

		// if we can't change directory to /addon
		// Hide the addon button
		if(  chdir( path ) != 0  ) {
			i.del->set_visible(false);
			i.del->disable();

			// if list contains only one header, one pakset entry without addons
			// store path to pakset temporary, reset later if more choices available
			umgebung_t::objfilename = (std::string)i.button->get_text() + "/";
			// if umgebung_t::objfilename is non-empty then simmain.cc will close the window immediately
		}
		i.button->align_to(i.del,ALIGN_EXTERIOR_H | ALIGN_LEFT, koord(D_H_SPACE,0) );
		width = max( width, proportional_string_width( i.button->get_text() ) );
		y += D_BUTTON_HEIGHT;
	}
	chdir( umgebung_t::program_dir );

	if(entries.get_count() > this->num_sections+1) {
		// empty path as more than one pakset is present, user has to choose
		umgebung_t::objfilename = "";
	}
	button_frame.set_groesse ( koord (addon_button_width + width + D_H_SPACE, y) );
	scrolly.set_groesse( button_frame.get_groesse() + koord( D_H_SPACE + button_t::gui_scrollbar_size.x, 0 ) );

	set_fenstergroesse( koord(
		D_MARGIN_LEFT + scrolly.get_groesse().x + D_MARGIN_RIGHT,
		D_TITLEBAR_HEIGHT + D_MARGIN_TOP + D_EDIT_HEIGHT + D_V_SPACE + y + D_DIVIDER_HEIGHT + notice_label.get_groesse().y + D_MARGIN_BOTTOM
	));
}


void pakselector_t::set_fenstergroesse(koord groesse)
{
	// Adjust max window size
	groesse.x = max(groesse.x,D_MARGIN_LEFT + notice_label.get_groesse().x + D_MARGIN_RIGHT);

	if(groesse.y > display_get_height()-umgebung_t::iconsize.y-D_STATUSBAR_HEIGHT) {
		groesse.y = display_get_height()-umgebung_t::iconsize.y-D_STATUSBAR_HEIGHT;
	}

	if(groesse.x > display_get_width()) {
		groesse.x = display_get_width();
	}

	gui_frame_t::set_fenstergroesse(groesse);
	groesse = get_fenstergroesse();

	// Adjust scrolly
	scrolly.set_groesse( koord(
		groesse.x - scrolly.get_pos().x - D_MARGIN_RIGHT,
		groesse.y - scrolly.get_pos().y - D_TITLEBAR_HEIGHT - D_DIVIDER_HEIGHT - notice_label.get_groesse().y - D_MARGIN_BOTTOM
	));

	// Adjust filename buttons
	KOORD_VAL y = 0;
	KOORD_VAL button_width = groesse.x - D_MARGIN_LEFT - D_MARGIN_RIGHT - button_t::gui_scrollbar_size.x - 2*D_H_SPACE - addon_button_width;
	FOR(slist_tpl<dir_entry_t>, const& i, entries) {

		if (i.type == LI_HEADER && this->num_sections > 1) {
			y += D_BUTTON_HEIGHT;
			continue;
		}
		if (i.type == LI_HEADER && this->num_sections <= 1) {
			continue;
		}

		if (i.button->is_visible()) {
			// filename button is the only one resizing
			i.button->set_groesse( koord( button_width, D_BUTTON_HEIGHT ) );
			y += D_BUTTON_HEIGHT;
		}
	}

	// Adjust button frame
	button_frame.set_groesse ( koord(addon_button_width + button_width + D_H_SPACE, y) );

	// Adjust notice text
	divider1.align_to(&scrolly,ALIGN_EXTERIOR_V | ALIGN_TOP | ALIGN_LEFT);
	divider1.set_width(groesse.x-D_MARGIN_LEFT-D_MARGIN_RIGHT);
	notice_label.align_to(&divider1,ALIGN_EXTERIOR_V | ALIGN_TOP | ALIGN_LEFT );

}
