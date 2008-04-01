#ifndef gui_curiosity_edit_h
#define gui_curiosity_edit_h

#include "extend_edit.h"

#include "components/gui_label.h"

#include "../besch/haus_besch.h"

#include "../simwerkz.h"

#define MAX_BUILD_TYPE (6)


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
	* in top-level fenstern wird der Name in der Titelzeile dargestellt
	* @return den nicht uebersetzten Namen der Komponente
	* @author Hj. Malthaner
	*/
	const char* gib_name() const { return "curiosity builder"; }

	/**
	* Manche Fenster haben einen Hilfetext assoziiert.
	* @return den Dateinamen für die Hilfe, oder NULL
	* @author Hj. Malthaner
	*/
	const char* gib_hilfe_datei() const { return "curiosity_build.txt"; }

	/**
	* komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	* das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	* in dem die Komponente dargestellt wird.
	*
	* @author Hj. Malthaner
	*/
//	void zeichnen(koord pos, koord gr);

	/**
	* This method is called if an action is triggered
	* @author Hj. Malthaner
	*
	* Returns true, if action is done and no more
	* components should be triggered.
	* V.Meyer
	*/
	bool action_triggered(gui_komponente_t *komp, value_t extra);
};

#endif
