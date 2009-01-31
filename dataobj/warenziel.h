/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef dataobj_warenziel_h
#define dataobj_warenziel_h

#include "../halthandle_t.h"
#include "../besch/ware_besch.h"

class loadsave_t;

/**
 * Diese Klasse wird zur Verwaltung von Zielen von
 * Haltestellen benutzt. Grundlegende Elemente sind
 * eine Koordinate und ein Zeitstempel.
 *
 * This class is for the management of targets 
 * used by bus stops. Essential elements are a
 * coordinate and a time stamp.
 *
 * @author Hj. Malthaner
 */

class warenziel_t
{
private:
	halthandle_t halt;
	uint8 catg_index;

public:
	// don't use them, or fix them: Actually, these should check for stops
	// but they are needed for list search ...
	int operator==(const warenziel_t &wz) {
		return (halt == wz.get_zielhalt()  &&  catg_index==wz.get_catg_index());
	}
	int operator!=(const warenziel_t &wz) {
		return halt!=wz.get_zielhalt()  ||  catg_index!=wz.get_catg_index();
	}

	warenziel_t() { catg_index = 255; halt = halthandle_t();}

	warenziel_t(halthandle_t &h, const ware_besch_t *b) { halt = h; catg_index = b->get_catg_index(); }

	warenziel_t(loadsave_t *file);

	void set_zielhalt(halthandle_t &h) { halt = h; }
	const halthandle_t get_zielhalt() const { return halt; }

	uint8 get_catg_index() const { return catg_index; }

	void rdwr(loadsave_t *file);
};

#endif
