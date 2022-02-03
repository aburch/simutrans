/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "headquarter_info.h"
#include "simwin.h"
#include "../tool/simmenu.h"
#include "../world/simworld.h"
#include "../obj/gebaeude.h"
#include "../player/simplay.h"

headquarter_info_t::headquarter_info_t(player_t* player) : base_infowin_t(player->get_name(), player),
	headquarter_view(koord3d::invalid, scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) ) )
{
	this->player = player;

	set_embedded(&headquarter_view);

	upgrade.init(button_t::roundbox, "upgrade HQ");
	upgrade.add_listener(this);
	add_component(&upgrade);

	update();
	recalc_size();
}


void headquarter_info_t::update()
{
	// possibly update hq position
	const koord p2d = player->get_headquarter_pos();
	if (p2d != koord::invalid) {
		// update hq position
		grund_t *gr = welt->lookup_kartenboden(p2d);
		koord3d pos = gr->get_pos();
		headquarter_view.set_location(pos);

		gebaeude_t* hq = gr->find<gebaeude_t>();
		if (hq) {
			buf.clear();
			hq->info(buf);
		}
	}
	else {
		set_embedded(NULL);
	}
	// check for possible upgrades
	const char * c = tool_t::general_tool[TOOL_HEADQUARTER]->get_tooltip(player);
	if(c) {
		// only true, if the headquarter can be built/updated
		headquarter_tooltip.clear();
		headquarter_tooltip.append(c);
		upgrade.set_tooltip(headquarter_tooltip);
		upgrade.enable();
	}
	else {
		upgrade.disable();
		upgrade.set_tooltip(NULL);
	}
	reset_min_windowsize();
}


void headquarter_info_t::draw(scr_coord pos, scr_size size)
{
	update();

	if (get_embedded() == NULL) {
		// no headquarter anymore, destroy window
		destroy_win(this);
		return;
	}

	base_infowin_t::draw(pos, size);
}


bool headquarter_info_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(  comp == &upgrade  ) {
		welt->set_tool( tool_t::general_tool[TOOL_HEADQUARTER], player );
	}
	return true;
}
