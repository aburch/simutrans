/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_ground_info_h
#define gui_ground_info_h

#include "gui_frame.h"
#include "components/gui_location_view_t.h"
#include "../utils/cbuffer_t.h"
#include "components/gui_fixedwidth_textarea.h"

class grund_t;

/**
 * An adapter class to display info windows for ground (floor) objects
 *
 * @author Hj. Malthaner
 * @date 20-Nov-2001
 */
class grund_info_t : public gui_frame_t
{
protected:
	/**
	 * The ground we observe. The ground will delete this object
	 * if self deleted.
	 * @author Hj. Malthaner
	 */
	const grund_t* gr;
	location_view_t view;
	static cbuffer_t gr_info;
	gui_fixedwidth_textarea_t textarea;

public:
	grund_info_t(const grund_t* gr);

	virtual koord3d get_weltpos(bool);

	virtual bool is_weltpos();

	void zeichnen(koord pos, koord gr);

	virtual void map_rotate90( sint16 );
};

#endif
