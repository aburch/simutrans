/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "pakselector.h"
#include "pakinstaller.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../sys/simsys.h"
#include "../pathes.h"


pakselector_t::pakselector_t() :
	savegame_frame_t( NULL, true, NULL, true ),
	notice_label(&notice_buffer)
{
	// if true, we would call the installler afterwards
	pakinstaller_t::finish_install = false;

	// remove unnecessary buttons
	top_frame.remove_component( &input );
	savebutton.set_visible(false);
	cancelbutton.set_visible(false);


	// don't show list item labels
	label_enabled = false;

	fnlabel.set_text( "Choose one graphics set for playing:" );
	notice_buffer.printf("%s",
		"To avoid seeing this dialogue define a path by:\n"
		" - adding 'pak_file_path = pak/' to your simuconf.tab\n"
		" - using '-objects pakxyz/' on the command line"
	);
	notice_label.recalc_size();
	add_component(&notice_label);

	installbutton.init( button_t::roundbox, "Install" );
	installbutton.add_listener( &ps );
	add_component(&installbutton);

	resize(scr_coord(0,0));

	dr_chdir(env_t::base_dir);
	char bd[PATH_MAX], id[PATH_MAX];
	dr_getcwd(bd, lengthof(bd)-2);
	strcat(bd, PATH_SEPARATOR);
	dr_chdir(env_t::install_dir);
	dr_getcwd(id, lengthof(id)-2);
	strcat(id, PATH_SEPARATOR);

	add_path( bd );
	if(  strcmp(bd,id)  ) {
	}
	add_path(id);
	dr_chdir( env_t::user_dir );
	if(  !dr_chdir(USER_PAK_PATH)  ) {
		char ud[PATH_MAX];
		dr_getcwd(ud, lengthof(ud)-2 );
		strcat(ud, PATH_SEPARATOR);
		if(  strcmp(id, ud)  ) {
			add_path(ud);
		}
	}
}


/**
 * what to do after loading
 */
bool pakselector_t::item_action(const char *fullpath)
{
	env_t::pak_dir = fullpath;
	env_t::pak_dir += PATH_SEPARATOR;
	env_t::pak_name = (str_get_filename(fullpath, true)+PATH_SEPARATOR);
	env_t::default_settings.set_with_private_paks( false );

	return true;
}


bool pakselector_t::del_action(const char *fullpath)
{
	// cannot delete set => use this for selection
	env_t::pak_dir = fullpath;
	env_t::pak_dir += PATH_SEPARATOR;
	env_t::pak_name = str_get_filename(fullpath, true)+PATH_SEPARATOR;
	env_t::default_settings.set_with_private_paks( true );
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


void pakselector_t::fill_list()
{
	cbuffer_t path;

	// do the search ...
	savegame_frame_t::fill_list();

	// do not sort or the path names are in the wrong positions
//	entries.sort(dir_entry_t::compare);

	for(dir_entry_t &i : entries) {

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
		if(  dr_chdir( path ) != 0  ) {
			i.del->disable();

			// if list contains only one header, one pakset entry without addons
			// store path to pakset temporary, reset later if more choices available
			// if env_t::pak_dir is non-empty then simmain.cc will close the window immediately
			env_t::pak_name = (std::string)i.button->get_text() + PATH_SEPARATOR;
			env_t::pak_dir = (std::string)i.info + PATH_SEPARATOR;
		}
	}
	dr_chdir( env_t::base_dir );

	if(entries.get_count() > this->num_sections+1) {
		// empty path as more than one pakset is present, user has to choose
		env_t::pak_dir.clear();
		env_t::pak_name.clear();
	}
}


bool pakselector_install_action_t::action_triggered( gui_action_creator_t*, value_t)
{
	pakinstaller_t::finish_install = true;
	return true;
}
