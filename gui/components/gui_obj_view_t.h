#ifndef OBJ_VIEW_T_H
#define OBJ_VIEW_T_H

#include "gui_world_view_t.h"

class obj_t;
class karte_t;

/**
 * Displays a thing on the world
 */
class obj_view_t : public world_view_t
{
private:
	obj_t const *obj; /**< The object to display */

protected:
	koord3d get_location();

public:
	obj_view_t(karte_t *w, koord const size) :
	  world_view_t(w, size), obj(NULL) {}

	obj_view_t(obj_t const *d, koord const size);

	obj_t const *get_obj() const { return obj; }

	void set_obj( obj_t const *d ) { obj = d; }

	void zeichnen(koord offset) { internal_draw(offset, obj); }

	/**
	 * resize window in response to a resize event
	 * need to recalculate the list of offsets
	 */
	void set_groesse(koord groesse) OVERRIDE;
};

#endif
