/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/**
 * An adapter class to display info windows for ground (floor) objects
 *
 * @author Hj. Malthaner
 * @date 20-Nov-2001
 */

#ifndef gui_ground_info_h
#define gui_ground_info_h

#include "base_info.h"
#include "components/gui_location_view_t.h"
#include "../utils/cbuffer_t.h"
#include "components/gui_fixedwidth_textarea.h"

class grund_t;

class grund_info_t : public base_infowin_t
{
protected:
	/**
	 * The ground we observe. The ground will delete this object
	 * if self deleted.
	 * @author Hj. Malthaner
	 */
	const grund_t* gr;

	location_view_t view;

public:
	grund_info_t(const grund_t* gr);

	virtual koord3d get_weltpos(bool);

	virtual bool is_weltpos();

	void draw(scr_coord pos, scr_size size);

	virtual void map_rotate90( sint16 );
};

#endif
