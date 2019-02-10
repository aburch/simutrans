#include "base_info.h"


base_infowin_t::base_infowin_t(const char *name, const player_t *player) :
	gui_frame_t(name, player),
	textarea(&buf, D_DEFAULT_WIDTH-D_MARGIN_LEFT-D_MARGIN_RIGHT),
	embedded(NULL)
{
	set_table_layout(1,0);

	buf.clear();

	add_component(&textarea);
}


void base_infowin_t::set_embedded(gui_component_t *other)
{
	if (embedded) {
		remove_component(embedded);
	}
	// add the component to the gui-container,
	// but set to invisible/not rigid,
	// as it will spoil the size calculations otherwise
	embedded = other;
	add_component(embedded);

	if (embedded) {
		embedded->set_visible(false);
		embedded->set_rigid(false);
		textarea.set_reserved_area( embedded->get_size() + scr_size(D_H_SPACE,D_V_SPACE) );
	}
	else {
		textarea.set_reserved_area( scr_size(0,0) );
	}
	recalc_size();
}

void base_infowin_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);

	if (embedded) {
		// move it to right aligned
		embedded->set_pos( scr_coord( size.w-D_MARGIN_LEFT-embedded->get_size().w, textarea.get_pos().y) );
	}
}

void base_infowin_t::draw(scr_coord pos, scr_size size)
{
	gui_frame_t::draw(pos, size);
	if (embedded) {
		embedded->draw(pos + get_pos());
	}
}


void base_infowin_t::recalc_size()
{
	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}


bool base_infowin_t::infowin_event(const event_t *ev)
{
	if (embedded) {
		embedded->set_visible(true);
	}
	bool swallowed = gui_frame_t::infowin_event(ev);
	if (embedded) {
		embedded->set_visible(false);
	}
	return swallowed;
}
