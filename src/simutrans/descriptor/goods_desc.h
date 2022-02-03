/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_GOODS_DESC_H
#define DESCRIPTOR_GOODS_DESC_H


#include "obj_base_desc.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../network/checksum.h"

class checksum_t;

/**
 * Child nodes:
 *  0   Name
 *  1   Copyright
 *  2   Text: Name of measurement unit
 */
class goods_desc_t : public obj_named_desc_t {
	friend class goods_reader_t;
	friend class goods_manager_t;


	/// base value
	uint16 base_value;

	/**
	 * Value used in revenue calculation.
	 * Will be set by goods_manager_t.
	 */
	uint16 value;

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

public:
	// the measure for that good (crates, people, bags ... )
	const char *get_mass() const
	{
		return get_child<text_desc_t>(2)->get_text();
	}

	uint16 get_value() const { return value; }

	/**
	* @return speed bonus value of the good
	*/
	uint16 get_speed_bonus() const { return speed_bonus; }

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

	void calc_checksum(checksum_t *chk) const
	{
		chk->input(base_value);
		chk->input(catg);
		chk->input(speed_bonus);
		chk->input(weight_per_unit);
	}
};

#endif
