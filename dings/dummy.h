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
	dummy_ding_t(karte_t *welt, loadsave_t *file);
	dummy_ding_t(karte_t *welt, koord3d pos, spieler_t *sp);

	enum ding_t::typ gib_typ() const { return ding_t::undefined; }

	image_id gib_bild() const { return IMG_LEER; }

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
