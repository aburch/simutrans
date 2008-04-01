#ifndef gui_baum_edit_h
#define gui_baum_edit_h

#include "extend_edit.h"

#include "components/gui_label.h"

#include "../besch/baum_besch.h"
#include "../utils/cbuffer_t.h"

#include "../simwerkz.h"

#define MAX_BUILD_TYPE (6)



class baum_edit_frame_t : public extend_edit_gui_t
{
private:
	static wkz_plant_tree_t baum_tool;
	static char param_str[256];

	const baum_besch_t *besch;

	vector_tpl<const baum_besch_t *>baumlist;

	void fill_list( bool translate );

	virtual void change_item_info( sint32 i );

public:
	baum_edit_frame_t(spieler_t* sp,karte_t* welt);

	/**
	* in top-level fenstern wird der Name in der Titelzeile dargestellt
	* @return den nicht uebersetzten Namen der Komponente
	* @author Hj. Malthaner
	*/
	const char* gib_name() const { return "baum builder"; }

	/**
	* Manche Fenster haben einen Hilfetext assoziiert.
	* @return den Dateinamen für die Hilfe, oder NULL
	* @author Hj. Malthaner
	*/
	const char* gib_hilfe_datei() const { return "baum_build.txt"; }

	/**
	* This method is called if an action is triggered
	* @author Hj. Malthaner
	*
	* Returns true, if action is done and no more
	* components should be triggered.
	* V.Meyer
	*/
//	bool action_triggered(gui_komponente_t *komp, value_t extra);
};

#endif
