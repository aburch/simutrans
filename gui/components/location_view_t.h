#ifndef LOCATION_VIEW_T_H
#define LOCATION_VIEW_T_H

#include "gui_world_view_t.h"


class location_view_t : public world_view_t
{
	public:
		location_view_t(karte_t* const welt, koord3d const location, koord const size) : world_view_t(welt, size), location(location) {}

		/** Set the location to be displayed. */
		void set_location(koord3d const l) { location = l; }

		void map_rotate90(sint16 const new_ysize) { location.rotate90(new_ysize); }

		void zeichnen(koord offset) { internal_draw(offset, 0); }

	protected:
		koord3d get_location() { return location; }

	private:
		koord3d location; /**< The location to display. */
};

#endif
