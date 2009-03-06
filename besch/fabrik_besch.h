/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __FABRIK_BESCH_H
#define __FABRIK_BESCH_H

#include "obj_besch.h"
#include "haus_besch.h"
#include "skin_besch.h"
#include "ware_besch.h"
#include "../dataobj/koord.h"


/*
 * Fields are xref'ed from skin_besch_t
 */
class field_besch_t : public obj_besch_t {
	friend class factory_field_writer_t;
	friend class factory_field_reader_t;

private:
	uint8	has_winter;	// 0 or 1 for snow
	uint16 probability;	// between 0 ...10000
	uint16 max_fields;	// maximum number of fields around a single factory
	uint16 min_fields;	// number of fields to start with
	uint16 production_per_field;

public:
	const skin_besch_t *get_bilder() const { return static_cast<const skin_besch_t *>(get_child(0)); }
	const char *get_name() const { return get_bilder()->get_name(); }
	const char *get_copyright() const { return get_bilder()->get_copyright(); }

	uint8 has_snow_bild() const { return has_winter; }
	uint16 get_probability() const { return probability; }
	uint16 get_max_fields() const { return max_fields; }
	uint16 get_min_fields() const { return min_fields; }
	uint16 get_field_production() const { return production_per_field; }
};




/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Der Rauch einer Fabrik verweist auf eine allgemeine Rauchbeschreibung
 *
 *  Kindknoten:
 *	0   SKin
 */
class rauch_besch_t : public obj_besch_t {
	friend class factory_smoke_writer_t;
	friend class factory_smoke_reader_t;

private:
	koord pos_off;
	koord xy_off;
	sint16 zeitmaske;

public:
	const char *get_name() const { return get_bilder()->get_name(); }
	const char *get_copyright() const { return get_bilder()->get_copyright(); }
	const skin_besch_t *get_bilder() const { return static_cast<const skin_besch_t *>(get_child(0)); }

	// get the tile with the smoke
	koord get_pos_off( koord size, uint8 rotation) const {
		switch( rotation%4 ) {
			case 1: return koord( size.y-pos_off.y, pos_off.x );
			case 2: return koord( size.x-pos_off.x, size.y-pos_off.y );
			case 3: return koord( pos_off.y, size.x-pos_off.x );
		}
		return pos_off;
	}

	// offset in pixel (remember intern size TILE_STEPS==16)
	koord get_xy_off(uint8 rotation) const {
		switch( rotation%4 ) {
			case 1: return koord( 0, xy_off.y+xy_off.x/2 );
			case 2: return koord( -xy_off.x, xy_off.y );
			case 3: return koord( 0, xy_off.y-xy_off.x/2 );
		}
		return xy_off;
	}

	sint16 get_zeitmaske() const { return zeitmaske; }
};


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Ein Verbrauchsgut einer Fabriktyps
 *
 *  Kindknoten:
 *	0   Ware
 */
class fabrik_lieferant_besch_t : public obj_besch_t {
	friend class factory_supplier_reader_t;
	friend class factory_supplier_writer_t;

private:
	uint16  kapazitaet;
	uint16  anzahl;
	uint16  verbrauch;

public:
	const ware_besch_t *get_ware() const { return static_cast<const ware_besch_t *>(get_child(0)); }
	int get_kapazitaet() const { return kapazitaet; } //"capacity" (Babelfish)
	int get_anzahl() const { return anzahl; } //"number" (Babelfish)
	int get_verbrauch() const { return verbrauch; } //"consumption" (Babelfish)
};


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Eine Produktion eines Fabriktyps
 *
 *  Kindknoten:
 *	0   Ware
 */
class fabrik_produkt_besch_t : public obj_besch_t {
	friend class factory_product_writer_t;
	friend class factory_product_reader_t;

private:
    uint16 kapazitaet;

    /**
     * How much of this product is derived from one unit of factory
     * production? 256 means 1.0
     * @author Hj. Malthaner
     */
    uint16 faktor;

public:
	const ware_besch_t *get_ware() const { return static_cast<const ware_besch_t *>(get_child(0)); }
	uint32 get_kapazitaet() const { return kapazitaet; }
	uint32 get_faktor() const { return faktor; }
};


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Jetzt endlich die Ganze Fabrik
 *
 *  Kindknoten:
 *	0   Haus
 *	1   Rauch
 *	2   Lieferant 1
 *	3   Lieferant 2
 *	... ...
 *	n+1 Lieferant n
 *	n+2 Produkt 1
 *	n+3 Produkt 2
 *	... ...
 */
class fabrik_besch_t : public obj_besch_t {
	friend class factory_reader_t;
	friend class factory_writer_t;

public:
	enum platzierung {Land, Wasser, Stadt};

private:
	enum platzierung platzierung; //"placement" (Babelfish)
	uint16 produktivitaet; //"productivity" (Babelfish)
	uint16 bereich; //"range" (Babelfish)
	uint16 gewichtung;	// Wie wahrscheinlich soll der Bau sein? ("How likely will the building be?" (Google)). 
	uint8 kennfarbe; //"identification colour code" (Babelfish)
	uint16 lieferanten; //"supplier" (Babelfish)
	uint16 produkte; //"products" (Babelfish)
	uint8 fields;	// only if there are any ...
	uint16 pax_level;
	bool electricity_producer;

public:
	/*
	* Name und Copyright sind beim Gebäude gespeichert!
	*/
	const char *get_name() const { return get_haus()->get_name(); }
	const char *get_copyright() const { return get_haus()->get_copyright(); }
	const haus_besch_t *get_haus() const { return static_cast<const haus_besch_t *>(get_child(0)); }
	const rauch_besch_t *get_rauch() const { return static_cast<const rauch_besch_t *>(get_child(1)); }

	// we must take care, for the case of no producer/consumer
	const fabrik_lieferant_besch_t *get_lieferant(int i) const //"supplier" (Babelfish)
	{
		return (i >= 0 && i < lieferanten) ? static_cast<const fabrik_lieferant_besch_t *>(get_child(2 + i)) : NULL;
	}
	const fabrik_produkt_besch_t *get_produkt(int i) const
	{
		return (i >= 0 && i < produkte) ? static_cast<const fabrik_produkt_besch_t *>(get_child(2 + lieferanten + i)) : NULL;
	}
	const field_besch_t *get_field() const {
		if(!fields) return NULL;
		return static_cast<const field_besch_t *>(get_child(2 + lieferanten + produkte));
	}

	int get_lieferanten() const { return lieferanten; } //"supplier" (Babelfish)
	uint get_produkte() const { return produkte; }

	/* where to built */
	enum platzierung get_platzierung() const { return platzierung; }
	int get_gewichtung() const { return gewichtung;     }

	uint8 get_kennfarbe() const { return kennfarbe; } //"identification colour code" (Babelfish)

	void set_produktivitaet(int p) { produktivitaet=p; }
	int get_produktivitaet() const { return produktivitaet; }
	int get_bereich() const { return bereich; } //"range" (Babelfish)

	/* level for post and passenger generation */
	int get_pax_level() const { return pax_level; }

	int is_electricity_producer() const { return electricity_producer; }
};

#endif
