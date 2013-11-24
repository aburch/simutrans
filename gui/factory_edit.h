/*
 * Factories builder dialog
 */

#ifndef gui_factory_edit_h
#define gui_factory_edit_h

#include "extend_edit.h"
#include "components/gui_label.h"
#include "components/gui_numberinput.h"

class fabrik_besch_t;

class wkz_build_industries_land_t;
class wkz_build_industries_city_t;
class wkz_build_factory_t;


class factory_edit_frame_t : public extend_edit_gui_t
{
private:
	static wkz_build_industries_land_t land_chain_tool;
	static wkz_build_industries_city_t city_chain_tool;
	static wkz_build_factory_t fab_tool;
	static char param_str[256];

	const fabrik_besch_t *fab_besch;
	uint32 production;
	uint8 rotation; //255 for any

	char prod_str[32], rot_str[16];

	vector_tpl<const fabrik_besch_t *>fablist;

	button_t bt_city_chain;
	button_t bt_land_chain;

	button_t bt_left_rotate, bt_right_rotate;
	gui_label_t lb_rotation, lb_rotation_info;

	gui_numberinput_t inp_production;
	gui_label_t lb_production_info;

	void fill_list( bool translate );

	virtual void change_item_info( sint32 i );

public:
	factory_edit_frame_t(spieler_t* sp);

	/**
	* in top-level windows the name is displayed in titlebar
	* @return the non-translated component name
	* @author Hj. Malthaner
	*/
	const char* get_name() const { return "factorybuilder"; }

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	* @author Hj. Malthaner
	*/
	const char* get_hilfe_datei() const { return "factory_build.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
