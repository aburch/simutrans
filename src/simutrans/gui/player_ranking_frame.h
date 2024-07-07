/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_PLAYER_RANKING_GUI_H
#define GUI_PLAYER_RANKING_GUI_H


#include "gui_frame.h"
#include "../player/finance.h"
#include "components/gui_label.h"
#include "components/gui_chart.h"
#include "components/gui_button.h"
#include "components/action_listener.h"
#include "simwin.h"

#include "../tpl/vector_tpl.h"
#include "../utils/cbuffer.h"


class player_button_t : public button_t
{
	uint8 player_nr;

public:
	player_button_t(uint8 player_nr);

	// set color and name
	void update();

	uint8 get_player_nr() const { return player_nr; }
};

/**
 * Menu for the player list
 */
class player_ranking_frame_t : public gui_frame_t, private action_listener_t
{
public:
	enum
	{
		PR_REVENUE,
		PR_PROFIT,
		PR_MARGIN,
		PR_TRANSPORT_PAX,
		PR_TRANSPORT_MAIL,
		PR_TRANSPORT_GOODS,
		PR_CASH,
		PR_NETWEALTH,
		PR_CONVOIS,
//		PR_VEHICLES,
//		PR_HALTS,
		MAX_PLAYER_RANKING_CHARTS
	};
	static uint8 transport_type_option;

private:
	sint16 last_year;

	gui_chart_t chart;

	gui_aligned_container_t cont_players;
	gui_scrollpane_t scrolly;
	button_t bt_charts[MAX_PLAYER_RANKING_CHARTS];

	char years_back_s[MAX_PLAYER_HISTORY_YEARS][5];

	gui_combobox_t years_back_c, transport_type_c;
	uint16 transport_types[TT_OTHER];

	gui_label_buf_t lb_player_val[MAX_PLAYER_COUNT-1];

	sint64 p_chart_table[MAX_PLAYER_HISTORY_YEARS][MAX_PLAYER_COUNT-1];

	slist_tpl<player_button_t *> buttons;

	uint8 selected_item= PR_REVENUE;
	uint8 selected_player;

	bool is_chart_table_zero(uint8 player_nr) const;

public:
	player_ranking_frame_t(uint8 selected_player_nr=255);
	~player_ranking_frame_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char * get_help_filename() const OVERRIDE { return "player_ranking.txt"; }

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void update_chart(bool full_update);

	// since no information are needed to be saved to restore this, returning magic is enough
	uint32 get_rdwr_id() OVERRIDE { return magic_player_ranking; }

	void rdwr(loadsave_t* file) OVERRIDE;
};

#endif
