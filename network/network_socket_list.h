/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_NETWORK_SOCKET_LIST_H
#define NETWORK_NETWORK_SOCKET_LIST_H


#include "network.h"
#include "network_address.h"
#include "../simconst.h"
#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../utils/plainstring.h"

class network_command_t;
class packet_t;


/**
 * Class to store pairs of (address, nickname) for logging and admin purposes.
 */
class connection_info_t {
public:
	/// address of connection
	net_address_t address;

	/// client nickname
	plainstring nickname;

	connection_info_t() : address(), nickname() {}

	connection_info_t(const connection_info_t& other) : address(other.address), nickname(other.nickname) {}

	template<class F> void rdwr(F *packet)
	{
		address.rdwr(packet);
		packet->rdwr_str(nickname);
	}

	bool operator==(const connection_info_t& other) const;

	bool operator!=(const connection_info_t& other) const { return !(*this == other); }
};


class socket_info_t : public connection_info_t
{
public:
	enum connection_state_t
	{
		inactive  = 0, ///< client disconnected
		server    = 1, ///< server socket
		connected = 2, ///< connection established but client does not participate in the game yet
		playing   = 3, ///< client actively plays
		has_left  = 4, ///< was playing but left
		admin     = 5  ///< admin connection
	};

private:
	packet_t *packet;
	slist_tpl<packet_t *> send_queue;

public:
	connection_state_t state;
	SOCKET socket;
	uint16 player_unlocked;

public:
	socket_info_t() : connection_info_t(), packet(0), send_queue(), state(inactive), socket(INVALID_SOCKET), player_unlocked(0) {}

	~socket_info_t();

	/**
	 * marks all information as invalid
	 * closes socket, deletes packet
	 */
	void reset();

	bool is_active() const { return state != inactive; }

	/**
	 * receive the next command: continues receiving the packet
	 * if an error occurs while receiving the packet, (this) is reset
	 * @return the command if packet is fully received
	 */
	network_command_t* receive_nwc();

	/**
	 *
	 */
	void process_send_queue();

	void send_queue_append(packet_t *p);

	/**
	 * rdwr client information to packet
	 */
	void rdwr(packet_t *p);

	/**
	 * human players on this connection can play with in-game companies/players?
	 */
	bool is_player_unlocked(uint8 player_nr) const { return (player_nr < PLAYER_UNOWNED)  &&  ((player_unlocked & 1<<player_nr)!=0); }

	void unlock_player(uint8 player_nr) { if (player_nr < PLAYER_UNOWNED) player_unlocked |= 1<<player_nr; }
	void lock_player(uint8 player_nr) { if (player_nr < PLAYER_UNOWNED) player_unlocked &= ~(1<<player_nr); }
};

/**
 * static class that administers the list of client / server sockets
 */
class socket_list_t {
private:
	/**
	 * the list to hold'em'all
	 */
	static vector_tpl<socket_info_t*>list;

	static uint32 connected_clients;
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
	static void add_client( SOCKET sock, uint32 ip = 0 );

	/**
	 * @ returns true if socket is already in our list
	 */
	static bool has_client( SOCKET sock );

	/**
	 * @return true if client was found and removed
	 */
	static bool remove_client( SOCKET sock );

	static uint32 get_client_id( SOCKET sock );

	static bool is_valid_client_id( uint32 client_id ) {
		return client_id < list.get_count();
	}

	uint32 static get_count() { return list.get_count(); }

	static SOCKET get_socket( uint32 client_id ) {
		return client_id < list.get_count()  &&  list[client_id]->state != socket_info_t::inactive
			? list[client_id]->socket : INVALID_SOCKET;
	}

	static socket_info_t& get_client(uint32 client_id ) {
		assert (client_id < list.get_count());
		return *list[client_id];
	}

	/**
	 * unlocks/locks player for all clients, except client number except_client
	 */
	static void unlock_player_all(uint8 player_nr, bool unlock, uint32 except_client = list.get_count());

	/**
	 * send command to all clients
	 * @param only_playing_clients if true then send only to playing clients
	 * @param player_nr if != PLAYER_UNOWNED then only send to clients with this player unlocked
	 */
	static void send_all(network_command_t* nwc, bool only_playing_clients, uint8 player_nr = PLAYER_UNOWNED);

	static void change_state(uint32 id, socket_info_t::connection_state_t new_state);

	/**
	 * rdwr client-list information to packet
	 */
	static void rdwr(packet_t *p, vector_tpl<socket_info_t*> *writeto=&list);

private:
	static void book_state_change(socket_info_t::connection_state_t state, sint8 incr);

public: // from now stuff to deal with fd_set's

	/**
	 * @param offset pointer to an offset
	 * @return the first client whose bit is set in fd_set
	 */
	static SOCKET fd_isset(fd_set *fds, bool use_server_sockets, uint32 *offset=NULL);

	/**
	 * fill set with all active sockets
	 */
	static SOCKET fill_set(fd_set *fds);

	/**
	 * iterators to iterate through all sockets whose bits are set in fd_set
	 */
	class socket_iterator_t {
	protected:
		uint32 index; // index to the socket list
		fd_set *fds;  // fds as modified by select(), be sure it gets not deleted while the iterator is alive
		SOCKET current;
	public:
		socket_iterator_t(fd_set *fds);
		SOCKET get_current() const { return current; }
	};

	/**
	 * .. iterate through server sockets
	 */
	class server_socket_iterator_t : public socket_iterator_t {
	public:
		server_socket_iterator_t(fd_set *fds) : socket_iterator_t(fds) {}
		bool next();
	};
	/**
	 * .. and client sockets
	 */
	class client_socket_iterator_t : public socket_iterator_t {
	public:
		client_socket_iterator_t(fd_set *fds) : socket_iterator_t(fds) { index = server_sockets; }
		bool next();
	};
};
#endif
