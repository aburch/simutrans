/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_DUMMY_H
#define OBJ_DUMMY_H


#include "simobj.h"
#include "../display/simimg.h"


/**
 * A dummy type for old things, which are now ignored
 */
class dummy_obj_t : public obj_t
{
	public:
		dummy_obj_t(loadsave_t* file) :
			obj_t()
		{
			rdwr(file);
			// do not remove from this position, since there will be nothing
			obj_t::set_flag(obj_t::not_on_map);
		}

		typ      get_typ()   const OVERRIDE { return obj_t::undefined; }
		image_id get_image() const OVERRIDE { return IMG_EMPTY; }
};

#endif
