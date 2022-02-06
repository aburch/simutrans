/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../world/simworld.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "simwin.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer.h"
#include "../utils/csv.h"
#include "../simversion.h"


#include "../dataobj/translator.h"
#include "../network/network.h"
#include "../network/network_file_transfer.h"
#include "../network/network_cmd_ingame.h"
#include "../network/network_cmp_pakset.h"
#include "../dataobj/environment.h"
#include "../network/pakset_info.h"
#include "../player/simplay.h"
#include "server_frame.h"
#include "messagebox.h"
#include "help_frame.h"
#include "components/gui_divider.h"

static char nick_buf[256];
char server_frame_t::newserver_name[2048] = "";

class gui_minimap_t : public gui_component_t
{
	gameinfo_t *gi;
public:
	gui_minimap_t() { gi = NULL; }

	void set_gameinfo(gameinfo_t *gi) { this->gi = gi; }

	void draw( scr_coord offset ) OVERRIDE
	{
		if (gi) {
			scr_coord p = get_pos() + offset;
			scr_size mapsize( gi->get_map()->get_width(), gi->get_map()->get_height() );
			// 3D border around the map graphic
			display_ddd_box_clip_rgb(p.x, p.y, mapsize.w + 2, mapsize.h + 2, color_idx_to_rgb(MN_GREY0), color_idx_to_rgb(MN_GREY4) );
			display_array_wh( p.x + 1, p.y + 1, mapsize.w, mapsize.h, gi->get_map()->to_array() );
		}
	}

	scr_size get_min_size() const OVERRIDE
	{
		return gi ? scr_size(gi->get_map()->get_width()+2, gi->get_map()->get_height()+2) :  scr_size(0,0);
	}

	scr_size get_max_size() const OVERRIDE { return get_min_size(); }
};


server_frame_t::server_frame_t() :
	gui_frame_t( translator::translate("Game info") ),
	gi(welt),
	custom_valid(false),
	serverlist( gui_scrolled_list_t::listskin, gui_scrolled_list_t::scrollitem_t::compare ),
	game_text(&buf)
{
	map = new gui_minimap_t();
	update_info();
	map->set_gameinfo(&gi);

	set_table_layout(3,0);

	// When in network mode, display only local map info (and nickname changer)
	// When not in network mode, display server picker
	if (  !env_t::networkmode  ) {
		new_component<gui_label_t>("Select a server to join:" );
		new_component<gui_fill_t>();
		join.init(button_t::roundbox, "join game");
		join.disable();
		join.add_listener(this);
		add_component(&join);

		// Server listing
		serverlist.set_selection( 0 );
		add_component( &serverlist, 3 );
		serverlist.add_listener(this);

		// Show mismatched checkbox
		show_mismatched.init( button_t::square_state, "Show mismatched");
		show_mismatched.set_tooltip( "Show servers where game version or pakset does not match your client" );
		show_mismatched.add_listener( this );
		add_component( &show_mismatched );

		// Show offline checkbox
		show_offline.init( button_t::square_state, "Show offline");
		show_offline.set_tooltip( "Show servers that are offline" );
		show_offline.add_listener( this );
		add_component( &show_offline, 2 );

		new_component_span<gui_divider_t>(3);

		new_component_span<gui_label_t>("Or enter a server manually:", 3);

		// Add server input/button
		addinput.set_text( newserver_name, sizeof( newserver_name ) );
		addinput.add_listener( this );
		add_component( &addinput, 2 );

		add.init( button_t::roundbox, "Query server");
		add.add_listener( this );
		add_component( &add );

		new_component_span<gui_divider_t>(3);
	}

	if(!env_t::networkmode) {
		add_component(&revision, 2);

		find_mismatch.init(button_t::roundbox, "find mismatch");
		find_mismatch.add_listener(this);
		add_component(&find_mismatch);
	}
	else {
		add_component(&revision, 3);
	}
	show_mismatched.pressed = gi.get_game_engine_revision() == 0;

	add_component( &pak_version, 3 );
#if MSG_LEVEL>=4
	add_component( &pakset_checksum );
#endif
	new_component_span<gui_divider_t>(3);

	add_component(map);

	add_table(1,0)->set_force_equal_columns(true);
	{
		add_table(3,1);
		{
			add_component(&label_size);
			new_component<gui_fill_t>();
			add_component( &date );
		}
		end_table();

		add_component(&game_text);
	}
	end_table();
	new_component<gui_fill_t>();

	new_component_span<gui_divider_t>(3);

	new_component<gui_label_t>("Nickname:" );
	add_component( &nick, 2 );
	nick.add_listener(this);
	nick.set_text( nick_buf, lengthof( nick_buf ) );
	tstrncpy( nick_buf, env_t::nickname.c_str(), min( lengthof( nick_buf ), env_t::nickname.length() + 1 ) );

	if (  !env_t::networkmode  ) {
		// only update serverlist, when not already in network mode
		// otherwise desync to current game may happen
		update_serverlist();
	}

	set_resizemode( diagonal_resize );
	reset_min_windowsize();
}


void server_frame_t::update_error (const char* errortext)
{
	buf.clear();
	buf.printf("%s", errortext );

	map->set_gameinfo(NULL);
	date.buf().clear();
	label_size.buf().clear();
	revision.buf().clear();
	pak_version.set_text( "" );
	join.disable();
	find_mismatch.disable();
}


PIXVAL server_frame_t::update_info()
{
	map->set_gameinfo(&gi);

	// Compare with current world to determine status
	gameinfo_t current( welt );
	bool engine_match = false;
	bool pakset_match = false;

	// Zero means do not know => assume match
	if(  current.get_game_engine_revision() == 0  ||  gi.get_game_engine_revision() == 0  ||  current.get_game_engine_revision() == gi.get_game_engine_revision()  ) {
		engine_match = true;
	}

	if(  gi.get_pakset_checksum() == current.get_pakset_checksum()  ) {
		pakset_match = true;
		find_mismatch.disable();
	}
	else {
		find_mismatch.enable();
	}

	// State of join button
	if(  (serverlist.get_selection() >= 0  ||  custom_valid)  &&  pakset_match  &&  engine_match  ) {
		join.enable();
	}
	else {
		join.disable();
	}

	label_size.buf().printf("%ux%u\n", gi.get_size_x(), gi.get_size_y());
	label_size.update();

	// Update all text fields
	char temp[32];
	buf.clear();
	if (  gi.get_clients() != 255  ) {
		uint8 player = 0, locked = 0;
		for (  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
			if (  gi.get_player_type(i)&~player_t::PASSWORD_PROTECTED  ) {
				player ++;
				if (  gi.get_player_type(i)&player_t::PASSWORD_PROTECTED  ) {
					locked ++;
				}
			}
		}
		buf.printf( translator::translate("%u Player (%u locked)\n"), player, locked );
		buf.printf( translator::translate("%u Client(s)\n"), (unsigned)gi.get_clients() );
	}
	buf.printf( "%s %u\n", translator::translate("Towns"), gi.get_city_count() );
	number_to_string( temp, gi.get_citizen_count(), 0 );
	buf.printf( "%s %s\n", translator::translate("citicens"), temp );
	buf.printf( "%s %u\n", translator::translate("Factories"), gi.get_industries() );
	buf.printf( "%s %u\n", translator::translate("Convoys"), gi.get_convoi_count() );
	buf.printf( "%s %u\n", translator::translate("Stops"), gi.get_halt_count() );

	revision.buf().printf( "%s %u", translator::translate( "Revision:" ), gi.get_game_engine_revision() );
	revision.set_color( engine_match ? SYSCOL_TEXT : MONEY_MINUS );
	revision.update();

	pak_version.set_text( gi.get_pak_name() );
	pak_version.set_color( pakset_match ? SYSCOL_TEXT : SYSCOL_OBSOLETE );

#if MSG_LEVEL>=4
	pakset_checksum.buf().printf("%s %s",translator::translate( "Pakset checksum:" ), gi.get_pakset_checksum().get_str(8));
	pakset_checksum.update();
	pakset_checksum.set_color( pakset_match ? SYSCOL_TEXT : SYSCOL_TEXT_STRONG );
#endif

	date.buf().printf("%s", translator::get_date(gi.get_current_year(), gi.get_current_month()));
	date.update();
	set_dirty();
	resize(scr_size(0,0));

	return pakset_match  &&  engine_match ? SYSCOL_TEXT : SYSCOL_OBSOLETE;
}


bool server_frame_t::update_serverlist ()
{
	// Based on current dialog settings, should we show mismatched servers or not
	uint revision = 0;
	const char* pakset = NULL;
	gameinfo_t current( welt );

	if(  !show_mismatched.pressed  ) {
		revision = current.get_game_engine_revision();
		pakset   = current.get_pak_name();
	}

	// Download game listing from listings server into memory
	cbuffer_t buf;

	if(  const char *err = network_http_get( ANNOUNCE_SERVER, ANNOUNCE_LIST_URL, buf )  ) {
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
		if(  ret <= 0  ) {
			continue;
		}

		// Second field is dns name of server
		cbuffer_t serverdns;
		ret = csvdata.get_next_field( serverdns );
		dbg->message( "server_frame_t::update_serverlist", "serverdns: %s", serverdns.get_str() );
		if(  ret <= 0  ) {
			continue;
		}

		cbuffer_t serverdns2;
		// Strip default port
		if(  strcmp(serverdns.get_str() + serverdns.len() - 6, ":13353") == 0  ) {
			dbg->message( "server_frame_t::update_serverlist", "stripping default port from entry %s", serverdns.get_str() );
			serverdns2.append( serverdns.get_str(), strlen( serverdns.get_str() ) - 6 );
			serverdns = serverdns2;
			serverdns2.clear();
		}

		// Third field is server revision (use for filtering)
		cbuffer_t serverrevision;
		ret = csvdata.get_next_field( serverrevision );
		dbg->message( "server_frame_t::update_serverlist", "serverrevision: %s", serverrevision.get_str() );
		if(  ret <= 0  ) {
			continue;
		}

		uint32 serverrev = atol( serverrevision.get_str() );
		if(  revision != 0  &&  revision != serverrev  ) {
			// do not add mismatched servers
			dbg->warning( "server_frame_t::update_serverlist", "revision %i does not match our revision (%i), skipping", serverrev, revision );
			continue;
		}

		// Fourth field is server pakset (use for filtering)
		cbuffer_t serverpakset;
		ret = csvdata.get_next_field( serverpakset );
		dbg->message( "server_frame_t::update_serverlist", "serverpakset: %s", serverpakset.get_str() );
		if(  ret <= 0  ) {
			continue;
		}

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
		if(  ret > 0  ) {
			status = atol( serverstatus.get_str() );

			// sixth field on new list the alt dns, if there is one
			ret = csvdata.get_next_field(serverdns2);
			dbg->message("server_frame_t::update_serverlist", "altdns: %s", serverdns2.get_str());
		}

		// Only show offline servers if the checkbox is set
		if(  status == 1  ||  show_offline.pressed  ) {
			PIXVAL color = status == 1 ? SYSCOL_EMPTY : MONEY_MINUS;
			if(  pakset  &&  !strstart( serverpakset.get_str(), pakset )  ) {
				color = SYSCOL_OBSOLETE;
			}
			serverlist.new_component<server_scrollitem_t>( servername, serverdns, serverdns2, status, color );
			dbg->message( "server_frame_t::update_serverlist", "Appended %s (%s) to list", servername.get_str(), serverdns.get_str() );
		}

	} while( csvdata.next_line() );

	// Set no default selection
	serverlist.set_selection( -1 );
	serverlist.set_size(serverlist.get_size());

	set_dirty();
	resize(scr_size(0, 0));

	return true;
}


bool server_frame_t::infowin_event (const event_t *ev)
{
	return gui_frame_t::infowin_event( ev );
}


bool server_frame_t::action_triggered (gui_action_creator_t *comp, value_t p)
{
	// Selection has changed
	if(  &serverlist == comp  ) {
		if(  p.i <= -1  ) {
			join.disable();
			gi = gameinfo_t(welt);
			update_info();
		}
		else {
			join.disable();
			server_scrollitem_t *item = (server_scrollitem_t*)serverlist.get_element( p.i );
			if(  item->online()  ) {
				display_show_load_pointer(1);
				const char *err = network_gameinfo( ((server_scrollitem_t*)serverlist.get_element( p.i ))->get_dns(), &gi );
				if(  err  &&  *((server_scrollitem_t*)serverlist.get_element(p.i))->get_altdns()  ) {
					item->set_color(MONEY_MINUS);
					update_error(err);
					err = network_gameinfo(((server_scrollitem_t*)serverlist.get_element(p.i))->get_altdns(), &gi);
				}
				if(  err == NULL  ) {
					item->set_color( update_info() );
				}
				else {
					item->set_color( MONEY_MINUS );
					update_error( err );
				}
				display_show_load_pointer(0);
			}
			else {
				item->set_color( MONEY_MINUS );
				update_error( "Cannot connect to offline server!" );
			}
		}
	}
	else if (  &add == comp  ||  &addinput ==comp  ) {
		if (  newserver_name[0] != '\0'  ) {
			join.disable();

			dbg->warning("action_triggered()", "newserver_name: %s", newserver_name);

			display_show_load_pointer(1);
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
			display_show_load_pointer(0);
			serverlist.set_selection( -1 );
		}
	}
	else if (  &show_mismatched == comp  ) {
		show_mismatched.pressed ^= 1;
		serverlist.clear_elements();
		update_serverlist();
	}
	else if (  &show_offline == comp  ) {
		show_offline.pressed ^= 1;
		serverlist.clear_elements();
		update_serverlist();
	}
	else if (  &nick == comp  ) {
		char* nickname = nick.get_text();
		if (  env_t::networkmode  ) {
			// Only try and change the nick with server if we're in network mode
			if (  env_t::nickname != nickname  ) {
				env_t::nickname = nickname;
				nwc_nick_t* nwc = new nwc_nick_t( nickname );
				network_send_server( nwc );
			}
		}
		else {
			env_t::nickname = nickname;
		}
	}
	else if (  &join == comp  ) {
		char* nickname = nick.get_text();
		if (  strlen( nickname ) == 0  ) {
			// forbid joining?
		}
		env_t::nickname = nickname;
		std::string filename = "net:";

		// Prefer serverlist entry if one is selected
		if (  serverlist.get_selection() >= 0  ) {
			display_show_load_pointer(1);
			filename += ((server_scrollitem_t*)serverlist.get_selected_item())->get_dns();
			destroy_win( this );
			welt->load( filename.c_str() );
			display_show_load_pointer(0);
		}
		// If we have a valid custom server entry, connect to that
		else if (  custom_valid  ) {
			display_show_load_pointer(1);
			filename += newserver_name;
			destroy_win( this );
			welt->load( filename.c_str() );
			display_show_load_pointer(0);
		}
		else {
			dbg->error( "server_frame_t::action_triggered()", "join pressed without valid selection or custom server entry" );
			join.disable();
		}
	}
	else if (  &find_mismatch == comp  ) {
		if (  gui_frame_t *info = win_get_magic(magic_pakset_info_t)  ) {
			top_win( info );
		}
		else {
			std::string msg;
			if (  serverlist.get_selection() >= 0  ) {
				network_compare_pakset_with_server( ((server_scrollitem_t*)serverlist.get_selected_item())->get_dns(), msg );
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


void server_frame_t::draw (scr_coord pos, scr_size size)
{
	// update nickname if necessary
	if (  get_focus() != &nick  &&  env_t::nickname != nick_buf  ) {
		tstrncpy( nick_buf, env_t::nickname.c_str(), min( lengthof( nick_buf ), env_t::nickname.length() + 1 ) );
	}

	gui_frame_t::draw( pos, size );
}
