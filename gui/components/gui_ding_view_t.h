#ifndef DING_VIEW_T_H
#define DING_VIEW_T_H

#include "gui_world_view_t.h"

class ding_t;
class karte_t;

/**
 * Displays a thing on the world
 */
class ding_view_t : public world_view_t
{
private:
	ding_t const *ding; /**< The object to display */

protected:
	koord3d get_location();

public:
	ding_view_t(karte_t *w, koord const size) :
	  world_view_t(w, size), ding(NULL) {}

	ding_view_t(ding_t const *d, koord const size);

	ding_t const *get_ding() const { return ding; }

	void set_ding( ding_t const *d ) { ding = d; }

	void zeichnen(koord offset) { internal_draw(offset, ding); }

	/**
	 * resize window in response to a resize event
	 * need to recalculate the list of offsets
	 */
	void set_groesse(koord groesse) OVERRIDE;
};

#endif
