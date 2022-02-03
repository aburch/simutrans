/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_API_API_SIMPLE_H
#define SCRIPT_API_API_SIMPLE_H


#include "../../../squirrel/squirrel.h"
#include "../../dataobj/ribi.h"

/** @file api_simple.h Helper functions to export simple datatypes: ribi & friends */


/**
 * Use own type for ribis and slopes
 */
struct my_ribi_t {
	uint8 data;
	my_ribi_t(ribi_t::ribi r) : data(r) { }
	operator ribi_t::ribi() const { return data; }
};

struct my_slope_t {
	uint8 data;
	my_slope_t(slope_t::type r) : data(r) { }
	operator slope_t::type() const { return data; }
};


namespace script_api {

	struct mytime_t
	{
		uint32 raw;
		mytime_t(uint32 r_) : raw(r_) {}
	};

	struct mytime_ticks_t : public mytime_t
	{
		uint32 ticks;
		uint32 ticks_per_month;
		uint32 next_month_ticks;

		mytime_ticks_t(uint32 r, uint32 t, uint32 tpm, uint32 nmt) : mytime_t(r),
			ticks(t), ticks_per_month(tpm), next_month_ticks(nmt)
		{}
	};
};

#endif
