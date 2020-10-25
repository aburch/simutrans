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
#include "simwin.h"


/**
 * Menu for the player list
 */
class ki_kontroll_t : public gui_frame_t, private action_listener_t
{
private:
	gui_label_t *ai_income[MAX_PLAYER_COUNT-1];
	char account_str[MAX_PLAYER_COUNT-1][32];

	button_t		player_active[MAX_PLAYER_COUNT-2-1];
	button_t		player_get_finances[MAX_PLAYER_COUNT-1];
	button_t		player_change_to[MAX_PLAYER_COUNT-1];
	button_t		player_lock[MAX_PLAYER_COUNT-1];
	gui_combobox_t	player_select[MAX_PLAYER_COUNT-1];
	button_t		access_out[MAX_PLAYER_COUNT-1];
	button_t		access_in[MAX_PLAYER_COUNT-1];
	button_t		allow_take_over_of_company, cancel_take_over;
	button_t		take_over_player[MAX_PLAYER_COUNT - 1];

	gui_label_t		player_label;
	gui_label_t		password_label;
	gui_label_t		access_label;
	gui_label_t		cash_label;
	gui_label_t		company_takeovers;
	gui_label_t		lb_take_over_player[MAX_PLAYER_COUNT - 1];
	gui_label_t		lb_take_over_cost[MAX_PLAYER_COUNT - 1];

	cbuffer_t tooltip_out[MAX_PLAYER_COUNT];
	cbuffer_t tooltip_in[MAX_PLAYER_COUNT];

	button_t	freeplay;

	char text_take_over_cost[MAX_PLAYER_COUNT - 1][50];
	char text_allow_takeover[50];
	char text_cancel_takeover[50];

public:
	ki_kontroll_t();
	~ki_kontroll_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char * get_help_filename() const OVERRIDE {return "players.txt";}

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * Updates the dialogue window after changes to players states
	 * called from tool_change_player_t::init
	 * necessary for network games to keep dialoguess synchronous
	 */
	void update_data();

	// since no information are needed to be saved to restore this, returning magic is enough
	virtual uint32 get_rdwr_id() OVERRIDE { return magic_ki_kontroll_t; }
};

#endif
