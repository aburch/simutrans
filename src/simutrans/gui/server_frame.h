/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SERVER_FRAME_H
#define GUI_SERVER_FRAME_H


#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_combobox.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "../dataobj/gameinfo.h"
#include "../utils/cbuffer.h"

class gui_minimap_t;
/**
 * When connected to a network server, this dialog shows game information
 * When not connected, it provides the mechanism for joining a remote game
 */
class server_frame_t : public gui_frame_t, private action_listener_t
{
private:
	gameinfo_t gi;
	cbuffer_t buf;

	bool custom_valid;      // Custom server entry is valid

	gui_textinput_t addinput, nick;
	static char newserver_name[2048];

	gui_scrolled_list_t serverlist;
	button_t add, join, find_mismatch;
	button_t show_mismatched, show_offline;
	gui_label_t pak_version;
	gui_label_buf_t revision, date, label_size;
#if MSG_LEVEL>=4
	gui_label_buf_t pakset_checksum;
#endif
	gui_minimap_t *map;
	gui_textarea_t game_text;
	/**
	 * Server items to add to the listing panel
	 * Stores dnsname (for connection) and servername (for display in list)
	 * Also stores online/offline status of the server for filtering
	 * Adds get_dns() method to retrieve dnsname
	 */
	class server_scrollitem_t : public gui_scrolled_list_t::const_text_scrollitem_t {
	private:
		cbuffer_t servername;
		cbuffer_t serverdns;
		cbuffer_t server_altdns;
		bool status;
	public:
		server_scrollitem_t (const cbuffer_t& name, const cbuffer_t& dns, const cbuffer_t& altdns, bool status, PIXVAL col) :
			gui_scrolled_list_t::const_text_scrollitem_t( NULL, col ),
			servername( name ), serverdns( dns ),
			server_altdns(altdns),
			status( status )
		{
			servername.append( " (" );
			servername.append( serverdns.get_str() );
			if (  server_altdns.len() > 0  ) {
				servername.append(" or ");
				servername.append(server_altdns.get_str());
			}
			servername.append(")");
		}
		char const* get_text () const OVERRIDE { return servername.get_str(); }
		char const* get_dns() const { return serverdns.get_str(); }
		char const* get_altdns() const { return server_altdns.get_str(); }
		void set_text (char const* newtext) OVERRIDE { servername.clear(); servername.append( newtext ); serverdns.clear(); serverdns.append( newtext ); }
		bool online () { return status; }
	};

	/**
	 * Update UI fields with data from the current state of gameinfo_t gi
	 */
	PIXVAL update_info();

	/**
	 * Update UI fields to show connection errors
	 */
	void update_error ( const char* );

	/**
	 * Update server listing (retrieve from listings server)
	 * Display depends on the state of the show_mismatched and
	 * show_offline checkboxes
	 */
	bool update_serverlist ();

public:
	server_frame_t();

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	/**
	 * Return name of file which contains associated help text for this window
	 * @return Help file name, nor NULL if no help file exists
	 */
	const char *get_help_filename() const OVERRIDE {return "server.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;
};

#endif
