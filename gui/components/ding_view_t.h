#ifndef DING_VIEW_T_H
#define DING_VIEW_T_H

#include "../../simdings.h"
#include "gui_world_view_t.h"


class ding_view_t : public world_view_t
{
	public:
		ding_view_t(ding_t const* d, koord const size) : world_view_t(d->get_welt(), size), ding(d) {}

		ding_t const* get_ding() const { return ding; }

		void zeichnen(koord offset) { internal_draw(offset, ding); }

	protected:
		koord3d get_location() { return ding->get_pos(); }

	private:
		ding_t const* ding; /**< The object to display */
};

#endif
