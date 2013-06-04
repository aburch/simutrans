/*
 * Player list
 * Hj. Malthaner, 2000
 */

#ifndef gui_spieler_h
#define gui_spieler_h

#include "../simconst.h"

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_combobox.h"
#include "components/gui_label.h"
#include "components/action_listener.h"


class karte_t;

/**
 * Menu for the player list
 * @author Hj. Malthaner
 */
class ki_kontroll_t : public gui_frame_t, private action_listener_t
{
private:
	gui_label_t *ai_income[MAX_PLAYER_COUNT-1];
	char account_str[MAX_PLAYER_COUNT-1][32];

	button_t	player_active[MAX_PLAYER_COUNT-2-1];
	button_t	player_get_finances[MAX_PLAYER_COUNT-1];
	button_t	player_change_to[MAX_PLAYER_COUNT-1];
	button_t	player_lock[MAX_PLAYER_COUNT-1];
	gui_combobox_t player_select[MAX_PLAYER_COUNT-1];

	button_t	freeplay;

	static karte_t *welt;

public:
	ki_kontroll_t(karte_t *welt);
	~ki_kontroll_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const {return "players.txt";}

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord pos, koord gr);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * Updates the dialogue window after changes to players states
	 * called from wkz_change_player_t::init
	 * necessary for network games to keep dialogues synchronous
	 * @author dwachs
	 */
	void update_data();

	// since no information are needed to be saved to restore this, returning magic is enough
	virtual uint32 get_rdwr_id() { return magic_ki_kontroll_t; }
};

#endif
