/*
 * gui_textinput.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_components_gui_speedbar_h
#define gui_components_gui_speedbar_h

#include "../../ifc/gui_komponente.h"
#include "../../tpl/slist_tpl.h"
#include "../ifc/action_listener.h"

struct event_t;

/**
 *
 * @author Volker Meyer
 * @date  12.06.2003
 */
class gui_speedbar_t : public gui_komponente_t
{
private:
    struct info_t {
	int color;
	const int *value;
	int last;
    };

    slist_tpl <info_t> values;

    int base;
    bool vertical;

public:
    /**
     * Konstruktor
     *
     */
    gui_speedbar_t() { base = 100; vertical = false; }


    /**
     * Destruktor
     *
     */
    ~gui_speedbar_t() {}


    void add_color_value(const int *value, int color);

    void set_base(int base) { this->base = base;  }

    void set_vertical(bool vertical) { this->vertical = vertical; }


    /**
     * Zeichnet die Komponente
     */
    void zeichnen(koord offset) const;
};

#endif
