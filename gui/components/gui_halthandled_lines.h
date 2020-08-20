/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_HALTHANDLED_LINES_H
#define GUI_COMPONENTS_GUI_HALTHANDLED_LINES_H


#include "gui_container.h"
#include "../simwin.h"

#include "gui_component.h"

#include "../../simhalt.h"

// A GUI component of the station handled lines
class gui_halthandled_lines_t : public gui_container_t
{
private:
	halthandle_t halt;

public:
	gui_halthandled_lines_t(halthandle_t halt);

	void set_halt(halthandle_t h) { this->halt = h; }

	void draw(scr_coord offset);
};

#endif
