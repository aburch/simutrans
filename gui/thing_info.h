/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_thing_info_h
#define gui_thing_info_h

#include "../simdebug.h"
#include "../simdings.h"
#include "gui_frame.h"
#include "components/ding_view_t.h"
#include "components/gui_fixedwidth_textarea.h"
#include "../utils/cbuffer_t.h"

/**
 * An adapter class to display info windows for things (objects)
 *
 * @author Hj. Malthaner
 * @date 22-Nov-2001
 */
class ding_infowin_t : public gui_frame_t
{
protected:
	ding_view_t view;

	static cbuffer_t buf;

	gui_fixedwidth_textarea_t textarea;

public:
	ding_infowin_t(const ding_t* ding);

	ding_t const* get_ding() const { return view.get_ding(); }

	/**
	 * @return window title
	 *
	 * @author Hj. Malthaner
	 * @see simwin
	 */
	virtual char const* get_name() const { return get_ding()->get_name(); }

	/**
	 * @return the text to display in the info window
	 *
	 * @author Hj. Malthaner
	 * @see simwin
	 */
	virtual void info(cbuffer_t& buf) const { get_ding()->info(buf); }

	/**
	* komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	* das Fenster, d.h. es sind die Bildschirmkoordinaten des Fensters
	* in dem die Komponente dargestellt wird.
	*/
	virtual void zeichnen(koord pos, koord gr);
};


#endif
