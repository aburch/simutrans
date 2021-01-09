/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_GROUNDOBJ_EDIT_H
#define GUI_GROUNDOBJ_EDIT_H


#include "extend_edit.h"
#include "components/gui_image.h"


class groundobj_desc_t;
class tool_plant_groundobj_t;

/*
 * The groundobj builder
 */
class groundobj_edit_frame_t : public extend_edit_gui_t
{
private:
	enum {BY_NAME, BY_TRANSLATION, BY_COST };

	static tool_plant_groundobj_t groundobj_tool;
	static cbuffer_t param_str;

	const groundobj_desc_t *desc;

	gui_image_t groundobj_image;
	gui_combobox_t cb_sortedby;

	vector_tpl<const groundobj_desc_t *>groundobj_list;

	void fill_list( bool translate ) OVERRIDE;

	void change_item_info( sint32 i ) OVERRIDE;

public:
	groundobj_edit_frame_t(player_t* player_);

	/**
	* in top-level windows the name is displayed in titlebar
	* @return the non-translated component name
	*/
	const char* get_name() const { return "groundobj builder"; }

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	*/
	const char* get_help_filename() const OVERRIDE { return "groundobj_build.txt"; }
};

#endif
