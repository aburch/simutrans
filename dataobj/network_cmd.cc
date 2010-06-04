#include "network_cmd.h"
#include "network.h"
#include "network_packet.h"

#include "../simtools.h"
#include "../simwerkz.h"
#include "../simworld.h"
#include "../player/simplay.h"

#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif


#ifdef DO_NOT_SEND_HASHES
static vector_tpl<uint16>hashes_ok;	// bit set, if this player on that client is not protected
#endif

network_command_t* network_command_t::read_from_socket(SOCKET s)
{
	// receive packet
	packet_t *p = new packet_t(s);
	// check data
	if (p->has_failed()  ||  !p->check_version()) {
		delete p;
		return NULL;
	}
	network_command_t* nwc = NULL;
	switch (p->get_id()) {
		case NWC_JOIN:	      nwc = new nwc_join_t(); break;
		case NWC_SYNC:        nwc = new nwc_sync_t(); break;
		case NWC_GAME:        nwc = new nwc_game_t(); break;
		case NWC_READY:       nwc = new nwc_ready_t(); break;
		case NWC_TOOL:        nwc = new nwc_tool_t(); break;
		case NWC_CHECK:       nwc = new nwc_check_t(); break;
		default:
			dbg->warning("network_command_t::read_from_socket", "received unknown packet id %d", p->get_id());
	}
	if (nwc) {
		if (!nwc->receive(p) ||  p->has_failed()) {
			delete nwc;
			nwc = NULL;
		}
	}
	return nwc;
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
	dbg->message("network_command_t::rdwr", "rdwr packet_id=%d, client_id=%ld", id, our_client_id);
}


void network_command_t::prepare_to_send()
{
	// saves the data to the packet
	if(!ready) {
		rdwr();
	}
}


void network_command_t::send(SOCKET s)
{
	prepare_to_send();
	packet->send(s);
}


bool network_command_t::is_local_cmd()
{
	return (our_client_id == (uint32)network_get_client_id());
}


void nwc_join_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_long(client_id);
	packet->rdwr_byte(answer);
}


bool nwc_join_t::execute(karte_t *welt)
{
	if (umgebung_t::server) {
		dbg->warning("nwc_join_t::execute", "");
		// TODO: check whether we can send a file
		nwc_join_t nwj;
		nwj.client_id = network_get_client_id(packet->get_sender());
		nwj.answer = nwj.client_id>0 ? 1 : 0;
		nwj.rdwr();
		nwj.send( packet->get_sender());
		if (nwj.answer == 1) {
			// now send sync command
			nwc_sync_t *nws = new nwc_sync_t(welt->get_sync_steps() + umgebung_t::server_frames_ahead, nwj.client_id);
			network_send_all(nws, false);
		}
	}
	return true;
}


bool nwc_ready_t::execute(karte_t *welt)
{
	if (umgebung_t::server) {
		// unpause the sender ie send ready_t back
		nwc_ready_t nwc(sync_steps);
		nwc.send( this->packet->get_sender());
	}
	else {
		dbg->warning("nwc_ready_t::execute", "set sync_steps=%d",sync_steps);
		welt->network_game_set_pause(false, sync_steps);
	}
	return true;
}


void nwc_ready_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_long(sync_steps);
}

void nwc_game_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_long(len);
}

network_world_command_t::network_world_command_t(uint16 id, uint32 sync_step)
: network_command_t(id)
{
	this->sync_step = sync_step;
}

void network_world_command_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_long(sync_step);
}

bool network_world_command_t::execute(karte_t *welt)
{
	dbg->warning("network_world_command_t::execute","do_command %d at sync_step %d world now at %d", get_id(), get_sync_step(), welt->get_sync_steps());
	// want to execute something in the past?
	if (get_sync_step() < welt->get_sync_steps()) {
		welt->network_disconnect();
	}
	else {
		welt->command_queue_append(this);
	}
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
	dbg->warning("nwc_sync_t::do_command", "sync_steps %d", get_sync_step());
	// transfer game, all clients need to sync (save, reload, and pause)
	// now save and send
	chdir( umgebung_t::user_dir );
	const char *filename = "server-network.sve";
	if (!umgebung_t::server ) {
		char fn[256];
		sprintf( fn, "client%i-network.sve", network_get_client_id() );
		filename = fn;
		welt->speichern(filename, false );

		long old_sync_steps = welt->get_sync_steps();
		welt->laden(filename );

		// pause clients, restore steps
		welt->network_game_set_pause(true, old_sync_steps);

		// tell server we are ready
		network_command_t *nwc = new nwc_ready_t(old_sync_steps);
		network_send_server(nwc);
	}
	else {
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
		welt->speichern(filename, false );

		// ok, now sending game
		// this sends nwc_game_t
		const char *err = network_send_file( client_id, "server-network.sve" );
		if (err) {
			dbg->warning("nwc_sync_t::do_command","send game failed with: %s", err);
		}
		// TODO: send command queue to client

		long old_sync_steps = welt->get_sync_steps();
		welt->laden(filename );

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
		welt->network_game_set_pause(false, old_sync_steps);

		// unpause the client that received the game
		// we do not want to wait for him (maybe loading failed due to pakset-errors)
		nwc_ready_t nwc(old_sync_steps);
		nwc.send(network_get_socket(client_id));
	}
}

void nwc_check_t::rdwr()
{
	network_world_command_t::rdwr();
	packet->rdwr_long(server_random_seed);
	packet->rdwr_long(server_sync_step);
}


nwc_tool_t::nwc_tool_t(spieler_t *sp, werkzeug_t *wkz, koord3d pos_, uint32 sync_steps, bool init_)
: network_world_command_t(NWC_TOOL, sync_steps)
{
	pos = pos_;
	player_nr = sp->get_player_nr();
	wkz_id = wkz->get_id();
	const char *dfp = wkz->get_default_param();
	if (dfp) {
		default_param = strdup(dfp);
	}
	else {
		default_param = 0;
	}
	exec = false;
	init = init_;
	tool_client_id = 0;
	flags = wkz->flags;
}


nwc_tool_t::nwc_tool_t(const nwc_tool_t &nwt)
: network_world_command_t(NWC_TOOL, nwt.get_sync_step())
{
	pos = nwt.pos;
	player_nr = nwt.player_nr;
	wkz_id = nwt.wkz_id;
	default_param = strdup(nwt.default_param);
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
		dbg->warning("nwc_tool_t::rdwr", "rdwr id=%d client=%d plnr=%d pos=%s wkzid=%d defpar=%s init=%d exec=%d flags=%d", id, tool_client_id, player_nr, pos.get_str(), wkz_id, default_param, init, exec, flags);
	//}
}

bool nwc_tool_t::execute(karte_t *welt)
{
	dbg->warning("nwc_tool_t::execute", "sync_steps %d %s wkz %d %s", get_sync_step(), exec ? "exec" : "send", wkz_id, init ? "init" : "work");
	if (exec) {
		// append to command queue
		dbg->warning("nwc_tool_t::execute", "append steps=%d current sync=%d", get_sync_step(),welt->get_sync_steps());
		return network_world_command_t::execute(welt);
	}
	else if (umgebung_t::server) {
		// special care for unpause command
		if (wkz_id == (WKZ_PAUSE | SIMPLE_TOOL)  &&  welt->is_paused()) {
			nwc_ready_t *nwt = new nwc_ready_t(welt->get_sync_steps());
			network_send_all(nwt, true);
			welt->network_game_set_pause(false, welt->get_sync_steps());
		}
		else {
			// copy data, sets tool_client_id to sender client_id
			nwc_tool_t *nwt = new nwc_tool_t(*this);
			nwt->exec = true;
			nwt->sync_step = welt->get_sync_steps() + umgebung_t::server_frames_ahead;
			network_send_all(nwt, false);
		}
	}
	return true;
}



vector_tpl<nwc_tool_t::tool_node_t> nwc_tool_t::tool_list;

void nwc_tool_t::do_command(karte_t *welt)
{
	dbg->warning("nwc_tool_t::do_command", "steps %d wkz %d %s", get_sync_step(), wkz_id, init ? "init" : "work");
	if (exec) {
		bool local = tool_client_id == network_get_client_id();
		werkzeug_t *wkz = NULL;
		spieler_t *sp = welt->get_spieler(player_nr);
		if (sp == NULL) {
			return;
		}
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
			tool_node_t &tool_node = tool_list[index];

			if (tool_node.wkz == NULL  ||  tool_node.wkz->get_id() != wkz_id) {
				if (tool_node.wkz) {
						// only exit, if it is not the same tool again ...
						tool_node.wkz->exit(welt,sp);
						if (tool_node.default_param) {
							free((void*)tool_node.default_param);
							tool_node.default_param = NULL;
						}
						delete tool_node.wkz;
				}
				tool_node.wkz = create_tool(wkz_id);
			}
			if (tool_node.wkz) {
				if (tool_node.default_param) {
					free((void*)tool_node.default_param);
					tool_node.default_param = NULL;
				}
				tool_node.default_param = strdup(default_param);
				tool_node.wkz->set_default_param( tool_node.default_param );
				wkz = tool_node.wkz;
			}
		}
		else {
			// local player applied a tool
			wkz = welt->get_werkzeug(player_nr);
			if (wkz  &&
				(wkz->get_id() != wkz_id  ||
				 (wkz->get_default_param()==NULL  &&  default_param!=NULL) ||
				 (wkz->get_default_param()!=NULL  &&  default_param==NULL) ||
				 (default_param!=NULL  &&  strcmp(wkz->get_default_param(),default_param)!=0))) {
				wkz = NULL;
			}
			if (wkz == NULL) {
				// get the right tool
				vector_tpl<werkzeug_t*> &wkz_list = wkz_id&GENERAL_TOOL ? werkzeug_t::general_tool : wkz_id&SIMPLE_TOOL ? werkzeug_t::simple_tool : werkzeug_t::dialog_tool;
				for(uint32 i=0; i<wkz_list.get_count(); i++) {
					if (wkz_list[i]  &&  wkz_list[i]->get_id()==wkz_id &&  (wkz_list[i]->get_default_param()!=NULL ? strcmp(wkz_list[i]->get_default_param(),default_param)==0 : default_param==NULL)) {
						wkz = wkz_list[i];
						break;
					}
				}
				if (wkz==NULL) {
					if(  wkz_id&GENERAL_TOOL  ) {
						wkz = werkzeug_t::general_tool[wkz_id&0xFFF];
					}
					else if(  wkz_id&SIMPLE_TOOL  ) {
						wkz = werkzeug_t::simple_tool[wkz_id&0xFFF];
					}
					else if(  wkz_id&DIALOGE_TOOL  ) {
						wkz = werkzeug_t::dialog_tool[wkz_id&0xFFF];
					}
					// does this have any side effects??
					//wkz->set_default_param(default_param);
				}
			}
		}
		if(  wkz  ) {
			const char* old_default_param = wkz->get_default_param();
			wkz->set_default_param(default_param);
			wkz->flags = flags | (local ? werkzeug_t::WFL_LOCAL : 0);
			dbg->warning("command","%d:%d:%s:%d",wkz_id&0xFFF,init,wkz->get_tooltip(sp),wkz->flags);
			if(  init  ) {
				if(local) {
					welt->local_set_werkzeug(wkz, sp);
				}
				else {
					wkz->init( welt, sp );
				}
			}
			else {
				const char *err = wkz->work( welt, sp, pos );
				// only local players or AIs get the callback
				if (local  ||  sp->get_ai_id()!=spieler_t::HUMAN) {
					sp->tell_tool_result(wkz, pos, err, local);
				}
			}
			wkz->set_default_param(old_default_param);
			wkz->flags = 0;
		}

	}
}
