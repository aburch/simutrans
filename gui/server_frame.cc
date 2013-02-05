/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simworld.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simwin.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"
#include "../utils/csv.h"
#include "../simversion.h"


#include "../dataobj/translator.h"
#include "../dataobj/network.h"
#include "../dataobj/network_file_transfer.h"
#include "../dataobj/network_cmd_ingame.h"
#include "../dataobj/network_cmp_pakset.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/pakset_info.h"
#include "../player/simplay.h"
#include "server_frame.h"
#include "messagebox.h"
#include "help_frame.h"

static char nick_buf[256];
char server_frame_t::newserver_name[2048] = "";

server_frame_t::server_frame_t(karte_t* w) :
	gui_frame_t( translator::translate("Game info") ),
	welt(w),
	gi(welt),
	serverlist( gui_scrolled_list_t::select )
{
	update_info();
	display_map = true;

	const int ww = !umgebung_t::networkmode ? 320 : 280;  // Window width
	sint16 pos_y = D_MARGIN_TOP;      // Initial location

	// When in network mode, display only local map info (and nickname changer)
	// When not in network mode, display server picker
	if (  !umgebung_t::networkmode  ) {
		info_list.set_pos( koord( D_MARGIN_LEFT, pos_y ) );
		info_list.set_text( "Select a server to join:" );
		add_komponente( &info_list );
		pos_y += LINESPACE;
		pos_y += D_V_SPACE;

		// Server listing
		const int serverlist_height = D_BUTTON_HEIGHT * 6;
		serverlist.set_pos( koord( D_MARGIN_LEFT, pos_y ) );
		serverlist.set_groesse( koord( ww - D_MARGIN_LEFT - D_MARGIN_RIGHT, serverlist_height ) );
		serverlist.add_listener( this );
		serverlist.set_selection( 0 );
		serverlist.set_highlight_color( 0 );
		add_komponente( &serverlist );

		add_komponente( &add );

		pos_y += serverlist_height;
		pos_y += D_V_SPACE;

		// Show mismatched checkbox
		if (  !show_mismatched.pressed  ) {
			show_mismatched.init( button_t::square_state, "Show mismatched", koord( D_MARGIN_LEFT, pos_y ), koord( (ww - D_MARGIN_LEFT - D_H_SPACE - D_MARGIN_RIGHT) / 2, D_BUTTON_HEIGHT) );
			show_mismatched.set_tooltip( "Show servers where game version or pakset does not match your client" );
			show_mismatched.add_listener( this );
			add_komponente( &show_mismatched );
		}

		// Show offline checkbox
		if (  !show_offline.pressed  ) {
			show_offline.init( button_t::square_state, "Show offline", koord( D_MARGIN_LEFT + (ww - D_MARGIN_LEFT - D_H_SPACE - D_MARGIN_RIGHT) / 2 + D_H_SPACE, pos_y ), koord( (ww - D_MARGIN_LEFT - D_H_SPACE - D_MARGIN_RIGHT) / 2, D_BUTTON_HEIGHT) );
			show_offline.set_tooltip( "Show servers that are offline" );
			show_offline.add_listener( this );
			add_komponente( &show_offline );
		}


		pos_y += D_BUTTON_HEIGHT;
		pos_y += D_V_SPACE * 2;                // GUI line goes here

		info_manual.set_pos( koord( D_MARGIN_LEFT, pos_y ) );
		info_manual.set_text( "Or enter a server manually:" );
		add_komponente( &info_manual );
		pos_y += LINESPACE;
		pos_y += D_V_SPACE;	// TODO less?

		// Add server input/button
		addinput.set_pos( koord( D_MARGIN_LEFT, pos_y ) );
		addinput.set_text( newserver_name, sizeof( newserver_name ) );
		addinput.set_groesse( koord( ww - D_MARGIN_LEFT - D_MARGIN_RIGHT - D_BUTTON_WIDTH - D_H_SPACE, D_BUTTON_HEIGHT ) );
		addinput.add_listener( this );
		add_komponente( &addinput );

		add.init( button_t::box, "Query server", koord( ww - D_BUTTON_WIDTH - D_MARGIN_RIGHT, pos_y ), koord( D_BUTTON_WIDTH, D_BUTTON_HEIGHT) );
		add.add_listener( this );

		pos_y += D_BUTTON_HEIGHT;
		pos_y += D_V_SPACE * 2;                // GUI line goes here
	}

	revision.set_pos( koord( D_MARGIN_LEFT, pos_y ) );
	add_komponente( &revision );
	show_mismatched.pressed = gi.get_game_engine_revision() == 0;

	pos_y += LINESPACE;

	pak_version.set_pos( koord( D_MARGIN_LEFT, pos_y ) );
	add_komponente( &pak_version );

#if DEBUG>=4
	pos_y += LINESPACE;
	pakset_checksum.set_pos( koord( D_MARGIN_LEFT, pos_y ) );
	add_komponente( &pakset_checksum );
#endif

	pos_y += LINESPACE;
	pos_y += D_V_SPACE * 2;        // GUI line goes here

	date.set_pos( koord( ww - D_MARGIN_LEFT, pos_y ) );
	date.set_align( gui_label_t::right );
	add_komponente( &date );

	// Leave room for elements added during draw phase (multiline text + map)
	pos_y += LINESPACE * 8;

	pos_y += D_V_SPACE * 2;        // GUI line goes here

	const int nick_width = 80;
	nick_label.set_pos( koord( D_MARGIN_LEFT, pos_y ) );
	nick_label.set_text( "Nickname:" );
	add_komponente( &nick_label );

	nick.set_pos( koord( D_MARGIN_LEFT + D_H_SPACE + nick_width, pos_y ) );
	nick.add_listener(this);
	nick.set_text( nick_buf, lengthof( nick_buf ) );
	nick.set_groesse( koord( ww - D_MARGIN_LEFT - D_H_SPACE - D_MARGIN_RIGHT - nick_width, D_BUTTON_HEIGHT ) );
	tstrncpy( nick_buf, umgebung_t::nickname.c_str(), min( lengthof( nick_buf ), umgebung_t::nickname.length() + 1 ) );
	add_komponente( &nick );

	pos_y += D_BUTTON_HEIGHT;

	if (  !umgebung_t::networkmode  ) {
		pos_y += D_V_SPACE * 2;                // GUI line goes here

		const int button_width = 112;

		find_mismatch.init( button_t::box, "find mismatch", koord( ww - D_MARGIN_RIGHT - D_H_SPACE - button_width * 2, pos_y ), koord( button_width, D_BUTTON_HEIGHT) );
		find_mismatch.add_listener( this );
		add_komponente( &find_mismatch );

		join.init( button_t::box, "join game", koord( ww - D_MARGIN_RIGHT - button_width, pos_y ), koord( button_width, D_BUTTON_HEIGHT) );
		join.disable();
		join.add_listener( this );
		add_komponente( &join );

		// only update serverlist, when not already in network mode
		// otherwise desync to current game may happen
		update_serverlist();

		pos_y += D_BUTTON_HEIGHT;
	}

	pos_y += D_MARGIN_BOTTOM;
	pos_y += D_TITLEBAR_HEIGHT;

	set_fenstergroesse( koord( ww, pos_y ) );
	set_min_windowsize( koord( ww, pos_y ) );
	set_resizemode( no_resize );
}


void server_frame_t::update_error (const char* errortext)
{
	buf.clear();
	display_map = false;
	date.set_text( errortext );
	revision.set_text( "" );
	pak_version.set_text( "" );
	join.disable();
	find_mismatch.disable();
}


void server_frame_t::update_info ()
{

	display_map = true;

	// Compare with current world to determine status
	gameinfo_t current( welt );
	bool engine_match = false;
	bool pakset_match = false;

	// Zero means do not know => assume match
	if (  current.get_game_engine_revision() == 0  ||  gi.get_game_engine_revision() == 0  ||  current.get_game_engine_revision() == gi.get_game_engine_revision()  ) {
		engine_match = true;
	}

	if (  gi.get_pakset_checksum() == current.get_pakset_checksum()  ) {
		pakset_match = true;
		find_mismatch.disable();
	}
	else {
		find_mismatch.enable();
	}

	// State of join button
	if (  (serverlist.get_selection() >= 0  ||  custom_valid)  &&  pakset_match  &&  engine_match  ) {
		join.enable();
	}
	else {
		join.disable();
	}

	// Update all text fields
	char temp[32];
	buf.clear();
	buf.printf( "%ux%u\n", gi.get_groesse_x(), gi.get_groesse_y() );
	if (  gi.get_clients() != 255  ) {
		uint8 player = 0, locked = 0;
		for (  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
			if (  gi.get_player_type(i)&~spieler_t::PASSWORD_PROTECTED  ) {
				player ++;
				if (  gi.get_player_type(i)&spieler_t::PASSWORD_PROTECTED  ) {
					locked ++;
				}
			}
		}
		buf.printf( translator::translate("%u Player (%u locked)\n"), player, locked );
		buf.printf( translator::translate("%u Client(s)\n"), (unsigned)gi.get_clients() );
	}
	buf.printf( "%s %u\n", translator::translate("Towns"), gi.get_anzahl_staedte() );
	number_to_string( temp, gi.get_einwohnerzahl(), 0 );
	buf.printf( "%s %s\n", translator::translate("citicens"), temp );
	buf.printf( "%s %u\n", translator::translate("Factories"), gi.get_industries() );
	buf.printf( "%s %u\n", translator::translate("Convoys"), gi.get_convoi_count() );
	buf.printf( "%s %u\n", translator::translate("Stops"), gi.get_halt_count() );

	revision_buf.clear();
	revision_buf.printf( "%s %u", translator::translate( "Revision:" ), gi.get_game_engine_revision() );
	revision.set_text( revision_buf );
	revision.set_color( engine_match ? COL_BLACK : COL_RED );

	pak_version.set_text( gi.get_pak_name() );
	pak_version.set_color( pakset_match ? COL_BLACK : COL_RED );

#if DEBUG>=4
	pakset_checksum_buf.clear();
	pakset_checksum_buf.printf("%s %s",translator::translate( "Pakset checksum:" ), gi.get_pakset_checksum().get_str(8));
	pakset_checksum.set_text( pakset_checksum_buf );
	pakset_checksum.set_color( pakset_match ? COL_BLACK : COL_RED );
#endif

	time.clear();
	char const* const month = translator::get_month_name(gi.get_current_month());
	switch (umgebung_t::show_month) {
		case umgebung_t::DATE_FMT_GERMAN:
		case umgebung_t::DATE_FMT_MONTH:
		case umgebung_t::DATE_FMT_SEASON:
		case umgebung_t::DATE_FMT_GERMAN_NO_SEASON:
			time.printf( "%s %d", month, gi.get_current_year() );
			break;

		case umgebung_t::DATE_FMT_US:
		case umgebung_t::DATE_FMT_JAPANESE:
		case umgebung_t::DATE_FMT_JAPANESE_NO_SEASON:
		case umgebung_t::DATE_FMT_US_NO_SEASON:
			time.printf( "%i/%s", gi.get_current_year(), month );
			break;
	}
	date.set_text( time );
	set_dirty();
}


bool server_frame_t::update_serverlist ()
{
	// Based on current dialog settings, should we show mismatched servers or not
	uint revision = 0;
	const char* pakset = NULL;

	if (  !show_mismatched.pressed  ) {
		revision = gi.get_game_engine_revision();
		pakset = gi.get_pak_name();
	}

	// Download game listing from listings server into memory
	cbuffer_t buf;

	if (  const char *err = network_http_get( ANNOUNCE_SERVER, ANNOUNCE_LIST_URL, buf )  ) {
		dbg->error( "server_frame_t::update_serverlist", "could not download list: %s", err );
		return false;
	}

	// Parse listing into CSV_t object
	CSV_t csvdata( buf.get_str() );
	int ret;

	dbg->message( "server_frame_t::update_serverlist", "CSV_t: %s", csvdata.get_str() );

	// For each listing entry, determine if it matches the version supplied to this function
	do {
		// First field is display name of server
		cbuffer_t servername;
		ret = csvdata.get_next_field( servername );
		dbg->message( "server_frame_t::update_serverlist", "servername: %s", servername.get_str() );
		// Skip invalid lines
		if (  ret <= 0  ) { continue; }

		// Second field is dns name of server
		cbuffer_t serverdns;
		ret = csvdata.get_next_field( serverdns );
		dbg->message( "server_frame_t::update_serverlist", "serverdns: %s", serverdns.get_str() );
		if (  ret <= 0  ) { continue; }
		cbuffer_t serverdns2;
		// Strip default port
		if (  strcmp(serverdns.get_str() + strlen(serverdns.get_str()) - 6, ":13353") == 0  ) {
			dbg->message( "server_frame_t::update_serverlist", "stripping default port from entry %s", serverdns.get_str() );
			serverdns2.append( serverdns.get_str(), strlen( serverdns.get_str() ) - 6 );
			serverdns = serverdns2;
		}

		// Third field is server revision (use for filtering)
		cbuffer_t serverrevision;
		ret = csvdata.get_next_field( serverrevision );
		dbg->message( "server_frame_t::update_serverlist", "serverrevision: %s", serverrevision.get_str() );
		if (  ret <= 0  ) { continue; }
		uint32 serverrev = atol( serverrevision.get_str() );
		if (  revision != 0  &&  revision != serverrev  ) {
			// do not add mismatched servers
			dbg->warning( "server_frame_t::update_serverlist", "revision %i does not match our revision (%i), skipping", serverrev, revision );
			continue;
		}

		// Fourth field is server pakset (use for filtering)
		cbuffer_t serverpakset;
		ret = csvdata.get_next_field( serverpakset );
		dbg->message( "server_frame_t::update_serverlist", "serverpakset: %s", serverpakset.get_str() );
		if (  ret <= 0  ) { continue; }
		// now check pakset match
		if (  pakset != NULL  ) {
			if (!strstart(serverpakset.get_str(), pakset)) {
				dbg->warning( "server_frame_t::update_serverlist", "pakset '%s' does not match our pakset ('%s'), skipping", serverpakset.get_str(), pakset );
				continue;
			}
		}

		// Fifth field is server online/offline status (use for colour-coding)
		cbuffer_t serverstatus;
		ret = csvdata.get_next_field( serverstatus );
		dbg->message( "server_frame_t::update_serverlist", "serverstatus: %s", serverstatus.get_str() );

		uint32 status = 0;
		if (  ret > 0  ) {
			status = atol( serverstatus.get_str() );
		}

		// Only show offline servers if the checkbox is set
		if (  status == 1  ||  show_offline.pressed  ) {
			serverlist.append_element( new server_scrollitem_t( servername, serverdns, status, status == 1 ? COL_BLUE : COL_RED ) );
			dbg->message( "server_frame_t::update_serverlist", "Appended %s (%s) to list", servername.get_str(), serverdns.get_str() );
		}

	} while ( csvdata.next_line() );

	// Set no default selection
	serverlist.set_selection( -1 );

	return true;
}


bool server_frame_t::infowin_event (const event_t *ev)
{
	return gui_frame_t::infowin_event( ev );
}


bool server_frame_t::action_triggered (gui_action_creator_t *komp, value_t p)
{
	// Selection has changed
	if (  &serverlist == komp  ) {
		if (  p.i <= -1  ) {
			join.disable();
			gi = gameinfo_t(welt);
			update_info();
		}
		else {
			join.disable();
			if (  ((server_scrollitem_t*)serverlist.get_element( p.i ))->online()  ) {
				const char *err = network_gameinfo( ((server_scrollitem_t*)serverlist.get_element( p.i ))->get_dns(), &gi );
				if (  err == NULL  ) {
					serverlist.get_element( p.i )->set_color( COL_BLACK );
					update_info();
				}
				else {
					serverlist.get_element( p.i )->set_color( COL_RED );
					update_error( "Server did not respond!" );
				}
			}
			else {
				serverlist.get_element( p.i )->set_color( COL_RED );
				update_error( "Cannot connect to offline server!" );
			}
		}
	}
	else if (  &add == komp  ||  &addinput ==komp  ) {
		if (  newserver_name[0] != '\0'  ) {
			join.disable();

			dbg->warning("action_triggered()", "newserver_name: %s", newserver_name);

			const char *err = network_gameinfo( newserver_name, &gi );
			if (  err == NULL  ) {
				custom_valid = true;
				update_info();
			}
			else {
				custom_valid = false;
				join.disable();
				update_error( "Server did not respond!" );
			}
			serverlist.set_selection( -1 );
		}
	}
	else if (  &show_mismatched == komp  ) {
		show_mismatched.pressed ^= 1;
		serverlist.clear_elements();
		update_serverlist();
	}
	else if (  &show_offline == komp  ) {
		show_offline.pressed ^= 1;
		serverlist.clear_elements();
		update_serverlist();
	}
	else if (  &nick == komp  ) {
		char* nickname = nick.get_text();
		if (  umgebung_t::networkmode  ) {
			// Only try and change the nick with server if we're in network mode
			if (  umgebung_t::nickname != nickname  ) {
				umgebung_t::nickname = nickname;
				nwc_nick_t* nwc = new nwc_nick_t( nickname );
				network_send_server( nwc );
			}
		}
		else {
			umgebung_t::nickname = nickname;
		}
	}
	else if (  &join == komp  ) {
		char* nickname = nick.get_text();
		if (  strlen( nickname ) == 0  ) {
			// forbid joining?
		}
		umgebung_t::nickname = nickname;
		std::string filename = "net:";

		// Prefer serverlist entry if one is selected
		if (  serverlist.get_selection() >= 0  ) {
			filename += ((server_scrollitem_t*)serverlist.get_element(serverlist.get_selection()))->get_dns();
			destroy_win( this );
			welt->load( filename.c_str() );
		}
		// If we have a valid custom server entry, connect to that
		else if (  custom_valid  ) {
			filename += newserver_name;
			destroy_win( this );
			welt->load( filename.c_str() );
		}
		else {
			dbg->error( "server_frame_t::action_triggered()", "join pressed without valid selection or custom server entry" );
			join.disable();
		}
	}
	else if (  &find_mismatch == komp  ) {
		if (  gui_frame_t *info = win_get_magic(magic_pakset_info_t)  ) {
			top_win( info );
		}
		else {
			std::string msg;
			if (  serverlist.get_selection() >= 0  ) {
				network_compare_pakset_with_server( ((server_scrollitem_t*)serverlist.get_element(serverlist.get_selection()))->get_dns(), msg );
			}
			else if (  custom_valid  ) {
				network_compare_pakset_with_server( newserver_name, msg );
			}
			else {
				dbg->error( "server_frame_t::action_triggered()", "find_mismatch pressed without valid selection or custom server entry" );
				find_mismatch.disable();
			}
			if (  !msg.empty()  ) {
				help_frame_t *win = new help_frame_t();
				win->set_text( msg.c_str() );
				create_win( win, w_info, magic_pakset_info_t );
			}
		}
	}
	return true;
}


void server_frame_t::zeichnen (koord pos, koord gr)
{
	// update nickname if necessary
	if (  get_focus() != &nick  &&  umgebung_t::nickname != nick_buf  ) {
		tstrncpy( nick_buf, umgebung_t::nickname.c_str(), min( lengthof( nick_buf ), umgebung_t::nickname.length() + 1 ) );
	}

	gui_frame_t::zeichnen( pos, gr );

	sint16 pos_y = pos.y + D_TITLEBAR_HEIGHT;
	pos_y += D_MARGIN_TOP;

	if (  !umgebung_t::networkmode  ) {
		pos_y += LINESPACE;             // List info text
		pos_y += D_V_SPACE;             // padding
		pos_y += D_BUTTON_HEIGHT * 6;   // serverlist gui_scrolled_list_t
		pos_y += D_V_SPACE;             // padding
		pos_y += D_BUTTON_HEIGHT;       // show_mismatched + show_offline
		pos_y += D_V_SPACE;             // padding
		display_ddd_box_clip( pos.x + D_MARGIN_LEFT, pos_y, gr.x - D_MARGIN_LEFT - D_MARGIN_RIGHT, 0, MN_GREY0, MN_GREY4 );
		pos_y += D_V_SPACE;             // padding
		pos_y += LINESPACE;             // Manual connect info text
		pos_y += D_V_SPACE;             // padding
		pos_y += D_BUTTON_HEIGHT;       // add server button/textinput
		pos_y += D_V_SPACE;             // padding
		display_ddd_box_clip( pos.x + D_MARGIN_LEFT, pos_y, gr.x - D_MARGIN_LEFT - D_MARGIN_RIGHT, 0, MN_GREY0, MN_GREY4 );
		pos_y += D_V_SPACE;             // padding
	}

	pos_y += LINESPACE;     // revision + date
	pos_y += LINESPACE;     // pakset version

#if DEBUG>=4
	pos_y += LINESPACE;     // pakset_checksum
#endif

	pos_y += D_V_SPACE;     // padding
	display_ddd_box_clip( pos.x + D_MARGIN_LEFT, pos_y, gr.x - D_MARGIN_LEFT - D_MARGIN_RIGHT, 0, MN_GREY0, MN_GREY4 );
	pos_y += D_V_SPACE;     // padding

	const koord mapsize( gi.get_map()->get_width(), gi.get_map()->get_height() );

	// Map graphic (offset in 3D border by 1px)
	if (  display_map  ) {
		// 3D border around the map graphic
		display_ddd_box_clip( pos.x + D_MARGIN_LEFT, pos_y, mapsize.x + 2, mapsize.y + 2, MN_GREY0, MN_GREY4 );
		display_array_wh( pos.x + D_MARGIN_LEFT + 1, pos_y + 1, mapsize.x, mapsize.y, gi.get_map()->to_array() );
	}

	// Descriptive server text
	display_multiline_text( pos.x + D_MARGIN_LEFT + 1 + mapsize.x + 2 + D_H_SPACE, pos_y, buf, COL_BLACK );

	pos_y += LINESPACE * 8;   // Spacing for the multiline_text above

	pos_y += D_V_SPACE;
	display_ddd_box_clip( pos.x + D_MARGIN_LEFT, pos_y, gr.x - D_MARGIN_LEFT - D_MARGIN_RIGHT, 0, MN_GREY0, MN_GREY4 );
	pos_y += D_V_SPACE;
	pos_y += D_BUTTON_HEIGHT; // Nick entry

	// Buttons at bottom of dialog
	if (  !umgebung_t::networkmode  ) {
		pos_y += D_V_SPACE;
		display_ddd_box_clip( pos.x + D_MARGIN_LEFT, pos_y, gr.x - D_MARGIN_LEFT - D_MARGIN_RIGHT, 0, MN_GREY0, MN_GREY4 );
		pos_y += D_V_SPACE;

		// drawing twice, but otherwise it will not overlay image
		serverlist.zeichnen( pos + koord( 0, 16 ) );
	}
}
