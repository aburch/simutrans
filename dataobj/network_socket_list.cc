#include "network_socket_list.h"
#include "network_cmd.h"
#include "umgebung.h"

/**
 * list: contains _all_ sockets
 *       with the convention that all server_sockets are at indices 0..server_sockets-1
 */
vector_tpl<socket_info_t>socket_list_t::list(20);

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
void socket_list_t::book_state_change(uint8 state, sint8 incr)
{
	switch (state) {
		case socket_info_t::server:
			// do not change
			break;
		case socket_info_t::connected:
			connected_clients += incr;
			break;
		case socket_info_t::playing:
			playing_clients += incr;
			break;
		case socket_info_t::inactive:
		default:
			break;
	}
}
void socket_list_t::change_state(uint32 id, uint8 new_state)
{
	book_state_change(list[id].state, -1);
	list[id].state = new_state;
	book_state_change(list[id].state, +1);
}

void socket_list_t::reset()
{
	for(uint32 j=0; j<list.get_count(); j++) {
		list[j].reset();
	}
	connected_clients = 0;
	playing_clients = 0;
	server_sockets = 0;
}

void socket_list_t::reset_clients()
{
	for(uint32 j=server_sockets; j<list.get_count(); j++) {
		list[j].reset();
	}
	connected_clients = 0;
	playing_clients = 0;
}

void socket_list_t::add_client( SOCKET sock )
{
	uint32 i = list.get_count();
	// check whether socket already added
	for(uint32 j=server_sockets; j<list.get_count(); j++) {
		if (list[j].socket == sock  &&  list[j].state != socket_info_t::inactive) {
			return;
		}
		if (list[j].state == socket_info_t::inactive  &&  i == list.get_count()) {
			i = j;
		}
	}
	if (i == list.get_count()) {
		list.append(socket_info_t());
	}
	list[i].socket = sock;
	change_state(i, socket_info_t::connected);

	network_set_socket_nodelay( sock );
}

void socket_list_t::add_server( SOCKET sock )
{
	assert(connected_clients==0  &&  playing_clients==0);
	uint32 i = server_sockets;
	// check whether socket already added
	for(uint32 j=0; j<server_sockets; j++) {
		if (list[j].socket == sock  &&  list[j].state == socket_info_t::server) {
			return;
		}
		if (list[j].state == socket_info_t::inactive  &&  i == server_sockets) {
			i = j;
		}
	}
	if (i == server_sockets) {
		list.insert_at(server_sockets, socket_info_t());
		server_sockets++;
	}
	list[i].socket = sock;
	change_state(i, socket_info_t::server);

	network_set_socket_nodelay( sock );
}


bool socket_list_t::remove_client( SOCKET sock )
{
	dbg->message("socket_list_t::remove_client", "remove client socket[%d]", sock);
	for(uint32 j=0; j<list.get_count(); j++) {
		if (list[j].socket == sock) {
			change_state(j, socket_info_t::inactive);
			list[j].reset();

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
		if (list[j].state != socket_info_t::inactive  &&  list[j].socket == sock) {
			return j;
		}
	}
	return list.get_count();
}

void socket_list_t::send_all(network_command_t* nwc, bool only_playing_clients)
{
	for(uint32 i=server_sockets; i<list.get_count(); i++) {
		if (list[i].is_active()  &&  list[i].socket!=INVALID_SOCKET
			&&  (!only_playing_clients  ||  list[i].state == socket_info_t::playing)) {
			nwc->send(list[i].socket);
		}
	}
}


int socket_list_t::fill_set(fd_set *fds)
{
	int s_max = 0;
	for(uint32 i=0; i<list.get_count(); i++) {
		if (list[i].state != socket_info_t::inactive  &&  list[i].socket!=INVALID_SOCKET) {
			SOCKET s = list[i].socket;
			s_max = max( (int)s, (int)s_max );
			FD_SET( s, fds );
		}
	}
	return s_max+1;
}


SOCKET socket_list_t::fd_isset(fd_set *fds, bool use_server_sockets)
{
	const uint32 begin = use_server_sockets ? 0              : server_sockets;
	const uint32 end   = use_server_sockets ? server_sockets : list.get_count();

	for(uint32 i=begin; i<end; i++) {
		const SOCKET socket = list[i].socket;
		if (socket!=INVALID_SOCKET) {
			if(  FD_ISSET(socket, fds)  ) {
				return socket;
			}
		}
	}
	return INVALID_SOCKET;
}
