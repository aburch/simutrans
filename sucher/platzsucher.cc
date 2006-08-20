/*
 * platzsucher.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * Author: V. Meyer
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simintr.h"
#include "platzsucher.h"

pos_liste_t::pos_liste_t(short max_radius)
{
    this->max_radius = max_radius + 1;
    spalten = new short[this->max_radius];

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


short pos_liste_t::suche_beste_reihe()
{
    int best_dist = -1;
    short beste_reihe = -1;

    for(short i = 0; i < radius; i++) {
	if(spalten[i] < radius) {
	    int dist = (int)i * i + spalten[i] * spalten[i];

	    if(best_dist == -1 || dist < best_dist) {
		best_dist = dist;
		beste_reihe = i;
	    }
	}
    }
    if(radius < max_radius && best_dist > radius * radius)
	return -1;
    else
	return beste_reihe;
}


bool pos_liste_t::gib_naechste_pos(koord &k)
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
	return gib_pos(k);
    }
    return false;
}


bool pos_liste_t::gib_pos(koord &k)
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


pos_liste_wh_t::pos_liste_wh_t(short max_radius, short b, short h) :
    pos_liste_t(max_radius)
{
    neu_starten(b, h);
}


void pos_liste_wh_t::neu_starten(short b, short h)
{
    this->b = b;
    this->h = h;
    dx = dy = 0;
    pos_liste_t::neu_starten();
}


bool pos_liste_wh_t::gib_naechste_pos(koord &k)
{
    gib_pos(k);

    if(k.x == 0 && k.y == 0 && (dx > 0 || dy > 0)) {
	if(dx > 0) {
	    dx--;
	}
	else {
	    if(dy > 0) {
		dy--;
		dx = b - 1;
	    }
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
	if(pos_liste_t::gib_naechste_pos(k)) {
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


bool platzsucher_t::ist_platz_ok(koord pos, int b, int h) const
{
    if(pos.x < 0 || pos.y < 0 || pos.x + b >= welt->gib_groesse() || pos.y + h >= welt->gib_groesse()) {
	return false;
    }
    koord k(b, h);

    while(k.y-- > 0) {
	k.x = b;
	while(k.x-- > 0) {
	    if(!ist_feld_ok(pos, k)) {
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

bool platzsucher_t::ist_feld_ok(koord /*pos*/, koord /*d*/) const
{
    return true;
}

koord platzsucher_t::suche_platz(koord start, int b, int h, bool *r)
{
    pos_liste_wh_t psuch1(welt->gib_groesse(), b, h);

    this->b = b;
    this->h = h;

    koord rel1, rel2;

    if(r && b != h) {
	//
	// Hier suchen wir auch gedrehte Positionen.
	//
	pos_liste_wh_t psuch2(welt->gib_groesse(), h, b);

	if(ist_platz_ok(start, b, h)) {
	    *r = false;
	    return start;
	}
	if(ist_platz_ok(start, h, b)) {
	    *r = true;
	    return start;
	}
	while(psuch1.gib_naechste_pos(rel1) && psuch2.gib_naechste_pos(rel2)) {
	    if(ist_platz_ok(start + rel1, b, h)) {
		*r = false;
    		return start + rel1;
	    }
	    if(ist_platz_ok(start + rel2, h, b)) {
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
	if(ist_platz_ok(start, b, h)) {
	    return start;
	}
	while(psuch1.gib_naechste_pos(rel1)) {
	    if(ist_platz_ok(start + rel1, b, h)) {
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

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
