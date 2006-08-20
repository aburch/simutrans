/*
 *
 *  bruecke_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 */

#ifndef __BRUECKE_BESCH_H
#define __BRUECKE_BESCH_H


#include "skin_besch.h"
#include "bildliste_besch.h"
#include "text_besch.h"
#include "../boden/wege/weg.h"


/**
 *  class:
 *      bruecke_besch_t()
 *
 *  Autor:
 *      Volker Meyer, Hj. Malthaner
 *
 *  Beschreibung:
 *      Beschreibung einer Brücke.
 *
 *  Kindknoten:
 *	0   Cursor (Hajo: 14-Feb-02: now also icon image)
 *	1   Vordergrundbilder
 *	2   Hintergrundbilder
 */
class bruecke_besch_t : public obj_besch_t {
    friend class bridge_writer_t;
    friend class bridge_reader_t;

    uint32  topspeed;
    uint32  preis;

    /**
     * Maintenance cost per square/month
     * @author Hj. Malthaner
     */
    uint32 maintenance;

    uint8  wegtyp;
    uint8 pillars_every;	// =0 off
    uint8 max_length;	// =0 off, else maximum length

public:
    /*
     * Nummerierung all der verschiedenen Schienstücke
     */
    enum img_t {
	NS_Segment, OW_Segment, N_Start, S_Start, O_Start, W_Start, N_Rampe, S_Rampe, O_Rampe, W_Rampe, NS_Pillar, OW_Pillar
    };

    /*
     * Name und Copyright sind beim Cursor gespeichert!
     */
    const char *gib_name() const
    {
        return gib_cursor()->gib_name();
    }
    const char *gib_copyright() const
    {
	return gib_cursor()->gib_copyright();
    }
    int gib_hintergrund(img_t img) const
    {
	const bild_besch_t *bild = static_cast<const bildliste_besch_t *>(gib_kind(0))->gib_bild(img);
	if(bild) {
	    return bild->bild_nr;
	}
	return -1;
    }
    int gib_vordergrund(img_t img) const
    {
	const bild_besch_t *bild = static_cast<const bildliste_besch_t *>(gib_kind(1))->gib_bild(img);
	if(bild) {
	    return bild->bild_nr;
	}
	return -1;
  }
    static img_t gib_simple(ribi_t::ribi ribi);
    static img_t gib_start(ribi_t::ribi ribi);
    static img_t gib_rampe(ribi_t::ribi ribi);
    static img_t gib_pillar(ribi_t::ribi ribi);


    const skin_besch_t *gib_cursor() const
    {
	return static_cast<const skin_besch_t *>(gib_kind(2));
    }


    weg_t::typ gib_wegtyp() const
    {
	return static_cast<weg_t::typ>(wegtyp);
    }


    int gib_preis() const
    {
	return preis;
    }


    int gib_wartung() const
    {
	return maintenance;
    }


    /**
     * Determines max speed in km/h allowed on this bridge
     * @author Hj. Malthaner
     */
    int  gib_topspeed() const
    {
	return topspeed;
    }


    /**
     * Distance of pillars (=0 for no pillars)
     * @author prissi
     */
    int  gib_pillar() const
    {
	return pillars_every;
    }

    /**
     * maximum bridge span (=0 for infinite)
     * @author prissi
     */
    int  gib_max_length() const
    {
	return max_length;
    }
};

#endif // __BRUECKE_BESCH_H
