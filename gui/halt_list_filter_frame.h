/*
 * halt_list_filter_frame.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */
#include "gui_frame.h"
#include "gui_label.h"
#include "ifc/action_listener.h"
#include "button.h"
#include "halt_list_frame.h"
#include "components/gui_textinput.h"

class spieler_t;


/**
 * Displays a filter settings for the halt list
 *
 * @author V. Meyer
 */
class halt_list_filter_frame_t : public gui_frame_t , private action_listener_t
{
private:
    /*
     * Helper class for the entries of the srollable list of goods.
     * Needed since a button_t does not know its parent.
     */
    class ware_item_t : public button_t {
	const ware_besch_t *ware_ab;
	const ware_besch_t *ware_an;
	halt_list_filter_frame_t *parent;
    public:
	ware_item_t(halt_list_filter_frame_t *parent, const ware_besch_t *ware_ab, const ware_besch_t *ware_an)
	{ this->ware_ab = ware_ab; this->ware_an = ware_an; this->parent = parent; }

	virtual void infowin_event(const event_t *ev) {
	    if(IS_LEFTRELEASE(ev)) {
		parent->ware_item_triggered(ware_ab, ware_an);
	    }
	    button_t::infowin_event(ev);
	}
	virtual void zeichnen(koord offset) const {
	    if(ware_ab) {
		const_cast<ware_item_t *>(this)->pressed = parent->gib_ware_filter_ab(ware_ab);
	    }
	    if(ware_an) {
		const_cast<ware_item_t *>(this)->pressed = parent->gib_ware_filter_an(ware_an);
	    }
	    button_t::zeichnen(offset);
	}
    };

    /*
     * As long we do not have resource scripts, we display make
     * some tables for the main attributes of each button.
     */
    enum { FILTER_BUTTONS=12 };

    static koord filter_buttons_pos[FILTER_BUTTONS];
    static halt_list_frame_t::filter_flag_t filter_buttons_types[FILTER_BUTTONS];
    static const char *filter_buttons_text[FILTER_BUTTONS];

    /*
     * We are bound to this window. All filter states are stored in main_frame.
     */
    halt_list_frame_t *main_frame;

    /*
     * All gui elements of this dialog:
     */
    button_t filter_buttons[FILTER_BUTTONS];

    gui_textinput_t name_filter_input;

    button_t typ_filter_enable;

    button_t ware_alle_ab;
    button_t ware_keine_ab;
    button_t ware_invers_ab;

    gui_scrollpane_t ware_scrolly_ab;
    gui_container_t ware_cont_ab;

    button_t ware_alle_an;
    button_t ware_keine_an;
    button_t ware_invers_an;

    gui_scrollpane_t ware_scrolly_an;
    gui_container_t ware_cont_an;

public:
    /**
     * Konstruktor. Erzeugt alle notwendigen Subkomponenten.
     * @author V. Meyer
     */
    halt_list_filter_frame_t(spieler_t *sp, halt_list_frame_t *main_frame);

    /**
     * Destruktor.
     * @author V. Meyer
     */
    ~halt_list_filter_frame_t();

    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author V. Meyer
     */
    virtual void infowin_event(const event_t *ev);

    /**
     * This method is called if an action is triggered
     * @author V. Meyer
     */
    virtual bool action_triggered(gui_komponente_t *);

    /*
     * Propagate funktion from main_frame for ware_item_t
     * @author V. Meyer
     */
    bool gib_ware_filter_ab(const ware_besch_t *ware) const { return main_frame->gib_ware_filter_ab(ware); }
    bool gib_ware_filter_an(const ware_besch_t *ware) const { return main_frame->gib_ware_filter_an(ware); }

    /*
     * Handler for ware_item_t event.
     * @author V. Meyer
     */
    void ware_item_triggered(const ware_besch_t *ware_ab, const ware_besch_t *ware_an);

    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author V. Meyer
     */
    virtual void zeichnen(koord pos, koord gr);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author V. Meyer
     */
    virtual const char * gib_hilfe_datei() const {return "haltlist_filter.txt"; }
};
