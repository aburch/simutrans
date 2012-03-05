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
#include "../dataobj/network_cmp_pakset.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/pakset_info.h"
#include "../player/simplay.h"
#include "server_frame.h"
#include "messagebox.h"
#include "help_frame.h"



server_frame_t::server_frame_t(karte_t* w) :
	gui_frame_t( translator::translate("Game info") ),
	welt(w),
	gi(welt)
{
	update_info();

	sint16 pos_y = 4;

	// only show serverlist, when not already in network mode
	if(  !umgebung_t::networkmode  ) {
		serverlist.set_pos( koord(2,pos_y) );
		serverlist.set_groesse( koord(236,D_BUTTON_HEIGHT) );
		serverlist.add_listener(this);
		serverlist.set_selection( 0 );
		add_komponente( &serverlist );
		pos_y += 6+D_BUTTON_HEIGHT;

		add.init( button_t::box, "add server", koord( 4, pos_y ), koord( 112, D_BUTTON_HEIGHT) );
		add.add_listener(this);
		add_komponente( &add );

		if(  !show_all_rev.pressed  ) {
			show_all_rev.init( button_t::square_state, "Show all", koord( 124, pos_y+2 ), koord( 112, D_BUTTON_HEIGHT) );
			show_all_rev.set_tooltip( "Show even servers with wrong version or pakset" );
			show_all_rev.add_listener(this);
			add_komponente( &show_all_rev );
		}
		pos_y += D_BUTTON_HEIGHT+8;
	}

	revision.set_pos( koord( 4, pos_y ) );
	add_komponente( &revision );
	show_all_rev.pressed = gi.get_game_engine_revision()==0;

	pos_y += LINESPACE;
	pak_version.set_pos( koord( 4, pos_y ) );
	add_komponente( &pak_version );

#if DEBUG>=4
	pos_y += LINESPACE;
	pakset_checksum.set_pos( koord( 4, pos_y ) );
	add_komponente( &pakset_checksum );
#endif

	pos_y += LINESPACE+8;
	date.set_pos( koord( 240-4, pos_y ) );
	date.set_align( gui_label_t::right );
	add_komponente( &date );

	pos_y += LINESPACE*8+8;

	if(  !umgebung_t::networkmode  ) {
		find_mismatch.init( button_t::box, "find mismatch", koord( 4, pos_y ), koord( 112, D_BUTTON_HEIGHT) );
		find_mismatch.add_listener(this);
		add_komponente( &find_mismatch );

		join.init( button_t::box, "join game", koord( 124, pos_y ), koord( 112, D_BUTTON_HEIGHT) );
		join.add_listener(this);
		add_komponente( &join );

		// only update serverlist, when not already in network mode
		// otherwise desync to current game may happen
		if (  show_all_rev.pressed  ) {
			update_serverlist( 0, NULL );
		}
		else {
			update_serverlist( gi.get_game_engine_revision(), gi.get_pak_name() );
		}

		pos_y += 6+D_BUTTON_HEIGHT;
	}
	pos_y += 16;

	set_fenstergroesse( koord( 240, pos_y ) );
	set_min_windowsize( koord( 240, pos_y ) );
	set_resizemode( no_resize );
}


void server_frame_t::update_info()
{
	char temp[32];

	buf.clear();
	buf.printf( "%ux%u\n", gi.get_groesse_x(), gi.get_groesse_y() );
	if(  gi.get_clients()!=255  ) {
		uint8 player=0, locked = 0;
		for(  uint8 i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
			if(  gi.get_player_type(i)&~spieler_t::PASSWORD_PROTECTED  ) {
				player ++;
				if(  gi.get_player_type(i)&spieler_t::PASSWORD_PROTECTED  ) {
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
#if DEBUG==4
	pakset_checksum_buf.clear();
#endif
	find_mismatch.disable();
	if(  serverlist.get_selection()>=0  ) {
		// need to compare with our world now
		gameinfo_t current(welt);

		revision_buf.printf( "%s %u", translator::translate( "Revision:" ), gi.get_game_engine_revision() );
		revision.set_text( revision_buf );
		// zero means do not know => assume match
		revision.set_color( current.get_game_engine_revision()==0  ||  gi.get_game_engine_revision()==0  ||  current.get_game_engine_revision()==gi.get_game_engine_revision() ? COL_BLACK : COL_RED );

		pak_version.set_text( gi.get_pak_name() );
		// this will be using CRC when implemented
		pak_version.set_color( gi.get_pakset_checksum() == current.get_pakset_checksum()  ? COL_BLACK : COL_RED );

#if DEBUG>=4
		pakset_checksum_buf.printf("%s %s",translator::translate( "Pakset checksum:" ), gi.get_pakset_checksum().get_str(8));
		pakset_checksum.set_color(gi.get_pakset_checksum() == current.get_pakset_checksum() ? COL_BLACK : COL_RED );
		pakset_checksum.set_text(pakset_checksum_buf);
#endif
		if(  revision.get_color()==COL_BLACK  &&  pak_version.get_color()==COL_BLACK  &&  current.get_pakset_checksum()==gi.get_pakset_checksum()  ) {
			join.enable();
		}
		else {
			join.disable();
			if(  !(gi.get_pakset_checksum()==current.get_pakset_checksum())  ) {
				find_mismatch.enable();
			}
		}
	}
	else {
		revision_buf.printf( "%s %u", translator::translate( "Revision:" ), gi.get_game_engine_revision() );
		revision.set_text( revision_buf );
		revision.set_color( COL_BLACK );
		pak_version.set_text( gi.get_pak_name() );
		pak_version.set_color( COL_BLACK );
#if DEBUG>=4
		pakset_checksum_buf.printf("%s %s",translator::translate( "Pakset checksum:" ), gi.get_pakset_checksum().get_str(8));
		pakset_checksum.set_text(pakset_checksum_buf);
#endif
		join.disable();
	}

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
}


bool server_frame_t::update_serverlist ( uint revision, const char *pakset )
{
	dbg->message( "server_frame_t::update_serverlist", "called with revision: %i, pakset: %s", revision, pakset );

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

		// Add matching server entry to list
		//char serverentry[4096];
		// const char *format = "%s (%s)";
		// sprintf(serverentry, format, servername, serverdns);
		//const char *format = "%s";
		//sprintf( serverentry, format, serverdns );

		// now add entry

		// TODO - Need to decouple the text which is displayed in the listing box from the actual DNS/IP entry which is used to connect to the server in question

		serverlist.append_element( new gui_scrolled_list_t::var_text_scrollitem_t( serverdns.get_str(), COL_BLUE ) );
		dbg->message( "server_frame_t::update_serverlist", "Appended %s to list", serverdns.get_str() );
	} while ( csvdata.next_line() );

	// Set no default selection
	serverlist.set_selection( -1 );

	return true;
}


bool server_frame_t::infowin_event(const event_t *ev)
{
	bool swallowed = gui_frame_t::infowin_event( ev );
	if(  ev->ev_class == EVENT_KEYBOARD  &&  ev->ev_code == SIM_KEY_ENTER  ) {
		action_triggered( &serverlist, value_t(serverlist.get_selection()) );
	}
	return swallowed;
}


bool server_frame_t::action_triggered( gui_action_creator_t *komp, value_t p )
{
	if(  &serverlist == komp  ) {
		if(  p.i<=-1  ) {
			join.disable();
			gi = gameinfo_t(welt);
			update_info();
		}
		else {
			join.disable();
			const char *err = network_gameinfo( serverlist.get_element(p.i)->get_text(), &gi );
			if(  err==NULL  ) {
				serverlist.get_element(p.i)->set_color( COL_BLACK );
				update_info();
			}
			else {
				serverlist.get_element(p.i)->set_color( COL_RED );
				buf.clear();
				date.set_text( "Server did not respond!" );
				revision.set_text( "" );
				pak_version.set_text( "" );
			}
		}
		set_dirty();
	}
	else if(  &add == komp  ) {
		serverlist.append_element( new gui_scrolled_list_t::var_text_scrollitem_t( "Enter address", COL_BLUE ) );
		serverlist.set_selection( serverlist.count_elements()-1 );
	}
	else if(  &show_all_rev == komp  ) {
		show_all_rev.pressed ^= 1;
		if ( show_all_rev.pressed ) {
			update_serverlist( 0, NULL );
		} else {
			update_serverlist( gi.get_game_engine_revision(), gi.get_pak_name() );
		}
	}
	else if(  &join == komp  ) {
		if(  serverlist.get_selection()==-1  ) {
			dbg->error( "server_frame_t::action_triggered()", "join pressed without valid selection" );
			join.disable();
		}
		else {
			std::string filename = "net:";
			filename += serverlist.get_element(serverlist.get_selection())->get_text();
			destroy_win(this);
			welt->laden(filename.c_str());
		}
	}
	else if(  &find_mismatch == komp  ) {
		if (gui_frame_t *info = win_get_magic(magic_pakset_info_t)) {
			top_win(info);
		}
		else {
			std::string msg;
			network_compare_pakset_with_server(serverlist.get_element(serverlist.get_selection())->get_text(), msg);
			if(  !msg.empty()  ) {
				help_frame_t *win = new help_frame_t();
				win->set_text(msg.c_str());
				create_win(win, w_info, magic_pakset_info_t);
			}
		}
	}
	return true;
}


void server_frame_t::zeichnen(koord pos, koord gr)
{
	gui_frame_t::zeichnen( pos, gr );

	sint16 pos_y = pos.y+16;
	if(  !umgebung_t::networkmode  ) {
		pos_y += 10+D_BUTTON_HEIGHT*2+4;
		display_ddd_box_clip( pos.x+4, pos_y, 240-8, 0, MN_GREY0, MN_GREY4);
	}

#if DEBUG>=4
	pos_y += LINESPACE;
#endif
	pos_y += LINESPACE*2+8;
	display_ddd_box_clip( pos.x+4, pos_y, 240-8, 0, MN_GREY0, MN_GREY4);

	pos_y += 4;
	display_multiline_text( pos.x+4, pos_y, buf, COL_BLACK );

	pos_y += LINESPACE*8;
	const koord mapsize( gi.get_map()->get_width(), gi.get_map()->get_height() );
	display_ddd_box_clip( pos.x+240-7-mapsize.x, pos_y-mapsize.y-3, mapsize.x+2, mapsize.y+2, MN_GREY0,MN_GREY4);
	display_array_wh( pos.x+240-6-mapsize.x, pos_y-mapsize.y-2, mapsize.x, mapsize.y, gi.get_map()->to_array() );

	if(  !umgebung_t::networkmode  ) {
		pos_y += 4;
		display_ddd_box_clip( pos.x+4, pos_y, 240-8, 0, MN_GREY0, MN_GREY4);

		// drawing twice, but otherwise it will not overlay image
		serverlist.zeichnen( pos+koord(0,16) );
	}
}
