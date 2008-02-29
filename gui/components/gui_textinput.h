/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_components_gui_textinput_h
#define gui_components_gui_textinput_h

#include "../../ifc/gui_action_creator.h"


/**
 * Ein einfaches Texteingabefeld. Es hat keinen eigenen Textpuffer,
 * nur einen Zeiger auf den Textpuffer, der von jemand anderem Bereitgestellt
 * werden muss.
 *
 * @date 19-Apr-01
 * @author Hj. Malthaner
 */
class gui_textinput_t : public gui_komponente_action_creator_t
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
    long max;

    /**
     * position of text cursor
     * @author hsiegeln
     */
     long cursor_pos;

public:
    gui_textinput_t();

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
    char *gib_text() const {return text;}

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
    void zeichnen(koord offset);
};

#endif
