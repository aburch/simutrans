/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string>
#include <stdio.h>

#include "../simworld.h"
#include "load_relief_frame.h"
#include "welt.h"
#include "simwin.h"
#include "../dataobj/translator.h"
#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../dataobj/height_map_loader.h"

/**
 * Action, started on button pressing
 * @author Hansjörg Malthaner
 */
bool load_relief_frame_t::item_action(const char *fullpath)
{
	sets->heightfield = fullpath;

	if (gui_frame_t *new_world_gui = win_get_magic( magic_welt_gui_t )) {
		static_cast<welt_gui_t*>(new_world_gui)->update_preview(true);
	}
	return false;
}


load_relief_frame_t::load_relief_frame_t(settings_t* const sets) : savegame_frame_t( NULL, false, "maps/", env_t::show_delete_buttons )
{
	new_format.init( button_t::square_automatic, "Maximize height levels");
	new_format.pressed = env_t::new_height_map_conversion;
	bottom_left_frame.add_component( &new_format );

	const std::string extra_path = env_t::program_dir + env_t::objfilename + "maps/";
	this->add_path(extra_path.c_str());

	set_name(translator::translate("Lade Relief"));
	this->sets = sets;
	sets->heightfield = "";
}


const char *load_relief_frame_t::get_info(const char *fullpath)
{
	static char size[64];

	sint16 w, h;
	sint8 *h_field ;
	height_map_loader_t hml(new_format.pressed);

	if(hml.get_height_data_from_file(fullpath, (sint8)sets->get_groundwater(), h_field, w, h, true )) {
		sprintf( size, "%i x %i", w, h );
		env_t::new_height_map_conversion = new_format.pressed;
		return size;
	}
	return "";
}


bool load_relief_frame_t::check_file( const char *fullpath, const char * )
{
	sint16 w, h;
	sint8 *h_field;

	height_map_loader_t hml(new_format.pressed);
	if(hml.get_height_data_from_file(fullpath, (sint8)sets->get_groundwater(), h_field, w, h, true )) {
		return w>0  &&  h>0;
		env_t::new_height_map_conversion = new_format.pressed;
	}
	return false;
}
