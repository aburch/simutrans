#ifndef boden_wege_dock_h
#define boden_wege_dock_h

#include "weg.h"
#include "../../dataobj/loadsave.h"
#include "../../utils/cbuffer_t.h"

/**
 * Am Dock können Schiffe anlegen
 *
 * @author Hj. Malthaner
 */
class kanal_t : public weg_t
{

public:
	static const weg_besch_t *default_kanal;

	kanal_t(karte_t *welt, loadsave_t *file);
	kanal_t(karte_t *welt);

	virtual void calc_bild(koord3d) { weg_t::calc_bild(); }
	waytype_t get_waytype() const {return water_wt;}
	virtual void rdwr(loadsave_t *file);
};

#endif
