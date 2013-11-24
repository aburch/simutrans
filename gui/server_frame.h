/**
 * server_frame.h
 *
 * Server game listing and current game information window
 */

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
 * When connected to a network server, this dialog shows game information
 * When not connected, it provides the mechanism for joining a remote game
 * @author Hj. Malthaner
 */
class server_frame_t : public gui_frame_t, private action_listener_t
{
private:
	gameinfo_t gi;
	cbuffer_t buf, time, revision_buf, pakset_checksum_buf;

	bool display_map;       // Controls minimap display
	bool custom_valid;      // Custom server entry is valid

	gui_textinput_t addinput, nick;
	static char newserver_name[2048];

	gui_scrolled_list_t serverlist;
	button_t add, join, find_mismatch;
	button_t show_mismatched, show_offline;
	gui_label_t info_list, info_manual, revision, pak_version, date, nick_label;
#if DEBUG>=4
	gui_label_t pakset_checksum;
#endif

	/**
	 * Server items to add to the listing panel
	 * Stores dnsname (for connection) and servername (for display in list)
	 * Also stores online/offline status of the server for filtering
	 * Adds get_dns() method to retrieve dnsname
	 * @author Timothy Baldock <tb@entropy.me.uk>
	 */
	class server_scrollitem_t : public gui_scrolled_list_t::const_text_scrollitem_t {
	private:
		cbuffer_t servername;
		cbuffer_t serverdns;
		bool status;
	public:
		server_scrollitem_t (const cbuffer_t& name, const cbuffer_t& dns, bool status, uint8 col) : gui_scrolled_list_t::const_text_scrollitem_t( NULL, col ), servername( name ), serverdns( dns ), status( status ) { servername.append( " (" ); servername.append( serverdns.get_str() ); servername.append( ")" ); }
		char const* get_text () const OVERRIDE { return servername.get_str(); }
		char const* get_dns () const { return serverdns.get_str(); }
		void set_text (char const* newtext) OVERRIDE { servername.clear(); servername.append( newtext ); serverdns.clear(); serverdns.append( newtext ); }
		bool online () { return status; }
	};

	/**
	 * Update UI fields with data from the current state of gameinfo_t gi
	 */
	void update_info();

	/**
	 * Update UI fields to show connection errors
	 * @author Timothy Baldock <tb@entropy.me.uk>
	 */
	void update_error ( const char* );

	/**
	 * Update server listing (retrieve from listings server)
	 * Display depends on the state of the show_mismatched and
	 * show_offline checkboxes
	 * @author Timothy Baldock <tb@entropy.me.uk>
	 */
	bool update_serverlist ();

public:
	server_frame_t();

	void draw(scr_coord pos, scr_size size);

	/**
	 * Return name of file which contains associated help text for this window
	 * @return Help file name, nor NULL if no help file exists
	 * @author Hj. Malthaner
	 */
	const char *get_hilfe_datei() const {return "server.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;
};

#endif
