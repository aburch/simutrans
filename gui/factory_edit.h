#ifndef gui_factory_edit_h
#define gui_factory_edit_h

#include "extend_edit.h"

#include "components/gui_label.h"

#include "../besch/haus_besch.h"
#include "../besch/fabrik_besch.h"
#include "../utils/cbuffer_t.h"

typedef struct {
	const fabrik_besch_t *besch;
	uint32 production;
	bool ignore_climates;
	uint8 rotation; //255 for any
} build_fab_struct;


#define MAX_BUILD_TYPE (6)

class factory_edit_frame_t : public extend_edit_gui_t
{
private:
	build_fab_struct bfs;

	char prod_str[32];

	vector_tpl<const fabrik_besch_t *>fablist;

	button_t bt_city_chain;
	button_t bt_land_chain;

	button_t bt_up_production, bt_down_production;
	gui_label_t lb_production;

	void fill_list( bool translate );

	virtual void change_item_info( sint32 i );

public:
	factory_edit_frame_t(spieler_t* sp,karte_t* welt);

	/**
	* in top-level fenstern wird der Name in der Titelzeile dargestellt
	* @return den nicht uebersetzten Namen der Komponente
	* @author Hj. Malthaner
	*/
	const char* gib_name() const { return "factory builder"; }

	/**
	* Manche Fenster haben einen Hilfetext assoziiert.
	* @return den Dateinamen für die Hilfe, oder NULL
	* @author Hj. Malthaner
	*/
	const char* gib_hilfe_datei() const { return "factory_build.txt"; }

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
