/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_FACTORY_EDIT_H
#define GUI_FACTORY_EDIT_H


#include "extend_edit.h"
#include "components/gui_numberinput.h"
#include "../utils/cbuffer.h"

class factory_desc_t;

class tool_build_land_chain_t;
class tool_city_chain_t;
class tool_build_factory_t;


/*
 * Factories builder dialog
 */
class factory_edit_frame_t : public extend_edit_gui_t
{
private:
	static tool_build_land_chain_t land_chain_tool;
	static tool_city_chain_t city_chain_tool;
	static tool_build_factory_t fab_tool;
	static cbuffer_t param_str;

	const factory_desc_t *fac_desc;
	uint32 production;

	vector_tpl<const factory_desc_t *>factory_list;

	button_t bt_city_chain;
	button_t bt_land_chain;

	gui_numberinput_t inp_production;

	void fill_list() OVERRIDE;

	void change_item_info( sint32 i ) OVERRIDE;

public:
	factory_edit_frame_t(player_t* player);

	static bool sortreverse;

	/**
	* in top-level windows the name is displayed in titlebar
	* @return the non-translated component name
	*/
	const char* get_name() const { return "factorybuilder"; }

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL

	*/
	const char* get_help_filename() const OVERRIDE { return "factory_build.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void set_windowsize(scr_size size) OVERRIDE;
};

#endif
