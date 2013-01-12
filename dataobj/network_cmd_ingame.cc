#include "network_cmd_ingame.h"
#include "network.h"
#include "network_file_transfer.h"
#include "network_packet.h"
#include "network_socket_list.h"
#include "network_cmp_pakset.h"

#include "loadsave.h"
#include "gameinfo.h"
#include "../simtools.h"
#include "../simmenu.h"
#include "../simsys.h"
#include "../simversion.h"
#include "../simwin.h"
#include "../dataobj/umgebung.h"
#include "../player/simplay.h"
#include "../gui/player_frame_t.h"
#include "../utils/cbuffer_t.h"


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
		case NWC_ROUTESEARCH: nwc = new nwc_routesearch_t(); break;
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
		if(  fd.wr_open( "serverinfo.sve", loadsave_t::xml_bzip2, "info", SERVER_SAVEGAME_VER_NR, EXPERIMENTAL_VER_NR )  ) {
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


SOCKET nwc_join_t::pending_join_client = INVALID_SOCKET;


void nwc_join_t::rdwr()
{
	network_command_t::rdwr();
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
			if (info.is_player_unlocked(player_nr)  ||  info.is_player_unlocked(1)  ||  info.state == socket_info_t::admin) {
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

		welt->speichern(fn, SERVER_SAVEGAME_VER_NR, EXPERIMENTAL_VER_NR, false );
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
		if(file.wr_open(fn, loadsave_t::save_mode, "hashes", SAVEGAME_VER_NR, EXPERIMENTAL_VER_NR)) 
		{
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
		welt->speichern(fn, SERVER_SAVEGAME_VER_NR, EXPERIMENTAL_VER_NR, false );

		// ok, now sending game
		// this sends nwc_game_t
		const char *err = network_send_file( client_id, fn );
		if (err) {
			dbg->warning("nwc_sync_t::do_command","send game failed with: %s", err);
		}

		else {
			// Knightly : synchronise the iteration limits
			SOCKET sock = socket_list_t::get_socket(client_id);
			if(  sock==INVALID_SOCKET  ||  !nwc_routesearch_t::transmit_active_limit_set(sock, welt->get_sync_steps(), new_map_counter)  ) {
				dbg->warning("nwc_sync_t::do_command", "send of NWC_ROUTESEARCH failed");
			}
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

slist_tpl<nwc_routesearch_t::client_entry_t> nwc_routesearch_t::client_entries;
path_explorer_t::limit_set_t nwc_routesearch_t::active_limit_set;
path_explorer_t::limit_set_t nwc_routesearch_t::min_limit_set;
uint32 nwc_routesearch_t::last_update_sync_step = 0;
uint16 nwc_routesearch_t::accumulated_updates = 0;


void nwc_routesearch_t::rdwr()
{
	network_world_command_t::rdwr();
	limit_set.rdwr(packet);
	packet->rdwr_bool(apply_limits);
	dbg->warning("nwc_routesearch_t::rdwr", "rdwr limits=(%u, %u, %u, %llu, %u) apply_limits=%u",
		limit_set.rebuild_connexions, limit_set.filter_eligible, limit_set.fill_matrix, limit_set.explore_paths, limit_set.reroute_goods, apply_limits);
}


bool nwc_routesearch_t::execute(karte_t *world)
{
	if(  get_map_counter()!=world->get_map_counter()  ) {
		// since iteration limits are not specific to any world
		// -> it is safe to execute in spite of the difference in map counters
		dbg->warning("nwc_routesearch_t::execute", "different map counters: command=%u world=%u", get_map_counter(), world->get_map_counter());
	}
	if(  apply_limits  ) {
		dbg->warning("nwc_routesearch_t::execute", "to be applied: limits=(%u, %u, %u, %llu, %u)", limit_set.rebuild_connexions, limit_set.filter_eligible,
			limit_set.fill_matrix, limit_set.explore_paths, limit_set.reroute_goods);
		// check if the command's sync step is valid
		if(  get_sync_step()<world->get_sync_steps()  ) {
			// this command should be executed in the past
			world->network_disconnect();
			dbg->warning("nwc_routesearch_t::execute", "attempt to execute in the past (sync step: command=%u < world=%u) results in disconnection", get_sync_step(), world->get_sync_steps());
			return true;	// to delete network command
		}
		// append to command queue
		world->command_queue_append(this);
		dbg->warning("nwc_routesearch_t::execute", "add to world command queue (sync step: command=%u world=%u)", get_sync_step(), world->get_sync_steps());
		return false;		// network command now stored in world command queue -> do not delete it
	}
	else if(  umgebung_t::server  ) {
		// if there is an existing entry for the client -> just update the limit set
		bool found = false;
		for(  slist_tpl<client_entry_t>::iterator iter=client_entries.begin(), end=client_entries.end();  iter!=end;  ++iter  ) {
			if(  iter->client_id==our_client_id  ) {
				iter->limit_set = this->limit_set;
				found = true;
			}
		}
		// client entry not found -> append to the list
		if(  !found  ) {
			client_entries.append( client_entry_t(our_client_id, limit_set) );
		}
		dbg->warning("nwc_routesearch_t::execute", "%s client_id=%u limits=(%u, %u, %u, %llu, %u)", found ? "update" : "append", our_client_id,
			limit_set.rebuild_connexions, limit_set.filter_eligible, limit_set.fill_matrix, limit_set.explore_paths, limit_set.reroute_goods);
		// check if min limit set has to be updated
		path_explorer_t::limit_set_t new_min_set = calc_min_limits();
		if(  new_min_set!=min_limit_set  ) {
			min_limit_set = new_min_set;
			last_update_sync_step = world->get_sync_steps();
			++accumulated_updates;
			dbg->warning("nwc_routesearch_t::execute", "min_limit_set updated: last_update_sync_step=%u accumulated_updates=%u", last_update_sync_step, accumulated_updates);
		}
	}
	return true;
}


void nwc_routesearch_t::do_command(karte_t *world)
{
	// apply the limits
	path_explorer_t::set_limits(limit_set);
	dbg->warning("nwc_routesearch_t::do_command", "apply limits=(%u, %u, %u, %llu, %u)",
		limit_set.rebuild_connexions, limit_set.filter_eligible, limit_set.fill_matrix, limit_set.explore_paths, limit_set.reroute_goods);
}


void nwc_routesearch_t::check_for_transmission(karte_t *world)
{
	// transmit min limit set if there has been 8 or more updates, or if no further update after a specified number of sync steps
	if(  accumulated_updates>=8  ||  (accumulated_updates>0  &&
		 world->get_sync_steps()-last_update_sync_step>=4*world->get_settings().get_frames_per_step())  ) {
		network_send_all( new nwc_routesearch_t(world->get_sync_steps() + 1, world->get_map_counter(), min_limit_set, true), false );
		last_update_sync_step = 0;
		accumulated_updates = 0;
		active_limit_set = min_limit_set;
		dbg->warning("nwc_routesearch_t::check_for_transmission", "transmit sync_step=%u map_counter=%u limits=(%u, %u, %u, %llu, %u)",
			world->get_sync_steps() + 1, world->get_map_counter(), min_limit_set.rebuild_connexions, min_limit_set.filter_eligible,
			min_limit_set.fill_matrix, min_limit_set.explore_paths, min_limit_set.reroute_goods);
	}
}


bool nwc_routesearch_t::transmit_active_limit_set(SOCKET client_socket, uint32 sync_step, uint32 map_counter)
{
	// check if the active limit set is valid -> if not, initialise the active limit set
	if(  active_limit_set==path_explorer_t::limit_set_t()  ) {
		active_limit_set = path_explorer_t::get_active_limits();
	}
	// now send out the active limit set
	nwc_routesearch_t nwrs(sync_step, map_counter, active_limit_set, true);
	const bool success = nwrs.send(client_socket);
	dbg->warning("nwc_routesearch_t::transmit_active_limit_set", "transmit %s sync_step=%u map_counter=%u limits=(%u, %u, %u, %llu, %u)",
		success ? "succeeded" : "failed", sync_step, map_counter, active_limit_set.rebuild_connexions, active_limit_set.filter_eligible,
		active_limit_set.fill_matrix, active_limit_set.explore_paths, active_limit_set.reroute_goods);
	return success;
}


void nwc_routesearch_t::remove_client_entry(uint32 client_id)
{
	// find and remove a client entry with matching ID, if any
	bool entry_removed = false;
	for(  slist_tpl<client_entry_t>::iterator iter=client_entries.begin(), end=client_entries.end();  iter!=end;  ++iter  ) {
		if(  iter->client_id==client_id  ) {
			client_entries.erase(iter);
			entry_removed = true;
			dbg->warning("nwc_routesearch_t::remove_client_entry", "remove client entry id=%u", client_id);
			break;
		}
	}
	// update the min limit set where necessary
	if(  entry_removed  ) {
		path_explorer_t::limit_set_t new_min_set = calc_min_limits();
		if(  new_min_set!=min_limit_set  ) {
			min_limit_set = new_min_set;
			last_update_sync_step = 0;	// force transmission of min limit set
			++accumulated_updates;
			dbg->warning("nwc_routesearch_t::remove_client_entry", "min_limit_set updated: last_update_sync_step=%u accumulated_updates=%u limits=(%u, %u, %u, %llu, %u)",
				last_update_sync_step, accumulated_updates, min_limit_set.rebuild_connexions, min_limit_set.filter_eligible, min_limit_set.fill_matrix, min_limit_set.explore_paths, min_limit_set.reroute_goods);
		}
	}
}


void nwc_routesearch_t::reset()
{
	client_entries.clear();
	active_limit_set = path_explorer_t::limit_set_t();
	min_limit_set = path_explorer_t::limit_set_t();
	last_update_sync_step = 0;
	accumulated_updates = 0;
	dbg->warning("nwc_routesearch_t::reset", "all static variables are reset");
}


path_explorer_t::limit_set_t nwc_routesearch_t::calc_min_limits()
{
	// create an initial set with max values
	path_explorer_t::limit_set_t result_set(true);
	// compare result set against limit sets of client entries, update it to min values
	for(  slist_tpl<client_entry_t>::const_iterator iter=client_entries.begin(), end=client_entries.end();  iter!=end;  ++iter  ) {
		result_set.find_min_with(iter->limit_set);
	}
	return result_set;
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
	packet->rdwr_long(tool_client_id);
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
	const char *dfp = wkz->get_default_param(sp);
	default_param = dfp ? strdup(dfp) : NULL;
	init = init_;
	tool_client_id = 0;
	flags = wkz->flags;
	last_sync_step = sp->get_welt()->get_last_checklist_sync_step();
	last_checklist = sp->get_welt()->get_last_checklist();
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
			dbg->error("nwc_tool_t::clone", "wanted to execute(%d) from another world", get_id());
			return NULL; // indicate failure
		}
		if ( player_nr < PLAYER_UNOWNED  &&  !socket_list_t::get_client(our_client_id).is_player_unlocked(player_nr) ) {
			dbg->warning("nwc_tool_t::clone", "client %d not allowed to act as player %d", our_client_id, player_nr);
			return NULL; // indicate failure
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


void nwc_tool_t::tool_node_t::client_set_werkzeug(werkzeug_t* &wkz_new, const char* new_param, bool store, karte_t *welt, spieler_t *sp)
{
	assert(wkz_new);
	// call init, before calling work
	wkz_new->set_default_param(new_param);
	if (wkz_new->init(welt, sp)  ||  store) {
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
				tool_node->client_set_werkzeug(wkz, default_param, true, welt, sp);
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
				tool_node->client_set_werkzeug(wkz, default_param, false, welt, sp);
			}
			// call WORK
			else {
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
				// add message via tool
				cbuffer_t buf;
				buf.printf(translator::translate("[Admin] %s", welt->get_settings().get_name_language_id()), text);
				werkzeug_t *w = create_tool( WKZ_ADD_MESSAGE_TOOL | SIMPLE_TOOL );
				w->set_default_param( buf );
				welt->set_werkzeug( w, NULL );
				// since init always returns false, it is save to delete immediately
				delete w;
			}
			break;

		case SRVC_SHUTDOWN: {
			welt->beenden( true );
			break;
		}

		case SRVC_FORCE_SYNC: {
			// send sync command
			const uint32 new_map_counter = welt->generate_new_map_counter();
			nwc_sync_t *nw_sync = new nwc_sync_t(welt->get_sync_steps() + 1, welt->get_map_counter(), -1, new_map_counter);
			network_send_all(nw_sync, false);
			break;
		}

		default: ;
	}
	return true; // to delete
}

