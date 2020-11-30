/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_GOODS_DESC_H
#define DESCRIPTOR_GOODS_DESC_H


#include "obj_base_desc.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../network/checksum.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/piecewise_linear_tpl.h"
// Simworld is for adjusted speed bonus
//#include "../simworld.h"
#include "image.h"

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

/**
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

	/**
	* Category of the good
	*/
	uint8 catg;

	/**
	* total index, all ware with same catg_index will be compatible,
	* including special freight
	* assigned during registration
	*/
	uint8 catg_index;

	/**
	 * index of the type,
	 * assigned during registration
	 */
	uint8 goods_index;

	uint8 color;

	/**
	* Bonus for fast transport given in percent!
	*/
	uint16 speed_bonus;

	/**
	* Weight in KG per unit of this good
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
	*/
	uint8 get_catg() const { return catg; }

	/**
	* @return Category of the good
	*/
	uint8 get_catg_index() const { return catg_index; }

	/**
	* @return internal index (just a number, passenger, then mail, then something ... )
	*/
	uint8 get_index() const { return goods_index; }

	/**
	* @return weight in KG per unit of the good
	*/
	uint16 get_weight_per_unit() const { return weight_per_unit; }

	/**
	* @return Name of the category of the good
	*/
	const char * get_catg_name() const;

	/**
	* @return goods category symbol
	* @author Ranran, March 2019
	*/
	image_id get_catg_symbol() const;

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
	*/
	bool is_interchangeable(const goods_desc_t *other) const
	{
		return catg_index == other->get_catg_index();
	}

	/**
	* @return color for good table and waiting bars
	*/
	PIXVAL get_color() const { return color_idx_to_rgb(color); }
	uint8 get_color_index() const { return color; }

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
	* Now deprecated for most purposes, retained just to check
	* whether to discard goods that have been waiting too long.
	*/
	uint16 get_speed_bonus() const { return speed_bonus; }

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

	void fix_number_of_classes();
};

#endif
