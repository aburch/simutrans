/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "kennfarbe.h"
#include "../world/simworld.h"
#include "../simskin.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../player/simplay.h"
#include "components/gui_label.h"
#include "components/gui_image.h"
#include "../tool/simmenu.h"

/**
 * Buttons forced to be square ...
 */
class choose_color_button_t : public button_t
{
	scr_coord_val w;
public:
	choose_color_button_t() : button_t()
	{
		w = max(D_BUTTON_HEIGHT, display_get_char_width('X') + D_BUTTON_PADDINGS_X);
	}
	scr_size get_min_size() const OVERRIDE
	{
		return scr_size(w, D_BUTTON_HEIGHT);
	}
};


farbengui_t::farbengui_t(player_t *player_) :
	gui_frame_t( translator::translate("Farbe"), player_ ),
	txt(&buf)
{
	player = player_;
	buf.clear();
	buf.append(translator::translate("COLOR_CHOOSE\n"));

	set_table_layout(1,0);

	add_table(2,1);
	// Info text
	txt.recalc_size();
	add_component( &txt );

	// Picture
	new_component<gui_image_t>(skinverwaltung_t::color_options->get_image_id(0), player->get_player_nr(), ALIGN_NONE, true);
	end_table();

	// Player's primary color label
	new_component<gui_label_t>("Your primary color:");

	// Get all colors (except the current player's)
	uint32 used_colors1 = 0;
	uint32 used_colors2 = 0;
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  i!=player->get_player_nr()  &&  welt->get_player(i)  ) {
			used_colors1 |= 1 << (welt->get_player(i)->get_player_color1() / 8);
			used_colors2 |= 1 << (welt->get_player(i)->get_player_color2() / 8);
		}
	}

	add_table(14,2);
	// Primary color buttons
	for(unsigned i=0;  i<28;  i++) {
		player_color_1[i] = new_component<choose_color_button_t>();
		player_color_1[i]->init( button_t::box_state, (used_colors1 & (1<<i) ? "X" : " "));
		player_color_1[i]->background_color = color_idx_to_rgb(i*8+4);
		player_color_1[i]->add_listener(this);
	}
	player_color_1[player->get_player_color1()/8]->pressed = true;
	end_table();

	// Player's secondary color label
	new_component<gui_label_t>("Your secondary color:");

	add_table(14,2);
	// Secondary color buttons
	for(unsigned i=0;  i<28;  i++) {
		player_color_2[i] = new_component<choose_color_button_t>();
		player_color_2[i]->init( button_t::box_state, (used_colors2 & (1<<i) ? "X" : " "));
		player_color_2[i]->background_color = color_idx_to_rgb(i*8+4);
		player_color_2[i]->add_listener(this);
	}
	player_color_2[player->get_player_color2()/8]->pressed = true;
	end_table();

	reset_min_windowsize();
}


/**
 * This method is called if an action is triggered
 */
bool farbengui_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	for(unsigned i=0;  i<28;  i++) {

		// new player 1 color?
		if(comp==player_color_1[i]) {
			for(unsigned j=0;  j<28;  j++) {
				player_color_1[j]->pressed = false;
			}
			player_color_1[i]->pressed = true;
			//env_t::default_settings.set_default_player_color(player->get_player_nr(), player->get_player_color1(), player->get_player_color2());

			// re-colour a player
			cbuffer_t buf;
			buf.printf( "1%u,%i", player->get_player_nr(), i*8);
			tool_t *w = create_tool( TOOL_RECOLOUR_TOOL | SIMPLE_TOOL );
			w->set_default_param( buf );
			world()->set_tool( w, player );
			// since init always returns false, it is save to delete immediately
			delete w;
			return true;
		}

		// new player color 2?
		if(comp==player_color_2[i]) {
			for(unsigned j=0;  j<28;  j++) {
				player_color_2[j]->pressed = false;
			}
			player_color_2[i]->pressed = true;
			//env_t::default_settings.set_default_player_color(player->get_player_nr(), player->get_player_color1(), player->get_player_color2());

			// re-colour a player
			cbuffer_t buf;
			buf.printf( "2%u,%i", player->get_player_nr(), i*8);
			tool_t *w = create_tool( TOOL_RECOLOUR_TOOL | SIMPLE_TOOL );
			w->set_default_param( buf );
			world()->set_tool( w, player );
			// since init always returns false, it is save to delete immediately
			delete w;
			return true;
		}

	}

	return false;
}
