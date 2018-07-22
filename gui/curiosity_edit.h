/*
 * Curiosity (attractions) builder dialog
 */

#ifndef gui_curiosity_edit_h
#define gui_curiosity_edit_h

#include "extend_edit.h"

#include "components/gui_label.h"


class tool_build_house_t;
class building_desc_t;

class curiosity_edit_frame_t : public extend_edit_gui_t
{
private:
	static tool_build_house_t* haus_tool;
	static char param_str[256];

	const building_desc_t *desc;
	uint8 rotation;

	char rot_str[16];

	vector_tpl<const building_desc_t *>building_list;

	button_t bt_city_attraction;
	button_t bt_land_attraction;
	button_t bt_monuments;

	button_t bt_left_rotate, bt_right_rotate;
	gui_label_t lb_rotation, lb_rotation_info;

	void fill_list( bool translate );

	virtual void change_item_info( sint32 i );

public:
	curiosity_edit_frame_t(player_t* player);

	/**
	* in top-level windows the name is displayed in titlebar
	* @return the non-translated component name
	* @author Hj. Malthaner
	*/
	const char* get_name() const { return "curiosity builder"; }

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	* @author Hj. Malthaner
	*/
	const char* get_help_filename() const { return "curiosity_build.txt"; }

	/**
	* Draw new component. The values to be passed refer to the window
	* i.e. It's the screen coordinates of the window where the
	* component is displayed.
	* @author Hj. Malthaner
	*/
	void draw(scr_coord pos, scr_size size);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
