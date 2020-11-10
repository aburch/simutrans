/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_CONVOIINFO_H
#define GUI_COMPONENTS_GUI_CONVOIINFO_H


#include "../../display/scr_coord.h"
#include "gui_container.h"
#include "gui_speedbar.h"
#include "../../convoihandle_t.h"
#include "gui_convoy_formation.h"
#include "gui_convoy_payloadinfo.h"


/**
 * Convoi info stats, like loading status bar
 * One element of the vehicle list display
 *
 * @author Hj. Malthaner
 */
class gui_convoiinfo_t : public gui_world_component_t
{
private:
	/**
	* Handle Convois to be displayed.
	* @author Hj. Malthaner
	*/
	convoihandle_t cnv;

	gui_speedbar_t filled_bar;
	gui_convoy_formation_t formation;
	gui_convoy_payloadinfo_t payload;

	bool show_line_name = true;

	uint8 display_mode = cnvlist_normal;

public:
	/**
	* @param cnv, the handler for the Convoi to be displayed.
	* @author Hj. Malthaner
	*/
	gui_convoiinfo_t(convoihandle_t cnv, bool show_line_name = true);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Draw the component
	* @author Hj. Malthaner
	*/
	void draw(scr_coord offset) OVERRIDE;

	void set_mode(uint8 mode);

	enum cl_display_mode_t { cnvlist_normal = 0, cnvlist_payload, cnvlist_formation, DISPLAY_MODES };

	static const char *cnvlist_mode_button_texts[DISPLAY_MODES];
};


#endif
