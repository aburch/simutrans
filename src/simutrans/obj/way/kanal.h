/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_WEGE_KANAL_H
#define BODEN_WEGE_KANAL_H


#include "weg.h"
#include "../../dataobj/loadsave.h"
#include "../../utils/cbuffer.h"

/**
 * Ships can be created on docks
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
