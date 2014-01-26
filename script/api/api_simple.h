#ifndef _API_SIMPLE_H__
#define _API_SIMPLE_H__

#include "../../squirrel/squirrel.h"
#include "../../dataobj/ribi.h"

/** @file api_simple.h Helper functions to export simple datatypes: ribi & friends */

namespace script_api {

	SQInteger push_ribi(HSQUIRRELVM vm, ribi_t::ribi ribi);

	ribi_t::ribi get_ribi(HSQUIRRELVM vm, SQInteger index);

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
