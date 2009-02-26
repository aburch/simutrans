#ifndef gui_convoy_assembler_h
#define gui_convoy_assembler_h

#include "action_listener.h"
#include "gui_button.h"
#include "gui_combobox.h"
#include "gui_divider.h"
#include "gui_image_list.h"
#include "gui_label.h"
#include "gui_scrollpane.h"
#include "gui_tab_panel.h"

#include "../gui_container.h"

#include "../../convoihandle_t.h"

#include "../../besch/vehikel_besch.h"

#include "../../ifc/gui_action_creator.h"
#include "../../ifc/gui_komponente.h"

#include "../../tpl/ptrhashtable_tpl.h"
#include "../../tpl/vector_tpl.h"


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

	/**
	 * Parameters to determine layout and behaviour of convoy images.
	 * Originally in simdepot.h.  Based in the code of:
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	static koord get_placement(waytype_t wt);
	static koord get_grid(waytype_t wt);

	waytype_t way_type;
	const bool weg_electrified;
	class karte_t *welt;

	// The selected convoy so far...
	vector_tpl<const vehikel_besch_t *> vehicles;

	// Last changed vehicle (added/deleted)
	const vehikel_besch_t *last_changed_vehicle;


	// If this is used for a depot, which depot_frame manages, else NULL
	class depot_frame_t *depot_frame;

	/* Gui parameters */
	koord placement;	// ...of first vehicle image
	int placement_dx;
	koord grid;		// Offsets for adjacent vehicle images
	int grid_dx;		// Horizontal offset adjustment for vehicles in convoy
	unsigned int max_convoy_length;
	int panel_rows;
	int convoy_tabs_skip;

	/* Gui elements */
	gui_label_t lb_convoi_count;
	gui_label_t lb_convoi_speed;

	button_t bt_obsolete;
	button_t bt_show_all;

	gui_tab_panel_t tabs;
	gui_divider_t   div_tabbottom;

	gui_label_t lb_veh_action;
	gui_combobox_t action_selector;

	vector_tpl<gui_image_list_t::image_data_t> convoi_pics;
	gui_image_list_t convoi;

	vector_tpl<gui_image_list_t::image_data_t> pas_vec;
	vector_tpl<gui_image_list_t::image_data_t> electrics_vec;
	vector_tpl<gui_image_list_t::image_data_t> loks_vec;
	vector_tpl<gui_image_list_t::image_data_t> waggons_vec;

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

	char txt_convoi_count[40];
	char txt_convoi_speed[80];

	enum { va_append, va_insert, va_sell };
	uint8 veh_action;

	// text for the tabs is defaulted to the train names
	static const char * get_electrics_name(waytype_t wt);
	static const char * get_passenger_name(waytype_t wt);
	static const char * get_zieher_name(waytype_t wt);
	static const char * get_haenger_name(waytype_t wt);

	/**
	 * A helper map to update loks_vec and waggons_Vec. All entries from
	 * loks_vec and waggons_vec are referenced here.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	ptrhashtable_tpl<const vehikel_besch_t *, gui_image_list_t::image_data_t *> vehicle_map;

	/**
	 * Draw the info text for the vehicle the mouse is over - if any.
	 * @author Volker Meyer, Hj. Malthaner
	 * @date  09.06.2003
	 * @update 09-Jan-04
	 */
	void draw_vehicle_info_text(koord pos);

	// for convoi image
	void image_from_convoi_list(uint nr);

	void image_from_storage_list(gui_image_list_t::image_data_t *bild_data);

	// add a single vehicle (helper function)
	void add_to_vehicle_list(const vehikel_besch_t *info);

	static const int VINFO_HEIGHT = 186;

public:
	// Used for listeners to know what has happened
	enum { clear_convoy_action, remove_vehicle_action, insert_vehicle_in_front_action, append_vehicle_action };

	gui_convoy_assembler_t(karte_t *w, waytype_t wt, bool electrified, signed char player_nr );

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
	bool action_triggered( gui_action_creator_t *komp, value_t extra);

	/**
	 * Update texts, image lists and buttons according to the current state.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void update_data();

	/* The gui_komponente_t interface */
	virtual void zeichnen(koord offset);

	void infowin_event(const event_t *ev);

	inline void clear_convoy() {vehicles.clear();}

	inline void remove_vehicle_at(int n) {vehicles.remove_at(n);}
	inline void append_vehicle(const vehikel_besch_t* vb, bool in_front) {vehicles.insert_at(in_front?0:vehicles.get_count(),vb);}

	/* Getter/setter methods */

	inline const vehikel_besch_t *get_last_changed_vehicle() const {return last_changed_vehicle;}

	inline karte_t *get_welt() const {return welt;}

	inline void set_depot_frame(depot_frame_t *df) {depot_frame=df;}

	inline const vector_tpl<const vehikel_besch_t *>* get_vehicles() const {return &vehicles;}
	void set_vehicles(convoihandle_t cnv);
	void set_vehicles(const vector_tpl<const vehikel_besch_t *>* vv);

	inline void set_convoy_tabs_skip(int skip) {convoy_tabs_skip=skip;}

	inline int get_convoy_clist_width() const {return vehicles.get_count() * (grid.x - grid_dx) + 2 * gui_image_list_t::BORDER;}

	inline int get_convoy_image_width() const {return get_convoy_clist_width() + placement_dx;}

	inline int get_convoy_image_height() const {return grid.y + 2 * gui_image_list_t::BORDER;}

	inline int get_convoy_height() const {
		int CINFO_HEIGHT = 14;
		return get_convoy_image_height() + CINFO_HEIGHT + 2 + LINESPACE * 2;
	}

	inline int get_vinfo_height() const { return VINFO_HEIGHT; }

	inline void set_panel_rows(int dy) {
		if (dy==-1) {
			panel_rows=3;
			return;
		}
		dy -= get_convoy_height() + convoy_tabs_skip + 8 + get_vinfo_height() + 17 + gui_tab_panel_t::HEADER_VSIZE + 2 * gui_image_list_t::BORDER;
		panel_rows = max(1, (dy/grid.y) );
	}

	inline int get_panel_height() const {return panel_rows * grid.y + gui_tab_panel_t::HEADER_VSIZE + 2 * gui_image_list_t::BORDER;}

	inline int get_min_panel_height() const {return grid.y + gui_tab_panel_t::HEADER_VSIZE + 2 * gui_image_list_t::BORDER;}

	inline int get_height() const {return get_convoy_height() + convoy_tabs_skip + 8 + get_panel_height() + get_vinfo_height();}

	inline int get_min_height() const {return get_convoy_height() + convoy_tabs_skip + 8 + get_min_panel_height() + get_vinfo_height();}
};

#endif