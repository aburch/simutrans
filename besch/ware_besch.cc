#include "ware_besch.h"
#include "../simworld.h"
#include "../dataobj/einstellungen.h"

static const char * catg_names[32] = {
  "special freight",
  "CATEGORY_01",	// was "piece goods",
  "CATEGORY_02",	// was "bulk goods",
  "CATEGORY_03",	// was "oil/gasoline",
  "CATEGORY_04",	// was "cooled goods",
  "CATEGORY_05",	// was "liquid food",
  "CATEGORY_06",	// was "long goods",
  "CATEGORY_07",

  "CATEGORY_08",
  "CATEGORY_09",
  "CATEGORY_10",
  "CATEGORY_11",
  "CATEGORY_12",
  "CATEGORY_13",
  "CATEGORY_14",
  "CATEGORY_15",

  "CATEGORY_16",
  "CATEGORY_17",
  "CATEGORY_18",
  "CATEGORY_19",
  "CATEGORY_20",
  "CATEGORY_21",
  "CATEGORY_22",
  "CATEGORY_23",

  "CATEGORY_24",
  "CATEGORY_25",
  "CATEGORY_26",
  "CATEGORY_27",
  "CATEGORY_28",
  "CATEGORY_29",
  "CATEGORY_30",
  "CATEGORY_31",
};


/**
 * @return Name of the category of the good
 * @author Hj. Malthaner
 */
const char * ware_besch_t::get_catg_name() const
{
	return catg_names[catg & 31];
}

/**
 * Given the "normal" speed bonus and a distance (currently in tiles -- FIXME)
 * calculate the adjusted speed bonus.
 * There are several special effects at work here:
 * Speed bonus "phases in" above a certain distance before reaching its maximum
 * at a certain distance
 *
 * Written by Nathanael Nerode (neroden) based on code by James Petts
 */

// Note that this is the static version -- pure computation
static uint16 ware_besch_t::get_adjusted_speed_bonus(const karte_t* world, uint32 distance_in, uint16 base_bonus_rating)
{
	// Avoid overflow.  This is 2^31 / 1000
	assert (distance_in < 2147483);
	// Multiply everything by 1000 for happier computations... later we will switch to meters...
	sint32 distance = distance_in * 1000;
	sint32 min_distance;
	sint32 median_distance;
	sint32 max_distance;
	sint32 multiplier;
	if (world) {
		uint16 min_distance_short = world->get_settings().get_min_bonus_max_distance();
		min_distance = (sint32)min_distance_short * 1000;
		uint16 median_distance_short = world->get_settings().get_median_bonus_distance();
		median_distance = (sint32)median_distance_short * 1000;
		uint16 max_distance_short = world->get_settings().get_max_bonus_min_distance();
		max_distance = (sint32)max_distance_short * 1000;
		uint16 multiplier_short = world->get_settings().get_max_bonus_multiplier_percent();
		multiplier = (sint32)multiplier_short * 1000;

		// Sanity checks on values
		assert(max_distance > median_distance);
		assert(median_distance > min_distance);
		assert(median_distance == 0 || median_distance > min_distance);
		assert(multiplier > 100);
	}
	else { // no world... should rarely happen
		// default values as used in pak128.britain as of June 2013
		min_distance = 4000;
		median_distance = 75000;
		max_distance = 250000;
		multiplier = 300;
	}

	if(distance <= min_distance)
	{
		return 0;
	}

	if(distance >= max_distance)
	{
		return base_bonus_rating * multiplier / 100;
	}

	if(median_distance == 0)
	{
		// There is no median, so scale evenly.
		const sint32 percentage = ((distance - min_distance) * 100) / (max_distance - min_distance);
		assert(percentage >= 0);
		const sint32 return_figure = ((base_bonus_rating * multiplier) * percentage) / 10000;
		return (uint16)return_figure;
	}

	// There is a median, so scale differently each side of the median.
	if(distance < median_distance)
	{
		const sint32 percentage = ((distance - min_distance) * 100) / (median_distance - min_distance);
		assert(percentage >= 0);
		const sint32 return_figure =  base_bonus_rating * percentage / 100;
		return (uint16)return_figure;
	}
	else // (distance >= median_distance)
	{
		const sint32 percentage = ((distance - median_distance) * 100) / (max_distance - median_distance);
		assert(percentage >= 0);
		const sint32 return_figure = base_bonus_rating + (base_bonus_rating * (multiplier - 100) * percentage) / 10000;
		return (uint16)return_figure;
	}
	// Can't get here
}

/**
 * This is used solely when refunds must be calculated -- hopefully rarely.
 * Unfortunately, we don't keep track of the revenue booked earlier for passengers
 * when we need to make refunds, so we have to refund an approximation.
 *
 * We also don't know the actual speed of travel for the previous trip, or
 * even the waytype used (so, no average speed either).
 *
 * The approximation is chosen to be 2x the base fare ("no speedbonus") for the minimum distance.
 * This is in the same units as get_fare_with_speedbonus.
 */
sint64 ware_besch_t::get_refund(uint32 tile_distance) const
{
 	sint64 fare = get_fare(tile_distance, 0);
	return fare * 2000;
}
