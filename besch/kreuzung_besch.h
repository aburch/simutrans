/*
 *
 *  kreuzung_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
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
	const bild_besch_t *gib_bild(int ns, bool open, int phase) const
	{
		if(open) {
			return (static_cast<const bildliste_besch_t *>(gib_kind(2+ns)))->gib_bild(phase);
		}
		else {
			return (static_cast<const bildliste_besch_t *>(gib_kind(4+ns)))->gib_bild(phase);
		}
	}

	waytype_t get_waytype(int i) const { return (waytype_t)(i? wegtyp1 : wegtyp2); }
	uint32 gib_maxspeed(int i) const { return i ? topspeed1 : topspeed2; }
	uint16 gib_phases(bool open) const { return static_cast<const bildliste_besch_t *>(gib_kind(open ? 2 : 3))->gib_anzahl(); }
	uint32 gib_animation_time(bool open) const { return open ? open_animation_time : closed_animation_time; }

	sint8 gib_sound() const { return sound; }
};

#endif
