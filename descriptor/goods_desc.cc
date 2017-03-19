#include "goods_desc.h"
#include "../simworld.h"
#include "../dataobj/settings.h"

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
const char * goods_desc_t::get_catg_name() const
{
	return catg_names[catg & 31];
}


/**
 * Reset the scaled values.
 */
void goods_desc_t::set_scale(uint16 scale_factor)
{
	scaled_values.clear();
	uint16 new_price;
	uint32 new_distance;
	ITERATE(values, i)
	{
		// There are three things going on here:
		// (1) price is per km, divide by 1000 to get per meter
		// (2) price is in simcents, multiply by 4096 to get "internal units"
		// (3) all prices must be divided by 3 for silly historical reasons
		// The choice of 4096 allows for less precision loss than otherwise
		new_price = values[i].price * 4096 / 3000;
		new_distance = values[i].to_distance * 1000;
		scaled_values.append(fare_stage_t(new_distance, new_price));
	}
}


/**
 * This method returns the *total* fare for these
 * goods over the given distance, in *meters*.
 *
 * This is the base fare, not adjusted for speedbonus...
 * This returns values in units of 1/4096 of a simcent
 *
 * In extreme cases this may return values as high as:
 * 2^15 * 2 (length + width of map) * 1000 (meters per tile) * 4096 * 1000 (large fare)
 * which is roughly 2^48.... running up against the limits of 64-bit math...
 *
 * This is very different from the similarly-named routine in standard
 * @author: jamespetts, neroden
 */
sint64 goods_desc_t::get_base_fare(uint32 distance_meters, uint32 starting_distance) const
{
	sint64 total_fare = 0;
	uint16 per_meter_fare;
	uint32 remaining_distance = distance_meters;
	ITERATE(scaled_values, i)
	{
		per_meter_fare = scaled_values[i].price;
		if(i < scaled_values.get_count() - 1 && starting_distance >= scaled_values[i].to_distance)
		{
			starting_distance -= scaled_values[i].to_distance;
			continue;
		}
		if(scaled_values[i].to_distance >= remaining_distance || i == scaled_values.get_count() - 1)
		{
			// The last item in the list must trigger the use of the full remaining distance.
			total_fare += (sint64)per_meter_fare * remaining_distance;
			break;
		}
		else
		{
			total_fare += (sint64)per_meter_fare * (scaled_values[i].to_distance - starting_distance);
			remaining_distance -= (scaled_values[i].to_distance - starting_distance);
			starting_distance = 0;
		}
	}
	return total_fare;
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
 * This should be altered for future fast computation.
 *
 * OK.  Now consider the base fare.
 * The maximum fare is 4 times the base fare...
 * The minimum fare is 1/4 the base fare...
 * The standard fare is super freaking complicated for no good reason...
 * Roughly speaking, it is
 * standard_fare = base_fare * (1 + relative_speed_percentage * speed_bonus_rating)
 * where speed_bonus_rating is the official speed bonus "percentage", usually a one or
 * two digit number.
 *
*/

/**
 * This variant of get_fare gives the revenue *with* the speedbonus adjustment.
 * This does not take comfort and catering/TPO revenue into account.
 *
 * This returns values in units of 1/4096 of a simcent
 *
 * This must take the *relative speed percentage* as an argument.
 * See above for how this is defined; it is always a percent, with no greater
 * precision than that.  (Consider changing this.)
 * It will stay in the range of a sint16 unless the pak is completely broken.
 */
sint64 goods_desc_t::get_fare_with_speedbonus(sint16 relative_speed_percentage, uint32 distance_meters, uint32 starting_distance) const
{
	sint64 base_fare = get_base_fare(distance_meters, starting_distance);
		// We must be able to multiply by, say, 2^16;
		// otherwise we may overflow computation computing the standard fare.
		// Note that we may be getting a number as large as 2^48, so this is a real risk.
		// It would require an unusually unbalanced pakset, however.
		// Fares are usually orders of magnitude smaller than this.
	assert (base_fare <= 0x00008FFFFFFFFFFFll );
		// If the assertion fails, we can't safely multiply by 2^16.
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
 * This is the even more complex version used for passenger and mail revenue, which
 * takes comfort and catering into account.
 *
 * It has to take the world as an argument for dumb reasons.
 */
sint64 goods_desc_t::get_fare_with_comfort_catering_speedbonus(karte_t* world,
				uint8 comfort, uint8 catering_level, sint64 journey_tenths,
				sint16 relative_speed_percentage, uint32 distance_meters, uint32 starting_distance) const
{
	sint64 fare = get_fare_with_speedbonus(relative_speed_percentage, distance_meters, starting_distance);

	if (fare <= 0) {
		// Quick escape and sanity check
		return 0;
	}

	if ( get_index() == goods_manager_t::INDEX_PAS ) {
		/*
		 * Passengers: apply luxury bonus or discomfort penalty
		 */

		// Grab the tolerable comfort from the settings table
		const sint16 tolerable_comfort = world->get_settings().tolerable_comfort(journey_tenths);
		// See how far off we are
		const sint16 comfort_diff = (sint16)comfort - tolerable_comfort;
		// This gets the "full" percentage bonus or penalty -- it may be negative!
		const sint64 multiplier = world->get_settings().base_comfort_revenue(comfort_diff);

		// Comfort has less of an effect for shorter trips.  This gets the derating factor
		// as a percentage (2 digits)
		const sint64 comfort_modifier = world->get_settings().comfort_derating(journey_tenths);

		// Combine the derating factor with the full percentage to get...
		const sint64 comfort_fare = (fare * multiplier * comfort_modifier) / 10000ll;

		// Always receive minimum of 95% of fare even with discomfort penalty
		fare = max(fare + comfort_fare, fare * 19 / 20 );

		if (catering_level > 0) {
			/*
			 * We have catering.  Apply catering revenue.
			 */
			assert (catering_level <= 5);
			// Use the catering revenues table for this catering level. It is a functional.
			fare += world->get_settings().catering_revenues[catering_level](journey_tenths);
		}
	} else if ( get_index() == goods_manager_t::INDEX_MAIL ) {
		if (catering_level > 0) {
			/*
			 * It's a TPO.  Apply TPO revenue.
			 */
			// Use the TPO revenue table.  It is a functional.
			fare += world->get_settings().tpo_revenues(journey_tenths);
		}
	}
	return fare;
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
sint64 goods_desc_t::get_refund(uint32 distance_meters) const
{
 	sint64 fare = get_base_fare(distance_meters);
	return fare * 2;
}
