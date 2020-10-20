/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_WEGE_KANAL_H
#define BODEN_WEGE_KANAL_H


#include "weg.h"
#include "../../dataobj/loadsave.h"
#include "../../utils/cbuffer_t.h"

/**
 * Ships can be created on docks
 */
class kanal_t : public weg_t
{

public:
	static const way_desc_t *default_kanal;

	kanal_t(loadsave_t *file);
	kanal_t();

	//waytype_t get_waytype() const {return water_wt;}
	virtual void rdwr(loadsave_t *file);
};

#endif
