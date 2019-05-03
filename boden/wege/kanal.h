#ifndef boden_wege_dock_h
#define boden_wege_dock_h

#include "weg.h"
#include "../../dataobj/loadsave.h"
#include "../../utils/cbuffer_t.h"

/**
 * Ships can be created on docks
 *
 * @author Hj. Malthaner
 */
class kanal_t : public weg_t
{
public:
	static const way_desc_t *default_kanal;

	kanal_t(loadsave_t *file);
	kanal_t();

	waytype_t get_waytype() const OVERRIDE {return water_wt;}
	void rdwr(loadsave_t *file) OVERRIDE;
};

#endif
