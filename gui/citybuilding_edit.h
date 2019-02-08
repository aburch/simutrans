/*
 * The citybuilding editor (urban buildings builder)
 */

#ifndef gui_citybuilding_edit_h
#define gui_citybuilding_edit_h

#include "extend_edit.h"

#include "components/gui_building.h"
#include "components/gui_combobox.h"
#include "../utils/cbuffer_t.h"

class building_desc_t;
class tool_build_house_t;


class citybuilding_edit_frame_t : public extend_edit_gui_t
{
private:
	static tool_build_house_t haus_tool;
	static cbuffer_t param_str;

	const building_desc_t *desc;
	uint8 rotation;

	vector_tpl<const building_desc_t *>building_list;

	button_t bt_res;
	button_t bt_com;
	button_t bt_ind;

	gui_combobox_t cb_rotation;

	void fill_list( bool translate ) OVERRIDE;

	void change_item_info( sint32 i ) OVERRIDE;

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
	const char* get_help_filename() const OVERRIDE { return "citybuilding_build.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
