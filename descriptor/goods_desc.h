/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *  Copyright 2013 James Petts, Nathanael Nerode (neroden)
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __WARE_BESCH_H
#define __WARE_BESCH_H

#include "obj_base_desc.h"
#include "../simcolor.h"
#include "../network/checksum.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/piecewise_linear_tpl.h"
// Simworld is for adjusted speed bonus
//#include "../simworld.h"

class checksum_t;

// This has to have internal computations in uint64 to avoid internal computational overflow
// in worst-case scenario.
// distance: uint32, speedbonus: uint16, computation: sint64
// Signed computation is needed because the speedbonus values may *drop* as distance increases
typedef piecewise_linear_tpl<uint32, uint16, sint64> adjusted_speed_bonus_t;

struct fare_stage_t
{
	fare_stage_t(uint32 d, uint16 p)
	{
		price = p;
		to_distance = d;
	}
	fare_stage_t()
	{
		price = 0;
		to_distance = 0;
	}
	uint16 price;
	uint32 to_distance;
};

/**
 *  @author Volker Meyer, James Petts, neroden
 *
 *  Child nodes:
 *	0   Name
 *	1   Copyright
 *	2   Text: Name of measurement unit
 */
class goods_desc_t : public obj_named_desc_t {
	friend class goods_reader_t;
	friend class goods_manager_t;

	vector_tpl<fare_stage_t> values;
	vector_tpl<fare_stage_t> base_values;
	vector_tpl<fare_stage_t> scaled_values;

	adjusted_speed_bonus_t adjusted_speed_bonus;

	/**
	* Category of the good
	* @author Hj. Malthaner
	*/
	uint8 catg;

	/**
	* total index, all ware with same catg_index will be compatible,
	* including special freight
	* assigned during registration
	* @author prissi
	*/
	uint8 catg_index;

	/**
	 * index of the type,
	 * assigned during registration
	 */
	uint8 goods_index;

	COLOR_VAL color;

	/**
	* Bonus for fast transport given in percent!
	* @author Hj. Malthaner
	*/
	uint16 speed_bonus;

	/**
	* Weight in KG per unit of this good
	* @author Hj. Malthaner
	*/
	uint16 weight_per_unit;

	/* 
	* The number of different classes in which this comes
	* This is used only for passengers and mail
	* @author: jamespetts, May 2017
	*/
	uint8 number_of_classes;

	/*
	* Collection of the percentages of the base revenue
	* charged for each class of mail or passengers expressed
	* as percentages of the base revenue.
	*/
	vector_tpl<uint16> class_revenue_percentages;

public:
	// the measure for that good (crates, people, bags ... )
	const char *get_mass() const
	{
		return get_child<text_desc_t>(2)->get_text();
	}

	/**
	* @return Category of the good
	* @author Hj. Malthaner
	*/
	uint8 get_catg() const { return catg; }

	/**
	* @return Category of the good
	* @author Hj. Malthaner
	*/
	uint8 get_catg_index() const { return catg_index; }

	/**
	* @return internal index (just a number, passenger, then mail, then something ... )
	* @author prissi
	*/
	uint8 get_index() const { return goods_index; }

	/**
	* @return weight in KG per unit of the good
	* @author Hj. Malthaner
	*/
	uint16 get_weight_per_unit() const { return weight_per_unit; }

	/**
	* @return Name of the category of the good
	* @author Hj. Malthaner
	*/
	const char * get_catg_name() const;

	/*
	* The number of different classes in which this comes
	* This is used only for passengers and mail
	* @author: jamespetts, May 2017
	*/
	uint8 get_number_of_classes() const { return number_of_classes; }

	/**
	* Checks if this good can be interchanged with the other, in terms of
	* transportability.
	*
	* Inline because called very often
	*
	* @author Hj. Malthaner
	*/
	bool is_interchangeable(const goods_desc_t *other) const
	{
		return catg_index == other->get_catg_index();
	}

	/**
	* @return color for good table and waiting bars
	* @author Hj. Malthaner
	*/
	COLOR_VAL get_color() const { return color; }

	void calc_checksum(checksum_t *chk) const
	{
		chk->input(catg);
		chk->input(speed_bonus);
		chk->input(weight_per_unit);
		chk->input(number_of_classes); 
	}

	/**
	 * Base fare without speedbonus adjustment (implements fare stages)
	 * In units of 1/4096 of a simcent, for computational precision
	 */
	sint64 get_base_fare(uint32 distance_meters, uint32 starting_distance = 0) const;

	/**
	* @return speed bonus value of the good
	* @author Hj. Malthaner
	*/
	uint16 get_speed_bonus() const { return speed_bonus; }

	/**
	 * Extended has two special effects:
	 * (1) Below a certain distance the speed bonus rating is zero;
	 * (2) The speed bonus "fades in" above that distance and enlarges as distance continues,
	 *     until a "maximum distance".
	 * This returns the actual speed bonus rating.
	 * Distance is given in METERS
	 *
	 */
	uint16 get_adjusted_speed_bonus(uint32 distance) const
	{
		// Use the functional... it should be loaded by goods_manager_t::cache_speedbonuses
		return adjusted_speed_bonus(distance);
	}
	
	/*
	* This gets the total fare taking into account fare stages, comfort and catering, as well as the class
	* in which this unit travelled (if relevant for mail and passengers). 
	* The old speed bonus is no longer used here.
	* 
	* NOTE: The g_class variable is *not* the class of these passengers/mail, but rather the class of 
	* accommodation in which it/they travelled on its/their journey.
	*
	* In units of 1/4096 of a simcent, for computational precision
	*
	* This code is adapted from earlier code written by Neroden, which used the old speed 
	* bonus system.
	*/
	sint64 get_total_fare(uint32 distance_meters, uint32 starting_distance = 0u, uint8 comfort = 0u, uint8 catering_level = 0u, uint8 g_class = 0u, sint64 journey_tenths = 0u) const;

	/**
	 * Estimate an appropriate refund for a trip of tile_distance length.
	 * Returns in the same units as get_total_fare.
	 *
	 * Hopefully called rarely!
	 */
	sint64 get_refund(uint32 distance_meters) const;

	/**
	 * Fill the "scaled_values" array from the "values" array.
	 */
	void set_scale(uint16 scale_factor);

	uint16 get_class_revenue_percentage(uint8 g_class)
	{
		return class_revenue_percentages[g_class];
	}
};

#endif
