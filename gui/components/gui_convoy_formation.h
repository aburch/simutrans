/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_CONVOY_FORMATION_H
#define GUI_COMPONENTS_GUI_CONVOY_FORMATION_H


#include "gui_container.h"
#include "gui_scrollpane.h"
#include "gui_speedbar.h"

#include "../../convoihandle_t.h"
#include "../simwin.h"


// content of convoy formation
class gui_convoy_formation_t : public gui_container_t
{
private:
	convoihandle_t cnv;
	uint8 mode= formation;

	enum { OK = 0, out_of_producton = 1, obsolete = 2, STAT_COLORS };

public:
	enum convoy_overview_t { appearance=0, capacities, formation, CONVOY_OVERVIEW_MODES };
	static const char *cnvlist_mode_button_texts[CONVOY_OVERVIEW_MODES];

	gui_convoy_formation_t(convoihandle_t cnv);

	void set_cnv(convoihandle_t c) { cnv = c; }
	void set_mode(uint8 m) { mode = m; }

	void draw(scr_coord offset);
	scr_size draw_formation(scr_coord offset) const;
	scr_size draw_vehicles(scr_coord offset, bool display_images) const;
	scr_size draw_capacities(scr_coord offset) const;
};

#endif
