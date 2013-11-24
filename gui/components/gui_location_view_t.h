#ifndef LOCATION_VIEW_T_H
#define LOCATION_VIEW_T_H

#include "gui_world_view_t.h"


/**
 * Displays a location on the world
 * @author Hj. Malthaner
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

	void draw(scr_coord offset) { internal_draw(offset, 0); }

	koord3d get_location() { return location; }
};


#endif
