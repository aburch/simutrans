/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "network_socket_list.h"
#include "network_cmd.h"
#include "network_cmd_ingame.h"
#include "network_packet.h"

#ifndef NETTOOL
#include "../dataobj/environment.h"
#endif


bool connection_info_t::operator==(const connection_info_t& other) const
{
	return (address.get_ip() == other.address.get_ip())  &&  ( strcmp(nickname.c_str(), other.nickname.c_str())==0 );
}


void socket_info_t::reset()
{
	delete packet;
	packet = NULL;
	while(!send_queue.empty()) {
		packet_t *p = send_queue.remove_first();
		delete p;
	}
	if (socket != INVALID_SOCKET) {
		network_close_socket(socket);
	}
	if (state != has_left) {
		state = inactive;
	}
	socket = INVALID_SOCKET;
	player_unlocked = 0;
}


network_command_t* socket_info_t::receive_nwc()
{
	if (!is_active()) {
		return NULL;
	}
	if (packet == NULL) {
		packet = new packet_t(socket);
	}

	packet->recv();

	if (packet->has_failed()) {
		// close this client (will delete packet)
		socket_list_t::remove_client(socket);
	}
	else if (packet->is_ready()) {
		// create command
		network_command_t *nwc = network_command_t::read_from_packet(packet);
		// the network_command takes care of deleting packet
		packet = NULL;
		return nwc;
	}
	return NULL;
}


void socket_info_t::process_send_queue()
{
	while(!send_queue.empty()) {
		packet_t *p = send_queue.front();
		p->send(socket, false);
		if (p->has_failed()) {
			// close this client, clear the send_queue
			socket_list_t::remove_client(socket);
			break;
		}
		else if (p->is_ready()) {
			// packet complete sent, remove from queue
			send_queue.remove_first();
			delete p;
			// proceed with next packet
		}
		else {
			break;
		}
	}
}


void socket_info_t::send_queue_append(packet_t *p)
{
	if (p) {
		if (!p->has_failed()) {
			send_queue.append(p);
		}
		else {
			delete p;
		}
	}
}

void socket_info_t::rdwr(packet_t *p)
{
	address.rdwr(p);
}

/**
 * list: contains _all_ sockets
 *       with the convention that all server_sockets are at indices 0..server_sockets-1
 */
vector_tpl<socket_info_t*>socket_list_t::list(20);

uint32 socket_list_t::connected_clients;
uint32 socket_list_t::playing_clients;
/**
 * server: max number of server_sockets ever created
 *         do not decrease!
 * client: number of server connections
 */
uint32 socket_list_t::server_sockets;

/**
 * book-keeping for the number of connected / playing clients
 */
void socket_list_t::book_state_change(socket_info_t::connection_state_t state, sint8 incr)
{
	switch (state) {
		case socket_info_t::connected:
			connected_clients += incr;
			break;
		case socket_info_t::playing:
			playing_clients += incr;
			break;

		// do not change
		case socket_info_t::inactive:
		case socket_info_t::server:
		default:
			break;
	}
}


void socket_list_t::change_state(uint32 id, socket_info_t::connection_state_t new_state)
{
	book_state_change(list[id]->state, -1);
	list[id]->state = new_state;
	list[id]->player_unlocked = 0;
	book_state_change(list[id]->state, +1);
}


void socket_list_t::reset()
{
	FOR(vector_tpl<socket_info_t*>, const i, list) {
		i->reset();
	}
	connected_clients = 0;
	playing_clients = 0;
	server_sockets = 0;
}


void socket_list_t::reset_clients()
{
	for(uint32 j=server_sockets; j<list.get_count(); j++) {
		list[j]->reset();
	}
	connected_clients = 0;
	playing_clients = 0;
}


void socket_list_t::add_client( SOCKET sock, uint32 ip )
{
	dbg->message("socket_list_t::add_client", "add client socket[%d] at address %xd", sock, ip);
	uint32 i = list.get_count();
	// check whether socket already added
	for(  uint32 j=server_sockets;  j<list.get_count();  j++  ) {
		if(  list[j]->socket == sock  &&  list[j]->state != socket_info_t::inactive  ) {
			return;
		}
		if(  list[j]->state == socket_info_t::inactive  &&  i == list.get_count()  ) {
			i = j;
		}
	}
	if(  i == list.get_count()  ) {
		list.append(new socket_info_t() );
	}
	list[i]->socket = sock;
	list[i]->address = net_address_t(ip, 0);
	change_state( i, socket_info_t::connected );

	network_set_socket_nodelay( sock );
}


void socket_list_t::add_server( SOCKET sock )
{
	dbg->message("socket_list_t::add_server", "add server socket[%d]", sock);
	assert(connected_clients==0  &&  playing_clients==0);
	uint32 i = server_sockets;
	// check whether socket already added
	for(uint32 j=0; j<server_sockets; j++) {
		if (list[j]->socket == sock  &&  list[j]->state == socket_info_t::server) {
			return;
		}
		if (list[j]->state == socket_info_t::inactive  &&  i == server_sockets) {
			i = j;
		}
	}
	if (i == server_sockets) {
		list.insert_at(server_sockets, new socket_info_t());
		server_sockets++;
	}
	list[i]->socket = sock;
	change_state(i, socket_info_t::server);
	if (i==0) {
#ifndef NETTOOL
		// set server nickname
		if (!env_t::nickname.empty()) {
			list[i]->nickname = env_t::nickname.c_str();
		}
		else {
			list[i]->nickname = "Server#0";
			env_t::nickname = list[i]->nickname.c_str();
		}
#endif
	}

	network_set_socket_nodelay( sock );
}


bool socket_list_t::remove_client( SOCKET sock )
{
	dbg->message("socket_list_t::remove_client", "remove client socket[%d]", sock);
	for(uint32 j=0; j<list.get_count(); j++) {
		if (list[j]->socket == sock) {

#ifdef NETTOOL
			if (list[j]->state == socket_info_t::playing) {
#else
			if (env_t::server  &&  list[j]->state == socket_info_t::playing) {
#endif
				change_state(j, socket_info_t::has_left);
			}
			else {
				change_state(j, socket_info_t::inactive);
			}
			list[j]->reset();

			network_close_socket(sock);
			return true;
		}
	}
	return false;
}


bool socket_list_t::has_client( SOCKET sock )
{
	return get_client_id(sock) < list.get_count();
}


uint32 socket_list_t::get_client_id( SOCKET sock ){
	for(uint32 j=0; j<list.get_count(); j++) {
		if (list[j]->state != socket_info_t::inactive  &&  list[j]->socket == sock) {
			return j;
		}
	}
	return list.get_count();
}


void socket_list_t::unlock_player_all(uint8 player_nr, bool unlock, uint32 except_client)
{
// nettool does not know about nwc_auth_player_t
#ifndef NETTOOL
	for(uint32 i=0; i<list.get_count(); i++) {
		if (i!=except_client  &&  (i==0  ||  list[i]->state == socket_info_t::playing) ) {
			uint16 old_player_unlocked = list[i]->player_unlocked;
			if (unlock) {
				list[i]->unlock_player(player_nr);
			}
			else {
				list[i]->lock_player(player_nr);
			}
			if (old_player_unlocked != list[i]->player_unlocked) {
				dbg->warning("socket_list_t::unlock_player_all", "old = %d  new = %d  id = %d", old_player_unlocked, list[i]->player_unlocked, i);
				// tell the player
				nwc_auth_player_t *nwc = new nwc_auth_player_t();
				nwc->player_unlocked = list[i]->player_unlocked;
				if (i==0) {
					network_send_server(nwc);
				}
				else {
					nwc->send(list[i]->socket);
					delete nwc;
				}
			}
		}
	}
#else
	(void) player_nr;
	(void) unlock;
	(void) except_client;
#endif
}


void socket_list_t::send_all(network_command_t* nwc, bool only_playing_clients, uint8 player_nr)
{
	if (nwc == NULL) {
		return;
	}
	for(uint32 i=server_sockets; i<list.get_count(); i++) {
		if (list[i]->is_active()  &&  list[i]->socket!=INVALID_SOCKET
			&&  (!only_playing_clients  ||  list[i]->state == socket_info_t::playing  ||  list[i]->state == socket_info_t::connected)
			&&  (player_nr >= PLAYER_UNOWNED  ||  list[i]->is_player_unlocked(player_nr))
		) {

			packet_t *p = nwc->copy_packet();
			list[i]->send_queue_append(p);
		}
	}
}


SOCKET socket_list_t::fill_set(fd_set *fds)
{
	SOCKET s_max = 0;
	FOR(vector_tpl<socket_info_t*>, const i, list) {
		if (i->state != socket_info_t::inactive && i->socket != INVALID_SOCKET) {
			SOCKET const s = i->socket;
			s_max = max( s, s_max );
			FD_SET( s, fds );
		}
	}
	return s_max+1;
}


SOCKET socket_list_t::fd_isset(fd_set *fds, bool use_server_sockets, uint32 *offset)
{
	const uint32 begin = offset ? *offset : (use_server_sockets ? 0 : server_sockets);
	const uint32 end   = use_server_sockets ? server_sockets : list.get_count();

	for(uint32 i=begin; i<end; i++) {
		const SOCKET socket = list[i]->socket;
		if (socket!=INVALID_SOCKET) {
			if(  FD_ISSET(socket, fds)  ) {
				if (offset) {
					*offset = i+1;
				}
				return socket;
			}
		}
	}
	if (offset) {
		*offset = end;
	}
	return INVALID_SOCKET;
}


socket_list_t::socket_iterator_t::socket_iterator_t(fd_set *fds)
{
	index = 0;
	this->fds = fds;
	current = INVALID_SOCKET;
}


bool socket_list_t::server_socket_iterator_t::next()
{
	current = socket_list_t::fd_isset(fds, true, &index);
	return current != INVALID_SOCKET;
}


bool socket_list_t::client_socket_iterator_t::next()
{
	current = socket_list_t::fd_isset(fds, false, &index);
	return current != INVALID_SOCKET;
}


void socket_list_t::rdwr(packet_t *p, vector_tpl<socket_info_t*> *list)
{
	assert(p->is_saving()  ||  list!=&socket_list_t::list);
	uint32 count = list->get_count();
	p->rdwr_long(count);
	for(uint32 i=0; i<count; i++) {
		if (p->is_loading()) {
			list->append(new socket_info_t());
		}

		uint8 s = (*list)[i]->state;
		p->rdwr_byte(s);
		(*list)[i]->state = socket_info_t::connection_state_t(s);

		if ( s==socket_info_t::playing) {
			(*list)[i]->rdwr(p);
		}
	}
}
