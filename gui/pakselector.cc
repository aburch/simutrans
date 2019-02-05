/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>

#include "pakselector.h"
#include "../dataobj/environment.h"
#include "../utils/cbuffer_t.h"
#include "../sys/simsys.h"


pakselector_t::pakselector_t() :
	savegame_frame_t(NULL, true, env_t::program_dir, true),
	notice_label(&notice_buffer)
{
	// remove unnecessary buttons
	top_frame.remove_component( &input );
	savebutton.set_visible(false);
	cancelbutton.set_visible(false);

	// don't show list item labels
	label_enabled = false;

	fnlabel.set_text("Choose one graphics set for playing:");
	notice_buffer.printf("%s",
		"To avoid seeing this dialogue define a path by:\n"
		" - adding 'pak_file_path = pak/' to your simuconf.tab\n"
		" - using '-objects pakxyz/' on the command line"
	);
	notice_label.recalc_size();
	add_component(&notice_label);

	resize(scr_coord(0, 0));
}


/**
 * what to do after loading
 */
bool pakselector_t::item_action(const char *fullpath)
{
	env_t::objfilename = get_filename(fullpath) + "/";
	env_t::default_settings.set_with_private_paks(false);

	return true;
}


bool pakselector_t::del_action(const char *fullpath)
{
	// cannot delete set => use this for selection
	env_t::objfilename = get_filename(fullpath) + "/";
	env_t::default_settings.set_with_private_paks(true);
	return true;
}


const char *pakselector_t::get_info(const char *)
{
	return "";
}


/**
 * This method returns true if filename is what we want and false if not.
 * A PAK directory is considered valid if the file ground.outside.pak exists.
*/
bool pakselector_t::check_file(const char *filename, const char *)
{
	cbuffer_t buf;
	buf.printf("%s/ground.Outside.pak", filename);

	// if we can open the file, it is valid.
	if (FILE* const f = dr_fopen(buf, "r")) {
		fclose(f);
		return true;
	}

	// the file was not found or couldn't be opened.
	return false;
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


void pakselector_t::fill_list()
{
	cbuffer_t path;

	// do the search ...
	savegame_frame_t::fill_list();

	entries.sort(dir_entry_t::compare);

	FOR(slist_tpl<dir_entry_t>, &i, entries) {

		if (i.type == LI_HEADER) {
			continue;
		}

		// look for addon directory
		path.clear();
		path.printf("%saddons/%s", env_t::user_dir, i.button->get_text());

		// reuse delete button as load-with-addons button
		delete i.del;
		i.del = new button_t();
		i.del->init(button_t::roundbox, "Load with addons");

		// if we can't change directory to /addon
		// Hide the addon button
		if (dr_chdir(path) != 0) {
			i.del->disable();

			// if list contains only one header, one pakset entry without addons
			// store path to pakset temporary, reset later if more choices available
			// if env_t::objfilename is non-empty then simmain.cc will close the window immediately
			env_t::objfilename = (std::string)i.button->get_text() + "/";
		}
	}
	dr_chdir(env_t::program_dir);

	if (entries.get_count() > this->num_sections + 1) {
		// empty path as more than one pakset is present, user has to choose
		env_t::objfilename = "";
	}
}
