/*
 * selection of paks at the start time
 */

#include <string>

#include "pakselector.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../simsys.h"

#define L_ADDON_WIDTH (150)

pakselector_t::pakselector_t() :
	savegame_frame_t( NULL, true, env_t::program_dir, true ),
	notice_label(&notice_buffer)
{
	// remove unnecessary buttons
	remove_komponente( &input );
	remove_komponente( &savebutton );
	remove_komponente( &cancelbutton );

	// don't show list item labels
	label_enabled = false;

	fnlabel.set_text( "Choose one graphics set for playing:" );
	notice_buffer.printf("%s",
		"To avoid seeing this dialogue define a path by:\n"
		" - adding 'pak_file_path = pak/' to your simuconf.tab\n"
		" - using '-objects pakxyz/' on the command line"
	);
	notice_label.recalc_size();
	add_komponente(&notice_label);

	addon_button_width = 2*D_H_SPACE + proportional_string_width( translator::translate("Load with addons") );

	resize(scr_coord(0,0));
}


/**
 * what to do after loading
 */
bool pakselector_t::item_action(const char *fullpath)
{
	env_t::objfilename = get_filename(fullpath)+"/";
	env_t::default_settings.set_with_private_paks( false );

	return true;
}


bool pakselector_t::del_action(const char *fullpath)
{
	// cannot delete set => use this for selection
	env_t::objfilename = get_filename(fullpath)+"/";
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
 /*
 // Since both save and cancel buttons has been removed, this will never happend.
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
*/

/**
 * This method returns true if filename is what we want and false if not.
 * A PAK directory is considered valid if the file ground.outside.pak exists.
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

	// the file was not found or couldn't be opened.
	return false;
}


void pakselector_t::fill_list()
{
	cbuffer_t path;
	scr_coord_val y = D_FOCUS_OFFSET_V;

	// do the search ...
	savegame_frame_t::fill_list();
	action_button_width = 0;

	entries.sort(dir_entry_t::compare);

	FOR(slist_tpl<dir_entry_t>, const& i, entries) {

		if (i.type == LI_HEADER) {
			continue;
		}

		// look for addon directory
		path.clear();
		path.printf("%saddons/%s", env_t::user_dir, i.button->get_text());
		i.del->set_pos(scr_coord(D_FOCUS_OFFSET_H,y));
		i.del->set_size( scr_size(addon_button_width,D_BUTTON_HEIGHT) );
		i.del->set_text("Load with addons");

		// if we can't change directory to /addon
		// Hide the addon button
		if(  chdir( path ) != 0  ) {
			i.del->set_visible(false);
			i.del->disable();

			// if list contains only one header, one pakset entry without addons
			// store path to pakset temporary, reset later if more choices available
			// if env_t::objfilename is non-empty then simmain.cc will close the window immediately
			env_t::objfilename = (std::string)i.button->get_text() + "/";
		}

		i.button->align_to(i.del, ALIGN_LEFT | ALIGN_EXTERIOR_H | ALIGN_TOP, scr_coord(D_H_SPACE,0) );
		action_button_width = max( action_button_width, proportional_string_width( i.button->get_text() ) );
		y += D_BUTTON_HEIGHT+D_FOCUS_OFFSET_V;
	}
	action_button_width += (D_H_SPACE<<1);
	chdir( env_t::program_dir );

	if(entries.get_count() > this->num_sections+1) {
		// empty path as more than one pakset is present, user has to choose
		env_t::objfilename = "";
	}

	button_frame.set_size ( scr_size (addon_button_width + D_H_SPACE + action_button_width + D_H_SPACE + D_SCROLLBAR_WIDTH, y) );
	set_windowsize( scr_size(
		D_MARGINS_X + addon_button_width + D_H_SPACE + action_button_width + D_H_SPACE + D_SCROLLBAR_WIDTH,
		D_TITLEBAR_HEIGHT + D_MARGINS_Y + D_EDIT_HEIGHT + D_V_SPACE + y + D_DIVIDER_HEIGHT + notice_label.get_size().h
	));

	resize( scr_coord(0,0));
}


void pakselector_t::set_windowsize(scr_size size)
{
	scr_coord_val y = D_FOCUS_OFFSET_V;

	// Adjust max window size
	size.w = max(size.w,D_MARGINS_X + notice_label.get_size().w);

	size.clip_rightbottom( scr_coord( display_get_width(), display_get_height()-env_t::iconsize.h-D_STATUSBAR_HEIGHT ) );

	gui_frame_t::set_windowsize(size);
	size = get_windowsize();

	// Adjust scrolly
	scrolly.set_size( scr_size(
		size.w - D_MARGINS_X,
		size.h - scrolly.get_pos().y - D_TITLEBAR_HEIGHT - D_DIVIDER_HEIGHT - notice_label.get_size().h - D_MARGIN_BOTTOM
	));
	action_button_width = scrolly.get_client().w - addon_button_width - D_H_SPACE - (D_FOCUS_OFFSET_H<<2);

	// Adjust filename buttons
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
			i.button->set_size( scr_size( action_button_width, D_BUTTON_HEIGHT ) );
			y += D_BUTTON_HEIGHT + D_FOCUS_OFFSET_V;
		}
	}

	// Adjust notice text
	divider1.align_to(&scrolly, ALIGN_TOP | ALIGN_EXTERIOR_V | ALIGN_LEFT);
	divider1.set_width(size.w-D_MARGIN_LEFT-D_MARGIN_RIGHT);
	notice_label.align_to(&divider1, ALIGN_TOP | ALIGN_EXTERIOR_V | ALIGN_LEFT );
}
