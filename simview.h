/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * This file is part of the Simugraph engine and may not be used
 * (see licence.txt)
 *
 * Usage for Iso-Angband is granted.
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

public:
	karte_ansicht_t(karte_t *welt);
	void display(bool dirty);
};

#endif
