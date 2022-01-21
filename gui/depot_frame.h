/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_DEPOT_FRAME_H
#define GUI_DEPOT_FRAME_H


#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/gui_image.h"
#include "components/gui_image_list.h"
#include "components/gui_textinput.h"
#include "components/gui_combobox.h"
#include "../tpl/ptrhashtable_tpl.h"
#include "../tpl/vector_tpl.h"
#include "components/gui_tab_panel.h"
#include "components/gui_button.h"
#include "components/action_listener.h"
#include "components/gui_scrollpane.h"
#include "components/gui_speedbar.h"
#include "../simtypes.h"
#include "../utils/cbuffer_t.h"
#include "../linehandle_t.h"
#include "../convoihandle_t.h"

class depot_t;
class vehicle_desc_t;
class depot_convoi_capacity_t;
class gui_aligned_container_zero_width_t;
/**
 * Depot frame, handles all interaction with a vehicle depot.
 */
class depot_frame_t : public gui_frame_t, public action_listener_t
{
private:
	/**
	 * The depot to display
	 */
	depot_t *depot;

	/**
	 * The current convoi to display.
	 */
	int icnv;

	/**
	 * show retired vehicles (same for all depot)
	 */
	static bool show_retired_vehicles;

	/**
	 * show retired vehicles (same for all depot)
	 */
	static bool show_all;

	/**
	 * Gui elements
	 */
	gui_label_buf_t lb_convois, lb_convoi_count;

	/// contains the current translation of "new convoi"
	const char* new_convoy_text;
	gui_combobox_t convoy_selector;

	button_t line_button; // goto line ...

	vector_tpl<gui_label_buf_t*> labels;
	gui_aligned_container_t *cont_veh_action;
	gui_aligned_container_zero_width_t *cont_vehicle_labels;

	depot_convoi_capacity_t* cont_convoi_capacity;

	gui_speedbar_fixed_length_t sb_convoi_length;
	sint32 convoi_length_ok_sb, convoi_length_slower_sb, convoi_length_too_slow_sb, convoi_tile_length_sb, new_vehicle_length_sb;

	button_t bt_start;
	button_t bt_schedule;
	button_t bt_destroy;
	button_t bt_sell;

	button_t bt_obsolete;
	button_t bt_show_all;

	gui_combobox_t sort_by;

	static char name_filter_value[64];
	gui_textinput_t name_filter_input;

	gui_tab_panel_t tabs;

	button_t bt_veh_action;

	/**
	 * buttons for new route-management
	 */
	button_t bt_new_line;
	button_t bt_change_line;
	button_t bt_copy_convoi;
	button_t bt_apply_line;

	vector_tpl<gui_image_list_t::image_data_t*> convoi_pics;
	gui_image_list_t convoi;

	gui_aligned_container_t cont_convoi;
	gui_scrollpane_t scrolly_convoi;

	/// image list of passenger cars
	vector_tpl<gui_image_list_t::image_data_t*> pas_vec;
	/// image list of electrified passenger carrier units
	vector_tpl<gui_image_list_t::image_data_t*> electrics_vec;
	/// image list of all other powered vehicles (and vehicles without freight capacity)
	vector_tpl<gui_image_list_t::image_data_t*> loks_vec;
	/// image list of all other cars (freight, non-powered)
	vector_tpl<gui_image_list_t::image_data_t*> waggons_vec;

	gui_image_list_t pas;
	gui_image_list_t electrics;
	gui_image_list_t loks;
	gui_image_list_t waggons;
	gui_scrollpane_t scrolly_pas;
	gui_scrollpane_t scrolly_electrics;
	gui_scrollpane_t scrolly_loks;
	gui_scrollpane_t scrolly_waggons;

	/// contains the current translation of "<no schedule set>"
	const char* no_schedule_text;
	/// contains the current translation of "<clear schedule>"
	const char* clear_schedule_text;
	/// contains the current translation of "<individual schedule>"
	const char* unique_schedule_text;
	/// contains the current translation of "<create new line>"
	const char* new_line_text;
	/// contains the current translation of "<promote to line>"
	const char* promote_to_line_text;
	/// "-----------" between header items and lines
	const char* line_seperator;

	gui_combobox_t line_selector;

	gui_combobox_t vehicle_filter;

	gui_image_t img_bolt;

	linehandle_t selected_line, last_selected_line;

	enum {
		va_append,
		va_insert,
		va_sell
	};
	uint8 veh_action;

	/**
	 * A helper map to update loks_vec and waggons_Vec. All entries from
	 * loks_vec and waggons_vec are referenced here.
	 */
	typedef ptrhashtable_tpl<vehicle_desc_t const*, gui_image_list_t::image_data_t*> vehicle_image_map;
	vehicle_image_map vehicle_map;

	/**
	 * Update the info text for the vehicle the mouse is over - if any.
	 */
	void update_vehicle_info_text(scr_coord pos);

	// cache old values
	uint32 old_vehicle_count;
	const vehicle_desc_t *old_veh_type;

	/**
	 * Calculate the values of the vehicles of the given type owned by the
	 * player.
	 */
	sint64 calc_restwert(const vehicle_desc_t *veh_type);

	/// true if already stored here
	bool is_in_vehicle_list(const vehicle_desc_t *info);

	/// add a single vehicle (helper function)
	void add_to_vehicle_list(const vehicle_desc_t *info);

	/// for convoi image
	void image_from_convoi_list(uint nr, bool to_end);

	void image_from_storage_list(gui_image_list_t::image_data_t *image_data);

	/// initialize everything
	void init(depot_t *depot);

public:
	// the next two are only needed for depot_t update notifications
	void activate_convoi( convoihandle_t cnv );

	int get_icnv() const { return icnv; }

	/**
	 * Update texts, image lists and buttons according to the current state.
	 */
	void update_data();

	// more general functions ...
	depot_frame_t(depot_t* depot = NULL);

	~depot_frame_t();

	/**
	 * Set the window size
	 */
	void set_windowsize(scr_size size) OVERRIDE;

	/**
	 * Create and fill loks_vec and waggons_vec.
	 */
	void build_vehicle_lists();

	/**
	 * Will update the tabs (don't show empty ones).
	 */
	void update_tabs();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char * get_help_filename() const OVERRIDE {return "depot.txt";}

	/**
	 * Does this window need a next button in the title bar?
	 * @return true if such a button is needed
	 */
	bool has_next() const OVERRIDE {return true;}

	koord3d get_weltpos(bool) OVERRIDE;
	bool is_weltpos() OVERRIDE;

	/**
	 * Open dialog for schedule entry.
	 */
	void open_schedule_editor();

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw the Frame
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	void apply_line();

	void set_selected_line(linehandle_t line) { selected_line = line; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE;

	void rdwr( loadsave_t * ) OVERRIDE;
};

#endif
