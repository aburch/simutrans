/*
 * map_frame.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#include <stdio.h>
#include <cmath>

#include "map_legend.h"
#include "karte.h"

#include "../simworld.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "../bauer/fabrikbauer.h"
#include "../utils/cstring_t.h"
#include "../dataobj/translator.h"
#include "../besch/fabrik_besch.h"
#include "../tpl/minivec_tpl.h"


koord map_legend_t::size;

// hopefully these are enough ...
static minivec_tpl <cstring_t> legend_names (128);
static minivec_tpl <int> legend_colors (128);


// @author hsiegeln
const char map_legend_t::map_type[MAX_MAP_TYPE][64] =
{
    "Towns",
    "Passagiere",
    "Post",
    "Fracht",
    "Status",
    "Service",
    "Traffic",
    "Origin",
    "Destination",
    "Waiting",
    "Tracks",
    "Speedlimit",
    "Powerlines",
    "Tourists",
    "Factories"
};

const int map_legend_t::map_type_color[MAX_MAP_TYPE] =
{
  7, 11, 15, 132, 23, 27, 31, 35, 241, 7, 11, 71, 57, 81
};

/**
 * Calculate button positions
 * @author Hj. Malthaner
 */
void map_legend_t::calc_button_positions()
{
	const int button_columns = (gib_fenstergroesse().x - 20) / 80;
	for (int type=0; type<MAX_MAP_TYPE; type++) {
		koord pos=koord(10 + 80*(type % button_columns), 4 +16 + 16 + 15 * ((int)type/button_columns-1));
		if(pos.y>gib_fenstergroesse().y) {
			pos.y = 4+16+16;
		}
		filter_buttons[type].init(button_t::box,translator::translate(map_type[type]), pos, koord(79, 14));
	}
}



/**
 * Konstruktor. Erzeugt alle notwendigen Subkomponenten.
 * @author Hj. Malthaner
 */
map_legend_t::map_legend_t(const karte_modell_t *welt) : gui_frame_t("Legend")
{
	legend_names.clear();
	legend_colors.clear();

	const stringhashtable_tpl<const fabrik_besch_t *> & fabesch = fabrikbauer_t::gib_fabesch();
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter (fabesch);

	// add factory names; shorten too long names
	while( iter.next() &&  iter.get_current_value()->gib_gewichtung()>0) {
		int i;

		cstring_t label (translator::translate(iter.get_current_value()->gib_name()));
		for(  i=12;  i<label.len()  &&  display_calc_proportional_string_len_width(label,i,true)<100;  i++  )
			;
		if(  i<label.len()  ) {
			label.set_at(i++, '.');
			label.set_at(i++, '.');
			label.set_at(i++, '\0');
		}

		legend_names.append(label);
		legend_colors.append(iter.get_current_value()->gib_kennfarbe());
	}

	// add filter buttons
	// @author hsiegeln
	for (int type=0; type<MAX_MAP_TYPE; type++) {
		filter_buttons[type].add_listener(this);
		filter_buttons[type].background = map_type_color[type];
		is_filter_active[type] = reliefkarte_t::gib_karte()->get_mode() == type;
		add_komponente(filter_buttons + type);
	}

	// Hajo: Hack: use static size if set by a former object
	if(size != koord(0,0)) {
		setze_fenstergroesse(size);
	}
	else {
		factory_offset = ((legend_names.get_count()+1)/2)*14;
		button_offset = 4+16+16+8+( (MAX_MAP_TYPE+2)/ 3)*15;
		setze_fenstergroesse(koord(260,button_offset+factory_offset));
	}

	// Hajo: Trigger layouting
	set_resizemode(diagonal_resize);

	setze_opaque(true);
}



/**
 * size window in response and save it in static size
 * @author (Mathew Hounsell)
 * @date   11-Mar-2003
 */
void map_legend_t::setze_fenstergroesse(koord groesse)
{
	DBG_MESSAGE("map_legend_t::setze_fenstergroesse()","size %i,%i",groesse.x,groesse.y);
	if( groesse.x<120 ) {
		groesse.x = 120;
	}
	int h = ((groesse.x-10)/110);
	factory_offset = (legend_names.get_count()+(h-1)/h)*14;
	h = ((groesse.x-20)/80);
	button_offset = 4+16+16+8+( (MAX_MAP_TYPE+(h-1))/ h)*15;
	if( groesse.y<button_offset ) {
		groesse.y = button_offset;
	}
	DBG_MESSAGE("map_legend_t::setze_fenstergroesse()","new size %i,%i",groesse.x,groesse.y);
	gui_frame_t::setze_fenstergroesse( groesse );
	map_legend_t::size = gib_fenstergroesse();   // Hajo: remember size
	calc_button_positions();
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   01-Jun-2002
 */
void map_legend_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	setze_fenstergroesse(size);
}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void map_legend_t::zeichnen(koord pos, koord gr)
{
	int i;

	// button state
	for (i = 0;i<MAX_MAP_TYPE;i++) {
		filter_buttons[i].pressed = is_filter_active[i];
	}
	// now the buttons and the background
	gui_frame_t::zeichnen(pos, gr);


	// color bar
	for (i = 0; i<12; i++) {
		display_fillbox_wh(pos.x + 30 + (11-i)*(size.x-60)/12, pos.y+18+3,  (size.x-60)/12, 7, reliefkarte_t::severity_color[11-i], false);
	}
	display_proportional(pos.x + 26, pos.y+18+2, translator::translate("min"), ALIGN_RIGHT, SCHWARZ, false);
	display_proportional(pos.x + size.x - 26, pos.y+18+2, translator::translate("max"), ALIGN_LEFT, SCHWARZ, false);

	// factories
	const int rows = (size.x-10)/110;
	const int width = (size.x-10)/rows;
	for(unsigned u=0; u<legend_names.get_count(); u++) {

		const int xpos = pos.x + (u%rows)*width + 8;
		const int ypos = pos.y+button_offset+(u/rows)*14;
		const int color = legend_colors.at(u);

		if(ypos+14>pos.y+gr.y) {
			break;
		}
		display_fillbox_wh(xpos, ypos+1, 7, 7, color, false);
		display_proportional( xpos+8, ypos, legend_names.get(u), ALIGN_LEFT, SCHWARZ, false);
	}
}



bool
map_legend_t::action_triggered(gui_komponente_t *komp)
{
	reliefkarte_t::gib_karte()->set_mode(-1);
	for (int i=0;i<MAX_MAP_TYPE;i++) {
		if (komp == &filter_buttons[i]) {
			if (is_filter_active[i] == true) {
				is_filter_active[i] = false;
			} else {
				reliefkarte_t::gib_karte()->set_mode(i);
				is_filter_active[i] = true;
			}
		} else {
			is_filter_active[i] = false;
		}
	}
	return true;
}
