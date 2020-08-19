/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CONVOI_DETAIL_T_H
#define GUI_CONVOI_DETAIL_T_H


#include "gui_frame.h"
#include "components/gui_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_speedbar.h"
#include "components/gui_button.h"
#include "components/gui_label.h"                  // 09-Dec-2001      Markus Weber    Added
#include "components/gui_combobox.h"
#include "components/gui_tab_panel.h"
#include "components/action_listener.h"
#include "../convoihandle_t.h"
#include "../gui/simwin.h"
#include "components/gui_convoy_formation.h"

class scr_coord;

/**
 * One element of the vehicle list display
 * @author prissi
 */
class gui_vehicleinfo_t : public gui_container_t
{
private:
	/**
	 * Handle the convoi to be displayed.
	 * @author Hj. Malthaner
	 */
	convoihandle_t cnv;

	gui_combobox_t class_selector;

	slist_tpl<gui_combobox_t *> class_selectors;

	vector_tpl<uint16> class_indices;

public:
	/**
	 * @param cnv, the handler for displaying the convoi.
	 * @author Hj. Malthaner
	 */
	gui_vehicleinfo_t(convoihandle_t cnv);

	void set_cnv( convoihandle_t c ) { cnv = c; }

	/**
	 * Draw the component
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord offset);
};

// content of payload info tab
class gui_convoy_payload_info_t : public gui_container_t
{
private:
	convoihandle_t cnv;
	bool show_detail = true;

public:
	gui_convoy_payload_info_t(convoihandle_t cnv);

	void set_cnv(convoihandle_t c) { cnv = c; }
	void set_show_detail(bool yesno) { show_detail = yesno; }

	void draw(scr_coord offset);
	void display_loading_bar(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PIXVAL color, uint16 loading, uint16 capacity, uint16 overcrowd_capacity);
};

// content of maintenance info tab
class gui_convoy_maintenance_info_t : public gui_container_t
{
private:
	convoihandle_t cnv;
	bool any_obsoletes;

public:
	gui_convoy_maintenance_info_t(convoihandle_t cnv);

	void set_cnv(convoihandle_t c) { cnv = c; }

	void draw(scr_coord offset);
};

/**
 * Displays an information window for a convoi
 *
 * @author Hj. Malthaner
 * @date 22-Aug-01
 */
class convoi_detail_t : public gui_frame_t , private action_listener_t
{
public:
	enum sort_mode_t { by_destination=0, by_via=1, by_amount_via=2, by_amount=3, SORT_MODES=4 };

private:
	convoihandle_t cnv;

	gui_scrollpane_t scrolly;
	gui_scrollpane_t scrolly_formation;
	gui_scrollpane_t scrolly_payload_info;
	gui_scrollpane_t scrolly_maintenance;
	gui_vehicleinfo_t veh_info;
	gui_convoy_formation_t formation;
	gui_convoy_payload_info_t payload_info;
	gui_convoy_maintenance_info_t maintenance;
	gui_tab_panel_t tabs;
	gui_container_t cont_payload;

	button_t sale_button;
	button_t withdraw_button;
	button_t retire_button;
	button_t class_management_button;

	button_t display_detail_button;

public:
	convoi_detail_t(convoihandle_t cnv);

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_help_filename() const OVERRIDE {return "convoidetail.txt"; }

	/**
	 * Set window size and adjust component sizes and/or positions accordingly
	 * @author Hj. Malthaner
	 */
	virtual void set_windowsize(scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * called when convoi was renamed
	 */
	void update_data() { set_dirty(); }

	// this constructor is only used during loading
	convoi_detail_t();

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_convoi_detail; }
};

#endif
