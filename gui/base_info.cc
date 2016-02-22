#include "base_info.h"

static scr_coord default_margin(LINESPACE, LINESPACE);


base_infowin_t::base_infowin_t(const char *name, const player_t *player) :
	gui_frame_t(name, player),
	textarea(&buf, 16*LINESPACE),
	embedded(NULL)
{
	buf.clear();
	textarea.set_pos(scr_coord(D_MARGIN_LEFT,D_MARGIN_TOP));
	textarea.set_size( scr_size(D_DEFAULT_WIDTH-D_MARGIN_LEFT-D_MARGIN_RIGHT, 0) );
	add_component(&textarea);
	recalc_size();
}


void base_infowin_t::set_embedded(gui_component_t *other)
{
	if (embedded) {
		remove_component(embedded);
	}
	embedded = other;
	add_component(embedded);

	if (embedded) {
		textarea.set_reserved_area( embedded->get_size() );
		add_component(embedded);
	}
	else {
		textarea.set_reserved_area( scr_size(0,0) );
	}
	recalc_size();
}


void base_infowin_t::recalc_size()
{
	scr_size size = textarea.get_size();
	size.w = max( D_DEFAULT_WIDTH-(D_MARGIN_LEFT + D_MARGIN_RIGHT), size.w );
	if (embedded) {
		// move it to right algined
		embedded->set_pos( scr_coord( size.w+D_MARGIN_LEFT+D_H_SPACE-embedded->get_size().w, D_MARGIN_TOP ) );
		textarea.set_reserved_area( embedded->get_size()+scr_size(D_H_SPACE,D_V_SPACE) );
		textarea.recalc_size();
		size.h = max( embedded->get_size().h, textarea.get_size().h );
	}
	else {
		textarea.set_reserved_area( scr_size(0,0) );
		textarea.recalc_size();
		size = textarea.get_size();
	}
	size += scr_size(D_MARGIN_LEFT + D_MARGIN_RIGHT, D_TITLEBAR_HEIGHT + D_MARGIN_TOP + D_MARGIN_BOTTOM);
	set_windowsize( size );
}
