#ifndef OBJ_BESCH_STD_NAME_H
#define OBJ_BESCH_STD_NAME_H

#include "text_besch.h"

class checksum_t;
class werkzeug_t;

/**
 * Common base class for all object descriptors, which get their name and
 * copyright information from child 0 and 1
 */
class obj_besch_std_name_t : public obj_besch_t {
	public:
		const char* get_name() const
		{
			return get_child<text_besch_t>(0)->get_text();
		}

		const char* get_copyright() const
		{
			text_besch_t const* const ts = get_child<text_besch_t>(1);
			if (!ts) return 0;
			char const* const text = ts->get_text();
			return text[0] != '\0' ? text : 0;
		}
};

/**
 * Base class for all stuff that depends on timeline.
 */
class obj_besch_timelined_t : public obj_besch_std_name_t {

protected:
	uint16 intro_date;    ///< this thing is available from this date
	uint16 obsolete_date; ///< this thing is available until this date

public:
	obj_besch_timelined_t() : obj_besch_std_name_t(),
		intro_date(0), obsolete_date(0) {}

	uint16 get_intro_year_month() const { return intro_date; }

	uint16 get_retire_year_month() const { return obsolete_date; }

	/// @return true if this object is available with timeline.
	bool is_available(const uint16 month_now) const
	{
		return month_now==0  ||  (intro_date<=month_now  &&  obsolete_date>month_now);
	}

	/// @return true if this is still not available
	bool is_future(const uint16 month_now) const
	{
		return month_now  &&  (intro_date > month_now);
	}

	/// @return true if this is obsolete
	bool is_retired(const uint16 month_now) const
	{
		return month_now  &&  (obsolete_date <= month_now);
	}

	void calc_checksum(checksum_t *chk) const;
};

/**
 * Base class for all transport related stuff.
 */
class obj_besch_transport_related_t : public obj_besch_timelined_t {

protected:
	sint32 base_maintenance;
	sint32 base_cost;

	sint32 maintenance;			///< monthly cost for bits_per_month=18
	sint32 cost;				///< cost to build this thing [1/100 credits] per tile/object
	uint8  wt;					///< waytype of this thing
	sint32 topspeed;			///< maximum allowed speed in km/h
	sint32 topspeed_gradient_1; ///< maximum allowed speed in km/h for a half/single height gradient
	sint32 topspeed_gradient_2; ///< maximum allowed speed in km/h for a single/double height gradient
	sint8 max_altitude;			///< Maximum height in tiles above sea level at which this way may be built
	uint8 max_vehicles_on_tile;	///< Maximum number of vehicles permitted on the tile at once. Only used for waterways. Default: 251

public:
	obj_besch_transport_related_t() : obj_besch_timelined_t(),
		base_maintenance(0), base_cost(0), 
		maintenance(0), cost(0), wt(255), topspeed(0), topspeed_gradient_1(0), topspeed_gradient_2(0) {}

	inline sint32 get_base_maintenance() const { return base_maintenance; }
	sint32 get_maintenance() const { return maintenance; }
	sint32 get_wartung() const { return maintenance; }

	inline sint32 get_base_cost() const { return base_cost; }
	inline sint32 get_base_price() const { return base_cost; }
	sint32 get_preis() const { return cost; }

	waytype_t get_waytype() const { return static_cast<waytype_t>(wt); }
	waytype_t get_wtyp() const { return get_waytype(); }

	sint32 get_topspeed() const { return topspeed; }
	sint32 get_topspeed_gradient_1() const { return topspeed_gradient_1; }
	sint32 get_topspeed_gradient_2() const { return topspeed_gradient_2; }
	sint32 get_geschw() const { return topspeed; }
	sint8 get_max_altitude() const { return max_altitude; }
	uint8 get_max_vehicles_on_tile() const { return max_vehicles_on_tile; }

	void set_scale(uint16 scale_factor)
	{
		cost = set_scale_generic<sint32>(base_cost, scale_factor);
		if (base_cost && !cost) cost = 1;
		maintenance = set_scale_generic<sint32>(base_maintenance, scale_factor);
		if (base_maintenance && !maintenance) maintenance = 1;
	}

	void calc_checksum(checksum_t *chk) const;
};

/**
 * Base class for all transport infrastructure.
 */
class obj_besch_transport_infrastructure_t : public obj_besch_transport_related_t {

protected:
	werkzeug_t *builder;  ///< default tool for building

public:

	werkzeug_t *get_builder() const {
		return builder;
	}

	void set_builder( werkzeug_t *w )  {
		builder = w;
	}
};

#endif
