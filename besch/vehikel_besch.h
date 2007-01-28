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

#include "obj_besch_std_name.h"
#include "ware_besch.h"
#include "bildliste_besch.h"
#include "bildliste2d_besch.h"
#include "skin_besch.h"
#include "sound_besch.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"


/**
 * Vehicle type description - all attributes of a vehicle type
 *
 *  child nodes:
 *	0   Name
 *	1   Copyright
 *	2   freight
 *	3   smoke
 *	4   empty 1d image list
 *	5   either 1d (freight_image_type==0) or 2d image list
 *	6   required leading vehicle 1
 *	7   required leading vehicle 2
 *	... ...
 *	n+5 required leading vehicle n
 *	n+6 allowed trailing vehicle 1
 *	n+7 allowed trailing vehicle 2
 *	... ...
 *	n+m+5 allowed trailing vehicle m
 *  n+m+6 freight for which special images are defined
 *
 * @author Volker Meyer, Hj. Malthaner, kierongreen
 */
class vehikel_besch_t : public obj_besch_std_name_t {
    friend class vehicle_writer_t;
    friend class vehicle_reader_t;
    friend class vehikelbauer_t;

public:
	/**
	 * Engine type
	 * @author Hj. Malthaner
	 */
	enum engine_t {
		 unknown=-1,
		steam=0,
		diesel,
		electric,
		bio,
		sail,
		fuel_cell,
		hydrogene,
		battery
	};


private:
	uint32  preis;
	uint16  zuladung;
	uint16  geschw;
	uint16  gewicht;
	uint32  leistung;
	uint16  betriebskosten;

	uint16  intro_date; // introduction date
	uint16  obsolete_date; //phase out at
	uint16  gear;       // engine gear (power multiplier), 64=100

	sint8  typ;         	// see weg_t for allowed types
	uint8 len;			// length (=8 is half a tile, the old default)
	sint8 sound;

	uint8  vorgaenger;	// all defined leading vehicles
	uint8  nachfolger;	// all defined trailer

	uint8  engine_type; // diesel, steam, electric (requires electrified ways), fuel_cell, etc.

	sint8 freight_image_type;	// number of freight images (displayed for different goods)


public:
	// since we have a second constructor
	vehikel_besch_t() { }

	// default vehicle (used for way seach and similar tasks)
	// since it has no images and not even a name knot any calls to this will case a crash
	vehikel_besch_t(uint8 wtyp, uint16 speed, engine_t engine) {
		freight_image_type = preis = zuladung = betriebskosten = intro_date = vorgaenger = nachfolger = 0;
		leistung = gewicht = 1;
		gear = 64;
		len = 8;
		sound = -1;
		typ = wtyp;
		engine_type = (uint8)engine;
		geschw = speed;
	}

	const ware_besch_t *gib_ware() const { return static_cast<const ware_besch_t *>(gib_kind(2)); }

	const skin_besch_t *gib_rauch() const { return static_cast<const skin_besch_t *>(gib_kind(3)); }

	image_id gib_basis_bild() const { return gib_bild_nr(ribi_t::dir_sued, gib_ware() ); }

	// returns the number of different directions
	uint8 gib_dirs() const { return (static_cast<const bildliste_besch_t *>(gib_kind(4)))->gib_bild(4) ? 8 : 4; }

	// return a matching image
	// beware, there are three class of vehicles
	// vehicles with and without freight images, and vehicles with different freight images
	// they can have 4 or 8 directions ...
	image_id gib_bild_nr(ribi_t::dir dir, const ware_besch_t *ware) const
	{
		const bild_besch_t *bild=0;
		const bildliste_besch_t *liste=0;

		if(freight_image_type>0  &&  ware!=NULL) {
			// more freight images and a freight: find the right one

			sint8 ware_index=0; // freight images: if not found use first freight

			for( sint8 i=0;  i<freight_image_type;  i++  ) {
				if(ware->gib_index()==static_cast<const ware_besch_t *>(gib_kind(6 + nachfolger + vorgaenger + i))->gib_index()) {
					ware_index = i;
					break;
				}
			}

			// vehicle has freight images and we want to use - get appropriate one (if no list then fallback to empty image)
			const bildliste2d_besch_t *liste2d = static_cast<const bildliste2d_besch_t *>(gib_kind(5));
			bild=liste2d->gib_bild(dir, ware_index);
			if(!bild) {
				if(dir>3) {
					bild = liste2d->gib_bild(dir - 4, ware_index);
				}
			}
			if (bild != NULL) return bild->gib_nummer();
		}

		// only try 1d freight image list for old style vehicles
		if(freight_image_type==0  &&  ware!=NULL) {
			liste = static_cast<const bildliste_besch_t *>(gib_kind(5));
		}
		else {
			liste = static_cast<const bildliste_besch_t *>(gib_kind(4));
		}

		if(!liste) {
			liste = static_cast<const bildliste_besch_t *>(gib_kind(4));
			if(!liste) {
				return IMG_LEER;
			}
		}

		bild = liste->gib_bild(dir);
		if(!bild) {
			if(dir>3) {
				bild = liste->gib_bild(dir - 4);
			}
			if(!bild) {
				return IMG_LEER;
			}
		}
		return bild->gib_nummer();
	}

	// Liefert die erlaubten Vorgaenger.
	// liefert gib_vorgaenger(0) == NULL, so bedeutet das entweder alle
	// Vorgänger sind erlaubt oder keine. Um das zu unterscheiden, sollte man
	// vorher hat_vorgaenger() befragen
	const vehikel_besch_t *gib_vorgaenger(int i) const
	{
		if(i < 0 || i >= vorgaenger) {
			return 0;
		}
		return static_cast<const vehikel_besch_t *>(gib_kind(6 + i));
	}

	int gib_vorgaenger_count() const { return vorgaenger; }

	// Liefert die erlaubten Nachfolger.
	// liefert gib_nachfolger(0) == NULL, so bedeutet das entweder alle
	// Nachfolger sind erlaubt oder keine. Um das zu unterscheiden, sollte
	// man vorher hat_nachfolger() befragen
	const vehikel_besch_t *gib_nachfolger(int i) const
	{
		if(i < 0 || i >= nachfolger) {
			return 0;
		}
		return static_cast<const vehikel_besch_t *>(gib_kind(6 + vorgaenger + i));
	}

	int gib_nachfolger_count() const { return nachfolger; }

	waytype_t gib_typ() const { return static_cast<waytype_t>(typ); }
	uint16 gib_zuladung() const { return zuladung; }
	uint32 gib_preis() const { return preis; }
	uint16 gib_geschw() const { return geschw; }
	uint16 gib_gewicht() const { return gewicht; }
	uint32 gib_leistung() const { return leistung; }
	uint16 gib_betriebskosten() const { return betriebskosten; }
	sint8 gib_sound() const { return sound; }

	/**
	* @return introduction year
	* @author Hj. Malthaner
	*/
	uint16 get_intro_year_month() const { return intro_date; }

	/**
	* @return time when obsolete
	* @author prissi
	*/
	uint16 get_retire_year_month() const { return obsolete_date; }

	// true if future
	bool is_future (const uint16 month_now) const
	{
		return month_now  &&  (intro_date > month_now);
	}

	// true if obsolete
	bool is_retired (const uint16 month_now) const
	{
		return month_now  &&  (obsolete_date <= month_now);
	}

	/**
	* 64 = 1.00
	* @return gear value
	* @author Hj. Malthaner
	*/
	uint16 get_gear() const { return gear; }

	/**
	* @return engine type
	* eletric engines require an electrified way to run
	* @author Hj. Malthaner
	*/
	uint8 get_engine_type() const { return engine_type; }

	/* @return the vehicles length in 1/8 of the normal len
	* @author prissi
	*/
	uint8 get_length() const { return len; }

};

#endif
