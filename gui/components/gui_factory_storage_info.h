/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_FACTORY_STORAGE_INFO_H
#define GUI_COMPONENTS_GUI_FACTORY_STORAGE_INFO_H


#include "gui_container.h"
#include "gui_scrollpane.h"
#include "gui_speedbar.h"

#include "../../simfab.h"
#include "../simwin.h"

#include "gui_component.h"

#include "../../simhalt.h"

class fabrik_t;

// A GUI component of the factory storage info
class gui_factory_storage_info_t : public gui_container_t
{
private:
	fabrik_t *fab;

public:
	gui_factory_storage_info_t(fabrik_t *factory);

	void set_fab(fabrik_t *f) { this->fab = f; }

	void draw(scr_coord offset);
	void recalc_size();
};


// A GUI component of connectable factory list
class gui_factory_connection_stat_t : public gui_world_component_t
{
private:
	fabrik_t *fab;
	vector_tpl<koord> fab_list; // connectable factory list(pos)
	uint32 line_selected;

	bool is_input_display; // which display is needed? - input or output

public:
	gui_factory_connection_stat_t(fabrik_t *factory, bool is_input_display);

	void set_fab(fabrik_t *f) { this->fab = f; }

	bool infowin_event(event_t const *ev) OVERRIDE;

	void recalc_size();

	void draw(scr_coord offset) OVERRIDE;
};


// A display of nearby freight stations for factory GUI
class gui_factory_nearby_halt_info_t : public gui_world_component_t
{
private:
	fabrik_t *fab;
	vector_tpl<nearby_halt_t> halt_list;
	uint32 line_selected;

public:
	gui_factory_nearby_halt_info_t(fabrik_t *factory);

	void set_fab(fabrik_t *f) { this->fab = f; }

	bool infowin_event(event_t const *ev) OVERRIDE;

	void recalc_size();
	void update();

	void draw(scr_coord offset) OVERRIDE;

};

#endif
