/*
 *
 *  tunnel_besch.h
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
#ifndef __TUNNEL_BESCH_H
#define __TUNNEL_BESCH_H

/*
 *  includes
 */
#include "text_besch.h"
#include "bildliste2d_besch.h"

/*
 *  class:
 *      tunnel_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste Hintergrund
 *	3   Bildliste Vordergrund
 */
 #include "skin_besch.h"

class tunnel_besch_t : public obj_besch_t {
    friend class tunnel_writer_t;

    static int hang_indices[16];

public:
    const char *gib_name() const
    {
	return static_cast<const text_besch_t *>(gib_kind(0))->gib_text();
    }
    const char *gib_copyright() const
    {
        return static_cast<const text_besch_t *>(gib_kind(1))->gib_text();
    }
    const bild_besch_t *gib_hintergrund(hang_t::typ hang) const
    {
	return static_cast<const bildliste_besch_t *>(gib_kind(2))->gib_bild(hang_indices[hang]);
    }
    int gib_hintergrund_nr(hang_t::typ hang) const
    {
	const bild_besch_t *besch = gib_hintergrund(hang);

	return besch ? besch->bild_nr : -1;
    }
    const bild_besch_t *gib_vordergrund(hang_t::typ hang) const
    {
	return static_cast<const bildliste_besch_t *>(gib_kind(3))->gib_bild(hang_indices[hang]);
    }
    int gib_vordergrund_nr(hang_t::typ hang) const
    {
	const bild_besch_t *besch = gib_vordergrund(hang);

	return besch ? besch->bild_nr : -1;
    }
    const skin_besch_t *gib_cursor() const
    {
	return static_cast<const skin_besch_t *>(gib_kind(4));
   }
};

#endif // __TUNNEL_BESCH_H
