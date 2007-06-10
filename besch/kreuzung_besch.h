/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 */
#ifndef __KREUZUNG_BESCH_H
#define __KREUZUNG_BESCH_H

#include "obj_besch_std_name.h"
#include "bild_besch.h"
#include "bildliste_besch.h"
#include "../simtypes.h"


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bild
 */
class kreuzung_besch_t : public obj_besch_std_name_t {
    friend class crossing_writer_t;
    friend class crossing_reader_t;

private:
	uint8 wegtyp1;
	uint8 wegtyp2;

	sint8 sound;

	uint32 closed_animation_time;
	uint32 open_animation_time;

	uint32 topspeed1;	// the topspeed depeds strongly on the crossing ...
	uint32 topspeed2;

public:
	/* the imagelists are:
	 * open_ns
	 * open_ew
	 * front_open_ns
	 * front_open_ew
	 * closed_ns
	 * ....
	 * =>ns=0 NorthSouth ns=1, East-West
	 */
	const bild_besch_t *gib_bild(int ns, bool open, int phase) const
	{
		if(open) {
			return (static_cast<const bildliste_besch_t *>(gib_kind(2+ns)))->gib_bild(phase);
		}
		else {
			const bildliste_besch_t *bl = (static_cast<const bildliste_besch_t *>(gib_kind(6+ns)));
			return bl ? bl->gib_bild(phase) : NULL;
		}
	}

	const bild_besch_t *gib_bild_after(int ns, bool open, int phase) const
	{
		if(open) {
			const bildliste_besch_t *bl = (static_cast<const bildliste_besch_t *>(gib_kind(4+ns)));
			return bl ? bl->gib_bild(phase) : NULL;
		}
		else {
			const bildliste_besch_t *bl = (static_cast<const bildliste_besch_t *>(gib_kind(8+ns)));
			return bl ? bl->gib_bild(phase) : NULL;
		}
	}

	waytype_t get_waytype(int i) const { return (waytype_t)(i==0? wegtyp1 : wegtyp2); }
	uint32 gib_maxspeed(int i) const { return i==0 ? topspeed1 : topspeed2; }
	uint16 gib_phases(bool open,bool front) const { return static_cast<const bildliste_besch_t *>(gib_kind(6-(4*open)+2*front))->gib_anzahl(); }
	uint32 gib_animation_time(bool open) const { return open ? open_animation_time : closed_animation_time; }

	sint8 gib_sound() const { return sound; }
};

#endif
