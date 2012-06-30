/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
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
#include "../simtypes.h"

class depot_t;
class vehikel_besch_t;
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
	int	icnv;

	/**
	 * The previous convoy being displayed
	 * @author Knightly
	 */
	convoihandle_t prev_cnv;

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
	button_t bt_prev;
	gui_textinput_t inp_name;
	gui_label_t lb_convois;
	button_t bt_next;

	gui_label_t lb_convoi_count;
	gui_label_t lb_convoi_speed;
	gui_label_t lb_convoi_value;
	gui_label_t lb_convoi_line;

	button_t bt_start;
	button_t bt_schedule;
	button_t bt_destroy;
	button_t bt_sell;

	button_t bt_obsolete;
	button_t bt_show_all;

	gui_tab_panel_t tabs;
	gui_divider_t   div_tabbottom;

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

	static char no_line_text[128];
	gui_combobox_t line_selector;

	gui_combobox_t vehicle_filter;
	gui_label_t lb_vehicle_filter;

	gui_image_t img_bolt;

	linehandle_t selected_line;

	char txt_convois[40];

	char txt_cnv_name[118];
	char txt_old_cnv_name[118];

	char txt_convoi_count[120];
	char txt_convoi_value[80];
	char txt_convoi_speed[80];
	char txt_convoi_line[128];

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
	void draw_vehicle_info_text(koord pos);

	/**
	 * Calulate the values of the vehicles of the given type owned by the
	 * player.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	sint32 calc_restwert(const vehikel_besch_t *veh_type);

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
	void image_from_convoi_list(uint nr);

	void image_from_storage_list(gui_image_list_t::image_data_t *bild_data);

	karte_t* get_welt() { return depot->get_welt(); }

public:
	// the next two are only needed for depot_t update notifications
	void activate_convoi( convoihandle_t cnv );

	/**
	 * Do the dynamic dialog layout
	 * @author Volker Meyer
	 * @date  18.06.2003
	 */
	void layout(koord *);

	/**
	 * Update texts, image lists and buttons according to the current state.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void update_data();

	/**
	 * Reset convoy name
	 * @author Knightly
	 */
	void reset_convoy_name(convoihandle_t cnv);

	/**
	 * Rename the convoy
	 * @author Knightly
	 */
	void rename_convoy(convoihandle_t cnv);

	// more general functions ...
	depot_frame_t(depot_t* depot);
	~depot_frame_t();

	/**
	 * Setzt die Fenstergroesse
	 * @author (Mathew Hounsell)
	 * @date   11-Mar-2003
	 */
	void set_fenstergroesse(koord groesse);

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
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
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

	/**
	 * Open dialog for schedule entry.
	 * @author Hj. Malthaner
	 */
	void fahrplaneingabe();

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Zeichnet das Frame
	 * @author Hansjörg Malthaner
	 */
	void zeichnen(koord pos, koord gr);

	// @author hsiegeln
	void apply_line();

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
