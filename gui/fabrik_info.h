/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_FABRIK_INFO_H
#define GUI_FABRIK_INFO_H


#include "../gui/simwin.h"

#include "factory_chart.h"
#include "components/action_listener.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_obj_view_t.h"
#include "components/gui_container.h"
#include "../utils/cbuffer_t.h"
#include "components/gui_speedbar.h"
#include "components/gui_factory_storage_info.h"

class welt_t;
class fabrik_t;
class gebaeude_t;
class button_t;


/**
 * info on city demand

 * @author
 */
class gui_fabrik_info_t : public gui_container_t
{
public:
	const fabrik_t* fab;

	gui_fabrik_info_t() {}

	void draw(scr_coord offset);
};


/**
 * Info window for factories
 * @author Hj. Malthaner
 */
class fabrik_info_t : public gui_frame_t, public action_listener_t
{
private:
	fabrik_t *fab;

	cbuffer_t info_buf, prod_buf;
	cbuffer_t factory_status;

	gui_tab_panel_t tabs;
	static sint16 tabstate;

	factory_goods_chart_t goods_chart;
	factory_chart_t chart;

	gui_label_t lbl_factory_status;
	gui_speedbar_t staffing_bar;
	sint32 staffing_level;
	sint32 staffing_level2;
	sint32 staff_shortage_factor;

	button_t details_button;

	obj_view_t view;

	char fabname[256];
	gui_textinput_t input;

	//button_t *stadtbuttons;

	gui_textarea_t prod, txt;

	gui_factory_storage_info_t storage;

	gui_scrollpane_t scrolly_info, scrolly_details;
	gui_container_t container_info, container_details;
	gui_factory_connection_stat_t all_suppliers, all_consumers;
	gui_label_t lb_suppliers, lb_consumers, lb_nearby_halts;
	gui_factory_nearby_halt_info_t nearby_halts;

	uint32 old_suppliers_count, old_consumers_count, old_stops_count;

	void rename_factory();

	void update_components();

	void set_tab_opened();

public:
	// refreshes all text and location pointers
	void update_info();

	fabrik_info_t(fabrik_t* fab, const gebaeude_t* gb);

	virtual ~fabrik_info_t();

	void init(fabrik_t* fab, const gebaeude_t* gb);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char *get_help_filename() const OVERRIDE {return "industry_info.txt";}

	virtual bool has_min_sizer() const OVERRIDE {return true;}

	virtual koord3d get_weltpos(bool) OVERRIDE { return fab->get_pos(); }

	virtual bool is_weltpos() OVERRIDE;

	virtual void set_windowsize(scr_size size) OVERRIDE;

	/**
	* Draw new component. The values to be passed refer to the window
	* i.e. It's the screen coordinates of the window where the
	* component is displayed.
	* @author Hj. Malthaner
	*/
	virtual void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	bool infowin_event(const event_t *ev) OVERRIDE;

	// rotated map need new info ...
	void map_rotate90(sint16) OVERRIDE;

	// this constructor is only used during loading
	fabrik_info_t();

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_factory_info; }
};

#endif
