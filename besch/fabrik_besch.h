/*
 *
 *  fabrik_besch.h
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
#ifndef __FABRIK_BESCH_H
#define __FABRIK_BESCH_H

/*
 *  includes
 */
#include "obj_besch.h"
#include "haus_besch.h"
#include "skin_besch.h"
#include "ware_besch.h"
#include "../dataobj/koord.h"


/*
 *  class:
 *      rauch_besch_t()
 *
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

    koord pos_off;
    koord xy_off;
    sint16 zeitmaske;

public:
    const char *gib_name() const
    {
        return gib_bilder()->gib_name();
    }
    const char *gib_copyright() const
    {
        return gib_bilder()->gib_copyright();
    }
    const skin_besch_t *gib_bilder() const
    {
	return static_cast<const skin_besch_t *>(gib_kind(0));
    }
    koord gib_pos_off() const
    {
    	return pos_off;
    }
    koord gib_xy_off() const
    {
    	return xy_off;
    }
    int gib_zeitmaske() const
    {
    	return zeitmaske;
    }
};


/*
 *  class:
 *      fabrik_lieferant_besch_t()
 *
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
    friend class factory_supplier_writer_t;

    uint16  kapazitaet;
    uint16  anzahl;
    uint16  verbrauch;

public:
    const ware_besch_t *gib_ware() const
    {
	return static_cast<const ware_besch_t *>(gib_kind(0));
    }
    int gib_kapazitaet() const
    {
	return kapazitaet;
    }
    int gib_anzahl() const
    {
	return anzahl;
    }
    int gib_verbrauch() const
    {
	return verbrauch;
    }
};

/*
 *  class:
 *      fabrik_produkt_besch_t()
 *
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

    uint16 kapazitaet;

    /**
     * How much of this product is derived from one unit of factory
     * production? 256 means 1.0
     * @author Hj. Malthaner
     */
    uint16 faktor;

public:
    const ware_besch_t *gib_ware() const
    {
	return static_cast<const ware_besch_t *>(gib_kind(0));
    }

    uint32 gib_kapazitaet() const
    {
	return kapazitaet;
    }

    uint32 gib_faktor() const
    {
      return faktor;
    }
};



/*
 *  class:
 *      fabrik_besch_t()
 *
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
    enum platzierung platzierung;
    uint16 produktivitaet;
    uint16 bereich;
    uint16 gewichtung;	// Wie wahrscheinlich soll der Bau sein?
    uint16 kennfarbe;
    uint16 lieferanten;
    uint16 produkte;
    uint16 pax_level;

public:
    /*
     * Name und Copyright sind beim Gebäude gespeichert!
     */
    const char *gib_name() const
    {
	return gib_haus()->gib_name();
    }
    const char *gib_copyright() const
    {
	return gib_haus()->gib_copyright();
    }
    const haus_besch_t *gib_haus() const
    {
	return static_cast<const haus_besch_t *>(gib_kind(0));
    }
    const rauch_besch_t *gib_rauch() const
    {
	return static_cast<const rauch_besch_t *>(gib_kind(1));
    }
    const fabrik_lieferant_besch_t *gib_lieferant(int i) const
    {
	return i >= 0 && i < lieferanten ?
	    static_cast<const fabrik_lieferant_besch_t *>(gib_kind(2 + i)) : 0;
    }
    const fabrik_produkt_besch_t *gib_produkt(int i) const
    {
	return i >= 0 && i < produkte ?
	    static_cast<const fabrik_produkt_besch_t *>(gib_kind(2 + lieferanten + i)) : 0;
    }

    int gib_lieferanten() const
    {
	return lieferanten;
    }
    int gib_produkte() const
    {
	return produkte;
    }

    /* where to built */
    enum platzierung gib_platzierung() const { return platzierung; }
    int gib_gewichtung() const { return gewichtung;     }

    int gib_kennfarbe() const { return kennfarbe; }

    int gib_produktivitaet() const { return produktivitaet; };
    int gib_bereich() const { return bereich; }

    /* level for post and passenger generation */
    int gib_pax_level() const { return pax_level; };
};


#endif // __FABRIK_BESCH_H
