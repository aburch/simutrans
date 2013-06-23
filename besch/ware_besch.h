/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *  Copyright 2013 James Petts, Nathanael Nerode (neroden)
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __WARE_BESCH_H
#define __WARE_BESCH_H

#include "obj_besch_std_name.h"
#include "../simcolor.h"
#include "../utils/checksum.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/piecewise_linear_tpl.h"
// Simworld is for adjusted speed bonus
#include "../simworld.h"

class checksum_t;

// This has to have internal computations in uint64 to avoid internal computational overflow
// in worst-case scenario.
// distance: uint32, speedbonus: uint16, computation: uint64
typedef piecewise_linear_tpl<uint32, uint16, uint64> adjusted_speed_bonus_t;

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
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Text Maßeinheit
 */
class ware_besch_t : public obj_besch_std_name_t {
	friend class good_reader_t;
	friend class warenbauer_t;

	/*
	* The base value is the one for multiplier 1000.
	*/
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

	// used for inderect index (saves 3 bytes per ware_t!)
	// assinged during registration
	uint8 ware_index;

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

public:
	// the measure for that good (crates, people, bags ... )
	const char *get_mass() const
	{
		return get_child<text_besch_t>(2)->get_text();
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
	uint8 get_index() const { return ware_index; }

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

	/**
	* Checks if this good can be interchanged with the other, in terms of
	* transportability.
	*
	* Inline because called very often
	*
	* @author Hj. Malthaner
	*/
	bool is_interchangeable(const ware_besch_t *other) const
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
		chk->input(base_values.get_count());
		ITERATE(base_values, i)
		{
			chk->input(base_values[i].to_distance);
			chk->input(base_values[i].price);
		}
		chk->input(catg);
		/*chk->input(catg_index);*/ // For some reason this line causes false mismatches.
		chk->input(speed_bonus);
		chk->input(weight_per_unit);
	}

	/**
	 * This method returns the *total* fare for these
	 * goods over the given distance, in *meters*.
	 *
	 * This is the base fare, not adjusted for speedbonus...
	 * This returns values in units of 1/1000 of a simcent
	 *
	 * This is very different from the similarly-named routine in standard
	 * @author: jamespetts, neroden
	 */
	sint64 get_fare(uint32 distance_meters, uint32 starting_distance = 0) const
	{
		sint64 total_fare = 0;
		uint16 per_tile_fare;
		uint32 remaining_distance = distance_meters;
		ITERATE(scaled_values, i)
		{
			per_tile_fare = scaled_values[i].price;
			if(i < scaled_values.get_count() - 1 && starting_distance >= scaled_values[i].to_distance)
			{
				starting_distance -= scaled_values[i].to_distance;
				continue;
			}

			if(scaled_values[i].to_distance >= remaining_distance || i == scaled_values.get_count() - 1)
			{
				// The last item in the list must trigger the use of the full remaining distance.
				total_fare += (sint64)per_tile_fare * remaining_distance;
				break;
			}
			else
			{
				total_fare += (sint64)per_tile_fare * (scaled_values[i].to_distance - starting_distance);
				remaining_distance -= (scaled_values[i].to_distance - starting_distance);
				starting_distance = 0;
			}
		}
		return total_fare;
	}

	void set_scale(uint16 scale_factor)
	{
		scaled_values.clear();
		uint16 new_price;
		uint32 new_distance;
		ITERATE(values, i)
		{
			// There are two things going on here:
			// (1) price is per km, divide by 1000 to get per meter
			// (2) price is in simcents, multiply by 1000 to get "internal units"
			new_price = values[i].price * (1000 / 1000);
			new_distance = values[i].to_distance * 1000;
			scaled_values.append(fare_stage_t(new_distance, new_price));
		}
	}

	/*
	 * Speed bonus handling
	 *
	 * The world has a reference speed.
	 * The packet of goods will be paid for based on a certain "revenue speed".
	 * (As of June 2013 this is the line's average speed for the trip.  It would
	 * be much simpler if it were the actual speed of the trip.)
	 * The revenue speed divided by the reference speed gives a ratio.
	 * Subtract 1 from that ratio;
	 * then multiply by 100 to get the "relative speed percentage" for speed.
	 * So if the speed is 10% faster than reference speed, the "relative speed percentage" is 10; 
	 * if it is 10% slower, the "relative speed percentage" is -10.
	 *
	 * The "speed bonus rating" is actually a factor, and a very funny one.
	 * If it is 1, then for every 1% improvement in speed, the revenue goes up by 0.1%.
	 * If it is 2, then for every 1% improvement in speed, the revenue goes up by 0.2%.
	 * If it is 10, then for every 1% improvement in speed, the revenue goes up by 1.0%.
	 *
	 * So when we multiply the speed percentage by the speed bonus rating we get a per 1000 number.
	 * This has to be altered for future fast computation.
	 *
	 * OK.  Now consider the base fare.
	 * The maximum fare is 4 times the base fare... divided by 3....
	 * The minimum fare is 1/4 the base fare... divided by 3....
	 * The standard fare is super freaking complicated for no good reason...
	 * Roughly speaking, it is
	 * standard_fare = base_fare * (1 + relative_speed_percentage * speed_bonus_rating)
	 * (divided by 3)
	 * where speed_bonus_rating is the official speed bonus "percentage", usually a one or
	 * two digit number.
	 *
	*/

	/**
	* @return speed bonus value of the good
	* @author Hj. Malthaner
	*/
	uint16 get_speed_bonus() const { return speed_bonus; }

	/**
	 * Experimental has two special effects:
	 * (1) Below a certain distance the speed bonus rating is zero;
	 * (2) The speed bonus "fades in" above that distance and enlarges as distance continues,
	 *     until a "maximum distance".
	 * This returns the actual speed bonus rating.
	 * Distance is given in METERS
	 *
	 */
	uint16 get_adjusted_speed_bonus(uint32 distance) const
	{
		// Use the functional... it should be loaded by warenbauer_t::cache_speedbonuses
		return adjusted_speed_bonus(distance);
	}

	/**
	 * This variant of get_fare gives the revenue *with* the speedbonus adjustment.
	 * Importantly, this gives the revenue in *special units*.
	 * Currently these units are 1/1000 of the regular simcent. (times three!  FIXME!)
	 * This will be changed for efficiency in future versions of this code --neroden
	 *
	 * This must take the *relative speed percentage* as an argument.
	 * See above for how this is defined; it is always a percent, with no greater
	 * precision than that.  (Consider changing this.)
	 * It will stay in the range of a sint16 unless the pak is completely broken.
	 */
	sint64 get_fare_with_speedbonus(sint16 relative_speed_percentage, uint32 distance_meters, uint32 starting_distance = 0) const
	{
		sint64 base_fare = get_fare(distance_meters, starting_distance);
			// We must be able to multiply by, say, 2^16;
			// otherwise we may overflow computation computing the standard fare.
			// This would require a completely unbalanced pakset;
			// fares are orders of magnitude smaller than this.
		assert (base_fare <= 0x0000FFFFFFFFll );
			// If the assertion fails, we can't safely multiply by 4 * 1024.
		sint64 min_fare = base_fare / 4ll; // minimum fare is 1/4 base
		sint64 max_fare = base_fare * 4ll; // maximum fare is 4 * base
		sint64 speed_bonus_rating = get_adjusted_speed_bonus(distance_meters);
		// Recall the screwy definition of the speed_bonus_rating from above.
		// Given a percentage of 1 and a speed bonus rating of 1, we increase revenue by 1/1000.
		// Percentage may be as high as 1000, bonus rating as high as 50...
		//
		// Consider altering the definition to divide by 1024.  It reduces revenue *slightly*
		// but not very much -- neroden
		sint64 bonus_per_mill = (sint64)relative_speed_percentage * speed_bonus_rating;
		sint64 standard_fare = base_fare + base_fare * bonus_per_mill / 1000ll;
		sint64 actual_fare = min( max_fare, max(min_fare, standard_fare) );
		return actual_fare;
	}

	/**
	 * Estimate an appropriate refund for a trip of tile_distance length.
	 * Returns in the same units as get_fare_with_speedbonus.
	 *
	 * Hopefully called rarely!
	 */
	sint64 get_refund(uint32 distance_meters) const;
};

#endif
