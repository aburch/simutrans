/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_PLAYER_FRAME_T_H
#define GUI_PLAYER_FRAME_T_H


#include "../simconst.h"

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_combobox.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "../utils/cbuffer_t.h"


/**
 * Menu for the player list
 * @author Hj. Malthaner
 */
class ki_kontroll_t : public gui_frame_t, private action_listener_t
{
	private:
		gui_label_buf_t
			*ai_income[MAX_PLAYER_COUNT-1]; // Income labels

		//gui_label_t
		//	lb_take_over_player[MAX_PLAYER_COUNT - 1],
		//	lb_take_over_cost[MAX_PLAYER_COUNT - 1];

		button_t
			player_active[MAX_PLAYER_COUNT-2-1],     // AI on/off button
			player_get_finances[MAX_PLAYER_COUNT-1], // Finance buttons
			player_change_to[MAX_PLAYER_COUNT-1],    // Set active player button
			*player_lock[MAX_PLAYER_COUNT-1],         // Set name & password button
			access_out[MAX_PLAYER_COUNT-1],
			access_in[MAX_PLAYER_COUNT-1],
			allow_take_over_of_company, cancel_take_over,
			take_over_player[MAX_PLAYER_COUNT - 1],
			freeplay;

		gui_combobox_t
			player_select[MAX_PLAYER_COUNT-1];

		cbuffer_t tooltip_out[MAX_PLAYER_COUNT];
		cbuffer_t tooltip_in[MAX_PLAYER_COUNT];

		char text_take_over_cost[MAX_PLAYER_COUNT - 1][50];
		char text_allow_takeover[50];
		char text_cancel_takeover[50];

		void update_income();

	public:
		ki_kontroll_t();

		/**
		 * Set the window associated helptext
		 * @return the filename for the helptext, or NULL
		 * @author Hj. Malthaner
		 */
		const char * get_help_filename() const OVERRIDE {return "players.txt";}

		/**
		 * Draw new component. The values to be passed refer to the window
		 * i.e. It's the screen coordinates of the window where the
		 * component is displayed.
		 * @author Hj. Malthaner
		 */
		void draw(scr_coord pos, scr_size size) OVERRIDE;

		bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

		/**
		 * Updates the dialogue window after changes to players states
		 * called from tool_change_player_t::init
		 * necessary for network games to keep dialogues synchronous
		 * @author dwachs
		 */
		void update_data();

		// since no information are needed to be saved to restore this, returning magic is enough
		virtual uint32 get_rdwr_id() OVERRIDE { return magic_ki_kontroll_t; }
};

#endif
