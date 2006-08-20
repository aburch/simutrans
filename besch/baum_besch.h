/*
 *
 *  baum_besch.h
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
#ifndef __BAUM_BESCH_H
#define __BAUM_BESCH_H

/*
 *  includes
 */
#include "text_besch.h"
#include "bildliste2d_besch.h"

/*
 *  class:
 *      baum_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Beschreibung eines Baumes in Simutrans
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste2D
 */
class baum_besch_t : public obj_besch_t {
    friend class tree_writer_t;
    friend class tree_reader_t;

    uint8  hoehenlage;
    uint8  distribution_weight;

public:
	int gib_distribution_weight() const
	{
		return distribution_weight;
	}

	int gib_hoehenlage() const
	{
		return hoehenlage;
	}

	const char *gib_name() const
	{
		return static_cast<const text_besch_t *>(gib_kind(0))->gib_text();
	}

	const char *gib_copyright() const
	{
		return static_cast<const text_besch_t *>(gib_kind(1))->gib_text();
	}
	const bild_besch_t *gib_bild(int h, int i) const
	{
		return static_cast<const bildliste2d_besch_t *>(gib_kind(2))->gib_bild(i, h);
	}
	const int gib_seasons() const
	{
		return static_cast<const bildliste2d_besch_t *>(gib_kind(2))->gib_anzahl()/5;
	}
};


#endif // __BAUM_BESCH_H
