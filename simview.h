/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 */

#ifndef simview_h
#define simview_h

class karte_t;

/**
 * View-Klasse für Weltmodell.
 *
 * @author Hj. Malthaner
 */
class karte_ansicht_t
{
private:
	karte_t *welt;
	bool outside_visible;

public:
	karte_ansicht_t(karte_t *welt);
	void display(bool dirty);
	void display_region( koord lt, koord wh, sint16 y_min, const sint16 y_max, bool force_dirty, bool threaded );
};

#endif
