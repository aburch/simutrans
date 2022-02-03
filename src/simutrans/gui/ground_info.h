/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_GROUND_INFO_H
#define GUI_GROUND_INFO_H


#include "gui_frame.h"
#include "components/gui_location_view.h"
#include "../utils/cbuffer.h"
#include "components/gui_textarea.h"

class grund_t;

class grund_info_t : public gui_frame_t
{
protected:
	/**
	 * The ground we observe. The ground will delete this object
	 * if self deleted.
	 */
	cbuffer_t buf, empty;

	const grund_t* gr;

	location_view_t view;
	gui_textarea_t textarea, textarea2;

	void recalc_size();

public:
	grund_info_t(const grund_t* gr);

	koord3d get_weltpos(bool) OVERRIDE;

	bool is_weltpos() OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	void map_rotate90( sint16 ) OVERRIDE;
};

#endif
