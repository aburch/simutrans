/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef vehicle_builder_t_h
#define vehicle_builder_t_h


#include "../dataobj/koord3d.h"
#include "../simtypes.h"
#include <string>

class vehicle_t;
class player_t;
class convoi_t;
class vehicle_desc_t;
class goods_desc_t;
template <class T> class slist_tpl;


/**
 * Baut Fahrzeuge. Fahrzeuge sollten nicht direct instanziiert werden
 * sondern immer von einem vehicle_builder_t erzeugt werden.
 *
 * Builds vehicles. Vehicles should not be instantiated directly,
 * but always from a vehicle_builder_t produced. (Google)
 *
 * @author Hj. Malthaner
 */
class vehicle_builder_t
{
public:
	// Retained for compatibility only
	static void rdwr_speedbonus(loadsave_t *file); 

	static bool register_desc(vehicle_desc_t *desc);
	static bool successfully_loaded();

	static vehicle_t* build(koord3d k, player_t* player, convoi_t* cnv, const vehicle_desc_t* vb )
	{
		return build(k, player, cnv, vb, false);
	}

	static vehicle_t* build(koord3d k, player_t* player, convoi_t* cnv, const vehicle_desc_t* vb, bool upgrade, uint16 livery_scheme_index = 0 );

	static const vehicle_desc_t * get_info(const char *name);
	static slist_tpl<vehicle_desc_t*> const& get_info(waytype_t);

	/* extended search for vehicles for KI
	* @author prissi
	*/
	static const vehicle_desc_t *vehicle_search(waytype_t typ,const uint16 month_now,const uint32 target_power,const sint32 target_speed, const goods_desc_t * target_freight, bool include_electric, bool not_obsolete );

	/* for replacement during load time
	 * prev_veh==NULL equals leading of convoi
	 */
	static const vehicle_desc_t *get_best_matching( waytype_t wt, const uint16 month_now, const uint32 target_weight, const uint32 target_power, const sint32 target_speed, const goods_desc_t * target_freight, bool not_obsolete, const vehicle_desc_t *prev_veh, bool is_last );
};

#endif
