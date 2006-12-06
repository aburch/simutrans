/*
 * fabrikbauer.h
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef fabrikbauer_t_h
#define fabrikbauer_t_h

#include "../tpl/slist_tpl.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../dataobj/koord3d.h"

class haus_besch_t;
class ware_besch_t;
class fabrik_besch_t;
class stadt_t;
class karte_t;
class spieler_t;
class fabrik_t;

/**
 * Diese Klasse baut Fabriken. Niemals Fabriken selbst instanziiren,
 * immer den Fabrikbauer benutzen!
 * @author Hj. Malthaner
 * @version $Revision: 1.2 $
 */
class fabrikbauer_t
{
	/**
	* Diese Klasse wird in verteile_industrie benötigt.
	* Sie merkt sich, was wir als nächstes bauen wollen.
	* @author V. Meyer
	*/
	struct bau_info_t {
		const fabrik_besch_t	*info;
		const haus_besch_t	*besch;
		int		rotate;
		koord	dim;

		void random(slist_tpl <const fabrik_besch_t *> &fab);
	};

	/**
	* Diese Klasse wird in verteile_industrie benötigt.
	* Sie dient dazu, daß  wir uns merken, bei welcher
	* Stadt schon welche Endfabrike gebaut wurde.
	* @author V. Meyer
	*/
	struct stadt_fabrik_t {
		const stadt_t	    *stadt;
		const fabrik_besch_t   *info;

		int operator != (const stadt_fabrik_t &x) const {
			return stadt != x.stadt || info != x.info;
		}
		int operator == (const stadt_fabrik_t &x) const {
			return !(*this != x);
		}
	};

	/*
	* Diese Liste enthält jeweils, die Städte, die von der
	* angegebenen Fabrik schon "einen mehr" haben.
	*/
	static slist_tpl <stadt_fabrik_t> gebaut;

	static stringhashtable_tpl<const fabrik_besch_t *> table;

	static int finde_anzahl_hersteller(const ware_besch_t *ware);
	static const fabrik_besch_t * finde_hersteller(const ware_besch_t *ware,int nr=0);

public:
	static void register_besch(fabrik_besch_t *besch);

	/* Create a certain numer of tourist attractions
	* @author prissi
	*/
	static void verteile_tourist(karte_t* welt, int max_number);

	/* Create a certain numer of industries
	* @author prissi
	*/
	static void verteile_industrie(karte_t* welt, int max_number, bool in_city);

	/**
	* Gives a factory description for a factory type
	* @author Hj.Malthaner
	*/
	static const fabrik_besch_t * gib_fabesch(const char *fabtype);

	/**
	* Gives the factory description table
	* @author Hj.Malthaner
	*/
	static const stringhashtable_tpl<const fabrik_besch_t*>& gib_fabesch() { return table; }

	/* returns a random consumer
	* @author prissi
	*/
	static const fabrik_besch_t *get_random_consumer(bool in_city,climate_bits cl);

	/**
	* vorbedingung: pos ist für fabrikbau geeignet
	* @return: Anzahl gebauter Fabriken
	* @author Hj.Malthaner
	*/
	static int baue_hierarchie(karte_t *welt, koord3d *parent, const fabrik_besch_t *info, int rotate, koord3d *pos, spieler_t *sp);

private:
	// bauhilfen
	static koord3d finde_zufallsbauplatz(karte_t *welt, koord3d pos, int radius, koord groesse,bool on_water, const haus_besch_t *besch);

	/**
	 * baue fabrik nach Angaben in info
	 * @author Hj.Malthaner
	 */
	static fabrik_t * baue_fabrik(karte_t *welt, koord3d *parent, const fabrik_besch_t *info, int rotate, koord3d pos, spieler_t *sp);
};

#endif
