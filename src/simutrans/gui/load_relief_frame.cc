/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <stdio.h>

#include "../world/simworld.h"
#include "load_relief_frame.h"
#include "welt.h"
#include "simwin.h"
#include "../dataobj/translator.h"
#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../dataobj/height_map_loader.h"
#include "components/gui_scrolled_list.h"


static const char *load_mode_texts[env_t::NUM_HEIGHT_CONV_MODES] =
{
	"legacy (small heights)",
	"legacy (large heights)",
	"linear",
	"clamp"
};


class load_relief_mode_scrollitem_t : public gui_scrolled_list_t::const_text_scrollitem_t
{
public:
	load_relief_mode_scrollitem_t(env_t::height_conversion_mode mode) :
		gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(load_mode_texts[(int)mode]), SYSCOL_TEXT),
		mode(mode)
	{}

	env_t::height_conversion_mode get_mode() const { return mode; }

private:
	env_t::height_conversion_mode mode;
};


/**
 * Action, started on button pressing
 */
bool load_relief_frame_t::item_action(const char *fullpath)
{
	sets->heightfield = fullpath;

	if (gui_frame_t *new_world_gui = win_get_magic( magic_welt_gui_t )) {
		const gui_scrolled_list_t::scrollitem_t *selected = load_mode.get_selected_item();
		if (selected) {
			env_t::height_conv_mode = static_cast<const load_relief_mode_scrollitem_t *>(selected)->get_mode();
		}

		static_cast<welt_gui_t*>(new_world_gui)->update_preview(true);
	}

	return false;
}


load_relief_frame_t::load_relief_frame_t(settings_t* const sets) : savegame_frame_t( NULL, false, "maps/", env_t::show_delete_buttons )
{
	gui_aligned_container_t *table = bottom_left_frame.add_table(2, 1);
		load_mode_label.init(translator::translate("Load mode:"), scr_coord(0, 0));
		load_mode_label.set_visible(true);
		table->add_component( &load_mode_label );

		load_mode.init(scr_coord(0, 0));
		load_mode.new_component<load_relief_mode_scrollitem_t>(env_t::HEIGHT_CONV_LEGACY_SMALL);
		load_mode.new_component<load_relief_mode_scrollitem_t>(env_t::HEIGHT_CONV_LEGACY_LARGE);
		load_mode.new_component<load_relief_mode_scrollitem_t>(env_t::HEIGHT_CONV_LINEAR);
		load_mode.new_component<load_relief_mode_scrollitem_t>(env_t::HEIGHT_CONV_CLAMP);
		load_mode.set_selection(env_t::HEIGHT_CONV_LINEAR);
		table->add_component( &load_mode );
	bottom_left_frame.end_table();

	const std::string extra_path = env_t::pak_dir + "maps/";
	this->add_path(extra_path.c_str());

	set_name(translator::translate("Lade Relief"));
	this->sets = sets;
	sets->heightfield = "";
}


const char *load_relief_frame_t::get_info(const char *fullpath)
{
	static char size[64];

	sint16 w, h;
	sint8 *h_field = NULL;
	const sint8 min_h = world()->get_settings().get_minimumheight();
	const sint8 max_h = world()->get_settings().get_maximumheight();

	const gui_scrolled_list_t::scrollitem_t *selected = load_mode.get_selected_item();
	env_t::height_conversion_mode new_mode = env_t::height_conv_mode;

	if (selected) {
		new_mode = static_cast<const load_relief_mode_scrollitem_t *>(selected)->get_mode();
	}

	height_map_loader_t hml(min_h, max_h, env_t::height_conv_mode);
	if(hml.get_height_data_from_file(fullpath, (sint8)sets->get_groundwater(), h_field, w, h, true )) {
		sprintf( size, "%i x %i", w, h );
		env_t::height_conv_mode = new_mode;
		return size;
	}

	return "";
}


bool load_relief_frame_t::check_file( const char *fullpath, const char * )
{
	const sint8 min_h = world()->get_settings().get_minimumheight();
	const sint8 max_h = world()->get_settings().get_maximumheight();

	const gui_scrolled_list_t::scrollitem_t *selected = load_mode.get_selected_item();
	env_t::height_conversion_mode new_mode = env_t::height_conv_mode;

	if (selected) {
		new_mode = static_cast<const load_relief_mode_scrollitem_t *>(selected)->get_mode();
	}

	height_map_loader_t hml(min_h, max_h, env_t::height_conv_mode);
	sint16 w, h;
	sint8 *h_field = NULL;

	if(hml.get_height_data_from_file(fullpath, (sint8)sets->get_groundwater(), h_field, w, h, true )) {
		env_t::height_conv_mode = new_mode;
		return w>0  &&  h>0;
	}

	return false;
}
