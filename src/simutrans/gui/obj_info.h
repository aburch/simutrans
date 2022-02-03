/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_OBJ_INFO_H
#define GUI_OBJ_INFO_H


#include "../simdebug.h"
#include "../obj/simobj.h"
#include "base_info.h"
#include "components/gui_obj_view.h"

/**
 * An adapter class to display info windows for things (objects)
 */
class obj_infowin_t : public base_infowin_t
{
protected:
	obj_view_t view;

public:
	obj_infowin_t(const obj_t* obj);

	obj_t const* get_obj() const { return view.get_obj(); }

	koord3d get_weltpos(bool) OVERRIDE { return get_obj()->get_pos(); }

	bool is_weltpos() OVERRIDE;

	// refill buffer
	void fill_buffer();

	/**
	* Draw new component. The values to be passed refer to the window
	* i.e. It's the screen coordinates of the window where the
	* component is displayed.
	*/
	void draw(scr_coord pos, scr_size size) OVERRIDE;
};


#endif
