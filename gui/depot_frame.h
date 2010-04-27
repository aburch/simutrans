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
#include "components/gui_textinput.h"
#include "components/gui_combobox.h"
#include "components/gui_button.h"
#include "components/action_listener.h"
#include "components/gui_scrollpane.h"
#include "components/gui_convoy_assembler.h"
#include "../simtypes.h"
#include "../simdepot.h"

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
	 * Gui elements
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	button_t bt_prev;
	gui_textinput_t inp_name;
	gui_label_t lb_convois;
	button_t bt_next;

	gui_label_t lb_convoi_value;
	gui_label_t lb_convoi_line;

	button_t bt_start;
	button_t bt_schedule;
	button_t bt_destroy;
	button_t bt_sell;

	/**
	 * buttons for new route-management
	 * @author hsiegeln
	 */
	button_t bt_new_line;
	button_t bt_change_line;
	button_t bt_copy_convoi;
	button_t bt_apply_line;

	// Specifies the traction types handled by
	// this depot.
	// @author: jamespetts, April 2010
	gui_label_t lb_traction_types;

	static char no_line_text[128];
	gui_combobox_t line_selector;

	gui_convoy_assembler_t convoy_assembler;

	gui_image_t img_bolt;

	linehandle_t selected_line;

	/**
	 * Data fields for use with gui elements.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	char txt_title[60];

	char txt_convois[40];

	char txt_cnv_name[118];

	char txt_convoi_value[40];
	char txt_convoi_line[128];

	char txt_traction_types[256];

	/**
	 * Does this window need a min size button in the title bar?
	 * @return true if such a button is needed
	 * @author Hj. Malthaner
	 */
	bool has_min_sizer() const {return true;}

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

	// more general functions ...
	depot_frame_t(depot_t* depot);

	/**
	 * Setzt die Fenstergroesse
	 * @author (Mathew Hounsell)
	 * @date   11-Mar-2003
	 */
	void set_fenstergroesse(koord groesse);

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

	/**
	 * Open dialog for schedule entry.
	 * @author Hj. Malthaner
	 */
	void fahrplaneingabe();

	void infowin_event(const event_t *ev);

	/**
	 * Zeichnet das Frame
	 * @author Hansjörg Malthaner
	 */
	void zeichnen(koord pos, koord gr);

	// @author hsiegeln
	void apply_line();

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);
	inline depot_t *get_depot() const {return depot;}
	inline convoihandle_t get_convoy() const {return depot->get_convoi(icnv);}
	inline void update_convoy() {icnv<0?convoy_assembler.clear_convoy():convoy_assembler.set_vehicles(get_convoy());}
	inline void build_vehicle_lists() { convoy_assembler.build_vehicle_lists(); }
	// Check the electrification
	bool check_way_electrified(bool init = false);
	int get_icnv() const { return icnv; }
};

#endif
