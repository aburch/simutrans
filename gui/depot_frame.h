/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * The depot window, where to buy convois
 */

#ifndef gui_depot_frame2_t_h
#define gui_depot_frame2_t_h

#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/gui_image.h"
#include "components/gui_image_list.h"
#include "components/gui_textinput.h"
#include "components/gui_combobox.h"
#include "components/gui_divider.h"
#include "../tpl/ptrhashtable_tpl.h"
#include "../tpl/vector_tpl.h"
#include "components/gui_tab_panel.h"
#include "components/gui_button.h"
#include "components/action_listener.h"
#include "components/gui_scrollpane.h"
#include "components/gui_speedbar.h"
#include "../simtypes.h"
#include "../utils/cbuffer_t.h"

class depot_t;
class vehikel_besch_t;


class depot_convoi_capacity_t : public gui_container_t
{
private:
	uint32 total_pax;
	uint32 total_mail;
	uint32 total_goods;
public:
	depot_convoi_capacity_t();
	void set_totals(uint32 pax, uint32 mail, uint32 goods);
	void draw(scr_coord offset);
};


/**
 * Depot frame, handles all interaction with a vehicle depot.
 *
 * @author Hansjörg Malthaner
 * @date 22-Nov-01
 */
class depot_frame_t : public gui_frame_t,
                      public action_listener_t
{
private:
	/**
	 * The depot to display
	 * @author Hansjörg Malthaner
	 */
	depot_t *depot;

	/**
	 * The current convoi to display.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	int icnv;

	/* show retired vehicles (same for all depot)
	* @author prissi
	*/
	static bool show_retired_vehicles;

	/* show retired vehicles (same for all depot)
	* @author prissi
	*/
	static bool show_all;

	/**
	 * Gui elements
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	gui_label_t lb_convois;

	/// contains the current translation of "new convoi"
	const char* new_convoy_text;
	gui_combobox_t convoy_selector;

	button_t line_button;	// goto line ...

	gui_label_t lb_convoi_count;
	gui_label_t lb_convoi_speed;
	gui_label_t lb_convoi_cost;
	gui_label_t lb_convoi_value;
	gui_label_t lb_convoi_power;
	gui_label_t lb_convoi_weight;
	gui_label_t lb_convoi_line;

	depot_convoi_capacity_t cont_convoi_capacity;

	gui_speedbar_t sb_convoi_length;
	sint32 convoi_length_ok_sb, convoi_length_slower_sb, convoi_length_too_slow_sb, convoi_tile_length_sb, new_vehicle_length_sb;

	button_t bt_start;
	button_t bt_schedule;
	button_t bt_destroy;
	button_t bt_sell;

	button_t bt_obsolete;
	button_t bt_show_all;

	gui_tab_panel_t tabs;
	gui_divider_t div_tabbottom;

	gui_label_t lb_veh_action;
	button_t bt_veh_action;

	/**
	 * buttons for new route-management
	 * @author hsiegeln
	 */
	button_t bt_new_line;
	button_t bt_change_line;
	button_t bt_copy_convoi;
	button_t bt_apply_line;

	vector_tpl<gui_image_list_t::image_data_t*> convoi_pics;
	gui_image_list_t convoi;

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
	gui_container_t cont_pas;
	gui_container_t cont_electrics;
	gui_container_t cont_loks;
	gui_container_t cont_waggons;

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
	gui_label_t lb_vehicle_filter;

	gui_image_t img_bolt;

	linehandle_t selected_line, last_selected_line;

	cbuffer_t txt_convois;

	cbuffer_t txt_convoi_count;
	cbuffer_t txt_convoi_value;
	cbuffer_t txt_convoi_speed;
	cbuffer_t txt_convoi_cost;
	cbuffer_t txt_convoi_power;
	cbuffer_t txt_convoi_weight;

	scr_coord_val second_column_x; // x position of the second text column

	enum { va_append, va_insert, va_sell };
	uint8 veh_action;

	/**
	 * A helper map to update loks_vec and waggons_Vec. All entries from
	 * loks_vec and waggons_vec are referenced here.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	typedef ptrhashtable_tpl<vehikel_besch_t const*, gui_image_list_t::image_data_t*> vehicle_image_map;
	vehicle_image_map vehicle_map;

	/**
	 * Draw the info text for the vehicle the mouse is over - if any.
	 * @author Volker Meyer, Hj. Malthaner
	 * @date  09.06.2003
	 * @update 09-Jan-04
	 */
	void draw_vehicle_info_text(scr_coord pos);

	/**
	 * Calculate the values of the vehicles of the given type owned by the
	 * player.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	sint64 calc_restwert(const vehikel_besch_t *veh_type);

	/**
	 * Does this window need a min size button in the title bar?
	 * @return true if such a button is needed
	 * @author Hj. Malthaner
	 */
	bool has_min_sizer() const {return true;}

	// true if already stored here
	bool is_contained(const vehikel_besch_t *info);

	// add a single vehicle (helper function)
	void add_to_vehicle_list(const vehikel_besch_t *info);

	// for convoi image
	void image_from_convoi_list(uint nr, bool to_end);

	void image_from_storage_list(gui_image_list_t::image_data_t *bild_data);

public:
	// the next two are only needed for depot_t update notifications
	void activate_convoi( convoihandle_t cnv );

	int get_icnv() const { return icnv; }

	/**
	 * Do the dynamic dialog layout
	 * @author Volker Meyer
	 * @date  18.06.2003
	 */
	void layout(scr_size *);

	/**
	 * Update texts, image lists and buttons according to the current state.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void update_data();

	// more general functions ...
	depot_frame_t(depot_t* depot);

	~depot_frame_t();

	/**
	 * Set the window size
	 * @author (Mathew Hounsell)
	 * @date   11-Mar-2003
	 */
	void set_windowsize(scr_size size);

	/**
	 * Create and fill loks_vec and waggons_vec.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void build_vehicle_lists();

	/*
	 * Will update the tabs (don't show empty ones).
	 * @author Gerd Wachsmuth
	 * @date 08.05.2009
	 */
	void update_tabs();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const {return "depot.txt";}

	/**
	 * Does this window need a next button in the title bar?
	 * @return true if such a button is needed
	 * @author Volker Meyer
	 */
	bool has_next() const {return true;}

	virtual koord3d get_weltpos(bool);
	virtual bool is_weltpos();

	/**
	 * Open dialog for schedule entry.
	 * @author Hj. Malthaner
	 */
	void fahrplaneingabe();

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw the Frame
	 * @author Hansjörg Malthaner
	 */
	void draw(scr_coord pos, scr_size size);

	// @author hsiegeln
	void apply_line();

	void set_selected_line(linehandle_t line) { selected_line = line; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
