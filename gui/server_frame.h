#ifndef gui_serverframe_h
#define gui_serverframe_h

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_combobox.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "components/gui_textinput.h"
#include "../dataobj/gameinfo.h"
#include "../utils/cbuffer_t.h"

/**
 * Show serverinfo and allows to join games
 * @author Hj. Malthaner
 */
class server_frame_t : public gui_frame_t, private action_listener_t
{
private:
	karte_t *welt;
	gameinfo_t gi;
	cbuffer_t	buf, time, revision_buf, pakset_checksum_buf;

	button_t add, join, find_mismatch;
	button_t show_all_rev;
	gui_combobox_t serverlist;
	gui_label_t revision, pak_version, date, nick_label;
	gui_textinput_t nick;
#if DEBUG>=4
	gui_label_t pakset_checksum;
#endif

	void update_info();

	/*
	 * Update server listing (retrieve from listings server)
	 * Only displays entries which match the revision and pakset provided
	 * Pass revision=0, pakset=NULL to see all entries
	 * @author Timothy Baldock <tb@entropy.me.uk>
	 */
	bool update_serverlist( uint32 revision, const char *pakset );

public:
	server_frame_t( karte_t *welt );

	void zeichnen(koord pos, koord gr);

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char *get_hilfe_datei() const {return "server.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;
};

#endif
