/*
 *  Copyright (c) 2006 by prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  Module description:
 *      signs on roads and other ways
 */

#ifndef __ROADSIGN_BESCH_H
#define __ROADSIGN_BESCH_H

#include "obj_base_desc.h"
#include "image_list.h"
#include "skin_desc.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"
#include "../network/checksum.h"


/*
 *  Autor:
 *      prissi
 *
 *  Description:
 *	Road signs
 *
 *  Child nodes:
 *	0   Name
 *	1   Copyright
 *	2   Image list
 */
class roadsign_desc_t : public obj_desc_transport_infrastructure_t {
	friend class roadsign_reader_t;

private:
	uint8 flags;

	sint8 offset_left; // default 14

	uint16 min_speed;	// 0 = no min speed

	/**
	 * Whether this signal/sign
	 * can be built underground
	 */
	uint8 allow_underground;

	// What group to which this signal belongs. 
	// This determines the signal boxes to which it can be linked.
	uint32 signal_group;

	// This determines how far that this can be placed from a signalbox. 
	// Note that, of this figure and the radius of the signalbox, the 
	// lowest value of the two determines whether the signal can be
	// built. This value is in meters. 0 = unlimited.
	uint32 max_distance_to_signalbox;

	// The number of aspects that this signal can display. 
	// This should be a number between 1 (for a fixed sign
	// board as in, e.g. ETRMS Level 2, and 5 for full 5
	// aspect signalling. Default: 2). Relevant only for
	// railway signals, not roadsigns (which are set to 0).
	// Note that SIGN_PRE_SIGNAL should now be used *only*
	// for signals that are purely pre-signals and have
	// no stop/danger aspect. 
	uint8 aspects;

	// True if this is a signal with a call-on aspect.
	bool has_call_on;

	// True if this is a choose signal with choose and
	// non-choose signal aspects (other than danger).
	bool has_selective_choose;

	// The working method that this signal engenders
	// if passed. 
	working_method_t working_method; 

	// Whether this is a permissive signal.
	bool permissive;

	// Whether this signal is an intermediate block type signal
	// in the absolute block working method.
	bool intermediate_block;

	// If this is true, even signals that would usually be normal
	// clear (track circuit block and cab signalling: not time interval)
	// will be normal danger. 
	bool normal_danger;

	// Whether this signal will clear only if the next signal
	// on the route is clear.
	bool double_block;

	// The maximum speed at which this signal may be approached.
	// Used for system speeds. 
	uint32 max_speed;

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
		END_OF_CHOOSE_AREA    = 1U << 7
	};

	int get_image_id(ribi_t::dir dir) const
	{
		image_t const* const image = get_child<image_list_t>(2)->get_image(dir);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	int get_count() const { return get_child<image_list_t>(2)->get_count(); }

	skin_desc_t const* get_cursor() const { return get_child<skin_desc_t>(3); }

	sint32 get_min_speed() const { return min_speed; }

	bool is_single_way() const { return (flags&ONE_WAY)!=0; }

	bool is_private_way() const { return (flags&PRIVATE_ROAD)!=0; }

	//  return true for a traffic light
	bool is_traffic_light() const { return (get_count()>4); }

	bool is_choose_sign() const { return flags&CHOOSE_SIGN; }

	//  return true for signal
	bool is_signal() const { return flags&SIGN_SIGNAL; }

	//  return true for presignal
	bool is_pre_signal() const { return flags&SIGN_PRE_SIGNAL && !is_combined_signal(); }

	bool is_combined_signal() const { return flags&SIGN_PRE_SIGNAL && aspects == 3 && working_method == absolute_block; }

	//  return true for single track section signal
	bool is_longblock_signal() const { return flags&SIGN_LONGBLOCK_SIGNAL; }

	bool is_end_choose_signal() const { return flags&END_OF_CHOOSE_AREA; }

	bool is_signal_type() const
	{
		return (flags&(SIGN_SIGNAL|SIGN_PRE_SIGNAL|SIGN_LONGBLOCK_SIGNAL))!=0;
	}

	// This is currently the definition of a station signal
	inline bool is_station_signal() const {return is_longblock_signal() && (get_working_method() == time_interval || get_working_method() == time_interval_with_telegraph || get_working_method() == absolute_block);}

	types get_flags() const { return (types)flags; }

	sint8 get_offset_left() const { return offset_left; }

	uint8 get_allow_underground() const { return allow_underground; }

	uint32 get_signal_group() const { return signal_group; }

	uint32 get_max_distance_to_signalbox() const { return max_distance_to_signalbox; }

	uint8 get_aspects() const { return aspects; }

	bool get_has_call_on() const { return has_call_on; }

	bool get_has_selective_choose() const { return has_selective_choose; }

	bool get_permissive() const { return permissive; }

	bool get_intermediate_block() const { return intermediate_block; }

	bool get_normal_danger() const { return normal_danger; }

	bool get_double_block() const { return double_block; }

	uint32 get_max_speed() const { return max_speed; }

	working_method_t get_working_method() const { return working_method; }

	void calc_checksum(checksum_t *chk) const
	{
		obj_desc_transport_infrastructure_t::calc_checksum(chk);
		chk->input(flags);
		chk->input(min_speed);
		chk->input(allow_underground);
		chk->input(signal_group);
		chk->input(base_maintenance); 
		chk->input(max_distance_to_signalbox);
		chk->input(aspects);
		chk->input(has_call_on);
		chk->input(has_selective_choose);
		chk->input(permissive);
		chk->input(intermediate_block);
		chk->input(normal_danger);
		chk->input(max_speed);
		chk->input(working_method); 
	}
};

ENUM_BITSET(roadsign_desc_t::types)

#endif
