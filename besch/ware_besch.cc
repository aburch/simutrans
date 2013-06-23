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
sint64 ware_besch_t::get_refund(uint32 distance_meters) const
{
 	sint64 fare = get_fare(distance_meters);
	return fare * 2;
}
