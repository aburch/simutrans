/*
 * factorylist_frame_t.cpp
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "factorylist_frame_t.h"
#include "factorylist_stats_t.h"
#include "../gui/gui_scrollpane.h"



factorylist_frame_t::factorylist_frame_t(karte_t * welt) : gui_frame_t(translator::translate("fl_title"))
{
	//DBG_DEBUG("factorylist_frame_t()","constructor begin");
	stats = new factorylist_stats_t(welt);
	//DBG_DEBUG("factorylist_frame_t()","constructor 1");
	head = new factorylist_header_t(stats);
	//DBG_DEBUG("factorylist_frame_t()","constructor 2");
	head->setze_pos(koord(0,0));
	add_komponente(head);
	scrolly = new gui_scrollpane_t(stats);
	scrolly->setze_pos(koord(0, 38));
	add_komponente(scrolly);

	setze_fenstergroesse(koord(350, 240));
	// a min-size for the window
	set_min_windowsize(koord(350, 240));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));

	setze_opaque(true);
	//DBG_DEBUG("factorylist_frame_t(): constructor ende","");
}


factorylist_frame_t::~factorylist_frame_t()
{
//DBG_DEBUG("factorylist_frame_t()","destructor");
delete scrolly;
scrolly = 0;
delete head;
head = 0;
delete stats;
stats = 0;
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void factorylist_frame_t::resize(const koord delta)
{
gui_frame_t::resize(delta);
// fensterhoehe - 16(title) -38 (header)
koord groesse = gib_fenstergroesse()-koord(0,54);
scrolly->setze_groesse(groesse);
}
