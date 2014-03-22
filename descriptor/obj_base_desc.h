#ifndef __OBJ_BASE_DESC_H
#define __OBJ_BASE_DESC_H

#include "text_desc.h"

class checksum_t;
class tool_t;

/**
 * Common base class for all object descriptors, which get their name and
 * copyright information from child 0 and 1
 */
class obj_named_desc_t : public obj_desc_t {
	public:
		const char* get_name() const
		{
			return get_child<text_desc_t>(0)->get_text();
		}

		const char* get_copyright() const
		{
			text_desc_t const* const ts = get_child<text_desc_t>(1);
			if (!ts) return 0;
			char const* const text = ts->get_text();
			return text[0] != '\0' ? text : 0;
		}
};

/**
 * Base class for all stuff that depends on timeline.
 */
class obj_desc_timelined_t : public obj_named_desc_t {

protected:
	uint16 intro_date;    ///< this thing is available from this date
	uint16 retire_date; ///< this thing is available until this date

public:
	obj_desc_timelined_t() : obj_named_desc_t(),
		intro_date(0), retire_date(0) {}

	uint16 get_intro_year_month() const { return intro_date; }

	uint16 get_retire_year_month() const { return retire_date; }

	/// @return true if this object is available with timeline.
	bool is_available(const uint16 month_now) const
	{
		return month_now==0  ||  (intro_date<=month_now  &&  retire_date>month_now);
	}

	/// @return true if this is still not available
	bool is_future(const uint16 month_now) const
	{
		return month_now  &&  (intro_date > month_now);
	}

	/// @return true if this is obsolete
	bool is_retired(const uint16 month_now) const
	{
		return month_now  &&  (retire_date <= month_now);
	}

	void calc_checksum(checksum_t *chk) const;
};

/**
 * Base class for all transport related stuff.
 */
class obj_desc_transport_related_t : public obj_desc_timelined_t {

protected:
	sint32 base_maintenance;
	sint32 base_cost;
	sint32 maintenance;			///< monthly cost for bits_per_month=18
	sint32 price;				///< cost to build this thing [1/100 credits] per tile/object
	uint8  wtyp;				///< waytype of this thing
	uint16 axle_load;			///< up to this load vehicle may pass (default 9999)
	sint32 topspeed;			///< maximum allowed speed in km/h
	sint32 topspeed_gradient_1; ///< maximum allowed speed in km/h for a half/single height gradient
	sint32 topspeed_gradient_2; ///< maximum allowed speed in km/h for a single/double height gradient
	sint8 max_altitude;			///< Maximum height in tiles above sea level at which this way may be built
	uint8 max_vehicles_on_tile;	///< Maximum number of vehicles permitted on the tile at once. Only used for waterways. Default: 251
	uint32 wear_capacity;		///< The total number of standard axle passes (*10,000 for precision) that this way can take
	uint32 monthly_base_wear;	///< The amount of wear (in the same units as above) as the way experienecs monthly from the weather etc. without taking into account vehicles passing upon it. Default 1/12th of 1/50th of the way's total capacity (i.e., will wear out in 50 years with no traffic) unless a waterway.
	uint32 base_way_only_cost;	///< The cost of upgrading/renewing only the way on the elevated way (without scale factor).
	uint32 way_only_cost;		///< The cost of upgrading/renewing only the way on the elevated way.
	uint8 upgrade_group;		///< The group of elevated ways between which this can be upgraded for the way only cost.

public:
	obj_desc_transport_related_t() : obj_desc_timelined_t(),
		base_maintenance(0), base_cost(0), 
               maintenance(0), price(0), axle_load(9999), wtyp(255), topspeed(0), topspeed_gradient_1(0), topspeed_gradient_2(0),
               base_way_only_cost(0), way_only_cost(0) {}

	inline sint32 get_base_maintenance() const { return base_maintenance; }
	inline sint32 get_maintenance() const { return maintenance; }

	inline sint32 get_base_cost() const { return base_cost; }
	inline sint32 get_base_price() const { return base_cost; }
	inline sint32 get_value() const { return price; }

	inline uint32 get_base_way_only_cost() const { return base_way_only_cost; }
	inline uint32 get_way_only_cost() const { return way_only_cost; }

	inline uint8 get_upgrade_group() const { return upgrade_group; }

	waytype_t get_waytype() const { return static_cast<waytype_t>(wtyp); }
	waytype_t get_wtyp() const { return get_waytype(); }

	sint32 get_topspeed() const { return topspeed; }
	sint32 get_topspeed_gradient_1() const { return topspeed_gradient_1; }
	sint32 get_topspeed_gradient_2() const { return topspeed_gradient_2; }
	sint8 get_max_altitude() const { return max_altitude; }
	uint8 get_max_vehicles_on_tile() const { return max_vehicles_on_tile; }

	inline uint16 get_axle_load() const { return axle_load; }

	inline uint32 get_wear_capacity() const { return wear_capacity; }

	inline uint32 get_monthly_base_wear() const { return monthly_base_wear; }

	void set_scale(uint16 scale_factor)
	{
		price = set_scale_generic<sint64>(base_cost, scale_factor);
		if (base_cost && !price) price = 1;
		maintenance = set_scale_generic<sint32>(base_maintenance, scale_factor);
		if (base_maintenance && !maintenance) maintenance = 1;
		way_only_cost = set_scale_generic<sint32>(base_way_only_cost, scale_factor);
		if (base_way_only_cost && !way_only_cost) way_only_cost = 1;
	}

	void calc_checksum(checksum_t *chk) const;
};

/**
 * Base class for all transport infrastructure.
 */
class obj_desc_transport_infrastructure_t : public obj_desc_transport_related_t {

protected:
	tool_t *builder;  ///< default tool for building

public:

	tool_t *get_builder() const {
		return builder;
	}

	void set_builder( tool_t *tool )  {
		builder = tool;
	}
};

#endif
