/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef replace_frame_t_h
#define replace_frame_t_h

#include "gui_frame.h"

#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_convoy_assembler.h"
#include "components/gui_convoy_label.h"
#include "components/gui_label.h"
#include "components/gui_numberinput.h"
#include "messagebox.h"


/**
 * Replace frame, makes convoys be marked for replacing.
 *
 * @author isidoro
 * @date Jan-09
 */
class replace_frame_t : public gui_frame_t,
                        public action_listener_t
{
private:
	/**
	 * The convoy to be replaced
	 */
	convoihandle_t cnv;

	bool replace_line;	// True if all convoys like this in its line are to be replaced
	bool replace_all;	// True if all convoys like this are to be replaced
	bool depot;		// True if convoy is to be sent to depot only
	bool autostart;		// True if convoy is to be sent to depot and restarted automatically
	enum {state_replace=0, state_sell, state_skip, n_states};
	uint8 state;
	uint8 replaced_so_far;
	sint32 money;

	/**
	 * Gui elements
	 */
	gui_convoy_label_t	lb_convoy;
	gui_label_t		lb_to_be_replaced;
	gui_convoy_assembler_t *convoy_assembler;
	gui_label_t		lb_money;
	button_t		bt_replace_line;
	button_t		bt_replace_all;
	button_t		bt_autostart;
	button_t		bt_depot;
	button_t		bt_mark;
	gui_label_t		lb_replace_cycle;
	gui_label_t		lb_replace;
	gui_label_t		lb_sell;
	gui_label_t		lb_skip;
	gui_label_t		lb_n_replace;
	gui_label_t		lb_n_sell;
	gui_label_t		lb_n_skip;
	gui_numberinput_t	numinp[n_states];
	char txt_money[16];
	char txt_n_replace[8];
	char txt_n_sell[8];
	char txt_n_skip[8];

	/**
	 * Update texts, image lists and buttons according to the current state.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void update_data();

	/**
	 * Do the dynamic dialog layout
	 */
	void layout(koord *);

	int total_width, min_total_width, total_height, min_total_height;

	// Some helper functions
	void update_total_height(int height);
	void update_total_width(int width);
	void replace_convoy(convoihandle_t cnv);
	inline void start_replacing() {state=state_replace; replaced_so_far=0;}
	uint8 get_present_state();

	karte_t* get_welt() { return cnv->get_welt(); }

	sint32 calc_total_cost();

public:
	replace_frame_t(convoihandle_t cnv, const char *name);
	virtual ~replace_frame_t();

	/**
	 * Setzt die Fenstergroesse
	 * @author (Mathew Hounsell)
	 * @date   11-Mar-2003
	 */
	void set_fenstergroesse(koord groesse);

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const {return "replace.txt";}

	void infowin_event(const event_t *ev);

	/**
	 * Zeichnet das Frame
	 * @author Hansjörg Malthaner
	 */
	void zeichnen(koord pos, koord gr);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);

	const convoihandle_t get_convoy() const { return cnv; }

};

#endif