/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CURIOSITY_EDIT_H
#define GUI_CURIOSITY_EDIT_H


#include "extend_edit.h"

class tool_build_house_t;
class building_desc_t;

/*
 * Curiosity (attractions) builder dialog
 */
class curiosity_edit_frame_t : public extend_edit_gui_t
{
private:
	static tool_build_house_t haus_tool;
	static cbuffer_t param_str;

	const building_desc_t *desc;

	vector_tpl<const building_desc_t *>building_list;

	button_t bt_city_attraction;
	button_t bt_land_attraction;
	button_t bt_monuments;

	void fill_list() OVERRIDE;
	void put_item_in_list(const building_desc_t* desc );

	void change_item_info( sint32 i ) OVERRIDE;

public:
	curiosity_edit_frame_t(player_t* player);

	static bool sortreverse;

	/**
	* in top-level windows the name is displayed in titlebar
	* @return the non-translated component name
	*/
	const char* get_name() const { return "curiosity builder"; }

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	*/
	const char* get_help_filename() const OVERRIDE { return "curiosity_build.txt"; }

	/**
	* Draw new component. The values to be passed refer to the window
	* i.e. It's the screen coordinates of the window where the
	* component is displayed.
	*/
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
