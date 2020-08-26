/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_CONVOY_ASSEMBLER_H
#define GUI_COMPONENTS_GUI_CONVOY_ASSEMBLER_H


#include "action_listener.h"
#include "gui_button.h"
#include "gui_combobox.h"
#include "gui_divider.h"
#include "gui_image.h"
#include "gui_image_list.h"
#include "gui_label.h"
#include "gui_scrollpane.h"
#include "gui_tab_panel.h"
#include "gui_speedbar.h"
#include "../gui_theme.h"

#include "gui_container.h"

#include "../../convoihandle_t.h"

#include "../../descriptor/vehicle_desc.h"

#include "gui_action_creator.h"

#include "../../tpl/ptrhashtable_tpl.h"
#include "../../tpl/vector_tpl.h"
#include "../../utils/cbuffer_t.h"

#define VEHICLE_FILTER_RELEVANT 1
#define VEHICLE_FILTER_GOODS_OFFSET 2

class depot_convoi_capacity_t : public gui_container_t
{
private:
	uint32 total_pax;
	uint32 total_standing_pax;
	uint32 total_mail;
	uint32 total_goods;

	uint8 total_pax_classes;
	uint8 total_mail_classes;

	int good_type_0 = -1;
	int good_type_1 = -1;
	int good_type_2 = -1;
	int good_type_3 = -1;
	int good_type_4 = -1;

	uint32 good_type_0_amount;
	uint32 good_type_1_amount;
	uint32 good_type_2_amount;
	uint32 good_type_3_amount;
	uint32 good_type_4_amount;
	uint32 rest_good_amount;

	uint8 highest_catering;
	bool is_tpo;

	// The selected convoy so far...
	vector_tpl<const vehicle_desc_t *> vehicles;
public:
	depot_convoi_capacity_t();
	void set_totals(uint32 pax, uint32 standing_pax, uint32 mail, uint32 goods, uint8 pax_classes, uint8 mail_classes, int good_0, int good_1, int good_2, int good_3, int good_4, uint32 good_0_amount, uint32 good_1_amount, uint32 good_2_amount, uint32 good_3_amount, uint32 good_4_amount, uint32 rest_good, uint8 catering, bool tpo);
	void draw(scr_coord offset);
};

/**
 * This class allows the player to assemble a convoy from vehicles.
 * The code was extracted from depot_frame_t and adapted by isidoro
 *   in order to be used elsewhere if needed (Jan-09).
 * The author markers of the original code have been preserved when
 *   possible.
 *
 * @author Hansjörg Malthaner
 * @date 22-Nov-01
 */
class gui_convoy_assembler_t :
	public gui_container_t,
	public gui_action_creator_t,
	public action_listener_t
{
	/* show retired vehicles (same for all depot)
	* @author prissi
	*/
	static bool show_retired_vehicles;

	/* show retired vehicles (same for all depot)
	* @author prissi
	*/
	static bool show_all;

	/* show outdated vehicles (same for all depot)
	*  outdated means production is stopped but not increase maintenance yet.
	*/
	static bool show_outdated_vehicles;

	/**
	 * Parameters to determine layout and behaviour of convoy images.
	 * Originally in simdepot.h.  Based in the code of:
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	static scr_coord get_placement(waytype_t wt);
	static scr_coord get_grid(waytype_t wt);

	waytype_t way_type;
	bool way_electrified;
	static class karte_ptr_t welt;

	// The selected convoy so far...
	vector_tpl<const vehicle_desc_t *> vehicles;

	// Last changed vehicle (added/deleted)
	const vehicle_desc_t *last_changed_vehicle;

	// If this is used for a depot, which depot_frame manages, else NULL
	class depot_frame_t *depot_frame;
	class replace_frame_t *replace_frame;

	/* Gui parameters */
	scr_coord placement;	// ...of first vehicle image
	sint32 placement_dx;
	scr_coord grid;		    // Offsets for adjacent vehicle images
	sint32 grid_dx;		// Horizontal offset adjustment for vehicles in convoy
	uint32 max_convoy_length;
	sint32 panel_rows;
	sint32 convoy_tabs_skip;

	/* Gui elements */
	gui_label_t lb_convoi_number;
	gui_label_t lb_convoi_count;
	gui_label_t lb_convoi_count_fluctuation;
	gui_label_t lb_convoi_tiles;
	gui_label_t lb_convoi_speed;
	gui_label_t lb_convoi_cost;
	gui_label_t lb_convoi_maintenance;
	gui_label_t lb_convoi_power;
	gui_label_t lb_convoi_weight;
	gui_label_t lb_convoi_brake_force;
	gui_label_t lb_convoi_axle_load;
	gui_label_t lb_convoi_way_wear_factor;
	gui_label_t lb_convoi_line;
	// Specifies the traction types handled by
	// this depot.
	// @author: jamespetts, April 2010
	gui_label_t lb_traction_types;
	gui_label_t lb_vehicle_count;
	// Display the load
	//gui_container_t cont_convoi_capacity;

	depot_convoi_capacity_t cont_convoi_capacity;

	gui_tile_occupancybar_t tile_occupancy;
	sint8 new_vehicle_length;
	//sint32 convoi_length_ok_sb, convoi_length_slower_sb, convoi_length_too_slow_sb, convoi_tile_length_sb, new_vehicle_length_sb;

	button_t bt_outdated;
	button_t bt_obsolete;
	button_t bt_show_all;

	gui_tab_panel_t tabs;
	gui_divider_t   div_tabbottom;

	gui_label_t lb_veh_action;
	gui_combobox_t action_selector;

	gui_label_t lb_too_heavy_notice;

	gui_label_t lb_livery_selector;
	gui_label_t lb_livery_counter;
	gui_combobox_t livery_selector;

	button_t bt_class_management;

	vector_tpl<gui_image_list_t::image_data_t*> convoi_pics;
	gui_image_list_t convoi;

	vector_tpl<gui_image_list_t::image_data_t*> pas_vec;
	vector_tpl<gui_image_list_t::image_data_t*> pas2_vec;
	vector_tpl<gui_image_list_t::image_data_t*> electrics_vec;
	vector_tpl<gui_image_list_t::image_data_t*> loks_vec;
	vector_tpl<gui_image_list_t::image_data_t*> waggons_vec;

	gui_image_list_t pas;
	gui_image_list_t pas2;
	gui_image_list_t electrics;
	gui_image_list_t loks;
	gui_image_list_t waggons;
	gui_scrollpane_t scrolly_pas;
	gui_scrollpane_t scrolly_pas2;
	gui_scrollpane_t scrolly_electrics;
	gui_scrollpane_t scrolly_loks;
	gui_scrollpane_t scrolly_waggons;
	gui_container_t cont_pas;
	gui_container_t cont_pas2;
	gui_container_t cont_electrics;
	gui_container_t cont_loks;
	gui_container_t cont_waggons;

	gui_combobox_t vehicle_filter;
	gui_label_t lb_vehicle_filter;

	cbuffer_t txt_convoi_number;
	cbuffer_t txt_convoi_count;
	cbuffer_t txt_convoi_tiles;
	cbuffer_t txt_convoi_maintenance;
	cbuffer_t txt_convoi_speed;
	cbuffer_t txt_convoi_cost;
	cbuffer_t txt_convoi_power;
	cbuffer_t txt_convoi_weight;
	cbuffer_t txt_convoi_brake_force;
	cbuffer_t tooltip_convoi_rolling_resistance;
	cbuffer_t txt_convoi_way_wear_factor;
	cbuffer_t txt_traction_types;
	cbuffer_t txt_vehicle_count;
	cbuffer_t txt_livery_count;
	cbuffer_t tooltip_convoi_acceleration;
	cbuffer_t tooltip_convoi_brake_distance;
	cbuffer_t tooltip_convoi_speed;
	cbuffer_t text_convoi_axle_load;
	char txt_convoi_count_fluctuation[6];

	KOORD_VAL second_column_x; // x position of the second text column

	enum { va_append, va_insert, va_sell, va_upgrade };
	uint8 veh_action;

	// text for the tabs is defaulted to the train names
	static const char * get_electrics_name(waytype_t wt);
	static const char * get_passenger_name(waytype_t wt);
	static const char * get_zieher_name(waytype_t wt);
	static const char * get_haenger_name(waytype_t wt);
	static const char * get_passenger2_name(waytype_t wt);

	/**
	 * A helper map to update loks_vec and waggons_Vec. All entries from
	 * loks_vec and waggons_vec are referenced here.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	typedef ptrhashtable_tpl<vehicle_desc_t const*, gui_image_list_t::image_data_t*> vehicle_image_map;
	vehicle_image_map vehicle_map;

	/**
	 * Draw the info text for the vehicle the mouse is over - if any.
	 * @author Volker Meyer, Hj. Malthaner
	 * @date  09.06.2003
	 * @update 09-Jan-04
	 */
	void draw_vehicle_info_text(const scr_coord& pos);

	// for convoi image
	void image_from_convoi_list(uint nr);

	void image_from_storage_list(gui_image_list_t::image_data_t* image_data);

	// add a single vehicle (helper function)
	void add_to_vehicle_list(const vehicle_desc_t *info);

	//static const sint16 VINFO_HEIGHT = 186 + 14;
	static const sint16 VINFO_HEIGHT = 300/*250*/;

	static uint16 livery_scheme_index;
	vector_tpl<uint16> livery_scheme_indices;
	//vector_tpl<uint16> cs_pas_0_indicies;
	vector_tpl<uint16> cs_pass_indicies;

	/* color bars for current convoi: */
	void init_convoy_color_bars(vector_tpl<const vehicle_desc_t *>*vehs);
	void set_vehicle_bar_shape(gui_image_list_t::image_data_t *pic, const vehicle_desc_t *v);

public:
	// Last selected vehicle filter
	static int selected_filter;

	// Used for listeners to know what has happened
	enum { clear_convoy_action, remove_vehicle_action, insert_vehicle_in_front_action, append_vehicle_action };

	enum { u_buy, u_upgrade };

	gui_convoy_assembler_t(waytype_t wt, signed char player_nr, bool electrified = true);
	void clear_vectors();
	virtual ~gui_convoy_assembler_t();
	/**
	 * Create and fill loks_vec and waggons_vec.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void build_vehicle_lists();

	/**
	 * Do the dynamic component layout
	 * @author Volker Meyer
	 * @date  18.06.2003
	 */
	void layout();

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *comp, value_t extra);

	/**
	 * Update texts, image lists and buttons according to the current state.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void update_data();
	void update_tabs();

	/* The gui_component_t interface */
	virtual void draw(scr_coord offset);

	bool infowin_event(const event_t *ev);

	inline void clear_convoy() {vehicles.clear();}

	inline void remove_vehicle_at(sint32 n) {vehicles.remove_at(n);}
	inline void append_vehicle(const vehicle_desc_t* vb, bool in_front) {vehicles.insert_at(in_front?0:vehicles.get_count(),vb);}

	/* Getter/setter methods */

	inline const vehicle_desc_t *get_last_changed_vehicle() const {return last_changed_vehicle;}

	inline void set_depot_frame(depot_frame_t *df) {depot_frame=df;}
	inline void set_replace_frame(replace_frame_t *rf) {replace_frame=rf;}

	inline vector_tpl<const vehicle_desc_t *>* get_vehicles() {return &vehicles;}
	inline const vector_tpl<gui_image_list_t::image_data_t* >* get_convoi_pics() const { return &convoi_pics; }
	void set_vehicles(convoihandle_t cnv);
	void set_vehicles(const vector_tpl<const vehicle_desc_t *>* vv);

	inline void set_convoy_tabs_skip(sint32 skip) {convoy_tabs_skip=skip;}

	inline sint16 get_convoy_clist_width() const {return (vehicles.get_count() < 24 ? 24 : vehicles.get_count()) * (grid.x - grid_dx) + 2 * gui_image_list_t::BORDER;}

	inline sint16 get_convoy_image_width() const {return get_convoy_clist_width() + placement_dx;}

	inline sint16 get_convoy_image_height() const {return grid.y + 2 * gui_image_list_t::BORDER;}

	inline sint16 get_convoy_height() const {return get_convoy_image_height() + LINESPACE * 5 + 6;}

	inline sint16 get_vinfo_height() const { return VINFO_HEIGHT; }

	void set_panel_rows(sint32 dy);

	inline sint16 get_panel_height() const {return (panel_rows * grid.y + D_TAB_HEADER_HEIGHT + 2 * gui_image_list_t::BORDER) - 4;}

	inline sint16 get_min_panel_height() const {return grid.y + D_TAB_HEADER_HEIGHT + 2 * gui_image_list_t::BORDER;}

	inline int get_height() const {return get_convoy_height() + convoy_tabs_skip + 8 + get_vinfo_height() + 23 + get_panel_height();}

	inline int get_min_height() const {return get_convoy_height() + convoy_tabs_skip + 8 + get_vinfo_height() + 23 + get_min_panel_height();}

	void set_electrified( bool ele );

	inline uint8 get_upgrade() const { return veh_action == va_upgrade; }
	inline uint8 get_action() const { return veh_action; }

	static uint16 get_livery_scheme_index() { return livery_scheme_index; }

	void set_traction_types(const char *traction_types_text) { txt_traction_types.clear(); txt_traction_types.append(traction_types_text); }

	inline void draw_vehicle_bar_help(scr_coord offset);
};

#endif
