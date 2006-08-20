/*
 *
 *  roadsign_besch.h
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
#ifndef __ROADSIGN_BESCH_H
#define __ROADSIGN_BESCH_H

/*
 *  includes
 */
#include "text_besch.h"
#include "bildliste_besch.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"

class skin_besch_t;

/*
 *  class:
 *      roadsign_besch_t()
 *
 *  Autor:
 *      prissi
 *
 *  Beschreibung:
 *	Straßenschildere
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste
 */
class roadsign_besch_t : public obj_besch_t {
	friend class roadsign_writer_t;
	friend class roadsign_reader_t;

	enum types {ONE_WAY=1, FREE_ROUTE=2 };

	uint8 flags;
	uint16 min_speed;	// 0: unused
	uint32 cost;

public:
	const char *gib_name() const
	{
		return static_cast<const text_besch_t *>(gib_kind(0))->gib_text();
	}

	const char *gib_copyright() const
	{
		return static_cast<const text_besch_t *>(gib_kind(1))->gib_text();
	}

	int gib_bild_nr(ribi_t::dir dir) const
	{
		const bild_besch_t *bild = static_cast<const bildliste_besch_t *>(gib_kind(2))->gib_bild(dir);
		return bild ? bild->bild_nr : -1;
	}
	int gib_bild_anzahl() const
	{
		return static_cast<const bildliste_besch_t *>(gib_kind(2))->gib_anzahl();
	}

	bool is_single_way() const
	{
		return (flags&ONE_WAY)!=0;
	}

	int gib_min_speed() const
	{
		return min_speed;
	}

	sint32 gib_preis() const
	{
		return cost;
	}

	const skin_besch_t *gib_cursor() const
	{
		return (const skin_besch_t *)gib_kind(3);
	}

	//  return true for a traffic light
	bool is_traffic_light() const { return (gib_bild_anzahl()>4); };

	bool is_free_route() const { return flags&FREE_ROUTE; };
};

#endif // __ROADSIGN_BESCH_H
