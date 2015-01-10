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
#ifdef INLINE_OBJ_TYPE
			obj_t(obj_t::undefined)
#else
			obj_t()
#endif
		{
			rdwr(file);
			// do not remove from this position, since there will be nothing
			obj_t::set_flag(obj_t::not_on_map);
		}

#ifdef INLINE_OBJ_TYPE
#else
		typ      get_typ()  const { return obj_t::undefined; }
#endif
		image_id get_bild() const { return IMG_LEER; }
};

#endif
