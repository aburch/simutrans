#include "network_cmd.h"
#include "network.h"
#include "network_packet.h"
#include "network_socket_list.h"
#include "network_cmp_pakset.h"

#include "loadsave.h"
#include "gameinfo.h"
#include "../simtools.h"
#include "../simwerkz.h"
#include "../simworld.h"
#include "../simmesg.h"
#include "../simversion.h"
#include "../player/simplay.h"

#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif


#ifdef DO_NOT_SEND_HASHES
static vector_tpl<uint16>hashes_ok;	// bit set, if this player on that client is not protected
#endif

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


// needed by world to kick clients if needed
SOCKET network_command_t::get_sender()
{
	return packet->get_sender();
}


network_command_t::network_command_t(uint16 id_)
{
	packet = new packet_t();
	id = id_;
	our_client_id = (uint32)network_get_client_id();
	ready = false;
}


// default constructor
network_command_t::network_command_t()
: packet(NULL), id(0), our_client_id(0), ready(false)
{}


bool network_command_t::receive(packet_t *p)
{
	ready = true;
	if(  packet  ) {
		delete packet;
	}
	packet = p;
	id = p->get_id();
	rdwr();
	return (!packet->has_failed());
}

network_command_t::~network_command_t()
{
	if (packet) {
		delete packet;
		packet = NULL;
	}
}

void network_command_t::rdwr()
{
	if (packet->is_saving()) {
		packet->set_id(id);
		ready = true;
	}
	packet->rdwr_long(our_client_id);
	dbg->message("network_command_t::rdwr", "%s packet_id=%d, client_id=%ld", packet->is_saving() ? "write" : "read", id, our_client_id);
}


void network_command_t::prepare_to_send()
{
	// saves the data to the packet
	if(!ready) {
		rdwr();
	}
}


bool network_command_t::send(SOCKET s)
{
	prepare_to_send();
	packet->send(s, true);
	bool ok = packet->is_ready();
	if (!ok) {
		dbg->warning("network_command_t::send", "send packet_id=%d to [%d] failed", id, s);
	}
	return ok;
}


bool network_command_t::is_local_cmd()
{
	return (our_client_id == (uint32)network_get_client_id());
}


packet_t* network_command_t::copy_packet() const
{
	if (packet) {
		return new packet_t(*packet);
	}
	else {
		return NULL;
	}
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
		nwj.rdwr();
		if (nwj.send( packet->get_sender())) {
			if (nwj.answer == 1) {
				// now send sync command
				nwc_sync_t *nws = new nwc_sync_t(welt->get_sync_steps() + 1, welt->get_map_counter(), nwj.client_id);
				network_send_all(nws, false);
				pending_join_client = packet->get_sender();
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
		all_map_counters.remove_at( all_map_counters.get_count()-1 );
	}
	all_map_counters.insert_at(0, map_counter_);
}


void nwc_ready_t::clear_map_counters()
{
	all_map_counters.clear();
}


bool nwc_ready_t::execute(karte_t *welt)
{
	if (umgebung_t::server) {
		// unpause the sender: send ready_t back
		// get the right map counter first
		// i.e. that one the comes after the mapcounter in this command
		uint32 mpc = welt->get_map_counter();
		for (uint32 i=1; i<all_map_counters.get_count(); i++) {
			if (all_map_counters[i] == map_counter) {
				mpc = all_map_counters[i-1];
				break;
			}
		}
		nwc_ready_t nwc(sync_steps, mpc);
		if (!nwc.send( this->packet->get_sender())) {
			dbg->warning( "nwc_ready_t::execute", "send of NWC_READY failed" );
		}
	}
	else {
		dbg->warning("nwc_ready_t::execute", "set sync_steps=%d map_counter=%d",sync_steps,map_counter);
		welt->set_map_counter(map_counter);
		welt->network_game_set_pause(false, sync_steps);
	}
	return true;
}


void nwc_ready_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_long(sync_steps);
	packet->rdwr_long(map_counter);
}

void nwc_game_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_long(len);
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
	dbg->warning("network_world_command_t::execute","do_command %d at sync_step %d world now at %d. Random flags: %d", get_id(), get_sync_step(), welt->get_sync_steps(), get_random_mode());
	// want to execute something in the past?
	if (get_sync_step() < welt->get_sync_steps()) {
		if (!ignore_old_events()) {
			dbg->warning("network_world_command_t::execute", "wanted to execute(%d) in the past. Random flags: %d", get_id(), get_random_mode());
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
}

// save, load, pause, if server send game
void nwc_sync_t::do_command(karte_t *welt)
{
	dbg->warning("nwc_sync_t::do_command", "sync_steps %d. Random flags: &d", get_sync_step(), get_random_mode());
	// save screen coordinates & offsets
	const koord ij = welt->get_world_position();
	const sint16 xoff = welt->get_x_off();
	const sint16 yoff = welt->get_y_off();
	// save active player
	const uint8 active_player = welt->get_active_player_nr();
	// transfer game, all clients need to sync (save, reload, and pause)
	// now save and send
	chdir( umgebung_t::user_dir );
	if (!umgebung_t::server ) {
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

		// tell server we are ready
		network_command_t *nwc = new nwc_ready_t( old_sync_steps, welt->get_map_counter() );
		network_send_server(nwc);
	}
	else {
		char fn[256];
		sprintf( fn, "server%d-network.sve", umgebung_t::server );

#ifdef DO_NOT_SEND_HASHES
		// remove passwords before transfer on the server and set default client mask
		pwd_hash_t player_hashes[PLAYER_UNOWNED];
		uint16 default_hashes = 0;
		for(  int i=0;  i<PLAYER_UNOWNED; i++  ) {
			spieler_t *sp = welt->get_spieler(i);
			if(  sp==NULL  ||  !sp->set_unlock(NULL)  ) {
				default_hashes |= (1<<i);
			}
			else {
				pwd_hash_t& p = sp->get_password_hash();
				player_hashes[i] = p;
				p.clear();
			}
		}
#endif
		bool old_restore_UI = umgebung_t::restore_UI;
		umgebung_t::restore_UI = true;
		welt->speichern(fn, SERVER_SAVEGAME_VER_NR, EXPERIMENTAL_VER_NR, false );

		// ok, now sending game
		// this sends nwc_game_t
		const char *err = network_send_file( client_id, fn );
		if (err) {
			dbg->warning("nwc_sync_t::do_command","send game failed with: %s", err);
		}
		// TODO: send command queue to client

		uint32 old_sync_steps = welt->get_sync_steps();
		welt->laden( fn );
		umgebung_t::restore_UI = old_restore_UI;

#ifdef DO_NOT_SEND_HASHES
		// restore password info
		for(  int i=0;  i<PLAYER_UNOWNED; i++  ) {
			spieler_t *sp = welt->get_spieler(i);
			if(  sp  ) {
				sp->get_password_hash() = player_hashes[i];
			}
		}
		hashes_ok.insert_at(client_id,default_hashes);
#endif
		// restore steps
		welt->network_game_set_pause( false, old_sync_steps);

		// generate new map_counter
		welt->reset_map_counter();

		// unpause the client that received the game
		// we do not want to wait for him (maybe loading failed due to pakset-errors)
		SOCKET sock = socket_list_t::get_socket(client_id);
		if (sock != INVALID_SOCKET) {
			nwc_ready_t nwc( old_sync_steps, welt->get_map_counter());
			if (nwc.send(sock)) {
				// Knightly : synchronise the iteration limits if necessary
				if(  welt->get_einstellungen()->get_default_path_option()!=2  ||
					 nwc_routesearch_t::transmit_active_limit_set(sock, old_sync_steps, welt->get_map_counter())  ) {
					socket_list_t::change_state(client_id, socket_info_t::playing);
				}
				else {
					dbg->warning("nwc_sync_t::do_command", "send of NWC_ROUTESEARCH failed");
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
		 world->get_sync_steps()-last_update_sync_step>=4*world->get_einstellungen()->get_frames_per_step())  ) {
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
	packet->rdwr_long(server_random_seed);
	packet->rdwr_long(server_sync_step);
	if (packet->is_loading()  &&  umgebung_t::server) {
		// server does not receive nwc_check_t-commands
		packet->failed();
	}
}


nwc_tool_t::nwc_tool_t(spieler_t *sp, werkzeug_t *wkz, koord3d pos_, uint32 sync_steps, uint32 map_counter, bool init_)
: network_world_command_t(NWC_TOOL, sync_steps, map_counter)
{
	pos = pos_;
	player_nr = sp ? sp->get_player_nr() : -1;
	wkz_id = wkz->get_id();
	const char *dfp = wkz->get_default_param(sp);
	default_param = dfp ? strdup(dfp) : NULL;
	exec = false;
	init = init_;
	tool_client_id = 0;
	flags = wkz->flags;
	last_sync_step = sp->get_welt()->get_last_random_seed_sync();
	last_random_seed = sp->get_welt()->get_last_random_seed();
}


nwc_tool_t::nwc_tool_t(const nwc_tool_t &nwt)
: network_world_command_t(NWC_TOOL, nwt.get_sync_step(), nwt.get_map_counter())
{
	pos = nwt.pos;
	player_nr = nwt.player_nr;
	wkz_id = nwt.wkz_id;
	default_param = nwt.default_param ? strdup(nwt.default_param) : NULL;
	init = nwt.init;
	tool_client_id = nwt.our_client_id;
	flags = nwt.flags;
}


nwc_tool_t::~nwc_tool_t()
{
	free( (void *)default_param );
}


void nwc_tool_t::rdwr()
{
	network_world_command_t::rdwr();
	packet->rdwr_long(last_sync_step);
	packet->rdwr_long(last_random_seed);
	packet->rdwr_byte(player_nr);
	sint16 posx = pos.x; packet->rdwr_short(posx); pos.x = posx;
	sint16 posy = pos.y; packet->rdwr_short(posy); pos.y = posy;
	sint8  posz = pos.z; packet->rdwr_byte(posz);  pos.z = posz;
	packet->rdwr_short(wkz_id);
	packet->rdwr_str(default_param);
	packet->rdwr_bool(init);
	packet->rdwr_bool(exec);
	packet->rdwr_long(tool_client_id);
	packet->rdwr_byte(flags);
	//if (packet->is_loading()) {
		dbg->warning("nwc_tool_t::rdwr", "rdwr id=%d client=%d plnr=%d pos=%s wkzid=%d defpar=%s init=%d exec=%d flags=%d. Random mode: %d", id, tool_client_id, player_nr, pos.get_str(), wkz_id, default_param, init, exec, flags, get_random_mode());
	//}
	if (packet->is_loading()  &&  umgebung_t::server  &&  exec) {
		// server does not receive exec-commands
		packet->failed();
	}
}

bool nwc_tool_t::execute(karte_t *welt)
{
	if (exec) {
		// append to command queue
		dbg->warning("nwc_tool_t::execute", "append sync_step=%d current sync_step=%d  wkz=%d %s, random flags: %d", get_sync_step(),welt->get_sync_steps(), wkz_id, init ? "init" : "work",  get_random_mode());
		return network_world_command_t::execute(welt);
	}
	else if (umgebung_t::server) {
		if (map_counter != welt->get_map_counter()) {
			// command from another world
			// maybe sent before sync happened -> ignore
			dbg->warning("nwc_tool_t::execute", "wanted to execute(%d) from another world. Random flags: %d", get_id(), get_random_mode());
			return true; // to delete cmd
		}
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
				return true;
			}
			else {
				// do not pause the game while a join process is active
				if (nwc_join_t::pending_join_client != INVALID_SOCKET) {
					return true;
				}
				// set pending_join_client to block connection attempts during pause
				nwc_join_t::pending_join_client = ~INVALID_SOCKET;
			}
		}
		// copy data, sets tool_client_id to sender client_id
		nwc_tool_t *nwt = new nwc_tool_t(*this);
		nwt->exec = true;
		nwt->sync_step = welt->get_sync_steps() + 1;
		nwt->last_sync_step = welt->get_last_random_seed_sync();
		nwt->last_random_seed = welt->get_last_random_seed();
		dbg->warning("nwc_tool_t::execute", "send sync_steps=%d  wkz=%d %s. Random flags: %d", nwt->get_sync_step(), wkz_id, init ? "init" : "work", get_random_mode());
		network_send_all(nwt, false);
	}
	return true;
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


void nwc_tool_t::tool_node_t::set_default_param(const char* param) {
	if (param == default_param) {
		return;
	}
	if (default_param) {
		free( (void *)default_param );
		default_param = NULL;
	}
	if (param) {
		default_param = strdup(param);
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
	wkz_new->flags = 0;
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


vector_tpl<nwc_tool_t::tool_node_t> nwc_tool_t::tool_list;


void nwc_tool_t::do_command(karte_t *welt)
{
	dbg->warning("nwc_tool_t::do_command", "steps %d wkz %d %s. Random flags: %d", get_sync_step(), wkz_id, init ? "init" : "work", get_random_mode());
	if (exec) {
		// commands are treated differently if they come from this client or not
		bool local = tool_client_id == network_get_client_id();

		// the tool that will be executed
		werkzeug_t *wkz = NULL;

		// pointer, where the active tool from a remote client is stored
		tool_node_t *tool_node = NULL;

		// if we took a tool from the static lists reset default_param after work
		bool reset_default_param = false;
		const char *old_default_param = NULL;

		spieler_t *sp = player_nr < MAX_PLAYER_COUNT ? welt->get_spieler(player_nr) : NULL;
		// our tool or from network?
		if (!local) {
			// do we have a tool for this client already?
			tool_node_t new_tool_node(NULL, player_nr, tool_client_id);
			uint32 index;
			if (tool_list.is_contained(new_tool_node)) {
				index = tool_list.index_of(new_tool_node);
			}
			else {
				tool_list.append(new_tool_node);
				index = tool_list.get_count()-1;
			}
			// this node stores the tool and its default_param
			tool_node = &(tool_list[index]);

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
		}
		else {
			// local player applied a tool
			// first try the active tool of our world
			wkz = player_nr < MAX_PLAYER_COUNT ? welt->get_werkzeug(player_nr) : NULL;
			if (wkz == NULL  ||  wkz->get_id() != wkz_id  ||  !cmp_default_param(wkz->get_default_param(sp), default_param)) {
				// get the right tool
				vector_tpl<werkzeug_t*> &wkz_list = wkz_id&GENERAL_TOOL ? werkzeug_t::general_tool : wkz_id&SIMPLE_TOOL ? werkzeug_t::simple_tool : werkzeug_t::dialog_tool;
				wkz = NULL;
				// first do a detailed search for tool that matches id and default_param
				for(uint32 i=0; i < wkz_list.get_count(); i++) {
					if (wkz_list[i]  &&  wkz_list[i]->get_id()==wkz_id  &&  cmp_default_param(wkz_list[i]->get_default_param(sp),default_param)) {
						wkz = wkz_list[i];
						break;
					}
				}
				if (wkz == NULL ) {
					// take default tool if nothing was found
					// this happens for all the convoi, line, etc.. tools
					wkz = wkz_list[wkz_id&0xFFF];
					// we need to reset default_param later otherwise it might point into nirwana soon
					reset_default_param = true;
					// get raw default_param sp==NULL
					old_default_param = wkz->get_default_param(NULL);
					// now set correct parameter
					wkz->set_default_param(default_param);
				}
			}
		}
		if(  wkz  ) {
			// set flags correctly
			if (local) {
				wkz->flags = flags | werkzeug_t::WFL_LOCAL;
			}
			else {
				wkz->flags = flags & ~werkzeug_t::WFL_LOCAL;
			}
			dbg->warning("command","id=%d init=%d defpar=%s flag=%d. Random flags: %d",wkz_id&0xFFF,init,default_param,wkz->flags, get_random_mode());
			// call INIT
			if(  init  ) {
				if(local) {
					welt->local_set_werkzeug(wkz, sp);
				}
				else {
					tool_node->client_set_werkzeug(wkz, default_param, false, welt, sp);
				}
			}
			// call WORK
			else {
				const char *err = wkz->work( welt, sp, pos );
				// only local players or AIs get the callback
				if (local  ||  sp->get_ai_id()!=spieler_t::HUMAN) {
					sp->tell_tool_result(wkz, pos, err, local);
				}
				if (err) {
					dbg->warning("command","failed with '%s'",err);
				}
			}
			// restore data
			if (wkz) {
				// .. reset flags
				wkz->flags = 0;
				if (reset_default_param) {
					// if we took a default tool reset default_param
					wkz->set_default_param(old_default_param);
				}
			}
		}
	}
}
