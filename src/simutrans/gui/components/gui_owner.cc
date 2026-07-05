

#include "../../display/simgraph.h"
#include "../../player/simplay.h"

#include "../../dataobj/environment.h"
#include "../../world/simworld.h"

#include "gui_owner.h"


void gui_owners_t::set_owners(uint16 mask)
{
	// draw ownership
	player_mask = mask;
	num_players = 0;
	for (uint16 i = 0; i < MAX_PLAYER_COUNT; i++) {
		if ((1 << i) & player_mask) {
			num_players++;
		}
	}
	set_visible(num_players > 1);
}

scr_size gui_owners_t::get_min_size() const
{
	if (num_players > 1) {
		if (env_t::horizontal_stripe_owner) {
			return scr_size(LINESPACE, LINESPACE);
		}
		else {
			return scr_size(num_players * LINESPACE - 4, LINESPACE);
		}
	}

	return scr_size(0, 0);
}

void gui_owners_t::draw(scr_coord offset)
{
	if (num_players > 1) {
		static PIXVAL playercolors[MAX_PLAYER_COUNT];
		for (uint16 j = 0, i = 0; i < MAX_PLAYER_COUNT; i++) {
			if (world()->get_player(i) && (player_mask & (1 << i)) != 0) {
				playercolors[j++] = gfx->palette_lookup(world()->get_player(i)->get_player_color1() + 3);
			}
		}
		gfx->draw_rect_colors_clipped(pos.x + offset.x, pos.y + offset.y, (env_t::horizontal_stripe_owner ? num_players * LINESPACE - 4 : LINESPACE), LINESPACE, playercolors, num_players, env_t::horizontal_stripe_owner, true  CLIP_NUM_DEFAULT);
	}
}
