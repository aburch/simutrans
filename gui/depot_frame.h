/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_DEPOT_FRAME_H
#define GUI_DEPOT_FRAME_H


#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/gui_textinput.h"
#include "components/gui_combobox.h"
#include "components/gui_button.h"
#include "components/action_listener.h"
#include "components/gui_scrollpane.h"
#include "components/gui_convoy_assembler.h"
#include "../simtypes.h"
#include "../simdepot.h"
#include "components/gui_speedbar.h"
#include "../simtypes.h"
#include "../utils/cbuffer_t.h"

class vehicle_desc_t;

/**
 * Depot frame, handles all interaction with a vehicle depot.
 */
class depot_frame_t : public gui_frame_t,
                      public action_listener_t
{
    friend class gui_convoy_assembler_t;

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
	 * Gui elements
	 */
	gui_label_t lb_convois;

	/// contains the current translation of "new convoi"
	const char* new_convoy_text;
	gui_combobox_t convoy_selector;

	button_t line_button;	// goto line ...

	gui_label_t lb_convoi_line;

//	gui_speedbar_t sb_convoi_length;
//	sint32 convoi_length_ok_sb, convoi_length_slower_sb, convoi_length_too_slow_sb, convoi_tile_length_sb, new_vehicle_length_sb;

	button_t bt_start;
	button_t bt_schedule;
	button_t bt_destroy;
	button_t bt_sell;
	button_t bt_details;

	cbuffer_t txt_convoi_cost;

	/**
	 * buttons for new route-management
	 */
//	button_t bt_new_line;
//	button_t bt_change_line;
	button_t bt_copy_convoi;
//	button_t bt_apply_line;

	// line selector stuff
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
	const char* line_separator;

	gui_combobox_t line_selector;
	button_t filter_btn_all_pas, filter_btn_all_mails, filter_btn_all_freights;
	// rebuild the line selector
	void build_line_list();
	// pas=1, mail=2, freight=3
	uint8 line_type_flags = 0;

	gui_convoy_assembler_t convoy_assembler;

	gui_image_t img_bolt;

	linehandle_t selected_line, last_selected_line;

	cbuffer_t txt_convois;

	/**
	 * Calculate the values of the vehicles of the given type owned by the
	 * player.
	 */
	sint64 calc_sale_value(const vehicle_desc_t *veh_type);

	/**
	 * Does this window need a min size button in the title bar?
	 * @return true if such a button is needed
	 */
	bool has_min_sizer() const OVERRIDE {return true;}

	// true if already stored here
	bool is_contained(const vehicle_desc_t *info);

	/// add a single vehicle (helper function)
	void add_to_vehicle_list(const vehicle_desc_t *info);

	/// for convoi image
	void image_from_convoi_list(uint nr, bool to_end);

	void image_from_storage_list(gui_image_list_t::image_data_t *image_data);

public:
	// the next two are only needed for depot_t update notifications
	void activate_convoi( convoihandle_t cnv );

	int get_icnv() const { return icnv; }

	/**
	 * Do the dynamic dialog layout
	 */
	void layout(scr_size *);

	/**
	 * Update texts, image lists and buttons according to the current state.
	 */
	void update_data();

	// more general functions ...
	depot_frame_t(depot_t* depot);

	/**
	 * Set the window size
	 */
	void set_windowsize(scr_size size) OVERRIDE;

	/**
	 * Create and fill loks_vec and waggons_vec.
	 */
	inline void build_vehicle_lists() { convoy_assembler.build_vehicle_lists(); }

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

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
	inline depot_t *get_depot() const {return depot;}
	inline convoihandle_t get_convoy() const {return depot->get_convoi(icnv);}
	inline void update_convoy() {icnv<0?convoy_assembler.clear_convoy():convoy_assembler.set_vehicles(get_convoy());}
	// Check the electrification
	bool check_way_electrified(bool init = false);

	void set_resale_value(uint32 nominal_cost = 0, sint64 resale_value = 0);
};

#endif
