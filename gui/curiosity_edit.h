/*
 * Curiosity (attractions) builder dialog
 */

#ifndef gui_curiosity_edit_h
#define gui_curiosity_edit_h

#include "extend_edit.h"

#include "components/gui_label.h"


class wkz_build_haus_t;
class haus_besch_t;

class curiosity_edit_frame_t : public extend_edit_gui_t
{
private:
	static wkz_build_haus_t haus_tool;
	static char param_str[256];

	const haus_besch_t *besch;
	uint8 rotation;

	char rot_str[16];

	vector_tpl<const haus_besch_t *>hauslist;

	button_t bt_city_attraction;
	button_t bt_land_attraction;
	button_t bt_monuments;

	button_t bt_left_rotate, bt_right_rotate;
	gui_label_t lb_rotation, lb_rotation_info;

	void fill_list( bool translate );

	virtual void change_item_info( sint32 i );

public:
	curiosity_edit_frame_t(spieler_t* sp,karte_t* welt);

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
	const char* get_hilfe_datei() const { return "curiosity_build.txt"; }

	/**
	* Draw new component. The values to be passed refer to the window
	* i.e. It's the screen coordinates of the window where the
	* component is displayed.
	* @author Hj. Malthaner
	*/
	void zeichnen(koord pos, koord gr);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
