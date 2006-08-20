/*
 * gui_textinput.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_components_gui_textinput_h
#define gui_components_gui_textinput_h

#include "../../ifc/gui_komponente.h"
#include "../../tpl/slist_tpl.h"
#include "../ifc/action_listener.h"

struct event_t;

/**
 * Ein einfaches Texteingabefeld. Es hat keinen eigenen Textpuffer,
 * nur einen Zeiger auf den Textpuffer, der von jemand anderem Bereitgestellt
 * werden muss.
 *
 * @date 19-Apr-01
 * @author Hj. Malthaner
 */
class gui_textinput_t : public gui_komponente_t
{
protected:

    /**
     * Der Stringbuffer.
     * @author Hj. Malthaner
     */
    char * text;


    /**
     * Maximallänge des Stringbuffers
     * @author Hj. Malthaner
     */
    int max;

    /**
     * Our listeners.
     * @author Hj. Malthaner
     */
    slist_tpl <action_listener_t *> listeners;


    /**
     * Inform all listeners that an action was triggered.
     * @author Hj. Malthaner
     */
    void call_listeners();

    /**
     * position of text cursor
     * @author hsiegeln
     */
     int cursor_pos;

public:


    /**
     * Add a new listener to this text input field.
     * @author Hj. Malthaner
     */
    void add_listener(action_listener_t * l) {
	listeners.insert( l );
    }


    /**
     * Konstruktor
     *
     * @author Hj. Malthaner
     */
    gui_textinput_t();


    /**
     * Destruktor
     *
     * @author Hj. Malthaner
     */
    ~gui_textinput_t();


    /**
     * Setzt den Textpuffer
     *
     * @author Hj. Malthaner
     */
    void setze_text(char *text, int max);


    /**
     * Holt den Textpuffer
     *
     * @author Hj. Malthaner
     */
    char *gib_text() const {return text;};


    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    void infowin_event(const event_t *);


    /**
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    void zeichnen(koord offset) const;

};

#endif
