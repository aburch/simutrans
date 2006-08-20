/*
 * gui_divider.h
 *
 * Copyright (c) 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_components_gui_divider_h
#define gui_components_gui_divider_h

#include "../../ifc/gui_komponente.h"

struct event_t; //nötig???

/**
 * Eine einfache Trennlinie
 *
 * @date 30-Oct-01
 * @author Markus Weber
 */
class gui_divider_t : public gui_komponente_t
{
private:

    // Der Stringbuffer.
    // char * text;


    // Maximallänge des Stringbuffers
    // int max;

public:


    /**
     * Konstruktor
     *
     * @author Markus Weber
     */
    gui_divider_t();


    /**
     * Destruktor
     *
     * @author Markus Weber
     */
    ~gui_divider_t();


    /**
     * Setzt den Textpuffer
     *
     * @author Markus Weber
     */
    //void setze_text(char *text, int max) {this->text = text; this->max=max;};


    /**
     * Holt den Textpuffer
     *
     * @author Markus Weber
     */
    //char *gib_text() const {return text;};



    /**
     * Zeichnet die Komponente
     * @author Markus Weber
     */
    void zeichnen(koord offset) const;
};

#endif
