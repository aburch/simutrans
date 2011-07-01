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
#include "../tpl/weighted_vector_tpl.h"

class checksum_t;

/* Knightly : this besch will store data specific to each class of fields
 * Fields are xref'ed from skin_besch_t
 */
class field_class_besch_t : public obj_besch_t {
	friend class factory_field_class_reader_t;
	friend class factory_field_group_reader_t;		// Knightly : this is a special case due to besch restructuring

private:
	uint8  snow_image;			// 0 or 1 for snow
	uint16 production_per_field;
	uint16 storage_capacity;
	uint16 spawn_weight;

public:
	skin_besch_t const* get_bilder() const { return get_child<skin_besch_t>(0); }
	const char *get_name() const { return get_bilder()->get_name(); }
	const char *get_copyright() const { return get_bilder()->get_copyright(); }

	uint8 has_snow_image() const { return snow_image; }
	uint16 get_field_production() const { return production_per_field; }
	uint16 get_storage_capacity() const { return storage_capacity; }
	uint16 get_spawn_weight() const { return spawn_weight; }

	void calc_checksum(checksum_t *chk) const;
};


// Knightly : this besch now only contains common, shared data regarding fields
class field_group_besch_t : public obj_besch_t {
	friend class factory_field_group_reader_t;

private:
	uint16 probability;		// between 0 ...10000
	uint16 max_fields;		// maximum number of fields around a single factory
	uint16 min_fields;		// number of fields to start with
	uint16 field_classes;	// number of field classes

	weighted_vector_tpl<uint16> field_class_indices;

public:
	// fills the array, is only called once during alles_geladen() after resolve xrefs
	void init_field_class_indices()
	{
		if(  field_classes>0  ) {
			field_class_indices.clear();
			field_class_indices.resize( field_classes );
			for(  uint16 i=0  ;  i<field_classes  ;  ++i  ) {
				field_class_indices.append( i, get_field_class(i)->get_spawn_weight() );
			}
		}
	}

	uint16 get_probability() const { return probability; }
	uint16 get_max_fields() const { return max_fields; }
	uint16 get_min_fields() const { return min_fields; }
	uint16 get_field_class_count() const { return field_classes; }
	field_class_besch_t const* get_field_class(uint16 const idx) const { return idx < field_classes ? get_child<field_class_besch_t>(idx) : 0; }
	const weighted_vector_tpl<uint16> &get_field_class_indices() const { return field_class_indices; }

	void calc_checksum(checksum_t *chk) const;
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
	friend class factory_smoke_reader_t;

private:
	koord pos_off;
	koord xy_off;
	sint16 zeitmaske;

public:
	const char *get_name() const { return get_bilder()->get_name(); }
	const char *get_copyright() const { return get_bilder()->get_copyright(); }
	skin_besch_t const* get_bilder() const { return get_child<skin_besch_t>(0); }

	// get the tile with the smoke
	koord get_pos_off( koord size, uint8 rotation) const {
		switch( rotation%4 ) {
			case 1: return koord( size.y-pos_off.y, pos_off.x );
			case 2: return koord( size.x-pos_off.x, size.y-pos_off.y );
			case 3: return koord( pos_off.y, size.x-pos_off.x );
		}
		return pos_off;
	}

	// offset in pixel (depends on OBJECT_OFFSET_STEPS==16)
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

private:
	uint16  kapazitaet;
	uint16  anzahl;
	uint16  verbrauch;

public:
	ware_besch_t const* get_ware() const { return get_child<ware_besch_t>(0); }
	int get_kapazitaet() const { return kapazitaet; } //"capacity" (Babelfish)
	int get_anzahl() const { return anzahl; } //"number" (Babelfish)
	int get_verbrauch() const { return verbrauch; } //"consumption" (Babelfish)
	void calc_checksum(checksum_t *chk) const;
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
	ware_besch_t const* get_ware() const { return get_child<ware_besch_t>(0); }
	uint32 get_kapazitaet() const { return kapazitaet; }
	uint32 get_faktor() const { return faktor; }
	void calc_checksum(checksum_t *chk) const;
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

public:
	enum site_t { Land, Wasser, Stadt };

private:
	site_t platzierung; //"placement" (Babelfish)
	uint16 produktivitaet; //"productivity" (Babelfish)
	uint16 bereich; //"range" (Babelfish)
	uint16 gewichtung;	// Wie wahrscheinlich soll der Bau sein? ("How likely will the building be?" (Google)). 
	uint8 kennfarbe; //"identification colour code" (Babelfish)
	uint16 lieferanten; //"supplier" (Babelfish)
	uint16 produkte; //"products" (Babelfish)
	uint8 fields;	// only if there are any ...
	uint16 pax_level;
	uint16 electricity_proportion; // Modifier of electricity consumption (a legacy setting for Experimental only)
	uint16 inverse_electricity_proportion;
	bool electricity_producer;
	uint8 upgrades; // The industry types to which this industry can be upgraded.
	uint16 expand_probability;
	uint16 expand_minimum;
	uint16 expand_range;
	uint16 expand_times;
	uint16 electric_boost;
	uint16 pax_boost;
	uint16 mail_boost;
	uint16 electric_amount;
	uint16 pax_demand;
	uint16 mail_demand;

public:
	/*
	* Name und Copyright sind beim Gebäude gespeichert!
	*/
	const char *get_name() const { return get_haus()->get_name(); }
	const char *get_copyright() const { return get_haus()->get_copyright(); }
	haus_besch_t  const* get_haus()  const { return get_child<haus_besch_t>(0); }
	rauch_besch_t const* get_rauch() const { return get_child<rauch_besch_t>(1); }

	// we must take care, for the case of no producer/consumer
	const fabrik_lieferant_besch_t *get_lieferant(int i) const //"supplier" (Babelfish)
	{
		return 0 <= i && i < lieferanten ? get_child<fabrik_lieferant_besch_t>(2 + i) : 0;
	}
	const fabrik_produkt_besch_t *get_produkt(int i) const
	{
		return 0 <= i && i < produkte ? get_child<fabrik_produkt_besch_t>(2 + lieferanten + i) : 0;
	}
	const field_group_besch_t *get_field_group() const {
		if(!fields) {
			return NULL;
		}
		return get_child<field_group_besch_t>(2 + lieferanten + produkte);
	}

	int get_lieferanten() const { return lieferanten; } //"supplier" (Babelfish)
	uint get_produkte() const { return produkte; } // "Products" (Google)

	/* where to built */
	site_t get_platzierung() const { return platzierung; }
	int get_gewichtung() const { return gewichtung;     }

	uint8 get_kennfarbe() const { return kennfarbe; } //"identification colour code" (Babelfish)

	void set_produktivitaet(int p) { produktivitaet=p; }
	int get_produktivitaet() const { return produktivitaet; }
	int get_bereich() const { return bereich; } //"range" (Babelfish)

	/* level for post and passenger generation */
	int get_pax_level() const { return pax_level; }

	uint16 get_electricity_proportion() const { return electricity_proportion; }
	uint16 get_inverse_electricity_proportion() const { return inverse_electricity_proportion; }

	int is_electricity_producer() const { return electricity_producer; }

	const fabrik_besch_t *get_upgrades(int i) const { return (i >= 0 && i < upgrades) ? get_child<fabrik_besch_t>(2 + lieferanten + produkte + fields + i) : NULL; }

	int get_upgrades_count() const { return upgrades; }

	uint16 get_expand_probability() const { return expand_probability; }
	uint16 get_expand_minumum() const { return expand_minimum; }
	uint16 get_expand_range() const { return expand_range; }
	uint16 get_expand_times() const { return expand_times; }

	uint16 get_electric_boost() const { return electric_boost; }
	uint16 get_pax_boost() const { return pax_boost; }
	uint16 get_mail_boost() const { return mail_boost; }
	uint16 get_electric_amount() const { return electric_amount; }
	uint16 get_pax_demand() const { return pax_demand; }
	uint16 get_mail_demand() const { return mail_demand; }

	void calc_checksum(checksum_t *chk) const;
};

#endif
