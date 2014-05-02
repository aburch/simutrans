/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_thing_info_h
#define gui_thing_info_h

#include "../simdebug.h"
#include "../simobj.h"
#include "base_info.h"
#include "components/gui_obj_view_t.h"

/**
 * An adapter class to display info windows for things (objects)
 *
 * @author Hj. Malthaner
 * @date 22-Nov-2001
 */
class obj_infowin_t : public base_infowin_t
{
protected:
	obj_view_t view;

public:
	obj_infowin_t(const obj_t* obj);
	virtual ~obj_infowin_t() {}

	obj_t const* get_obj() const { return view.get_obj(); }

	virtual koord3d get_weltpos(bool) { return get_obj()->get_pos(); }

	virtual bool is_weltpos();

	/**
	 * @return the text to display in the info window
	 *
	 * @author Hj. Malthaner
	 * @see simwin
	 */
	virtual void info(cbuffer_t& buf) const { get_obj()->info(buf); }

	/**
	* Draw new component. The values to be passed refer to the window
	* i.e. It's the screen coordinates of the window where the
	* component is displayed.
	*/
	virtual void draw(scr_coord pos, scr_size size);
};


#endif
