#ifndef _NETWORK_SOCKET_LIST_H_
#define _NETWORK_SOCKET_LIST_H_

#include "network.h"
#include "../tpl/vector_tpl.h"

class network_command_t;

class socket_info_t {
public:
	enum {
		inactive	= 0, // client disconnected
		server		= 1, // server socket
		connected	= 2, // connection established but client does not participate in the game yet
		playing		= 3  // client actively plays
	};
	uint8 state;

	SOCKET socket;

	socket_info_t() : state(inactive), socket(INVALID_SOCKET) {}

	/**
	 * marks all information as invalid
	 * closes socket
	 */
	void reset() {
		if (is_active()) {
			network_close_socket(socket);
		}
		state = inactive;
		socket = INVALID_SOCKET;
	}

	bool is_active() const { return state != inactive; }
};

/**
 * static class that administrates the list of client / server sockets
 */
class socket_list_t {
private:
	static vector_tpl<socket_info_t>list;
	static uint32 connected_clients;;
	static uint32 playing_clients;
	static uint32 server_sockets;

public:
	static uint32 get_server_sockets() { return server_sockets; }
	static uint32 get_connected_clients() { return connected_clients; }
	static uint32 get_playing_clients() { return playing_clients; }

	/**
	 * clears list, closes all sockets
	 */
	static void reset();
	/**
	 * clears and closes all client sockets
	 */
	static void reset_clients();

	/**
	 * adds server socket
	 * assumes that no active client sockets are present
	 */
	static void add_server( SOCKET sock );

	/**
	 * server: adds client socket (ie connection to client)
	 * client: adds client socket (ie connection to server)
	 */
	static void add_client( SOCKET sock );

	/**
	 * @ returns true if socket is already in our list
	 */
	static bool has_client( SOCKET sock );

	/**
	 * @returns true if client was found and removed
	 */
	static bool remove_client( SOCKET sock );

	static uint32 get_client_id( SOCKET sock );

	static SOCKET get_socket( uint32 client_id ) {
		return client_id < list.get_count()  &&  list[client_id].state != socket_info_t::inactive
			? list[client_id].socket : INVALID_SOCKET;
	}

	static const socket_info_t& get_client(uint32 client_id ) {
		assert(client_id < list.get_count());
		return list[client_id];
	}

	/**
	 * @returns the first client whose bit is set in fd_set
	 */
	static SOCKET fd_isset(fd_set *fds, bool use_server_sockets);

	/**
	 * fill set with all active sockets
	 */
	static int fill_set(fd_set *fds);

	/**
	 * @returns for client returns socket of connection to server
	 */
	static SOCKET get_server_connection_socket() {
		return get_socket(0);
	}

	static void send_all(network_command_t* nwc, bool only_playing_clients);

	static void change_state(uint32 id, uint8 new_state);

private:
	static void book_state_change(uint8 state, sint8 incr);
};
#endif
