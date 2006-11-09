/*
 *
 *  bruecke_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  BEWARE: non-standard node structure!
 *	0   Vordergrundbilder
 *	1   Hintergrundbilder
 *	2   Cursor/Icon (Hajo: 14-Feb-02: now also icon image)
 */

#ifndef __BRUECKE_BESCH_H
#define __BRUECKE_BESCH_H


#include "skin_besch.h"
#include "bildliste_besch.h"
#include "text_besch.h"
#include "../simtypes.h"
#include "../simimg.h"

#include "../dataobj/ribi.h"


class bruecke_besch_t : public obj_besch_t {
    friend class bridge_writer_t;
    friend class bridge_reader_t;

private:
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

	// allowed eara
	uint16 intro_date;
	uint16 obsolete_date;

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
	const char *gib_name() const { return gib_cursor()->gib_name(); }
	const char *gib_copyright() const { return gib_cursor()->gib_copyright(); }

	const skin_besch_t *gib_cursor() const { return static_cast<const skin_besch_t *>(gib_kind(2)); }

	image_id gib_hintergrund(img_t img) const
	{
		const bild_besch_t *bild = static_cast<const bildliste_besch_t *>(gib_kind(0))->gib_bild(img);
		if(bild) {
			return bild->bild_nr;
		}
		return IMG_LEER;
	}

	image_id gib_vordergrund(img_t img) const
	{
		const bild_besch_t *bild = static_cast<const bildliste_besch_t *>(gib_kind(1))->gib_bild(img);
		if(bild) {
			return bild->bild_nr;
		}
		return IMG_LEER;
	}

	static img_t gib_simple(ribi_t::ribi ribi);
	static img_t gib_start(ribi_t::ribi ribi);
	static img_t gib_rampe(ribi_t::ribi ribi);
	static img_t gib_pillar(ribi_t::ribi ribi);

	waytype_t gib_waytype() const { return static_cast<waytype_t>(wegtyp); }

	sint32 gib_preis() const { return preis; }

	sint32 gib_wartung() const { return maintenance; }

	/**
	 * Determines max speed in km/h allowed on this bridge
	 * @author Hj. Malthaner
	 */
	uint32  gib_topspeed() const { return topspeed; }

	/**
	 * Distance of pillars (=0 for no pillars)
	 * @author prissi
	 */
	int  gib_pillar() const { return pillars_every; }

	/**
	 * maximum bridge span (=0 for infinite)
	 * @author prissi
	 */
	int  gib_max_length() const { return max_length; }

	/**
	 * @return introduction month
	 * @author Hj. Malthaner
	 */
	int get_intro_year_month() const { return intro_date; }

	/**
	 * @return time when obsolete
	 * @author prissi
	 */
	int get_retire_year_month() const { return obsolete_date; }
};

#endif
