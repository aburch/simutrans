/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Author: V. Meyer
 */

#include <string.h>
#include "../simtypes.h"
#include "../simdebug.h"
#include "../boden/grund.h"
#include "marker.h"



void marker_t::init(int welt_groesse_x,int welt_groesse_y)
{
	cached_groesse = welt_groesse_x;
	bits_groesse = (welt_groesse_x*welt_groesse_y + bit_mask) / (bit_unit);
	if(bits) {
		delete [] bits;
	}
	if(bits_groesse) {
		bits = new unsigned char[bits_groesse];
	}
	else {
		bits = NULL;
	}
	unmarkiere_alle();
}

marker_t::~marker_t()
{
	delete [] bits;
}

void marker_t::unmarkiere_alle()
{
	if(bits) {
		MEMZERON(bits, bits_groesse);
	}
	more.clear();
}

void marker_t::markiere(const grund_t *gr)
{
	if(gr != NULL) {
		if(gr->ist_karten_boden()) {
			// ground level
			const int bit = gr->get_pos().y*cached_groesse+gr->get_pos().x;
			bits[bit/bit_unit] |= 1 << (bit & bit_mask);
		}
		else if(!more.get(gr)) {
			more.set(gr, true);
		}
	}
}

void marker_t::unmarkiere(const grund_t *gr)
{
	if(gr != NULL) {
		if(gr->ist_karten_boden()) {
			// ground level
			const int bit = gr->get_pos().y*cached_groesse+gr->get_pos().x;
			bits[bit/bit_unit] &= ~(1 << (bit & bit_mask));
		}
		else {
			more.remove(gr);
		}
	}
}

bool marker_t::ist_markiert(const grund_t *gr) const
{
	if(gr==NULL) {
		return false;
	}
	if(gr->ist_karten_boden()) {
		// ground level
		const int bit = gr->get_pos().y*cached_groesse+gr->get_pos().x;
		return (bits[bit/bit_unit] & (1 << (bit & bit_mask))) != 0;
	}
	else {
		return more.get(gr);
	}
}
