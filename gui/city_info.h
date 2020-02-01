/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CITY_INFO_H
#define GUI_CITY_INFO_H


#include "../simcity.h"
#include "../gui/simwin.h"

#include "gui_frame.h"
#include "components/gui_chart.h"
#include "components/gui_textinput.h"
#include "components/action_listener.h"
#include "components/gui_label.h"
#include "components/gui_button.h"
#include "components/gui_tab_panel.h"
#include "../tpl/array2d_tpl.h"

class stadt_t;
template <class T> class sparse_tpl;

/**
 * Presents a window with city information
 *
 * @author Hj. Malthaner
 */
class city_info_t : public gui_frame_t, private action_listener_t
{
private:
	char name[256], old_name[256];

	stadt_t *city;

	scr_size minimaps_size; // size of minimaps
	scr_coord minimap2_offset; // position offset of second minimap

    button_t allow_growth;

	gui_textinput_t name_input;

	gui_tab_panel_t year_month_tabs;

	gui_chart_t chart, mchart;

	button_t filterButtons[MAX_CITY_HISTORY];

	array2d_tpl<PIXVAL> pax_dest_old, pax_dest_new;

	uint32 pax_destinations_last_change;

	void init_pax_dest( array2d_tpl<PIXVAL> &pax_dest );
	void add_pax_dest( array2d_tpl<PIXVAL> &pax_dest, const sparse_tpl<PIXVAL>* city_pax_dest );

	void rename_city();

	void reset_city_name();

public:
	city_info_t(stadt_t *city);

	virtual ~city_info_t();

	const char *get_help_filename() const OVERRIDE {return "citywindow.txt";}

	virtual koord3d get_weltpos(bool) OVERRIDE;

	virtual bool is_weltpos() OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void map_rotate90( sint16 ) OVERRIDE;

	// since we need to update the city pointer when topped
	bool infowin_event(event_t const*) OVERRIDE;

	void update_data();

	/**
	 * Does this window need a min size button in the title bar?
	 * @return true if such a button is needed
	 * @author Hj. Malthaner
	 */
	virtual bool has_min_sizer() const OVERRIDE {return true;}

	/**
	 * resize window in response to a resize event
	 */
	void resize(const scr_coord delta) OVERRIDE;

	// this constructor is only used during loading
	city_info_t();

	void rdwr(loadsave_t *) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_city_info_t; }
};

#endif
