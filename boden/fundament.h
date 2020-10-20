/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_FUNDAMENT_H
#define BODEN_FUNDAMENT_H


#include "grund.h"


/**
 * The foundation serves as base ground for all buildings in Simutrans.
 */
class fundament_t : public grund_t
{
protected:
	/**
	* The foundation always have the same image.
	*/
	void calc_image_internal(const bool calc_only_snowline_change);

public:
	fundament_t(loadsave_t *file, koord pos );
	fundament_t(koord3d pos,slope_t::type hang, bool build_up = true);

	const char *get_name() const OVERRIDE {return "Fundament";}

	typ get_typ() const { return fundament; }

	bool set_slope(slope_t::type) { slope = 0; return false; }
};

#endif
