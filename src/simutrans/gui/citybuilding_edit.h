/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CITYBUILDING_EDIT_H
#define GUI_CITYBUILDING_EDIT_H


#include "extend_edit.h"

#include "components/gui_building.h"
#include "../utils/cbuffer.h"

class building_desc_t;
class tool_build_house_t;


/*
 * The citybuilding editor (urban buildings builder)
 */
class citybuilding_edit_frame_t : public extend_edit_gui_t
{
private:
	static tool_build_house_t haus_tool;
	static cbuffer_t param_str;

	const building_desc_t *desc;

	vector_tpl<const building_desc_t *>building_list;

	button_t bt_res;
	button_t bt_com;
	button_t bt_ind;

	void fill_list() OVERRIDE;
	void put_item_in_list( const building_desc_t* desc );

	void change_item_info( sint32 i ) OVERRIDE;

public:
	citybuilding_edit_frame_t(player_t* player);

	static bool sortreverse;

	/**
	* in top-level windows the name is displayed in titlebar
	* @return the non-translated component name
	*/
	const char* get_name() const { return "citybuilding builder"; }

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	*/
	const char* get_help_filename() const OVERRIDE { return "citybuilding_build.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
