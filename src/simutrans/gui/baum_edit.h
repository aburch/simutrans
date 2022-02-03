/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_BAUM_EDIT_H
#define GUI_BAUM_EDIT_H


#include "extend_edit.h"
#include "components/gui_image.h"


class tree_desc_t;
class tool_plant_tree_t;

/*
 * The trees builder
 */
class baum_edit_frame_t : public extend_edit_gui_t
{
private:
	static tool_plant_tree_t baum_tool;
	static cbuffer_t param_str;

	const tree_desc_t *desc;

	gui_image_t tree_image;

	vector_tpl<const tree_desc_t *>tree_list;

	button_t bt_randomage;

	void fill_list() OVERRIDE;

	void change_item_info( sint32 i ) OVERRIDE;

public:
	baum_edit_frame_t(player_t* player_);

	static bool sortreverse;

	/**
	* in top-level windows the name is displayed in titlebar
	* @return the non-translated component name
	*/
	const char* get_name() const { return "baum builder"; }

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	*/
	const char* get_help_filename() const OVERRIDE { return "baum_build.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
