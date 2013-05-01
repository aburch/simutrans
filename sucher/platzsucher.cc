/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 *
 * Author: V. Meyer
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simintr.h"
#include "../simtypes.h"
#include "platzsucher.h"

pos_liste_t::pos_liste_t(sint16 max_radius)
{
	this->max_radius = max_radius + 1;
	spalten = new sint16[this->max_radius];

	neu_starten();
}

pos_liste_t::~pos_liste_t()
{
	delete [] spalten;
}


void pos_liste_t::neu_starten()
{
	radius = 1;
	spalten[0] = 0;
	reihe = 0;
	quadrant = 0;
}


sint16 pos_liste_t::suche_beste_reihe()
{
	sint16 best_dist = -1;
	sint16 beste_reihe = -1;

	for(sint16 i = 0; i < radius; i++) {
		if(spalten[i] < radius) {
			int dist = (int)i * i + spalten[i] * spalten[i];

			if(best_dist == -1 || dist < best_dist) {
				best_dist = dist;
				beste_reihe = i;
			}
		}
	}
	if(radius < max_radius && best_dist > radius * radius) {
		return -1;
	} else {
		return beste_reihe;
	}
}


bool pos_liste_t::get_naechste_pos(koord &k)
{
	if(reihe != -1) {
		if(quadrant++ == 4) {
			quadrant = 1;

			spalten[reihe]++;

			reihe = suche_beste_reihe();
			if(reihe == -1) {
				if(radius < max_radius) {
					spalten[radius++] = 0;
				}
				reihe = suche_beste_reihe();
			}
		}
	}
	if(reihe != -1) {
		if(quadrant == 1 && !reihe) {
			quadrant += 2;		// skip second 0, +/-y
		}
		if(quadrant % 2 == 1 && !spalten[reihe]) {
			quadrant ++;		// skip second +/-x, 0
		}
		return get_pos(k);
	}
	return false;
}


bool pos_liste_t::get_pos(koord &k)
{
	if(reihe != -1) {
		switch(quadrant) {
			case 1:
				k = koord(reihe, spalten[reihe]);
				return true;
			case 2:
				k = koord(reihe, (short)-spalten[reihe]);
				return true;
			case 3:
				k = koord((short)-reihe, spalten[reihe]);
				return true;
			case 4:
				k = koord(-reihe, -spalten[reihe]);
				return true;
		}
	}
	return false;
}


pos_liste_wh_t::pos_liste_wh_t(sint16 max_radius, sint16 b, sint16 h) :
    pos_liste_t(max_radius)
{
	neu_starten(b, h);
}


void pos_liste_wh_t::neu_starten(sint16 b, sint16 h)
{
	this->b = b;
	this->h = h;
	dx = dy = 0;
	pos_liste_t::neu_starten();
}


bool pos_liste_wh_t::get_naechste_pos(koord &k)
{
	get_pos(k);

	if(k.x == 0 && k.y == 0 && (dx > 0 || dy > 0)) {
		if(dx > 0) {
			dx--;
		}
		else if(dy > 0) {
			dy--;
			dx = b - 1;
		}
		k.x -= dx;
		k.y -= dy;
	}
	else if(dx > 0) {
		k.x -= --dx;
		if(k.y <= 0) {
			k.y -= h - 1;
		}
	}
	else if(dy > 0) {
		k.y -= --dy;
		if(k.x <= 0) {
			k.x -= b - 1;
		}
	}
	else {
		if(pos_liste_t::get_naechste_pos(k)) {
			if(k.y == 0) {
				dy = h - 1;
			}
			if(k.x == 0) {
				dx = b - 1;
			}
			if(k.y <= 0) {
				k.y -= h - 1;
			}
			if(k.x <= 0) {
				k.x -= b - 1;
			}
		}
		else {
			return false;
		}
	}
	return true;
}


bool platzsucher_t::ist_platz_ok(koord pos, sint16 b, sint16 h,climate_bits cl) const
{
	if(!welt->is_within_limits(pos)) {
		return false;
	}
	koord k(b, h);

	while(k.y-- > 0) {
		k.x = b;
		while(k.x-- > 0) {
			if(!ist_feld_ok(pos, k, cl)) {
				return false;
			}
		}
	}
	return true;
}

bool platzsucher_t::ist_randfeld(koord d) const
{
	return d.x == 0 || d.x == b - 1 || d.y == 0 || d.y == h - 1;
}

bool platzsucher_t::ist_feld_ok(koord /*pos*/, koord /*d*/, climate_bits /*cl*/) const
{
	return true;
}

koord platzsucher_t::suche_platz(koord start, sint16 b, sint16 h, climate_bits cl, bool *r)
{
	sint16 radius = max_radius > 0 ? max_radius : welt->get_size_max();
	pos_liste_wh_t psuch1(radius, b, h);

	this->b = b;
	this->h = h;

	koord rel1, rel2;

	if((r && *r) && b != h) {
		//
		// Hier suchen wir auch gedrehte Positionen.
		//
		pos_liste_wh_t psuch2(radius, h, b);

		if(ist_platz_ok(start, b, h, cl)) {
			*r = false;
			return start;
		}
		if(ist_platz_ok(start, h, b, cl)) {
			*r = true;
			return start;
		}
		while(psuch1.get_naechste_pos(rel1) && psuch2.get_naechste_pos(rel2)) {
			if(ist_platz_ok(start + rel1, b, h, cl)) {
				*r = false;
				return start + rel1;
			}
			if(ist_platz_ok(start + rel2, h, b, cl)) {
				*r = true;
				return start + rel2;
			}
			INT_CHECK("simcity 1313");
		}
	}
	else {
		//
		// Suche ohne gedrehte Positionen.
		//
		if(ist_platz_ok(start, b, h, cl)) {
			return start;
		}
		while(psuch1.get_naechste_pos(rel1)) {
			if(ist_platz_ok(start + rel1, b, h, cl)) {
				if(r) {
					*r = false;
				}
				return start + rel1;
			}
			INT_CHECK("simcity 1314");
		}
	}
	return koord::invalid;
}
