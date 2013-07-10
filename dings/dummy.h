#ifndef dings_dummy_h
#define dings_dummy_h

#include "../simdings.h"
#include "../simimg.h"


/**
 * prissi: a dummy typ for old things, which are now ignored
 */
class dummy_ding_t : public ding_t
{
	public:
		dummy_ding_t(karte_t* welt, loadsave_t* file) :
#ifdef INLINE_DING_TYPE
		    ding_t(welt, ding_t::undefined)
#else
			ding_t(welt)
#endif
		{
			rdwr(file);
			// do not remove from this position, since there will be nothing
			ding_t::set_flag(ding_t::not_on_map);
		}

#ifdef INLINE_DING_TYPE
#else
		typ      get_typ()  const { return ding_t::undefined; }
#endif
		image_id get_bild() const { return IMG_LEER; }
};

#endif
