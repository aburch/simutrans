#ifndef gui_convoy_formation_h
#define gui_convoy_formation_h

#include "gui_container.h"
#include "gui_scrollpane.h"
#include "gui_speedbar.h"

#include "../../convoihandle_t.h"
#include "../simwin.h"


// content of convoy formation @Ranran
class gui_convoy_formaion_t : public gui_container_t
{
private:
	convoihandle_t cnv;

	enum { OK = 0, out_of_producton = 1, obsolete = 2, STAT_COLORS };
	uint8 status_to_color[STAT_COLORS]{ COL_DARK_GREEN, COL_OUT_OF_PRODUCTION, COL_OBSOLETE };

public:
	gui_convoy_formaion_t(convoihandle_t cnv);

	void set_cnv(convoihandle_t c) { cnv = c; }

	void draw(scr_coord offset);
};

#endif
