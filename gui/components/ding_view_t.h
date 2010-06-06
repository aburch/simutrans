#ifndef DING_VIEW_T_H
#define DING_VIEW_T_H

#include "../../simdings.h"
#include "gui_world_view_t.h"


class ding_view_t : public world_view_t
{
	public:
		ding_view_t::ding_view_t(ding_t const* d, koord const size) : world_view_t(d->get_welt(), size), ding(d) {}

		void zeichnen(koord const offset) { internal_draw(offset, ding); }

	protected:
		koord3d ding_view_t::get_location() { return ding->get_pos(); }

	private:
		ding_t const* ding; /**< The object to display */
};

#endif
