#include "network_cmd_ingame.h"
#include "network.h"
#include "network_file_transfer.h"
#include "network_packet.h"
#include "network_socket_list.h"
#include "network_cmp_pakset.h"
#include "network_cmd_scenario.h"

#include "loadsave.h"
#include "gameinfo.h"
#include "scenario.h"
#include "../simtools.h"
#include "../simmenu.h"
#include "../simsys.h"
#include "../simversion.h"
#include "../simwin.h"
#include "../simmesg.h"
#include "../dataobj/umgebung.h"
#include "../player/simplay.h"
#include "../gui/player_frame_t.h"
#include "../utils/cbuffer_t.h"
#include "../utils/csv.h"


network_command_t* network_command_t::read_from_packet(packet_t *p)
{
	// check data
	if (p==NULL  ||  p->has_failed()  ||  !p->check_version()) {
		delete p;
		dbg->warning("network_command_t::read_from_packet", "error in packet");
		return NULL;
	}
	network_command_t* nwc = NULL;
	switch (p->get_id()) {
		case NWC_GAMEINFO:    nwc = new nwc_gameinfo_t(); break;
		case NWC_NICK:	      nwc = new nwc_nick_t(); break;
		case NWC_CHAT:	      nwc = new nwc_chat_t(); break;
		case NWC_JOIN:	      nwc = new nwc_join_t(); break;
		case NWC_SYNC:        nwc = new nwc_sync_t(); break;
		case NWC_GAME:        nwc = new nwc_game_t(); break;
		case NWC_READY:       nwc = new nwc_ready_t(); break;
		case NWC_TOOL:        nwc = new nwc_tool_t(); break;
		case NWC_CHECK:       nwc = new nwc_check_t(); break;
		case NWC_PAKSETINFO:  nwc = new nwc_pakset_info_t(); break;
		case NWC_SERVICE:     nwc = new nwc_service_t(); break;
		case NWC_AUTH_PLAYER: nwc = new nwc_auth_player_t(); break;
		case NWC_CHG_PLAYER:  nwc = new nwc_chg_player_t(); break;
		case NWC_SCENARIO:    nwc = new nwc_scenario_t(); break;
		case NWC_SCENARIO_RULES:
		                      nwc = new nwc_scenario_rules_t(); break;
		default:
			dbg->warning("network_command_t::read_from_socket", "received unknown packet id %d", p->get_id());
	}
	if (nwc) {
		if (!nwc->receive(p) ||  p->has_failed()) {
			dbg->warning("network_command_t::read_from_packet", "error while reading cmd from packet");
			delete nwc;
			nwc = NULL;
		}
	}
	return nwc;
}


void nwc_gameinfo_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_long(len);

}


// will send the gameinfo to the client
bool nwc_gameinfo_t::execute(karte_t *welt)
{
	if (umgebung_t::server) {
		dbg->message("nwc_gameinfo_t::execute", "");
		// TODO: check whether we can send a file
		nwc_gameinfo_t nwgi;
		// init the rest of the packet
		SOCKET s = packet->get_sender();
		loadsave_t fd;
		if(  fd.wr_open( "serverinfo.sve", loadsave_t::xml_bzip2, "info", SERVER_SAVEGAME_VER_NR )  ) {
			gameinfo_t gi(welt);
			gi.rdwr( &fd );
			fd.close();
			// get gameinfo size
			FILE *fh = fopen( "serverinfo.sve", "rb" );
			fseek( fh, 0, SEEK_END );
			nwgi.len = ftell( fh );
			rewind( fh );
//			nwj.client_id = network_get_client_id(s);
			nwgi.rdwr();
			if ( nwgi.send( s ) ) {
				// send gameinfo
				while(  !feof(fh)  ) {
					char buffer[1024];
					int bytes_read = (int)fread( buffer, 1, sizeof(buffer), fh );
					uint16 dummy;
					if(  !network_send_data(s,buffer,bytes_read,dummy,250)) {
						dbg->warning( "nwc_gameinfo_t::execute", "Client closed connection during transfer" );
						break;
					}
				}
			}
			else {
				dbg->warning( "nwc_gameinfo_t::execute", "send of NWC_GAMEINFO failed" );
			}
			fclose( fh );
			remove( "serverinfo.sve" );
		}
		socket_list_t::remove_client( s );
	}
	else {
		len = 0;
	}
	return true;
}


void nwc_nick_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_str(nickname);
}


/**
 * if server: checks whether nickname is taken and generates default nick
 */
bool nwc_nick_t::execute(karte_t *welt)
{
	if(umgebung_t::server) {
		uint32 client_id = socket_list_t::get_client_id(packet->get_sender());

		if(nickname==NULL) {
			goto generate_default_nick;
		}

		// check for same nick
		for(uint32 i = 0; i<socket_list_t::get_count(); i++) {
			socket_info_t& info = socket_list_t::get_client(i);
			if ( (info.state == socket_info_t::playing  ||  i==0)
				&&  i != client_id
				&&  (nickname == info.nickname.c_str()  ||  nickname == "Admin")  )
			{
				goto generate_default_nick;
			}
		}
		if (id == NWC_NICK) {
			// do not call this tool if called by nwc_join_t::execute
			nwc_nick_t::server_tools(welt, client_id, CHANGE_NICK, nickname);
		}
		return true;

generate_default_nick:
		// nick exists already
		// generate default nick
		cbuffer_t buf;
		buf.printf("Client#%d", client_id);
		nickname = (const char*)buf;
		return true;
	}
	else {
		umgebung_t::nickname = nickname!=NULL ? nickname.c_str() : "(null)";
	}
	return true;
}


void nwc_nick_t::server_tools(karte_t *welt, uint32 client_id, uint8 what, const char* nick)
{
	if (!socket_list_t::is_valid_client_id(client_id)) {
		return;
	}
	socket_info_t &info = socket_list_t::get_client(client_id);

	cbuffer_t buf;
	switch(what) {
		case WELCOME: {
			// welcome message
			buf.printf(translator::translate("Welcome, %s!", welt->get_settings().get_name_language_id()),
				   info.nickname.c_str());

			// Log chat message - please don't change order of fields
			CSV_t csv;
			csv.add_field( "connect" );
			csv.add_field( client_id );
			csv.add_field( info.address.get_str() );
			csv.add_field( info.nickname.c_str() );
			dbg->warning( "__ChatLog__", "%s", csv.get_str() );

			break;
		}
		case CHANGE_NICK: {
			if (nick==NULL  ||  info.nickname == nick) {
				return;
			}
			// change nickname
			buf.printf(translator::translate("%s now known as %s.", welt->get_settings().get_name_language_id()),
				   info.nickname.c_str(), nick);

			// Log chat message - please don't change order of fields
			CSV_t csv;
			csv.add_field( "namechange" );
			csv.add_field( client_id );
			csv.add_field( info.address.get_str() );
			csv.add_field( info.nickname.c_str() );
			csv.add_field( nick );
			dbg->warning( "__ChatLog__", "%s", csv.get_str() );

			info.nickname = nick;

			if (client_id > 0) {
				// send new nickname back to client
				nwc_nick_t nwc(nick);
				nwc.send(info.socket);
			}
			else {
				// human at server
				umgebung_t::nickname = nick;
			}
			break;
		}
		case FAREWELL: {
			buf.printf(translator::translate("%s has left.", welt->get_settings().get_name_language_id()),
				   info.nickname.c_str());

			// Log chat message - please don't change order of fields
			CSV_t csv;
			csv.add_field( "disconnect" );
			csv.add_field( client_id );
			csv.add_field( info.address.get_str() );
			csv.add_field( info.nickname.c_str() );
			dbg->warning( "__ChatLog__", "%s", csv.get_str() );

			break;
		}
		default: return;
	}
	werkzeug_t *w = create_tool( WKZ_ADD_MESSAGE_TOOL | SIMPLE_TOOL );
	w->set_default_param( buf );
	// queue tool for network
	nwc_tool_t *nwc = new nwc_tool_t(NULL, w, koord3d::invalid, 0, welt->get_map_counter(), true);
	network_send_server(nwc);
	// since init always returns false, it is safe to delete immediately
	delete w;
}


void nwc_chat_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_str( message );
	packet->rdwr_byte( player_nr );
	packet->rdwr_str( clientname );
	packet->rdwr_str( destination );

	dbg->warning("nwc_chat_t::rdwr", "rdwr message=%s plnr=%d clientname=%s destination=%s", message.c_str(), player_nr, clientname.c_str(), destination.c_str());
}


void nwc_chat_t::add_message (karte_t* welt) const
{
	dbg->warning("nwc_chat_t::add_message", "");
	cbuffer_t buf;  // Output which will be printed to chat window

	PLAYER_COLOR_VAL color = player_nr < PLAYER_UNOWNED  ?  welt->get_spieler( player_nr )->get_player_color1()  :  COL_WHITE;
	uint16 flag = message_t::chat;

	if (  destination == NULL  ) {
		if (  player_nr < PLAYER_UNOWNED  ) {
			buf.printf( "%s <%s>: %s", clientname.c_str(), welt->get_spieler( player_nr )->get_name(), message.c_str() );
		}
		else {
			buf.printf( "%s: %s", clientname.c_str(), message.c_str() );
		}
	}
	else {
		// Whisper, do not store message in savegame
		flag |= message_t::local_flag;
		if (  player_nr < PLAYER_UNOWNED  ) {
			buf.printf( "%s <%s> --> %s: %s", clientname.c_str(), welt->get_spieler( player_nr )->get_name(), destination.c_str(), message.c_str() );
		}
		else {
			buf.printf( "%s --> %s: %s", clientname.c_str(), destination.c_str(), message.c_str() );
		}
	}
	welt->get_message()->add_message( buf.get_str(), koord::invalid, flag, color, IMG_LEER );
}


bool nwc_chat_t::execute (karte_t* welt)
{
	if (  message == NULL  ) {
		dbg->warning("nwc_chat_t::execute", "null message");
		return true;
	}


	// Relay message to all listening clients
	if (  umgebung_t::server  ) {
		uint32 client_id = socket_list_t::get_client_id( packet->get_sender() );

		dbg->warning("nwc_chat_t::execute", "server, client id: %d", client_id);

		// Clients can only send messages as companies they have unlocked
		if (  player_nr < PLAYER_UNOWNED  &&  !socket_list_t::get_client( client_id ).is_player_unlocked( player_nr )  ) {
			dbg->warning("nwc_chat_t::execute", "attempt to send message as locked company by client %d, redirecting to PLAYER_UNOWNED", client_id);
			player_nr = PLAYER_UNOWNED;
		}

		// Otherwise forward message as appropriate
		socket_info_t &info = socket_list_t::get_client( client_id );

		nwc_chat_t* nwchat = new nwc_chat_t( message, player_nr, info.nickname.c_str(), destination );

		if (  destination == NULL  ) {
			// Do not send messages to ourself (server)
			network_send_all( nwchat, true );

			// Act on message (for display of messages on server, and to keep record of messages for new clients joining)
			add_message(welt);

			// Log chat message - please don't change order of fields
			CSV_t csv;
			csv.add_field( "chat" );
			csv.add_field( client_id );
			csv.add_field( info.address.get_str() );
			csv.add_field( info.nickname.c_str() );
			csv.add_field( player_nr );
			csv.add_field( player_nr < PLAYER_UNOWNED ? welt->get_spieler( player_nr )->get_name() : "" );
			csv.add_field( message.c_str() );
			dbg->warning( "__ChatLog__", "%s", csv.get_str() );
		}
		else {
			// Send to a specific client
			// Look up a client with a matching name, if none matches
			// send a message back saying that client doesn't exist

			// Check if destination nick exists
			for (  uint32 i = 0;  i < socket_list_t::get_count();  i++  ) {
				socket_info_t& dest_info = socket_list_t::get_client(i);
				if (  (dest_info.state == socket_info_t::playing  ||  i == 0  )
					&&  i != client_id
					&&  destination == dest_info.nickname.c_str()  )
				{
					nwchat->send( dest_info.socket );
				}
			}

			// TODO also send a copy back to sending client for logging

			delete nwchat;

			// Log chat message - please don't change order of fields
			CSV_t csv;
			csv.add_field( "private" );
			csv.add_field( client_id );
			csv.add_field( info.address.get_str() );
			csv.add_field( info.nickname.c_str() );
			csv.add_field( player_nr );
			csv.add_field( player_nr < PLAYER_UNOWNED ? welt->get_spieler( player_nr )->get_name() : "" );
			csv.add_field( destination.c_str() );
			csv.add_field( message.c_str() );
			dbg->warning( "__ChatLog__", "%s", csv.get_str() );
		}
	}
	else {
		add_message(welt);
	}
	return true;
}



SOCKET nwc_join_t::pending_join_client = INVALID_SOCKET;

void nwc_join_t::rdwr()
{
	nwc_nick_t::rdwr();
	packet->rdwr_long(client_id);
	packet->rdwr_byte(answer);
}


bool nwc_join_t::execute(karte_t *welt)
{
	if(umgebung_t::server) {
		dbg->message("nwc_join_t::execute", "");
		// TODO: check whether we can send a file
		nwc_join_t nwj;
		nwj.client_id = socket_list_t::get_client_id(packet->get_sender());
		//save nickname
		if (socket_list_t::is_valid_client_id(nwj.client_id)) {
			// check nickname
			nwc_nick_t::execute(welt);
			nwj.nickname = nickname;
			socket_list_t::get_client(nwj.client_id).nickname = nickname;
		}

		// no other joining process active?
		nwj.answer = socket_list_t::get_client(nwj.client_id).is_active()  &&  pending_join_client == INVALID_SOCKET ? 1 : 0;
		DBG_MESSAGE( "nwc_join_t::execute", "client_id=%i active=%i pending_join_client=%i active=%d", socket_list_t::get_client_id(packet->get_sender()), socket_list_t::get_client(nwj.client_id).is_active(), pending_join_client, nwj.answer );
		nwj.rdwr();
		if(  nwj.send( packet->get_sender() )  ) {
			if(  nwj.answer==1  ) {
				// now send sync command
				const uint32 new_map_counter = welt->generate_new_map_counter();
				// since network_send_all() does not include non-playing clients -> send sync command separately to the joining client
				nwc_sync_t nw_sync(welt->get_sync_steps() + 1, welt->get_map_counter(), nwj.client_id, new_map_counter);
				nw_sync.rdwr();
				if(  nw_sync.send( packet->get_sender() )  ) {
					// now send sync command to the server and the remaining clients
					nwc_sync_t *nws = new nwc_sync_t(welt->get_sync_steps() + 1, welt->get_map_counter(), nwj.client_id, new_map_counter);
					network_send_all(nws, false);
					pending_join_client = packet->get_sender();
					DBG_MESSAGE( "nwc_join_t::execute", "pending_join_client now %i", pending_join_client);
					// unpause world
					if (welt->is_paused()) {
						welt->set_pause(false);
					}
				}
				else {
					dbg->warning("nwc_join_t::execute", "send of NWC_SYNC to the joining client failed");
				}
			}
		}
		else {
			dbg->warning( "nwc_join_t::execute", "send of NWC_JOIN failed" );
		}
	}
	return true;
}


/**
 * saves the history of map counters
 * the current one is at index zero, the older ones behind
 */
#define MAX_MAP_COUNTERS (7)
vector_tpl<uint32> nwc_ready_t::all_map_counters(MAX_MAP_COUNTERS);


void nwc_ready_t::append_map_counter(uint32 map_counter_)
{
	if (all_map_counters.get_count() == MAX_MAP_COUNTERS) {
		all_map_counters.pop_back();
	}
	all_map_counters.insert_at(0, map_counter_);
}


void nwc_ready_t::clear_map_counters()
{
	all_map_counters.clear();
}


bool nwc_ready_t::execute(karte_t *welt)
{
	if(  umgebung_t::server  ) {
		// compare checklist
		if(  welt->is_checklist_available(sync_step)  &&  checklist!=welt->get_checklist_at(sync_step)  ) {
			// client has gone out of sync
			socket_list_t::remove_client( get_sender() );
			char buf[256];
			welt->get_checklist_at(sync_step).print(buf, "server");
			checklist.print(buf, "client");
			dbg->warning("nwc_ready_t::execute", "disconnect client due to checklist mismatch : sync_step=%u %s", sync_step, buf);
			return true;
		}
		// check the validity of the map counter
		FOR(vector_tpl<uint32>, const i, all_map_counters) {
			if (i == map_counter) {
				// unpause the sender by sending nwc_ready_t back
				nwc_ready_t nwc(sync_step, map_counter, checklist);
				if(  !nwc.send( get_sender())  ) {
					dbg->warning("nwc_ready_t::execute", "send of NWC_READY failed");
				}
				return true;
			}
		}
		// no matching map counter -> disconnect client
		socket_list_t::remove_client( get_sender() );
		dbg->warning("nwc_ready_t::execute", "disconnect client id=%u due to invalid map counter", our_client_id);
	}
	else {
		dbg->warning("nwc_ready_t::execute", "set sync_step=%d where map_counter=%d", sync_step, map_counter);
		if(  map_counter==welt->get_map_counter()  ) {
			welt->network_game_set_pause(false, sync_step);
			welt->set_checklist_at(sync_step, checklist);
		}
		else {
			welt->network_disconnect();
			dbg->warning("nwc_ready_t::execute", "disconnecting due to map counter mismatch");
		}
	}
	return true;
}


void nwc_ready_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_long(sync_step);
	packet->rdwr_long(map_counter);
	checklist.rdwr(packet);
}

void nwc_game_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_long(len);
}



void nwc_auth_player_t::rdwr()
{
	network_command_t::rdwr();
	for(uint32 i=0; i<20; i++) {
		packet->rdwr_byte( hash[i] );
	}
	packet->rdwr_byte( player_nr );
	packet->rdwr_short(player_unlocked);
}


bool nwc_auth_player_t::execute(karte_t *welt)
{
	dbg->message("nwc_auth_player_t::execute","plnr = %d  unlock = %d  our_client_id = %d", player_nr, player_unlocked, our_client_id);

	if(  umgebung_t::server  &&  !(our_client_id==0  &&  player_nr==255)) {
		// sent to server, and not sent to player playing on server
		if (socket_list_t::is_valid_client_id(our_client_id)) {

			// player activated for this client?
			socket_info_t &info = socket_list_t::get_client(our_client_id);
			if (info.is_player_unlocked(player_nr)) {
				dbg->message("nwc_auth_player_t::execute","set pwd for plnr = %d", player_nr);

				// change password
				if (welt->get_spieler(player_nr)->access_password_hash() != hash) {
					welt->get_spieler(player_nr)->access_password_hash() = hash;
					// unlock all clients if new password is empty
					// otherwise lock all
					socket_list_t::unlock_player_all(player_nr, hash.empty(), our_client_id);
				}
			}
			else if (player_nr < PLAYER_UNOWNED) {
				// check password
				if (welt->get_spieler(player_nr)->access_password_hash() == hash) {

					dbg->message("nwc_auth_player_t::execute","unlock plnr = %d at our_client_id = %d", player_nr, our_client_id);

					info.unlock_player(player_nr);
					if (our_client_id == 0) {
						// unlock player on the server
						welt->get_spieler(player_nr)->unlock(true, false);
					}
				}
				if (our_client_id != 0) {
					// send unlock-info to player on the client (to clear unlock_pending flag)
					nwc_auth_player_t nwc;
					nwc.player_unlocked = info.player_unlocked;
					nwc.send( get_sender());
				}
			}
		}
	}
	else {
		for(uint8 i=0; i<PLAYER_UNOWNED; i++) {
			if (spieler_t *sp = welt->get_spieler(i)) {
				sp->unlock( player_unlocked & (1<<i), false);
			}
		}
	}
	// update the player window
	ki_kontroll_t* playerwin = (ki_kontroll_t*)win_get_magic(magic_ki_kontroll_t);
	if (playerwin) {
		playerwin->update_data();
	}
	return true;
}


void nwc_auth_player_t::init_player_lock_server(karte_t *welt)
{
	uint16 player_unlocked = 0;
	for(uint8 i=0; i<PLAYER_UNOWNED; i++) {
		// player not activated or password matches stored password
		spieler_t *sp = welt->get_spieler(i);
		if (sp == NULL  ||  sp->access_password_hash() == welt->get_player_password_hash(i) ) {
			player_unlocked |= 1<<i;
		}
		if (sp) {
			sp->unlock( player_unlocked & (1<<i), false);
		}
	}
	// get the local server socket
	socket_info_t &info = socket_list_t::get_client(0);
	info.player_unlocked = player_unlocked;
	dbg->message("nwc_auth_player_t::init_player_lock_server", "new = %d", player_unlocked);
}


network_world_command_t::network_world_command_t(uint16 id, uint32 sync_step, uint32 map_counter)
: network_command_t(id)
{
	this->sync_step = sync_step;
	this->map_counter = map_counter;
}

void network_world_command_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_long(sync_step);
	packet->rdwr_long(map_counter);
}

bool network_world_command_t::execute(karte_t *welt)
{
	DBG_MESSAGE("network_world_command_t::execute","do_command %d at sync_step %d world now at %d", get_id(), get_sync_step(), welt->get_sync_steps());
	// want to execute something in the past?
	if (get_sync_step() < welt->get_sync_steps()) {
		if (!ignore_old_events()) {
			dbg->warning("network_world_command_t::execute", "wanted to execute(%d) in the past", get_id());
			welt->network_disconnect();
		}
		return true; // to delete cmd
	}
	if (map_counter != welt->get_map_counter()) {
		// command from another world
		// could happen if we are behind and still have to execute the next sync command
		dbg->warning("network_world_command_t::execute", "wanted to execute(%d) from another world (mpc=%d)", get_id(), map_counter);
		if (umgebung_t::server) {
			return true; // to delete cmd
		}
		// map_counter has to be checked before calling do_command()
	}
	welt->command_queue_append(this);
	return false;
}

void nwc_sync_t::rdwr()
{
	network_world_command_t::rdwr();
	packet->rdwr_long(client_id);
	packet->rdwr_long(new_map_counter);
}

// save, load, pause, if server send game
void nwc_sync_t::do_command(karte_t *welt)
{
	dbg->warning("nwc_sync_t::do_command", "sync_steps %d", get_sync_step());
	// save screen coordinates & offsets
	const koord ij = welt->get_world_position();
	const sint16 xoff = welt->get_x_off();
	const sint16 yoff = welt->get_y_off();
	// save active player
	const uint8 active_player = welt->get_active_player_nr();
	// save lock state
	uint16 player_unlocked = 0;
	for(uint8 i=0; i<PLAYER_UNOWNED; i++) {
		if (spieler_t *sp = welt->get_spieler(i)) {
			if (!sp->is_locked()) {
				player_unlocked |= 1<<i;
			}
		}
	}
	// transfer game, all clients need to sync (save, reload, and pause)
	// now save and send
	chdir( umgebung_t::user_dir );
	if(  !umgebung_t::server  ) {
		char fn[256];
		sprintf( fn, "client%i-network.sve", network_get_client_id() );

		bool old_restore_UI = umgebung_t::restore_UI;
		umgebung_t::restore_UI = true;

		welt->speichern( fn, loadsave_t::autosave_mode, SERVER_SAVEGAME_VER_NR, false );
		uint32 old_sync_steps = welt->get_sync_steps();
		welt->laden( fn );
		umgebung_t::restore_UI = old_restore_UI;

		// pause clients, restore steps
		welt->network_game_set_pause( true, old_sync_steps);

		// apply new map counter
		welt->set_map_counter(new_map_counter);

		// tell server we are ready
		network_command_t *nwc = new nwc_ready_t( old_sync_steps, welt->get_map_counter(), welt->get_checklist_at(old_sync_steps) );
		network_send_server(nwc);
	}
	else {
		char fn[256];
		// first save password hashes
		sprintf( fn, "server%d-pwdhash.sve", umgebung_t::server );
		loadsave_t file;
		if(file.wr_open(fn, loadsave_t::save_mode, "hashes", SAVEGAME_VER_NR )) {
			welt->rdwr_player_password_hashes( &file );
			file.close();
		}

		// remove passwords before transfer on the server and set default client mask
		// they will be restored in karte_t::laden
		uint16 unlocked_players = 0;
		for(  int i=0;  i<PLAYER_UNOWNED; i++  ) {
			spieler_t *sp = welt->get_spieler(i);
			if(  sp==NULL  ||  sp->access_password_hash().empty()  ) {
				unlocked_players |= (1<<i);
			}
			else {
				sp->access_password_hash().clear();
			}
		}

		// save game
		sprintf( fn, "server%d-network.sve", umgebung_t::server );
		bool old_restore_UI = umgebung_t::restore_UI;
		umgebung_t::restore_UI = true;
		welt->speichern( fn, loadsave_t::save_mode, SERVER_SAVEGAME_VER_NR, false );

		// ok, now sending game
		// this sends nwc_game_t
		const char *err = network_send_file( client_id, fn );
		if (err) {
			dbg->warning("nwc_sync_t::do_command","send game failed with: %s", err);
		}

		uint32 old_sync_steps = welt->get_sync_steps();
		welt->laden( fn );
		umgebung_t::restore_UI = old_restore_UI;

		// restore steps
		welt->network_game_set_pause( false, old_sync_steps);

		// apply new map counter
		welt->set_map_counter(new_map_counter);

		// unpause the client that received the game
		// we do not want to wait for him (maybe loading failed due to pakset-errors)
		SOCKET sock = socket_list_t::get_socket(client_id);
		if(  sock != INVALID_SOCKET  ) {
			nwc_ready_t nwc( old_sync_steps, welt->get_map_counter(), welt->get_checklist_at(old_sync_steps) );
			if (nwc.send(sock)) {
				socket_list_t::change_state( client_id, socket_info_t::playing);
				if (socket_list_t::is_valid_client_id(client_id)) {
					socket_list_t::get_client(client_id).player_unlocked = unlocked_players;
					// send information about locked state
					nwc_auth_player_t nwc;
					nwc.player_unlocked = unlocked_players;
					nwc.send(sock);

					// welcome message
					nwc_nick_t::server_tools(welt, client_id, nwc_nick_t::WELCOME, NULL);
				}
			}
			else {
				dbg->warning( "nwc_sync_t::do_command", "send of NWC_READY failed" );
			}
		}
		nwc_join_t::pending_join_client = INVALID_SOCKET;
	}
	// restore screen coordinates & offsets
	welt->change_world_position(ij, xoff, yoff);
	welt->switch_active_player(active_player,true);
	// restore lock state
	for(uint8 i=0; i<PLAYER_UNOWNED; i++) {
		if (spieler_t *sp = welt->get_spieler(i)) {
			sp->unlock(player_unlocked & (1<<i));
		}
	}
}

void nwc_check_t::rdwr()
{
	network_world_command_t::rdwr();
	server_checklist.rdwr(packet);
	packet->rdwr_long(server_sync_step);
	if (packet->is_loading()  &&  umgebung_t::server) {
		// server does not receive nwc_check_t-commands
		packet->failed();
	}
}


void network_broadcast_world_command_t::rdwr()
{
	network_world_command_t::rdwr();
	packet->rdwr_bool(exec);

	if (packet->is_loading()  &&  umgebung_t::server  &&  exec) {
		// server does not receive exec-commands
		packet->failed();
	}
}


bool network_broadcast_world_command_t::execute(karte_t *welt)
{
	if (exec) {
		pre_execute();
		// append to command queue
		return network_world_command_t::execute(welt);
	}
	else if (umgebung_t::server) {
		// clone
		network_broadcast_world_command_t *nwc = clone(welt);
		if (nwc == NULL) {
			return true;
		}
		// next call to execute will put it in command queue
		nwc->exec = true;
		nwc->sync_step = welt->get_sync_steps() + 1;
		// broadcast
		network_send_all(nwc, false);
		// return true to delete this command only if clone() returned something new
		return true;
	}
	else {
		dbg->warning("network_broadcast_world_command_t::execute", "should not reach here!");
	}
	return true;
}


void nwc_chg_player_t::rdwr()
{
	network_broadcast_world_command_t::rdwr();
	packet->rdwr_byte(cmd);
	packet->rdwr_byte(player_nr);
	packet->rdwr_short(param);
}


network_broadcast_world_command_t* nwc_chg_player_t::clone(karte_t *welt)
{
	if (!socket_list_t::is_valid_client_id(our_client_id)) {
		return NULL;
	}
	socket_info_t const& info = socket_list_t::get_client(our_client_id); //.is_player_unlocked(player_nr)

	if (welt->change_player_tool(cmd, player_nr, param, info.is_player_unlocked(1), false)) {
		return new nwc_chg_player_t(get_sync_step(), get_map_counter(), cmd, player_nr, param);
	}
	return NULL;
}


void nwc_chg_player_t::do_command(karte_t *welt)
{
	welt->change_player_tool(cmd, player_nr, param, true, true);

	// update the window
	ki_kontroll_t* playerwin = (ki_kontroll_t*)win_get_magic(magic_ki_kontroll_t);
	if (playerwin) {
		playerwin->update_data();
	}
}


nwc_tool_t::nwc_tool_t() : network_broadcast_world_command_t(NWC_TOOL, 0, 0)
{
	default_param = NULL;
	custom_data = new memory_rw_t(custom_data_buf, lengthof(custom_data_buf), true);
}


nwc_tool_t::nwc_tool_t(spieler_t *sp, werkzeug_t *wkz, koord3d pos_, uint32 sync_steps, uint32 map_counter, bool init_)
: network_broadcast_world_command_t(NWC_TOOL, sync_steps, map_counter)
{
	pos = pos_;
	player_nr = sp ? sp->get_player_nr() : -1;
	wkz_id = wkz->get_id();
	wt = wkz->get_waytype();
	const char *dfp = wkz->get_default_param(sp);
	default_param = dfp ? strdup(dfp) : NULL;
	init = init_;
	tool_client_id = 0;
	flags = wkz->flags;
	last_sync_step = spieler_t::get_welt()->get_last_checklist_sync_step();
	last_checklist = spieler_t::get_welt()->get_last_checklist();
	// write custom data of wkz to our internal buffer
	custom_data = new memory_rw_t(custom_data_buf, lengthof(custom_data_buf), true);
	if (sp) {
		wkz->rdwr_custom_data(sp->get_player_nr(), custom_data);
	}
}


nwc_tool_t::nwc_tool_t(const nwc_tool_t &nwt)
: network_broadcast_world_command_t(NWC_TOOL, nwt.get_sync_step(), nwt.get_map_counter())
{
	pos = nwt.pos;
	player_nr = nwt.player_nr;
	wkz_id = nwt.wkz_id;
	wt = nwt.wt;
	default_param = nwt.default_param ? strdup(nwt.default_param) : NULL;
	init = nwt.init;
	tool_client_id = nwt.our_client_id;
	flags = nwt.flags;
	// copy custom data of wkz to our internal buffer
	custom_data = new memory_rw_t(custom_data_buf, lengthof(custom_data_buf), true);
	if (nwt.custom_data) {
		custom_data->append(*nwt.custom_data);
	}
}


nwc_tool_t::~nwc_tool_t()
{
	delete custom_data;
	free( (void *)default_param );
}


void nwc_tool_t::rdwr()
{
	network_broadcast_world_command_t::rdwr();
	packet->rdwr_long(last_sync_step);
	last_checklist.rdwr(packet);
	packet->rdwr_byte(player_nr);
	sint16 posx = pos.x; packet->rdwr_short(posx); pos.x = posx;
	sint16 posy = pos.y; packet->rdwr_short(posy); pos.y = posy;
	sint8  posz = pos.z; packet->rdwr_byte(posz);  pos.z = posz;
	packet->rdwr_short(wkz_id);
	packet->rdwr_short(wt);
	packet->rdwr_str(default_param);
	packet->rdwr_bool(init);
	packet->rdwr_long(tool_client_id);
	packet->rdwr_byte(flags);
	// copy custom data of wkz to/from packet
	if (packet->is_saving()) {
		// write to packet
		packet->append(*custom_data);
	}
	else {
		// read from packet
		custom_data->append_tail(*packet);
	}

	//if (packet->is_loading()) {
		dbg->warning("nwc_tool_t::rdwr", "rdwr id=%d client=%d plnr=%d pos=%s wkzid=%d defpar=%s init=%d flags=%d", id, tool_client_id, player_nr, pos.get_str(), wkz_id, default_param, init, flags);
	//}
}


void nwc_tool_t::pre_execute()
{
		if (custom_data->is_saving()) {
			// create new memory_rw_t that is in reading mode to read wkz data
			memory_rw_t *new_custom_data = new memory_rw_t(custom_data_buf, custom_data->get_current_index(), false);
			delete custom_data;
			custom_data = new_custom_data;
		}
		// append to command queue
		dbg->message("nwc_tool_t::pre_execute", "append sync_step=%d wkz=%d %s", get_sync_step(), wkz_id, init ? "init" : "work");
}

network_broadcast_world_command_t* nwc_tool_t::clone(karte_t *welt)
{
		if (map_counter != welt->get_map_counter()) {
			// command from another world
			// maybe sent before sync happened -> ignore
			dbg->warning("nwc_tool_t::clone", "wanted to execute(%d) from another world", get_id());
			return NULL; // indicate failure
		}
		// do not open dialog windows across network
		if (wkz_id & DIALOGE_TOOL) {
			return NULL; // indicate failure
		}
		// check for map editor tools - they need unlocked public player
		switch( wkz_id ) {
			case WKZ_CHANGE_CITY_SIZE | GENERAL_TOOL:
			case WKZ_BUILD_HAUS | GENERAL_TOOL:
			case WKZ_LAND_CHAIN | GENERAL_TOOL:
			case WKZ_CITY_CHAIN | GENERAL_TOOL:
			case WKZ_BUILD_FACTORY | GENERAL_TOOL:
			case WKZ_LINK_FACTORY | GENERAL_TOOL:
			case WKZ_ADD_CITYCAR | GENERAL_TOOL:
			case WKZ_INCREASE_INDUSTRY | SIMPLE_TOOL:
			case WKZ_STEP_YEAR | SIMPLE_TOOL:
			case WKZ_FILL_TREES | SIMPLE_TOOL:
				player_nr = 1;
			default: ;
		}
		if ( player_nr < PLAYER_UNOWNED  &&  !socket_list_t::get_client(our_client_id).is_player_unlocked(player_nr) ) {
			if (wkz_id == (WKZ_ADD_MESSAGE_TOOL|SIMPLE_TOOL)) {
				player_nr = PLAYER_UNOWNED;
			}
			else {
				dbg->warning("nwc_tool_t::clone", "client %d not allowed to act as player %d", our_client_id, player_nr);
				return NULL; // indicate failure
			}
		}
		// do scenario checks here, send error message back
		scenario_t *scen = welt->get_scenario();
		if ( scen->is_scripted() ) {
			if (!scen->is_tool_allowed(welt->get_spieler(player_nr), wkz_id, wt)) {
				dbg->warning("nwc_tool_t::clone", "wkz=%d  wt=%d tool not allowed", wkz_id, wt);
				// TODO return error message ?
				return NULL;
			}
			if (!init) {
				if (const char *err = scen->is_work_allowed_here(welt->get_spieler(player_nr), wkz_id, wt, pos) ) {
					nwc_tool_t *nwt = new nwc_tool_t(*this);
					nwt->wkz_id = WKZ_ERR_MESSAGE_TOOL | GENERAL_TOOL;
					free( nwt->default_param );
					nwt->default_param = strdup(err);
					nwt->last_sync_step = welt->get_last_checklist_sync_step();
					nwt->last_checklist = welt->get_last_checklist();
					dbg->warning("nwc_tool_t::clone", "send sync_steps=%d  wkz=%d  error=%s", nwt->get_sync_step(), wkz_id, err);
					return nwt;
				}
			}
		}

#if 0
#error "Pause does not reset nwc_join_t::pending_join_client properly. Disabled for now here and in simwerkz.h (wkz_pause_t)"
		// special care for unpause command
		if (wkz_id == (WKZ_PAUSE | SIMPLE_TOOL)) {
			if (welt->is_paused()) {
				// we cant do unpause in regular sync steps
				// sent ready-command instead
				nwc_ready_t *nwt = new nwc_ready_t(welt->get_sync_steps(), welt->get_map_counter());
				network_send_all(nwt, true);
				welt->network_game_set_pause(false, welt->get_sync_steps());
				// reset pending_join_client to allow connection attempts again
				nwc_join_t::pending_join_client = INVALID_SOCKET;
				return NULL; // indicate failure
			}
			else {
				// do not pause the game while a join process is active
				if(  nwc_join_t::pending_join_client != INVALID_SOCKET  ) {
					return NULL; // indicate failure
				}
				// set pending_join_client to block connection attempts during pause
				nwc_join_t::pending_join_client = ~INVALID_SOCKET;
			}
		}
#endif
		// copy data, sets tool_client_id to sender client_id
		nwc_tool_t *nwt = new nwc_tool_t(*this);
		nwt->last_sync_step = welt->get_last_checklist_sync_step();
		nwt->last_checklist = welt->get_last_checklist();
		dbg->warning("nwc_tool_t::clone", "send sync_steps=%d  wkz=%d %s", nwt->get_sync_step(), wkz_id, init ? "init" : "work");
		return nwt;
}


bool nwc_tool_t::ignore_old_events() const
{
	// messages are allowed to arrive at any time (return true if message)
	return wkz_id==(SIMPLE_TOOL|WKZ_ADD_MESSAGE_TOOL);
}


// compare default_param's (NULL pointers allowed
// @return true if default_param are equal
bool nwc_tool_t::cmp_default_param(const char *d1, const char *d2)
{
	if (d1) {
		return d2 ? strcmp(d1,d2)==0 : false;
	}
	else {
		return d2==NULL;
	}
}


void nwc_tool_t::tool_node_t::set_tool(werkzeug_t *wkz_) {
	if (wkz == wkz_) {
		return;
	}
	if (wkz) {
		delete wkz;
	}
	wkz = wkz_;
}


void nwc_tool_t::tool_node_t::client_set_werkzeug(werkzeug_t* &wkz_new, const char* new_param, karte_t *welt, spieler_t *sp)
{
	assert(wkz_new);
	// call init, before calling work
	wkz_new->set_default_param(new_param);
	if (wkz_new->init(welt, sp)) {
		// exit old tool
		if (wkz) {
			wkz->exit(welt, sp);
		}
		// now store tool and default_param
		set_tool(wkz_new); // will delete old tool here
		set_default_param(new_param);
		wkz->set_default_param(default_param);
	}
	else {
		// delete temporary tool
		delete wkz_new;
		wkz_new = NULL;
	}
}


vector_tpl<nwc_tool_t::tool_node_t*> nwc_tool_t::tool_list;


void nwc_tool_t::do_command(karte_t *welt)
{
	DBG_MESSAGE("nwc_tool_t::do_command", "steps %d wkz %d %s", get_sync_step(), wkz_id, init ? "init" : "work");
		// commands are treated differently if they come from this client or not
		bool local = tool_client_id == network_get_client_id();

		// the tool that will be executed
		werkzeug_t *wkz = NULL;

		// pointer, where the active tool from a remote client is stored
		tool_node_t *tool_node = NULL;

		// do we have a tool for this client already?
		FOR(vector_tpl<nwc_tool_t::tool_node_t*>, i, tool_list) {
			if (i->player_id == player_nr  &&  i->client_id  == tool_client_id) {
				tool_node = i;
				break;
			}
		}
		// this node stores the tool and its default_param
		if (tool_node == NULL) {
			tool_node = new tool_node_t(NULL, player_nr, tool_client_id);
			tool_list.append(tool_node);
		}

		spieler_t *sp = player_nr < PLAYER_UNOWNED ? welt->get_spieler(player_nr) : NULL;

		wkz = tool_node->get_tool();
		// create a new tool if necessary
		if (wkz == NULL  ||  wkz->get_id() != wkz_id  ||  !cmp_default_param(wkz->get_default_param(sp), default_param)) {
			wkz = create_tool(wkz_id);
			// before calling work initialize new tool / exit old tool
			if (!init) {
				// init command was not sent if wkz->is_init_network_safe() returned true
				wkz->flags = 0;
				// init tool and set default_param
				tool_node->client_set_werkzeug(wkz, default_param, welt, sp);
			}
		}

		if(  wkz  ) {
			// read custom data
			wkz->rdwr_custom_data(player_nr, custom_data);
			// set flags correctly
			if (local) {
				wkz->flags = flags | werkzeug_t::WFL_LOCAL;
			}
			else {
				wkz->flags = flags & ~werkzeug_t::WFL_LOCAL;
			}
			DBG_MESSAGE("nwc_tool_t::do_command","id=%d init=%d defpar=%s flag=%d",wkz_id&0xFFF,init,default_param,wkz->flags);
			// call INIT
			if(  init  ) {
				// we should be here only if wkz->init() returns false
				// no need to change active tool of world
				tool_node->client_set_werkzeug(wkz, default_param, welt, sp);
			}
			// call WORK
			else {
				// remove preview tiles of active tool
				two_click_werkzeug_t *active_wkz = dynamic_cast<two_click_werkzeug_t*>(welt->get_werkzeug(welt->get_active_player_nr()));
				if(active_wkz  &&  active_wkz->remove_preview_necessary()) {
					active_wkz->cleanup(true);
				}
				const char *err = wkz->work( welt, sp, pos );
				// only local players or AIs get the callback
				if (local  ||  sp->get_ai_id()!=spieler_t::HUMAN) {
					sp->tell_tool_result(wkz, pos, err, local);
				}
				if (err) {
					dbg->warning("nwc_tool_t::do_command","failed with '%s'",err);
				}
			}
			// reset flags
			if (wkz) {
				wkz->flags = 0;
			}
		}
}

// static list of execution times
vector_tpl<long> nwc_service_t::exec_time(SRVC_MAX);

extern address_list_t blacklist;

bool nwc_service_t::execute(karte_t *welt)
{
	const long time = dr_time();
	while(exec_time.get_count() <= SRVC_MAX) {
		exec_time.append(time - 60000);
	}
	if (flag>=SRVC_MAX  ||  time - exec_time[flag] < 60000  ||  !umgebung_t::server) {
		// wrong flag, last execution less than 1min ago, no server
		return true;  // to delete
	}
	// check whether admin connection is established
	const uint32 sender_id = socket_list_t::get_client_id(packet->get_sender());
	const bool admin_logged_in = socket_list_t::get_client(sender_id).state == socket_info_t::admin;
	if (!admin_logged_in  &&  (flag != SRVC_LOGIN_ADMIN  &&  flag != SRVC_ANNOUNCE_SERVER) ) {
		return true;  // to delete
	}

	switch(flag) {
		case SRVC_LOGIN_ADMIN: {
			nwc_service_t nws;
			nws.flag = SRVC_LOGIN_ADMIN;
			// check password
			bool ok = !umgebung_t::server_admin_pw.empty()  &&  umgebung_t::server_admin_pw.compare(text)==0;
			if (ok) {
				socket_list_t::get_client(sender_id).state = socket_info_t::admin;
			}
			nws.number = ok;
			nws.send(packet->get_sender());
			break;
		}

		case SRVC_ANNOUNCE_SERVER:
			// Startup announce, to force full details resend
			welt->announce_server( 0 );
			break;

		case SRVC_GET_CLIENT_LIST: {
			nwc_service_t nws;
			nws.flag = SRVC_GET_CLIENT_LIST;
			// send, socket list will be written in rdwr
			nws.send(packet->get_sender());
			break;
		}

		case SRVC_KICK_CLIENT:
		case SRVC_BAN_CLIENT: {
			bool ban = flag == SRVC_BAN_CLIENT;
			uint32 client_id = number;
			net_address_t address;
			SOCKET kick = socket_list_t::get_socket(client_id);
			if (kick!=INVALID_SOCKET) {
				socket_info_t &info = socket_list_t::get_client(client_id);
				address.ip = info.address.ip;
				if (info.state == socket_info_t::playing) {
					socket_list_t::remove_client(info.socket);
				}
			}
			if (ban  &&  address.ip) {
				fd_set fd;
				FD_ZERO(&fd);
				socket_list_t::fill_set(&fd);
				socket_list_t::client_socket_iterator_t iter(&fd);
				while(iter.next()) {
					SOCKET sock = iter.get_current();
					socket_info_t& info = socket_list_t::get_client(socket_list_t::get_client_id(sock));
					if (address.matches(info.address)) {
						socket_list_t::remove_client(sock);
					}
				}
				blacklist.append(address);
			}
			break;
		}
		case SRVC_GET_BLACK_LIST: {
			nwc_service_t nws;
			nws.flag = SRVC_GET_BLACK_LIST;
			// send, blacklist will be written in rdwr
			nws.send(packet->get_sender());
			break;
		}

		case SRVC_BAN_IP:
		case SRVC_UNBAN_IP: {
			net_address_t address(text);
			if (address.ip) {
				if (flag==SRVC_BAN_IP) {
					blacklist.append(address);
				}
				else {
					blacklist.remove(address);
				}
			}
			break;
		}

		case SRVC_ADMIN_MSG:
			if (text) {
				// Send message to all clients as Public Service
				// with reserved username Admin
				nwc_chat_t* nwchat = new nwc_chat_t( text, 1, "Admin" );
				network_send_all( nwchat, false );

				// Log chat message - please don't change order of fields
				CSV_t csv;
				csv.add_field( "adminmsg" );
				csv.add_field( text );
				dbg->warning( "__ChatLog__", "%s", csv.get_str() );
			}
			break;

		case SRVC_SHUTDOWN: {
			welt->beenden( true );
			break;
		}

		case SRVC_FORCE_SYNC: {
			const uint32 new_map_counter = welt->generate_new_map_counter();
			nwc_sync_t *nw_sync = new nwc_sync_t(welt->get_sync_steps() + 1, welt->get_map_counter(), -1, new_map_counter);

			if (welt->is_paused()) {
				if (socket_list_t::get_playing_clients() == 0) {
					// we can save directly without disturbing clients
					nw_sync->do_command(welt);
				}
				delete nw_sync;
			}
			else {
				// send sync command
				network_send_all(nw_sync, false);
			}
			break;
		}

		default: ;
	}
	return true; // to delete
}
