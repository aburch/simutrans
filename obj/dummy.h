#ifndef obj_dummy_h
#define obj_dummy_h

#include "../simobj.h"
#include "../display/simimg.h"


/**
 * prissi: a dummy typ for old things, which are now ignored
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

		typ      get_typ()  const { return obj_t::undefined; }
		image_id get_bild() const { return IMG_LEER; }
};

#endif
