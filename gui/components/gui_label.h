/*
 * gui_label.h
 *
 * just displays a text, will be auto-translated
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_gui_label_h
#define gui_gui_label_h

#include "../../ifc/gui_komponente.h"


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
    align_t align:4;

    /**
     * Farbe des Labels
     * @author Hansjörg Malthaner
     */
    uint8 color;

    const char * text;	// only for direct acess of non-translateable things. Do not use!

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
     * set the text without translation
     * @author Hansjörg Malthaner
     */
    void set_text_pointer(const char *text) { this->text = text; }

    /**
     * returns the pointer (i.e. for freeing untranslater contents)
     * @author Hansjörg Malthaner
     */
    const char * get_text_pointer() { return text; }

    /**
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    void zeichnen(koord offset) const;

    /**
     * Sets the colour of the label
     * @author Owen Rudge
     */

    void set_color(int colour) { this->color = colour; }

    /**
     * Sets the alignment of the label
     * @author Volker Meyer
     */

    void set_align(align_t align) { this->align = align; }
};

#endif
