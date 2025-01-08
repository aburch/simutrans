/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_WAYTYPE_TAB_PANEL_H
#define GUI_COMPONENTS_GUI_WAYTYPE_TAB_PANEL_H


#include "../../simtypes.h"
#include "../../simhalt.h"
#include "gui_tab_panel.h"

// panel that show the available waytypes
class gui_waytype_tab_panel_t :
	public gui_tab_panel_t
{
private:
	// since waytypes may change during timeline
	waytype_t tabs_to_waytype[9];

public:
	gui_waytype_tab_panel_t() { gui_tab_panel_t(); }

	// all tabs habe the same gui_component with this!
	void init_tabs(gui_component_t *c);

	waytype_t get_active_tab_waytype() const { return tabs_to_waytype[get_active_tab_index()]; }

	void set_active_tab_waytype(waytype_t wt);

	haltestelle_t::stationtyp get_active_tab_stationtype() const;

	waytype_t get_tab_waytype(int i) const { return 0<=i  &&  (uint32)i<get_count() ? tabs_to_waytype[i] : invalid_wt; }
};

#endif
