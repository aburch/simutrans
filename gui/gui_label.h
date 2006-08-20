/*
 * gui_label.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_gui_label_h
#define gui_gui_label_h

#include "../ifc/gui_komponente.h"


/**
 * Eine Label-Komponente
 *
 * @author Hj. Malthaner
 * @date 04-Mar-01
 * @version $Revision: 1.8 $
 *
 * Added Aligment support
 * @author: Volker Meyer
 * @date 25.05.03
 */
class gui_label_t : public gui_komponente_t
{
public:
    enum align_t {
	left,
	centered,
	right,
	money
    };
private:

    const char * text;

    align_t align;

    /**
     * Farbe des Labels
     * @author Hansjörg Malthaner
     */
    int color;

public:

    /**
     * Konstruktor
     * @author Hansjörg Malthaner
     */
    gui_label_t(const char *text);


    /**
     * Konstruktor
     * @author Hansjörg Malthaner
     */
    gui_label_t(const char *text, int color);

    /**
     * Konstruktor
     * @author Volker Meyer
     */
    gui_label_t(const char *text, int color, align_t align);


    /**
     * setzt den Text des Labels
     * @author Hansjörg Malthaner
     */
    void setze_text(const char *text);

    /**
     * holt den Text des Labels
     * @author Volker Meyer
     */
    const char *gib_text();

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

    /**
     * Sets the colour of the label
     * @author Owen Rudge
     */

    void set_color(int colour);

    /**
     * Sets the alignment of the label
     * @author Volker Meyer
     */

    void set_align(align_t align) { this->align = align; }
};

#endif
