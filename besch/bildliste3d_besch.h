/*
 *  Modified by James E. Petts, April 2011 from bildliste2d_besch.h, which is
 *  copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 * 
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __BILDLISTE3D_BESCH_H
#define __BILDLISTE3D_BESCH_H

#include "bildliste_besch.h"
#include "bildliste2d_besch.h"

/*
 *  Autors:
 *      Volker Meyer & jamespetts
 *
 *  Description:
 *      3-dimentional image array
 *
 *  Child nodes:
 *	0   1. Bildliste
 *	1   2. Bildliste
 *  2   3. Bildliste
 *	... ...
 */
class bildliste3d_besch_t : public obj_besch_t {
	friend class imagelist3d_reader_t;
	friend class imagelist3d_writer_t;

	uint16  anzahl;

public:
	bildliste3d_besch_t() : anzahl(0) {}

	uint16 get_anzahl() const { return anzahl; }

	bildliste2d_besch_t const* get_liste_2d(uint16 i)				  const { return i < anzahl ? get_child<bildliste2d_besch_t>(i)					: 0; }
	bildliste_besch_t   const* get_liste(uint16 i, uint16 j)		  const { return i < anzahl ? get_child<bildliste2d_besch_t>(i)->get_liste(j)	: 0; }
	bild_besch_t        const* get_image(uint16 i, uint16 j, uint16 k) const { return i < anzahl ? get_child<bildliste2d_besch_t>(i)->get_image(j, k)	: 0; }
};

#endif
