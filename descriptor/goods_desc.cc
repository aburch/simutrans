/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "goods_desc.h"
#include "../simworld.h"
#include "../dataobj/settings.h"
#include "../bauer/goods_manager.h"
#include "../simskin.h"
#include "../descriptor/skin_desc.h"

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
	if (catg == 0) {
		return get_name();
	}
	return catg_names[catg & 31];
}

image_id goods_desc_t::get_catg_symbol() const
{
	if (!catg) {
		switch (goods_index) {
		case goods_manager_t::INDEX_PAS:
			return skinverwaltung_t::passengers->get_image_id(0);
			break;
		case goods_manager_t::INDEX_MAIL:
			return skinverwaltung_t::mail->get_image_id(0);
			break;
		default:
			// TODO: Supports symbols for unique freight goods
			if (skinverwaltung_t::goods_categories) {
				return skinverwaltung_t::goods_categories->get_image_id(0);
			}
			return skinverwaltung_t::goods->get_image_id(0);
			break;
		}
	}
	else if (!skinverwaltung_t::goods_categories) {
		return IMG_EMPTY;
	}
	else {
		return skinverwaltung_t::goods_categories->get_image_id(catg);
	}
}

/**
 * Reset the scaled values.
 */
void goods_desc_t::set_scale(uint16)
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
 * This is the base fare, not adjusted.
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

sint64 goods_desc_t::get_total_fare(uint32 distance_meters, uint32 starting_distance, uint8 comfort, uint8 catering_level, uint8 g_class, sint64 journey_tenths) const
{
	sint64 fare = get_base_fare(distance_meters, starting_distance);

	// Apply the modifiers for passengers/mail: class, comfort and catering
	if (get_index() == goods_manager_t::INDEX_PAS || get_index() == goods_manager_t::INDEX_MAIL)
	{
		if (fare <= 0)
		{
			// Quick escape and sanity check
			return 0;
		}

		// First, class modifications.
		fare *= (sint64)class_revenue_percentages[g_class];
		fare /= 100ll;

		// Now, the comfort modifiers
		if (get_index() == goods_manager_t::INDEX_PAS)
		{
			/*
			* Passengers: apply luxury bonus or discomfort penalty
			*/

			// Grab the tolerable comfort from the settings table
			const sint16 tolerable_comfort = world()->get_settings().tolerable_comfort(journey_tenths);
			// See how far off we are
			const sint16 comfort_diff = (sint16)comfort - tolerable_comfort;
			// This gets the "full" percentage bonus or penalty -- it may be negative!
			const sint64 multiplier = world()->get_settings().base_comfort_revenue(comfort_diff);

			// Comfort has less of an effect for shorter trips.  This gets the derating factor
			// as a percentage (2 digits)
			const sint64 comfort_modifier = world()->get_settings().comfort_derating(journey_tenths);

			// Combine the derating factor with the full percentage to get...
			const sint64 comfort_fare = (fare * multiplier * comfort_modifier) / 10000ll;

			// Apply the comfort penalty/bonus to the fare
			fare += comfort_fare;

			if (catering_level > 0)
			{
				/*
				* We have catering.  Apply catering revenue.
				*/
				assert(catering_level <= 5);

				// Passengers can only afford to pay for catering at their class level. +1 because 0 means no catering at all.
				// The maximum catering spend is *also* limited by maximum journey time (which is dealt with below).
				// Note that this uses the accommodation class for the passengers rather than the inherent class. This is
				// because it is assumed that higher level catering is simply not available in lower levels of accommodation.
				catering_level = min(g_class + 1, catering_level);

				// Use the catering revenues table for this catering level. It is a functional.
				fare += world()->get_settings().catering_revenues[catering_level](journey_tenths);
			}
		}

		else if (get_index() == goods_manager_t::INDEX_MAIL)
		{
			if (catering_level > 0)
			{
				/*
				* It's a TPO.  Apply TPO revenue.
				*/

				// TODO: Consider how to deal with TPO revenue in the future.

				// Use the TPO revenue table.  It is a functional.
				fare += world()->get_settings().tpo_revenues(journey_tenths);
			}
		}
	}

	// TODO: Add inflation here

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
 * The approximation is chosen to be 2x the base fare for the minimum distance.
 * This is in the same units as get_total_fare
 */
sint64 goods_desc_t::get_refund(uint32 distance_meters) const
{
 	sint64 fare = get_base_fare(distance_meters);
	return fare * 2;
}

void goods_desc_t::fix_number_of_classes()
{
	// Only passengers and mail are allowed multiple classes
	if (goods_index >= 3 && number_of_classes > 1)
	{
		number_of_classes = 1;
		class_revenue_percentages.set_count(1);
	}
}
