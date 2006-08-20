/*
 * curiositylist_frame_t.cpp
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "curiositylist_frame_t.h"
#include "curiositylist_stats_t.h"
#include "../gui/gui_scrollpane.h"



curiositylist_frame_t::curiositylist_frame_t(karte_t * welt) : gui_frame_t(translator::translate("curlist_title"))
{
	stats = new curiositylist_stats_t(welt);
	header = new curiositylist_header_t(stats);
	header->setze_pos(koord(0,0));
	add_komponente(header);
	scrolly = new gui_scrollpane_t(stats);
	scrolly->setze_pos(koord(0,19));
	add_komponente(scrolly);

	setze_fenstergroesse(koord(400, 240));
	// a min-size for the window
	set_min_windowsize(koord(400, 240));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));

	setze_opaque(true);
	DBG_DEBUG("curiositylist_frame_t(): constructor ende","");
}


curiositylist_frame_t::~curiositylist_frame_t()
{
	DBG_DEBUG("curiositylist_frame_t()","destructor");
	delete header;
	header = 0;
	delete scrolly;
	scrolly = 0;
	delete stats;
	stats = 0;
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void curiositylist_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	// fensterhoehe - 16(title) - 19 (header)
	koord groesse = gib_fenstergroesse()-koord(0,35);
	scrolly->setze_groesse(groesse);
}
