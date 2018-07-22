/*
 * The citybuilding editor (urban buildings builder)
 */

#ifndef gui_citybuilding_edit_h
#define gui_citybuilding_edit_h

#include "extend_edit.h"

#include "components/gui_label.h"

class building_desc_t;
class tool_build_house_t;


class citybuilding_edit_frame_t : public extend_edit_gui_t
{
private:
	static tool_build_house_t* haus_tool;
	static char param_str[256];

	const building_desc_t *desc;
	uint8 rotation;

	char rot_str[16];

	vector_tpl<const building_desc_t *>building_list;

	button_t bt_res;
	button_t bt_com;
	button_t bt_ind;

	button_t bt_left_rotate, bt_right_rotate;
	gui_label_t lb_rotation, lb_rotation_info;

	void fill_list( bool translate );

	virtual void change_item_info( sint32 i );

public:
	citybuilding_edit_frame_t(player_t* player);

	/**
	* in top-level windows the name is displayed in titlebar
	* @return the non-translated component name
	* @author Hj. Malthaner
	*/
	const char* get_name() const { return "citybuilding builder"; }

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	* @author Hj. Malthaner
	*/
	const char* get_help_filename() const { return "citybuilding_build.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
