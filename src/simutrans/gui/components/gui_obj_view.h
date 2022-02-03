/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_OBJ_VIEW_T_H
#define GUI_COMPONENTS_GUI_OBJ_VIEW_T_H


#include "gui_world_view.h"

class obj_t;

/**
 * Displays a thing on the world
 */
class obj_view_t : public world_view_t
{
private:
	obj_t const *obj; /**< The object to display */

protected:
	koord3d get_location() OVERRIDE;

public:
	obj_view_t(scr_size const size) :
	  world_view_t(size), obj(NULL) {}

	obj_view_t(obj_t const *d, scr_size const size);

	obj_t const *get_obj() const { return obj; }

	void set_obj( obj_t const *d ) { obj = d; }

	void draw(scr_coord offset) OVERRIDE { internal_draw(offset, obj); }

	/**
	 * resize window in response to a resize event
	 * need to recalculate the list of offsets
	 */
	void set_size(scr_size size) OVERRIDE;
};

#endif
