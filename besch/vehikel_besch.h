/*
 *
 *  vehikel_besch.h
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
#ifndef __VEHIKEL_BESCH_H
#define __VEHIKEL_BESCH_H

/*
 *  includes
 */
#include "text_besch.h"
#include "ware_besch.h"
#include "bildliste_besch.h"
#include "skin_besch.h"
#include "../dataobj/ribi.h"

/**
 * Vehicle type description - all attributes of a vehicle type
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Ware
 *	3   Rauch
 *	4   Bildliste leer
 *	5   Bildliste mit Fracht
 *	6   erlaubter Vorgänger 1
 *	7   erlaubter Vorgänger 2
 *	... ...
 *	n+5 erlaubter Vorgänger n
 *	n+6 erlaubter Nachfolger 1
 *	n+7 erlaubter Nachfolger 2
 *	... ...
 *
 * @author Volker Meyer, Hj. Malthaner
 */
class vehikel_besch_t : public obj_besch_t {
    friend class vehicle_writer_t;
    friend class vehicle_reader_t;

public:

    // Hajo: be careful, some of these values must match the entries
    // in schiene_t
    enum weg_t { strasse=0,
		 schiene=1,
		 wasser=2,
		 luft=3,
		 // Hajo: unused ATM: schiene_elektrifiziert=4,
		 schiene_monorail=5,
		 schiene_maglev=6,
    };


    /**
     * Engine type
     * @author Hj. Malthaner
     */
    enum engine_t {
      steam,
      diesel,
      electric,
      bio,
      fuel_cell,
      hydrogene
    };


private:
    uint32  preis;
    uint32  zuladung;
    uint32  geschw;
    uint32  gewicht;
    uint32  leistung;
    uint32  betriebskosten;

    uint32  intro_date; // Hajo: introduction date
    uint8   gear;       // Hajo: engine gear (power multiplier)

    uint8  typ;         // Hajo: Das ist der weg_t
    sint8  sound;

    uint8  vorgaenger;	// Anzahl möglicher Vorgänger
    uint8  nachfolger;	// Anzahl möglicher Nachfolger

    uint8  engine_type; // Hajo: diesel, steam, electric

public:
    const char *gib_name() const
    {
        return static_cast<const text_besch_t *>(gib_kind(0))->gib_text();
    }
    const char *gib_copyright() const
    {
        return static_cast<const text_besch_t *>(gib_kind(1))->gib_text();
    }
    const ware_besch_t *gib_ware() const
    {
	return static_cast<const ware_besch_t *>(gib_kind(2));
    }
    const skin_besch_t *gib_rauch() const
    {
	return static_cast<const skin_besch_t *>(gib_kind(3));
    }
    int gib_basis_bild() const
    {
	return gib_bild_nr(ribi_t::dir_sued, false);
    }
    int gib_bild_nr(ribi_t::dir dir, bool empty) const
    {
	const bildliste_besch_t *liste = static_cast<const bildliste_besch_t *>(gib_kind(empty ? 4 : 5));

	if(!liste) {
	    liste = static_cast<const bildliste_besch_t *>(gib_kind(4));
	}
	const bild_besch_t *bild = liste->gib_bild(dir);

	if(!bild) {
	    bild = liste->gib_bild(dir - 4);
	    if(!bild) {
		return -1;
	    }
	}
	return bild->bild_nr;
    }
    //
    // Liefert die erlaubten Vorgänger.
    // liefert gib_vorgaenger(0) == NULL, so bedeutet das entweder alle
    // Vorgänger sind erlaubt oder keine. Um das zu unterscheiden, sollte man
    // vorher hat_vorgaenger() befragen
    //
    const vehikel_besch_t *gib_vorgaenger(int i) const
    {
	if(i < 0 || i >= vorgaenger) {
	    return 0;
	}
	return static_cast<const vehikel_besch_t *>(gib_kind(6 + i));
    }
    //
    // Liefert die erlaubten Nachfolger.
    // liefert gib_nachfolger(0) == NULL, so bedeutet das entweder alle
    // Nachfolger sind erlaubt oder keine. Um das zu unterscheiden, sollte
    // man vorher hat_nachfolger() befragen
    //
    const vehikel_besch_t *gib_nachfolger(int i) const
    {
	if(i < 0 || i >= nachfolger) {
	    return 0;
	}
	return static_cast<const vehikel_besch_t *>(gib_kind(6 + vorgaenger + i));
    }
    int gib_vorgaenger_count() const
    {
	return vorgaenger;
    }
    int gib_nachfolger_count() const
    {
	return nachfolger;
    }
    weg_t gib_typ() const
    {
	return static_cast<weg_t>(typ);
    }
    int gib_zuladung() const
    {
	return zuladung;
    }
    int gib_preis() const
    {
	return preis;
    }
    int gib_geschw() const
    {
	return geschw;
    }
    int gib_gewicht() const
    {
	return gewicht;
    }
    int gib_leistung() const
    {
	return leistung;
    }
    int gib_betriebskosten() const
    {
	return betriebskosten;
    }
    int gib_sound() const
    {
	return sound;
    }


    /**
     * @return introduction year
     * @author Hj. Malthaner
     */
    int get_intro_year() const {
      return intro_date >> 4;
    }


    /**
     * @return introduction month
     * @author Hj. Malthaner
     */
    int get_intro_month() const {
      return intro_date & 15;
    }


    /**
     * 64 = 1.00
     * @return gear value
     * @author Hj. Malthaner
     */
    int get_gear() const {
      return gear;
    }


    /**
     * @return engine type
     * @author Hj. Malthaner
     */
    uint8 get_engine_type() const {
      return engine_type;
    }
};

#endif // __VEHIKEL_BESCH_H
