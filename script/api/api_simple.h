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
};

#endif
