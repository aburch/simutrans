/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __WARE_BESCH_H
#define __WARE_BESCH_H

#include "obj_besch_std_name.h"
#include "../simcolor.h"
#include "../utils/checksum.h"
#include "../tpl/vector_tpl.h"
// Simworld is for adjusted speed bonus
#include "../simworld.h"

class checksum_t;

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

/*
 *  Autor:
 *      Volker Meyer
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
	 * This method returns the *total* fare for these
	 * goods over the given distance, in tiles. This is
	 * not the per-tile fare
	 * @author: jamespetts, November 2011
	 */
	sint64 get_fare(uint32 tile_distance, uint32 starting_distance = 0) const
	{
		sint64 total_fare = 0;
		uint16 per_tile_fare;
		uint32 remaining_distance = tile_distance;
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
			new_price = (values[i].price * scale_factor) / 1000;		
			new_distance = (values[i].to_distance * 1000) / scale_factor;
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
	 * then multiply by 100 to get the "magic number" (speed_base) for speed.
	 * So if the speed is 10% faster than reference speed,
	 * the "magic number" is 10; if it is 10% slower, the magic number is -10.
	 *
	 * OK.  Now consider the base fare.
	 * The maximum fare is 4 times the base fare... divided by 3....
	 * The minimum fare is 1/4 the base fare... divided by 3....
	 * The standard fare is super freaking complicated for no good reason...
	 * base_bonus = base_fare * (1 + speed_base * speed_bonus_rating)
	 * where speed_bonus_rating is the official speed bonus "percentage", usually a one or
	 * two digit number.
	 *
	 * The "speed bonus rating" is actually a factor, and a very funny one.
	 * If it is 1, then for every 1% improvement in speed, the revenue goes up by 0.1%.
	 * If it is 2, then for every 1% improvement in speed, the revenue goes up by 0.2%.
	 * If it is 10, then for every 1% improvement in speed, the revenue goes up by 1.0%.
	*/

	/**
	* @return speed bonus value of the good
	* @author Hj. Malthaner
	*/
	uint16 get_speed_bonus() const { return speed_bonus; }

	/**
	 * Experimental has two special effects:
	 * (1) Below a certain distance the speed bonus is zero;
	 * (2) The speed bonus "fades in" above that distance and enlarges as distance continues,
	 *     until a "maximum distance".
	 * This returns the actual speed bonus.
	 * Distance is given in TILES (FIXME)
	 */
	uint16 get_adjusted_speed_bonus(uint32 distance, const karte_t* w) const
	{
		return ware_besch_t::get_adjusted_speed_bonus(get_speed_bonus(), distance, w);
	}

	/**
	 * Static version takes speed bonus as argument
	 */
	static uint16 get_adjusted_speed_bonus(uint16 base_bonus, uint32 distance, const karte_t* w);

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
};

#endif
