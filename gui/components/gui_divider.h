/*
 * Copyright (c) 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_components_gui_divider_h
#define gui_components_gui_divider_h

#include "gui_komponente.h"
#include "../../display/simgraph.h"
#include "../../simskin.h"

class skinverwaltung_t;

/**
 * A horizontal divider line
 *
 * @date 30-Oct-01
 * @author Markus Weber
 */
class gui_divider_t : public gui_komponente_t
{
public:
	gui_divider_t() { groesse.y = D_DIVIDER_HEIGHT; }

	void init( koord xy, KOORD_VAL width, KOORD_VAL height = D_DIVIDER_HEIGHT ) {
		set_pos( xy );
		set_groesse( koord( width, height ) );
	};

	void set_width(KOORD_VAL width) { set_groesse(koord(width,groesse.y)); }

	virtual koord get_groesse() const { return koord(groesse.x,max(groesse.y,D_DIVIDER_HEIGHT)); }

	void zeichnen(koord offset) { display_img_stretch( gui_theme_t::divider, scr_rect( get_pos()+offset, get_groesse() ) ); }
};

#endif
