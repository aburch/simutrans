/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef warenbauer_t_h
#define warenbauer_t_h

#include "../tpl/vector_tpl.h"
#include "../tpl/stringhashtable_tpl.h"

class ware_desc_t;

/**
 * Factory-Class for Goods.
 *
 * @author Hj. Malthaner
 */
class warenbauer_t
{
private:
	static stringhashtable_tpl<const ware_desc_t *> desc_names;
	static vector_tpl<ware_desc_t *> waren;

	static ware_desc_t *load_passagiere;
	static ware_desc_t *load_post;
	static ware_desc_t *load_nichts;

	// number of different good classes;
	static uint8 max_catg_index;

public:
	enum { INDEX_PAS=0, INDEX_MAIL=1, INDEX_NONE=2 };

	static const ware_desc_t *passagiere;
	static const ware_desc_t *post;
	static const ware_desc_t *nichts; //"Nothing".

	static bool alles_geladen();
	static bool register_desc(ware_desc_t *desc);

	static uint8 get_max_catg_index() { return max_catg_index; }

	/**
	* Search the good 'name' information and return
	* its description. Return NULL when the Good is
	* unknown.
	*
	* @param name the non-translated good name
	* @author Hj. Malthaner/V. Meyer
	*/
	static const ware_desc_t *get_info(const char* name);

	static const ware_desc_t *get_info(uint16 idx) { return waren[idx]; }

	static ware_desc_t *get_modifiable_info(uint16 idx) { return waren[idx]; }

	static uint16 get_count() { return waren.get_count(); }

	// good by catg
	static const ware_desc_t *get_info_catg(const uint8 catg);

	// good by catg_index
	static const ware_desc_t *get_info_catg_index(const uint8 catg_index);

	/*
	 * allow to multiply all prices, 1000=1.0
	 * used for the beginner mode
	 */
	static void set_multiplier(sint32 multiplier, uint16 scale_factor);

	/*
	 * Update the cache of speedbonuses by distance
	 */
	static void cache_speedbonuses(uint32 min_d, uint32 med_d, uint32 max_d, uint16 multiplier);
};

#endif
