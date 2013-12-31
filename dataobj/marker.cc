/*
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include "../simtypes.h"
#include "../simdebug.h"
#include "../boden/grund.h"
#include "marker.h"

marker_t marker_t::the_instance;


void marker_t::init(int welt_groesse_x,int welt_groesse_y)
{
	// do not reallocate it, if same size ...
	cached_groesse = welt_groesse_x;
	int new_bits_groesse = (welt_groesse_x*welt_groesse_y + bit_mask) / (bit_unit);

	if(  bits_groesse != new_bits_groesse  ) {
		bits_groesse = new_bits_groesse;
		if(bits) {
			delete [] bits;
		}
		if(bits_groesse) {
			bits = new unsigned char[bits_groesse];
		}
		else {
			bits = NULL;
		}
	}
	unmark_all();
}

marker_t& marker_t::instance(int welt_groesse_x,int welt_groesse_y)
{
	the_instance.init(welt_groesse_x, welt_groesse_y);
	return the_instance;
}

marker_t::~marker_t()
{
	delete [] bits;
}

void marker_t::unmark_all()
{
	if(bits) {
		MEMZERON(bits, bits_groesse);
	}
	more.clear();
}

void marker_t::mark(const grund_t *gr)
{
	if(gr != NULL) {
		if(gr->ist_karten_boden()) {
			// ground level
			const int bit = gr->get_pos().y*cached_groesse+gr->get_pos().x;
			bits[bit/bit_unit] |= 1 << (bit & bit_mask);
		}
		else {
			more.set(gr, true);
		}
	}
}

void marker_t::unmark(const grund_t *gr)
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

bool marker_t::is_marked(const grund_t *gr) const
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

bool marker_t::test_and_mark(const grund_t *gr)
{
	if(gr != NULL) {
		if(gr->ist_karten_boden()) {
			// ground level
			const int bit = gr->get_pos().y*cached_groesse+gr->get_pos().x;
			if ((bits[bit/bit_unit] & (1 << (bit & bit_mask))) != 0) {
				return true;
			}
			bits[bit/bit_unit] |= 1 << (bit & bit_mask);
		}
		else {
			return more.set(gr, true);
		}
	}
	return false;
}
