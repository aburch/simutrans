/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */
#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/gui_scrollpane.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "convoi_frame.h"
#include "components/gui_textinput.h"

class spieler_t;


/**
 * Displays a filter settings for the convoi list
 *
 * @author V. Meyer
 */
class convoi_filter_frame_t : public gui_frame_t , private action_listener_t
{
private:
	/*
	* Helper class for the entries of the srollable list of goods.
	* Needed since a button_t does not know its parent.
	*/
	class ware_item_t : public button_t
	{
		const ware_besch_t *ware;
		convoi_filter_frame_t *parent;
	public:
		ware_item_t(convoi_filter_frame_t *parent, const ware_besch_t *ware)
		{
			this->ware = ware;
			this->parent = parent;
		}
		virtual void infowin_event(const event_t *ev) {
			if(IS_LEFTRELEASE(ev)) {
				parent->ware_item_triggered(ware);
			}
			button_t::infowin_event(ev);
		}
		virtual void zeichnen(koord offset) {
			pressed = parent->gib_ware_filter(ware);
			button_t::zeichnen(offset);
		}
	};

    /*
     * As long we do not have resource scripts, we display make
     * some tables for the main attributes of each button.
     */
    enum { FILTER_BUTTONS=17 };

    static koord filter_buttons_pos[FILTER_BUTTONS];
    static convoi_frame_t::filter_flag_t filter_buttons_types[FILTER_BUTTONS];
    static const char *filter_buttons_text[FILTER_BUTTONS];

    /*
     * We are bound to this window. All filter states are stored in main_frame.
     */
    convoi_frame_t *main_frame;

    /*
     * All gui elements of this dialog:
     */
    button_t filter_buttons[FILTER_BUTTONS];

    gui_textinput_t name_filter_input;

    button_t typ_filter_enable;

    button_t ware_alle;
    button_t ware_keine;
    button_t ware_invers;

    gui_scrollpane_t ware_scrolly;
    gui_container_t ware_cont;

public:
    /**
     * Konstruktor. Erzeugt alle notwendigen Subkomponenten.
     * @author V. Meyer
     */
    convoi_filter_frame_t(spieler_t *sp, convoi_frame_t *main_frame);

		~convoi_filter_frame_t();

    /*
     * Propagate funktion from main_frame for ware_item_t
     * @author V. Meyer
     */
    bool gib_ware_filter(const ware_besch_t *ware) const { return main_frame->gib_ware_filter(ware); }

    /*
     * Handler for ware_item_t event.
     * @author V. Meyer
     */
    void ware_item_triggered(const ware_besch_t *ware);

    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author V. Meyer
     */
    void zeichnen(koord pos, koord gr);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author V. Meyer
     */
    const char * gib_hilfe_datei() const {return "convoi_filter.txt"; }

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     *
     * Returns true, if action is done and no more
     * components should be triggered.
     * V.Meyer
     */
    bool action_triggered(gui_komponente_t *komp, value_t extra);
};
