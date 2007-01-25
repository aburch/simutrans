/*
 * warenbauer.h
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef warenbauer_t_h
#define warenbauer_t_h

#include "../besch/ware_besch.h"

#include "../tpl/vector_tpl.h"
#include "../tpl/stringhashtable_tpl.h"

class ware_besch_t;

/**
 * Factory-Klasse fuer Waren.
 *
 * @author Hj. Malthaner
 */
class warenbauer_t
{
private:
	static stringhashtable_tpl<const ware_besch_t *> besch_names;
	static vector_tpl<ware_besch_t *> waren;

	static ware_besch_t *load_passagiere;
	static ware_besch_t *load_post;
	static ware_besch_t *load_nichts;

public:
	static const ware_besch_t *passagiere;
	static const ware_besch_t *post;
	static const ware_besch_t *nichts;

	static bool alles_geladen();
	static bool register_besch(ware_besch_t *besch);
	/**
	* Sucht information zur ware 'name' und gibt die
	* Beschreibung davon zurück. Gibt NULL zurück wenn die
	* Ware nicht bekannt ist.
	*
	* @param name der nicht-übersetzte Warenname
	* @author Hj. Malthaner/V. Meyer
	*/
	static const ware_besch_t *gib_info(const char* name);

	static const ware_besch_t *gib_info(unsigned int idx)
	{
		return waren[idx];
	}

	static bool ist_fabrik_ware(const ware_besch_t *ware)
	{
		return ware != post && ware != passagiere && ware != nichts;
	}

	static int gib_index(const ware_besch_t *ware)
	{
		return ware->gib_index();
	}

	static unsigned int gib_waren_anzahl()
	{
		return waren.get_count();
	}

	// ware by catg
	static const ware_besch_t *gib_info_catg(const sint8 catg);

	/*
	 * allow to multiply all prices, 1000=1.0
	 * used for the beginner mode
	 */
	static void set_multiplier(sint32 multiplier);
};

#endif
