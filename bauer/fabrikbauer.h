/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef fabrikbauer_t_h
#define fabrikbauer_t_h

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
 */
class fabrikbauer_t
{
private:

	// nedded for crossconnections checks
	class fabs_to_crossconnect_t {
	public:
		fabrik_t *fab;
		sint32 demand;
		fabs_to_crossconnect_t() { fab = NULL; demand  = 0; }
		fabs_to_crossconnect_t(fabrik_t *f,sint32 d) { fab = f; demand  = d; }

		bool operator == (const fabs_to_crossconnect_t& x) const { return fab == x.fab; }
		bool operator != (const fabs_to_crossconnect_t& x) const { return fab != x.fab; }
	};

	static stringhashtable_tpl<const fabrik_besch_t *> table;

	static int finde_anzahl_hersteller(const ware_besch_t *ware, uint16 timeline);
	static const fabrik_besch_t * finde_hersteller(const ware_besch_t *ware, uint16 timeline);

public:
	static void register_besch(fabrik_besch_t *besch);
	static bool alles_geladen();

	/**
	 * Teilt dem Hausbauer mit, dass eine neue Karte geladen oder generiert wird.
	 * In diesem Fall müssen wir die Liste der Fabrikpositionen neu initialisieren
	 * @author V. Meyer
	 */
	static void neue_karte( karte_t * );

	/* Create a certain numer of tourist attractions
	 * @author prissi
	 */
	static void verteile_tourist(karte_t* welt, int max_number);

	/**
	* Gives a factory description for a factory type
	* @author Hj.Malthaner
	*/
	static const fabrik_besch_t * get_fabesch(const char *fabtype);

	/**
	* Gives the factory description table
	* @author Hj.Malthaner
	*/
	static const stringhashtable_tpl<const fabrik_besch_t*>& get_fabesch() { return table; }

	/* returns a random consumer
	* @author prissi
	*/
	static const fabrik_besch_t *get_random_consumer(bool electric,climate_bits cl, uint16 timeline );

	static fabrik_t* baue_fabrik(karte_t* welt, koord3d* parent, const fabrik_besch_t* info, sint32 initial_prod_base, int rotate, koord3d pos, spieler_t* spieler);

	/**
	 * vorbedingung: pos ist für fabrikbau geeignet
	 / number of chains is the maximum number of waren types for which suppliers chains are built
	 * @return: Anzahl gebauter Fabriken
	 * @author Hj.Malthaner
	 */
	static int baue_hierarchie(koord3d* parent, const fabrik_besch_t* info, sint32 initial_prod_base, int rotate, koord3d* pos, spieler_t* sp, int number_of_chains );

	/**
	 * Helper function for baue_hierachie(): builts the connections (chain) for one single product)
	 * @return: Anzahl gebauter Fabriken
	 */
	static int baue_link_hierarchie(const fabrik_t* our_fab, const fabrik_besch_t* info, int lieferant_nr, spieler_t* sp);

	/* this function is called, whenever it is time for industry growth
	 * If there is still a pending consumer, it will first complete another chain for it
	 * If not, it will decide to either built a power station (if power is needed)
	 * or built a new consumer near the indicated position
	 * @return: number of factories built
	 */
	static int increase_industry_density( karte_t *welt, bool tell_me );


private:
	// bauhilfen
	static koord3d finde_zufallsbauplatz(karte_t *welt, koord3d pos, int radius, koord groesse,bool on_water, const haus_besch_t *besch, bool ignore_climates);

	// check, if we have to rotate the factories before building this tree
	static bool can_factory_tree_rotate( const fabrik_besch_t *besch );
};

#endif
