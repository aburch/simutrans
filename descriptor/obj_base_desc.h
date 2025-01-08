/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_OBJ_BASE_DESC_H
#define DESCRIPTOR_OBJ_BASE_DESC_H


#include "text_desc.h"

class checksum_t;
class tool_t;

/**
 * Common base class for all object descriptors, which get their name and
 * copyright information from child 0 and 1
 */
class obj_named_desc_t : public obj_desc_t
{
public:
	const char *get_name() const
	{
		return get_child<text_desc_t>(0)->get_text();
	}

	const char *get_copyright() const
	{
		const text_desc_t *const ts = get_child<text_desc_t>(1);
		if (!ts) {
			return 0;
		}

		const char *const text = ts->get_text();
		return text[0] != '\0' ? text : NULL;
	}
};

/**
 * Base class for all stuff that depends on timeline.
 */
class obj_desc_timelined_t : public obj_named_desc_t {

protected:
	uint16 intro_date;  ///< this thing is available from this date
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
	sint32 maintenance;   ///< monthly cost for bits_per_month=18
	sint32 price;         ///< cost to build this thing [1/100 credits] per tile/object
	uint16 axle_load;     ///< up to this load vehicle may pass (default 9999)
	uint8  wtyp;          ///< waytype of this thing
	sint32 topspeed;      ///< maximum allowed speed in km/h

public:
	obj_desc_transport_related_t() : obj_desc_timelined_t(),
		maintenance(0), price(0), axle_load(9999), wtyp(255), topspeed(0) {}

	sint32 get_maintenance() const { return maintenance; }

	sint32 get_price() const { return price; }

	waytype_t get_waytype() const { return static_cast<waytype_t>(wtyp); }
	waytype_t get_wtyp() const { return get_waytype(); }

	sint32 get_topspeed() const { return topspeed; }

	uint16 get_axle_load() const { return axle_load; }

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
