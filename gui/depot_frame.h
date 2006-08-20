/*
 * depot_frame.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef gui_depot_frame2_t_h
#define gui_depot_frame2_t_h

#include "gui_frame.h"
#include "gui_label.h"
#include "image_list.h"
#include "components/gui_textinput.h"
#include "components/gui_combobox.h"
#include "components/gui_divider.h"
#include "../tpl/vector_tpl.h"
#include "tab_panel.h"
#include "button.h"
#include "ifc/image_list_listener.h"
#include "ifc/action_listener.h"
#include "gui_scrollpane.h"
#include "../simtypes.h"

class depot_t;
class vehikel_t;
class image_list_t;
class spieler_t;
/**
 * Depot frame, handles all interaction with a vehicle depot.
 *
 * @author Hansjörg Malthaner
 * @date 22-Nov-01
 */
class depot_frame_t : public gui_frame_t,
                      public image_list_listener_t,
                      public action_listener_t
{
private:
    karte_t *welt;

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

	/* show retired vehicles (same for all depot)
	 * @author prissi
	 */
	static bool show_retired_vehicles;

    /**
     * Gui elements
     * @author Volker Meyer
     * @date  09.06.2003
     */
    button_t bt_prev;
    gui_textinput_t inp_name;
    gui_label_t lb_convois;
    button_t bt_next;

    image_list_t *convoi;
    gui_label_t lb_convoi_count;
    gui_label_t lb_convoi_value;
    gui_label_t lb_convoi_line;

    button_t bt_start;
    button_t bt_schedule;
    button_t bt_destroy;
    button_t bt_sell;

    button_t bt_obsolete;

    tab_panel_t tabs;
    gui_divider_t   div_tabbottom;
    image_list_t    *pas;
    image_list_t    *loks;
    image_list_t    *waggons;
    gui_scrollpane_t *scrolly_pas;
    gui_scrollpane_t *scrolly_loks;
    gui_scrollpane_t *scrolly_waggons;
    gui_container_t *cont_pas;
    gui_container_t *cont_loks;
    gui_container_t *cont_waggons;

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

	static char no_line_text[128];
	gui_combobox_t line_selector;

	linehandle_t selected_line;

    /**
     * Data fields for use with gui elements.
     * @author Volker Meyer
     * @date  09.06.2003
     */
    char txt_title[60];

    char txt_convois[40];

    vector_tpl<image_list_t::image_data_t> convoi_pics;
    char txt_convoi_count[40];
    char txt_convoi_value[40];
    char txt_convoi_line[128];

    vector_tpl<image_list_t::image_data_t> *pas_vec;
    vector_tpl<image_list_t::image_data_t> *loks_vec;
    vector_tpl<image_list_t::image_data_t> *waggons_vec;

    enum { va_append, va_insert, va_sell };
    uint8 veh_action;

    /**
     * A helper map to update loks_vec and waggons_Vec. All entries from
     * loks_vec and waggons_vec are referenced here.
     * @author Volker Meyer
     * @date  09.06.2003
     */
    ptrhashtable_tpl<const vehikel_besch_t *, image_list_t::image_data_t *> vehicle_map;

    /**
     * Update texts, image lists and buttons according to the current state.
     * @author Volker Meyer
     * @date  09.06.2003
     */
    void update_data();

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
     * Do the dynamic dialog layout
     * @author Volker Meyer
     * @date  18.06.2003
     */
    void layout(koord *);

    /**
     * Does this window need a min size button in the title bar?
     * @return true if such a button is needed
     * @author Hj. Malthaner
     */
    virtual bool has_min_sizer() const {return true;}

	// true if already stored here
	bool is_contained(const vehikel_besch_t *info);

	// add a single vehicle (helper function)
	void add_to_vehicle_list(const vehikel_besch_t *info);

public:


    depot_frame_t(karte_t *welt, depot_t *depot);

    /**
     * Setzt die Fenstergroesse
     * @author (Mathew Hounsell)
     * @date   11-Mar-2003
     */
    virtual void setze_fenstergroesse(koord groesse);

    /**
     * Create and fill loks_vec and waggons_vec.
     * @author Volker Meyer
     * @date  09.06.2003
     */
    void build_vehicle_lists();

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return "depot.txt";};

    /**
     * Does this window need a next button in the title bar?
     * @return true if such a button is needed
     * @author Volker Meyer
     */
    virtual bool has_next() const {return true;};

    /**
     * Open dialog for schedule entry.
     * @author Hj. Malthaner
     */
    void fahrplaneingabe();

    /**
     * Implementiert einen image_list_listener_t.
     * @author Hj. Malthaner
     */
    virtual void bild_gewaehlt(image_list_t *, int bild_index);

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     */
    virtual bool action_triggered(gui_komponente_t *komp);

    virtual void infowin_event(const event_t *ev);

    /**
     * Zeichnet das Frame
     * @author Hansjörg Malthaner
     */
    void zeichnen(koord pos, koord gr);

    // @author hsiegeln
    void new_line();
    void change_line();
    void apply_line();
};

#endif
