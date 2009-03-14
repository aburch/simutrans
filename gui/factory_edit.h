#ifndef gui_factory_edit_h
#define gui_factory_edit_h

#include "extend_edit.h"
#include "components/gui_label.h"
#include "components/gui_numberinput.h"

class fabrik_besch_t;

class wkz_build_industries_land_t;
class wkz_build_industries_city_t;
class wkz_build_factory_t;

#define MAX_BUILD_TYPE (6)

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
	factory_edit_frame_t(spieler_t* sp,karte_t* welt);

	/**
	* in top-level fenstern wird der Name in der Titelzeile dargestellt
	* @return den nicht uebersetzten Namen der Komponente
	* @author Hj. Malthaner
	*/
	const char* get_name() const { return "factorybuilder"; }

	/**
	* Manche Fenster haben einen Hilfetext assoziiert.
	* @return den Dateinamen für die Hilfe, oder NULL
	* @author Hj. Malthaner
	*/
	const char* get_hilfe_datei() const { return "factory_build.txt"; }

	/**
	* This method is called if an action is triggered
	* @author Hj. Malthaner
	*
	* Returns true, if action is done and no more
	* components should be triggered.
	* V.Meyer
	*/
	bool action_triggered( gui_action_creator_t *komp, value_t extra);
};

#endif
