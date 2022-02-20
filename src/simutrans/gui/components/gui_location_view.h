/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_LOCATION_VIEW_H
#define GUI_COMPONENTS_GUI_LOCATION_VIEW_H


#include "gui_world_view.h"


/**
 * Displays a location on the world
 */
class location_view_t : public world_view_t
{
private:
	koord3d location; /**< The location to display. */

public:
	location_view_t(koord3d const location, scr_size const size) :
	  world_view_t(size), location(location) {}

	/** Set the location to be displayed. */
	void set_location(koord3d const l) { location = l; }

	void map_rotate90(sint16 const new_ysize) { location.rotate90(new_ysize); }

	void draw(scr_coord offset) OVERRIDE { internal_draw(offset, 0); }

	koord3d get_location() OVERRIDE { return location; }
};


#endif
