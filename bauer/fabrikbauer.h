/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef fabrikbauer_t_h
#define fabrikbauer_t_h

#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/weighted_vector_tpl.h"
#include "../dataobj/koord3d.h"

class haus_besch_t;
class ware_besch_t;
class fabrik_besch_t;
class stadt_t;
class karte_ptr_t;
class spieler_t;
class fabrik_t;


/**
 * This class builds factories. Never construct factories directly
 * but always by calling fabrikbauer_t::baue() (for a single factory)
 * or fabrikbauer_t::baue_hierachie() (for a full chain of suppliers).
 */
class fabrikbauer_t
{
private:
	static karte_ptr_t welt;

	/**
	 * @class fabs_to_crossconnect_t
	 * Used for cross-connection checks between factories.
	 * This is necessary for finding producers for factory supply.
	 */
	class fabs_to_crossconnect_t {
	public:
		fabrik_t *fab;		///< The factory
		sint32 demand;		///< To how many factories this factory needs to connect to

		fabs_to_crossconnect_t() { fab = NULL; demand = 0; }
		fabs_to_crossconnect_t(fabrik_t *f, sint32 d) { fab = f; demand = d; }

		bool operator == (const fabs_to_crossconnect_t& x) const { return fab == x.fab; }
		bool operator != (const fabs_to_crossconnect_t& x) const { return fab != x.fab; }
	};

	/// Table of all factories that can be built
	static stringhashtable_tpl<const fabrik_besch_t *> table;

	/// @returns the number of producers producing @p ware
	static int finde_anzahl_hersteller(const ware_besch_t *ware, uint16 timeline);

	/**
	 * Finds a random producer producing @p ware.
	 * @param timeline the current time (months)
	 */
	static void finde_hersteller(weighted_vector_tpl<const fabrik_besch_t *> &producer, const ware_besch_t *ware, uint16 timeline );

public:
	/// Registers the factory description so the factory can be built in-game.
	static void register_besch(fabrik_besch_t *besch);

	/**
	 * Initializes weighted vector for farm field class indices.
	 * @returns true
	 */
	static bool alles_geladen();

	/**
	 * Tells the factory builder a new map is being loaded or generated.
	 * In this case the list of all factory positions must be reinitialized.
	 */
	static void neue_karte();

	/// Creates a certain number of tourist attractions.
	static void verteile_tourist(int max_number);

	/// @returns a factory description for a factory name
	static const fabrik_besch_t * get_fabesch(const char *fabtype);

	/// @returns the table containing all factory descriptions
	static const stringhashtable_tpl<const fabrik_besch_t*>& get_fabesch() { return table; }

	/**
	 * @param electric true to limit search to electricity producers only
	 * @param cl allowed climates
	 * @returns a random consumer
	 */
	static const fabrik_besch_t *get_random_consumer(bool electric, climate_bits cl, uint16 timeline );

	/**
	 * Builds a single new factory.
	 *
	 * @param parent location of the parent factory
	 * @param info Description for the new factory
	 * @param initial_prod_base Initial base production (-1 to disable)
	 * @param rotate building rotation (0..3)
	 * @returns The newly constructed factory.
	 */
	static fabrik_t* baue_fabrik(koord3d* parent, const fabrik_besch_t* info, sint32 initial_prod_base, int rotate, koord3d pos, spieler_t* spieler);

	/**
	 * Builds a new full chain of factories. Precondition before calling this function:
	 * @p pos is suitable for factory construction and number of chains
	 * is the maximum number of good types for which suppliers chains are built
	 * (meaning there are no unfinished factory chains).
	 * @returns number of factories built
	 */
	static int baue_hierarchie(koord3d* parent, const fabrik_besch_t* info, sint32 initial_prod_base, int rotate, koord3d* pos, spieler_t* sp, int number_of_chains );

	/**
	 * Helper function for baue_hierachie(): builds the connections (chain) for one single product)
	 * @returns number of factories built
	 */
	static int baue_link_hierarchie(const fabrik_t* our_fab, const fabrik_besch_t* info, int lieferant_nr, spieler_t* sp);

	/**
	 * This function is called whenever it is time for industry growth.
	 * If there is still a pending consumer, this method will first complete another chain for it.
	 * If not, it will decide to either build a power station (if power is needed)
	 * or build a new consumer near the indicated position.
	 * @returns number of factories built
	 */
	static int increase_industry_density( bool tell_me );

private:
	/**
	 * Checks if the site at @p pos is suitable for construction.
	 * @param groesse Size of the building site
	 * @param water true to search on water
	 * @param cl allowed climates
	 */
	static bool ist_bauplatz(koord pos, koord groesse, bool water, bool is_fabrik, climate_bits cl);

	/**
	 * Find a random site to place a factory.
	 * @param radius Radius of the search circle around @p pos
	 * @param groesse size of the building site
	 */
	static koord3d finde_zufallsbauplatz(koord pos, int radius, koord groesse,bool on_water, const haus_besch_t *besch, bool ignore_climates, uint32 max_iterations);

	/**
	 * Checks if all factories in this factory tree can be rotated.
	 * This method is called recursively on all potential suppliers.
	 * @returns true if all factories in this tree can be rotated.
	 */
	static bool can_factory_tree_rotate( const fabrik_besch_t *besch );
};

#endif
