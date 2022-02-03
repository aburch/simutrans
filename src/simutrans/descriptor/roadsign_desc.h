/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_ROADSIGN_DESC_H
#define DESCRIPTOR_ROADSIGN_DESC_H


#include "obj_base_desc.h"
#include "image_list.h"
#include "skin_desc.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"
#include "../network/checksum.h"


/**
 * Road signs
 *
 * Child nodes:
 *  0   Name
 *  1   Copyright
 *  2   Image list
 */
class roadsign_desc_t : public obj_desc_transport_infrastructure_t {
	friend class roadsign_reader_t;

private:
	uint16 flags;

	sint8 offset_left; // default 14

	uint16 min_speed; // 0 = no min speed

public:
	enum types {
		NONE                  = 0,
		ONE_WAY               = 1U << 0,
		CHOOSE_SIGN           = 1U << 1,
		PRIVATE_ROAD          = 1U << 2,
		SIGN_SIGNAL           = 1U << 3,
		SIGN_PRE_SIGNAL       = 1U << 4,
		ONLY_BACKIMAGE        = 1U << 5,
		SIGN_LONGBLOCK_SIGNAL = 1U << 6,
		END_OF_CHOOSE_AREA    = 1U << 7,
		SIGN_PRIORITY_SIGNAL  = 1U << 8
	};

	image_id get_image_id(ribi_t::dir dir) const
	{
		image_t const* const image = get_child<image_list_t>(2)->get_image(dir);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	uint16 get_count() const { return get_child<image_list_t>(2)->get_count(); }

	skin_desc_t const* get_cursor() const { return get_child<skin_desc_t>(3); }

	uint16 get_min_speed() const { return min_speed; }

	bool is_single_way() const { return (flags & ONE_WAY) != 0; }

	bool is_private_way() const { return (flags & PRIVATE_ROAD) != 0; }

	//  return true for a traffic light
	bool is_traffic_light() const { return (get_count() > 4); }

	bool is_choose_sign() const { return (flags & CHOOSE_SIGN) != 0; }

	//  return true for signal
	bool is_simple_signal() const { return (flags & (
		SIGN_SIGNAL |
		SIGN_PRE_SIGNAL |
		SIGN_PRIORITY_SIGNAL |
		SIGN_LONGBLOCK_SIGNAL |
		CHOOSE_SIGN)) == SIGN_SIGNAL; }

	//  return true for presignal
	bool is_pre_signal() const { return (flags & SIGN_PRE_SIGNAL) != 0; }

	//  return true for priority signal
	bool is_priority_signal() const { return (flags & SIGN_PRIORITY_SIGNAL) != 0; }

	//  return true for single track section signal
	bool is_longblock_signal() const { return (flags & SIGN_LONGBLOCK_SIGNAL) != 0; }

	bool is_end_choose_signal() const { return (flags & END_OF_CHOOSE_AREA) != 0; }

	bool is_signal_type() const
	{
		return (flags&(
				SIGN_SIGNAL |
				SIGN_PRE_SIGNAL |
				SIGN_PRIORITY_SIGNAL |
				SIGN_LONGBLOCK_SIGNAL)
			) != 0;
	}

	uint16 get_flags() const { return flags; }

	sint8 get_offset_left() const { return offset_left; }

	void calc_checksum(checksum_t *chk) const
	{
		obj_desc_transport_infrastructure_t::calc_checksum(chk);
		chk->input(flags);
		chk->input(min_speed);
	}
};

ENUM_BITSET(roadsign_desc_t::types)

#endif
