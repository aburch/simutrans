/*
 * The trees builder
 */

#ifndef gui_baum_edit_h
#define gui_baum_edit_h

#include "extend_edit.h"

#include "components/gui_label.h"


class baum_besch_t;
class wkz_plant_tree_t;

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
	* in top-level windows the name is displayed in titlebar
	* @return the non-translated component name
	* @author Hj. Malthaner
	*/
	const char* get_name() const { return "baum builder"; }

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	* @author Hj. Malthaner
	*/
	const char* get_hilfe_datei() const { return "baum_build.txt"; }
};

#endif
