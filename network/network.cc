/* basic network functionality, borrowed from OpenTTD */

#if defined(__amiga__)
// warning: IPv6 will only work on Windows XP and up ...
#define USE_IP4_ONLY
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <string>

#include "network.h"
#include "network_address.h"
#include "network_packet.h"
#include "network_socket_list.h"
#include "network_cmd.h"
#include "network_cmd_ingame.h"
#include "network_cmp_pakset.h"
#include "../simconst.h"

#ifndef NETTOOL
#include "../dataobj/environment.h"
#endif

#ifdef NETTOOL
#include "../tpl/vector_tpl.h"
#endif

#include "../utils/simstring.h"
#include "../tpl/slist_tpl.h"

static bool network_active = false;
uint16 network_server_port = 0;

// list of received commands
static slist_tpl<network_command_t *> received_command_queue;

// blacklist
address_list_t blacklist;

void clear_command_queue()
{
	while(!received_command_queue.empty()) {
		network_command_t *nwc = received_command_queue.remove_first();
		delete nwc;
	}
}

#if USE_WINSOCK
#define RET_ERR_STR { DWORD errnr = WSAGetLastError(); if( errnr!=0 ) FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM,NULL,errnr,MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),err_str,sizeof(err_str),NULL); err = err_str; return INVALID_SOCKET; }
#else
#define RET_ERR_STR { err = err_str; return INVALID_SOCKET; }
#endif


// global client id
static uint32 client_id;

void network_set_client_id(uint32 id)
{
	client_id = id;
}

uint32 network_get_client_id()
{
	return client_id;
}


/**
 * Initializes the network core (as that is needed for some platforms
 * @return true if the core has been initialized, false otherwise
 */
static bool network_initialize()
{
	if(!network_active) {
		socket_list_t::reset();

#if USE_WINSOCK
		/* Let's load the network in windows */
		WSADATA wsa;
		if(int err = WSAStartup( MAKEWORD(2, 2), &wsa)) {
			dbg->warning("NetworkInitialize()","failed loading windows socket library with %i", err);
			return false;
		}
#endif
	}
	network_active = true;
	return true;
}


/**
 * Open a socket connected to a remote address
 * In case of failure err is populated with a meaningful error
 * @return a valid socket or INVALID_SOCKET if the connection fails
 */
SOCKET network_open_address(char const* cp, char const*& err)
{
	err = NULL;

	if (!network_initialize()) {
		err = "Cannot init network!";
		return INVALID_SOCKET;
	}

#ifdef USE_IP4_ONLY
	// Network load. Address format e.g.: "128.0.0.1:13353"
	char address[1024];
	static char err_str[256];
	uint16 port = 13353;
	const char *cp2 = strrchr(cp,':');
	if(cp2!=NULL) {
		port=atoi(cp2+1);
		// Copy the address part
		tstrncpy(address,cp,cp2-cp>sizeof(address)-1?sizeof(address)-1:cp2-cp+1);
		cp = address;
	}

	struct sockaddr_in server_name;
	memset(&server_name,0,sizeof(server_name));
	server_name.sin_family=AF_INET;
#if USE_WINSOCK
	bool ok = true;
	server_name.sin_addr.s_addr = inet_addr(cp);	// for windows we must first try to resolve the number
	if((int)server_name.sin_addr.s_addr==-1) {// Bad address
		ok = false;
		struct hostent *theHost;
		theHost = gethostbyname( cp );	// ... before resolving a name ...
		if(theHost) {
			server_name.sin_addr = *(struct in_addr *)theHost->h_addr_list[0];
			ok = true;
		}
	}
	if(!ok) {
#else
	/* inet_anon does not work on BeOS; but since gethostbyname() can
	 * do this job on all other systems too, we use it only:
	 * instead of if(inet_aton(cp,&server_name.sin_addr)==0) { // Bad address
	 */
	struct hostent *theHost;
	theHost = gethostbyname( cp );
	if(theHost) {
		server_name.sin_addr = *(struct in_addr *)theHost->h_addr_list[0];
	}
	else {// Bad address
#endif
		sprintf( err_str, "Bad address %s", cp );
		RET_ERR_STR;
	}
	server_name.sin_port=htons(port);

	SOCKET my_client_socket = socket(AF_INET,SOCK_STREAM,0);
	if(my_client_socket==INVALID_SOCKET) {
		err = "Cannot create socket";
		return INVALID_SOCKET;
	}

	if (connect(my_client_socket, (struct sockaddr*)&server_name, sizeof(server_name)) == -1) {
		sprintf(err_str, "Could not connect to %s", cp);
		RET_ERR_STR;
	}

#else

	// Address format e.g.: "example.com:13353", "128.0.0.1:13353" or "[::1]:80"
	// this should be replaced with a URI parser...
	static char err_str[256];

	// scan address string for key features
	const char *cp2 = strrchr( cp, ':' );
	const char *ipv6end = strrchr( cp, ']' );
	const char *ipv6start = strchr( cp, '[' );

	// extract port if available
	std::string cpport;
	if (  cp2 != NULL  &&  cp2 > ipv6end  ) {
		cpport = std::string(cp2 + 1);
	}
	else {
		cpport = std::string("13353");
		cp2 = strrchr(cp, '\0');
	}

	// remove brackets from IPv6 addresses
	const bool ipv6exists = ipv6start != NULL  &&  ipv6end != NULL;
	const char *const start = ipv6exists ? ipv6start + 1 : cp;
	const char *const end = ipv6exists ? ipv6end : cp2;

	// extract address
	const size_t addresslen = size_t(end - start);
	std::string const cpaddress(start, addresslen);

	SOCKET my_client_socket = INVALID_SOCKET;

	bool connected = false;

#ifdef NETTOOL
	// Nettool doesn't have env, so fake it
	vector_tpl<std::string> ips;
	ips.append_unique("::");
	ips.append_unique("0.0.0.0");
#else
	vector_tpl<std::string> const& ips = env_t::listen;
#endif
	// For each address in the list of listen addresses try and create a socket to transmit on
	// Use the first one which works
	for (uint i = 0; !connected && i != ips.get_count(); ++i) {
		std::string const& ip = ips[i];

		int ret;

		// Set up remote addrinfo
		struct addrinfo *remote;
		struct addrinfo remote_hints;
		memset( &remote_hints, 0, sizeof( struct addrinfo ) );
		remote_hints.ai_socktype = SOCK_STREAM;
		remote_hints.ai_family = PF_UNSPEC;

		// Test remote address to ensure it is valid
		// Fill out remote address structure
		if (  (ret = getaddrinfo(cpaddress.c_str(), cpport.c_str(), &remote_hints, &remote)) != 0) {
			sprintf( err_str, "Bad address %s", cp );
			RET_ERR_STR;
		}

		// Set up local addrinfo
		struct addrinfo *local;
		struct addrinfo local_hints;
		memset( &local_hints, 0, sizeof( struct addrinfo ) );
		local_hints.ai_socktype = SOCK_STREAM;
		local_hints.ai_family = PF_UNSPEC;

		// Insert listen address into local_hints struct to influence local address to bind to
		DBG_MESSAGE( "network_open_address()", "Preparing to bind address: %s", ip.c_str() );
		if (  (ret = getaddrinfo( ip.c_str(), 0, &local_hints, &local )) != 0  ) {
			dbg->warning( "network_open_address()", "Failed to getaddrinfo for %s, error was: %s", ip.c_str(), gai_strerror(ret) );
#ifndef NETTOOL
			env_t::listen.remove_at( i );
#endif
			i --;
			continue;
		}

		// Now try and open a socket to communicate with our remote endpoint
		struct addrinfo *walk_local;
		for(  walk_local = local;  !connected  &&  walk_local != NULL;  walk_local = walk_local->ai_next  ) {
			char ipstr_local[INET6_ADDRSTRLEN];

			// Validate address + get string representation for logging
			if (  (ret = getnameinfo( (walk_local->ai_addr), (socklen_t)walk_local->ai_addrlen, ipstr_local, sizeof(ipstr_local), NULL, 0, NI_NUMERICHOST )) !=0  ) {
				dbg->warning( "network_open_address()", "Call to getnameinfo() failed with error: \"%s\"", gai_strerror(ret) );
				continue;
			}

			DBG_MESSAGE( "network_open_address()", "Potential local address: %s", ipstr_local );

			my_client_socket = socket( walk_local->ai_family, walk_local->ai_socktype, walk_local->ai_protocol );

			if (  my_client_socket == INVALID_SOCKET  ) {
				DBG_MESSAGE( "network_open_address()", "Could not create socket! Error: \"%s\"", strerror(GET_LAST_ERROR()) );
				continue;
			}

			// Bind socket to local IP
			if (  bind( my_client_socket, walk_local->ai_addr, walk_local->ai_addrlen )  ) {
				DBG_MESSAGE( "network_open_address()", "Unable to bind socket to local IP address! Error: \"%s\"", strerror(GET_LAST_ERROR()) );
				network_close_socket( my_client_socket );
				my_client_socket = INVALID_SOCKET;
				continue;
			}

			// For each address in remote, try and connect
			struct addrinfo *walk_remote;
			for (  walk_remote = remote;  !connected  &&  walk_remote != NULL;  walk_remote = walk_remote->ai_next  ) {
  				char ipstr_remote[INET6_ADDRSTRLEN];

				// Validate remote address + get string representation for logging
				if (  (ret = getnameinfo( walk_remote->ai_addr, (socklen_t)walk_remote->ai_addrlen, ipstr_remote, sizeof(ipstr_remote), NULL, 0, NI_NUMERICHOST )) !=0  ) {
					dbg->warning( "network_open_address()", "Call to getnameinfo() failed with error: \"%s\"", gai_strerror(ret) );
					continue;
				}

				DBG_MESSAGE( "network_open_address()", "Potential remote address: %s", ipstr_remote );

				if (  connect( my_client_socket, walk_remote->ai_addr, (socklen_t)walk_remote->ai_addrlen ) != 0  ) {
					DBG_MESSAGE( "network_open_address()", "Could not connect using this socket. Error: \"%s\"", strerror(GET_LAST_ERROR()) );
					continue;
				}
				connected = true;
			}
			// If no connection throw away this socket and try the next one
			if (  !connected  ) {
				network_close_socket( my_client_socket );
				my_client_socket = INVALID_SOCKET;
			}
		}

		freeaddrinfo( local );
		freeaddrinfo( remote );
	}

	if (  my_client_socket == INVALID_SOCKET  ) {
		sprintf( err_str, "Could not connect to %s", cp );
		RET_ERR_STR;
	}
#endif
	return my_client_socket;
}


/**
 * Start up server on the port specified
 * Server will listen on all addresses specified in env_t::listen
 * @return true on success
 */
bool network_init_server( int port )
{
	// First activate network
	if (  !network_initialize()  ) {
		dbg->fatal( "network_init_server()", "Failed to initialize network!" );
	}
	socket_list_t::reset();

#ifdef USE_IP4_ONLY

	SOCKET my = socket( PF_INET, SOCK_STREAM, 0 );
	if (  my == INVALID_SOCKET  ) {
		dbg->fatal( "network_init_server()", "Failed to open socket!" );
		return false;
	}

	struct sockaddr_in name;
	name.sin_family = AF_INET;
	name.sin_port = htons( port );
	name.sin_addr.s_addr = htonl( INADDR_ANY );

	if (  bind( my, (struct sockaddr *)&name, sizeof( name ) ) == -1  ) {
		dbg->fatal( "network_init_server()", "Bind failed!" );
		return false;
	}

	/* Max pending connections */
	if (  listen( my, MAX_PLAYER_COUNT ) == -1  ) {
		dbg->fatal( "network_init_server()", "Listen failed for %i sockets!", MAX_PLAYER_COUNT );
		return false;
	}

	socket_list_t::add_server( my );

#else

#ifdef NETTOOL
	// Nettool doesn't have env, so fake it
	vector_tpl<std::string> ips;
	ips.append_unique("::");
	ips.append_unique("0.0.0.0");
#else
	vector_tpl<std::string> const& ips = env_t::listen;
#endif
	// For each address in the list of listen addresses try and create a socket to listen on
	FOR(vector_tpl<std::string>, const& ip, ips) {
		int ret;
		char port_nr[16];
		sprintf( port_nr, "%u", port );

		struct addrinfo *server;
		struct addrinfo hints;
		memset( &hints, 0, sizeof( struct addrinfo ) );
		hints.ai_socktype = SOCK_STREAM;

		// Insert potential listen address into hints struct to influence local address to bind to
		DBG_MESSAGE( "network_init_server()", "Preparing to bind address: \"%s\"", ip.c_str() );
		hints.ai_family = PF_UNSPEC;
		if (  (ret = getaddrinfo( ip.c_str(), port_nr, &hints, &server )) != 0  ) {
			dbg->fatal( "network_init_server()", "Call to getaddrinfo() failed for: \"%s\", error was: \"%s\" - check listen directive in simuconf.tab!", ip.c_str(), gai_strerror(ret) );
		}
		else {
			dbg->message( "network_init_server()", "Attempting to bind listening sockets for: \"%s\"\n", ip.c_str() );
		}

		SOCKET server_socket;
		struct addrinfo *walk;

		// Open a listen socket for each IP address specified by this entry in the listen list
		for (  walk = server;  walk != NULL;  walk = walk->ai_next  ) {
			char ipstr[INET6_ADDRSTRLEN];

			// Validate address + get string representation for logging
			if (  (ret = getnameinfo( walk->ai_addr, (socklen_t)walk->ai_addrlen, ipstr, sizeof(ipstr), NULL, 0, NI_NUMERICHOST )) != 0  ) {
				dbg->warning( "network_init_server()", "Call to getnameinfo() failed with error: \"%s\"", gai_strerror(ret) );
				continue;
			}

			DBG_MESSAGE( "network_init_server()", "Potential bind address: %s", ipstr );

			server_socket = socket( walk->ai_family, walk->ai_socktype, walk->ai_protocol );

			if (  server_socket == INVALID_SOCKET  ) {
				dbg->warning( "network_init_server()", "Could not create socket! Error: \"%s\"", strerror(GET_LAST_ERROR()) );
				continue;
			}

			/* Disable IPv4-mapped IPv6 addresses for this IPv6 listen socket
			   This ensures that we are using separate sockets for dual-stack, one for v4, one for v6 */
			if (  walk->ai_family == AF_INET6  ) {
				int const on = 1;
				if (  setsockopt(server_socket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char *>(&on), sizeof(on)) != 0  ) {
					dbg->warning( "network_init_server()", "Call to setsockopt(IPV6_V6ONLY) failed for: \"%s\", error was: \"%s\"", ip.c_str(), strerror(GET_LAST_ERROR()) );
					network_close_socket( server_socket );
					server_socket = INVALID_SOCKET;
					continue;
				}
			}
			// Enable reusing of local addresses
			int const enable = 1;
			if (  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&enable), sizeof(enable)) != 0  ) {
				dbg->warning( "network_init_server()", "Call to setsockopt(SO_REUSEADDR) failed for: \"%s\", error was: \"%s\"", ip.c_str(), strerror(GET_LAST_ERROR()) );
			}

			if (  bind( server_socket, walk->ai_addr, walk->ai_addrlen )  ) {
				/* Unable to bind a socket - abort execution as we are supposed to be a server on this interface */
				dbg->fatal( "network_init_server()", "Unable to bind socket to IP address: \"%s\", error was: \"%s\"", ipstr, strerror(GET_LAST_ERROR()) );
			}

			if (  listen( server_socket, 32 ) == -1  ) {
				/* Unable to listen on bound socket - abort execution as we are supposed to be a server on this interface */
				dbg->fatal( "network_init_server()", "Unable to set socket to listen for incoming connections on: \"%s\"", ipstr );
			}

			dbg->message( "network_init_server()", "Added valid listen socket for address: \"%s\"\n", ipstr);
			socket_list_t::add_server( server_socket );
		}
		freeaddrinfo( server );
	}

	// Fatal error if no server sockets could be opened, since we're supposed to be a server!
	if (  socket_list_t::get_server_sockets() == 0  ) {
		dbg->fatal( "network_init_server()", "Unable to add any server sockets!" );
	}
	else {
		printf("Server started, added %d server sockets\n", socket_list_t::get_server_sockets());
	}

#endif

	network_server_port = port;
	client_id = 0;

	network_reset_server();
	return true;
}


void network_set_socket_nodelay( SOCKET sock )
{
#if (defined(TCP_NODELAY)  ||  COLOUR_DEPTH == 0)
	// do not wait to join small (command) packets when sending (may cause 200ms delay!)
	// force this for dedicated servers
	int b = 1;
	setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&b, sizeof(b) );
#else
#warning TCP_NODELAY not defined. Expect multiplayer problems.
	(void)sock;
#endif
}


network_command_t* network_get_received_command()
{
	if (!received_command_queue.empty()) {
		return received_command_queue.remove_first();
	}
	return NULL;
}


/* do appropriate action for network games:
 * - server: accept connection to a new client
 * - all: receive commands and puts them to the received_command_queue
 */
network_command_t* network_check_activity(world_t *, int timeout)
{
	fd_set fds;
	FD_ZERO(&fds);

	socket_list_t::fill_set(&fds);

	// time out: MAC complains about too long timeouts
	struct timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000ul;

	int action = select( FD_SETSIZE, &fds, NULL, NULL, &tv );
	if(  action<=0  ) {
		// timeout: return command from the queue
		return network_get_received_command();
	}

	// accept new connection
	socket_list_t::server_socket_iterator_t iter_s(&fds);
	while(iter_s.next()) {
		SOCKET accept_sock = iter_s.get_current();

		if(  accept_sock!=INVALID_SOCKET  ) {
			struct sockaddr_in client_name;
			socklen_t size = sizeof(client_name);
			SOCKET s = accept(accept_sock, (struct sockaddr *)&client_name, &size);
			if(  s!=INVALID_SOCKET  ) {
#if USE_WINSOCK
				uint32 ip = ntohl((uint32)client_name.sin_addr.S_un.S_addr);
#else
				uint32 ip = ntohl((uint32)client_name.sin_addr.s_addr);
#endif
				if (blacklist.contains(net_address_t( ip ))) {
					// refuse connection
					network_close_socket(s);
					continue;
				}
#ifdef  __BEOS__
				char name[256];
				sprintf(name, "%lh", client_name.sin_addr.s_addr );
#else
				const char *name = inet_ntoa(client_name.sin_addr);
#endif
				dbg->message("check_activity()", "Accepted connection from: %s.",  name);
				socket_list_t::add_client(s, ip);
			}
		}
	}

	// receive from clients
	socket_list_t::client_socket_iterator_t iter_c(&fds);
	while(iter_c.next()) {
		SOCKET sender = iter_c.get_current();

		if (sender != INVALID_SOCKET  &&  socket_list_t::has_client(sender)) {
			uint32 client_id = socket_list_t::get_client_id(sender);
			network_command_t *nwc = socket_list_t::get_client(client_id).receive_nwc();
			if (nwc) {
				received_command_queue.append(nwc);
				dbg->warning( "network_check_activity()", "received cmd id=%d %s from socket[%d]", nwc->get_id(), nwc->get_name(), sender );
			}
			// errors are caught and treated in socket_info_t::receive_nwc
		}
	}
	return network_get_received_command();
}


void network_process_send_queues(int timeout)
{
	fd_set fds;
	FD_ZERO(&fds);

	socket_list_t::fill_set(&fds);

	// time out
	struct timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000ul;

	int action = select( FD_SETSIZE, NULL, &fds, NULL, &tv );

	if(  action<=0  ) {
		// timeout: return
		return;
	}

	// send to clients
	socket_list_t::client_socket_iterator_t iter_c(&fds);
	while(iter_c.next()  &&  action>0) {
		SOCKET sock = iter_c.get_current();

		if (sock != INVALID_SOCKET  &&  socket_list_t::has_client(sock)) {
			uint32 client_id = socket_list_t::get_client_id(sock);
			socket_list_t::get_client(client_id).process_send_queue();
			// errors are caught and treated in socket_info_t::process_send_queue
		}
		action --;
	}
}


bool network_check_server_connection()
{
	if(  !network_server_port  ) {
		// I am client

		// If I am playing, playing_clients should be at least one.
		if( socket_list_t::get_playing_clients()==0 ) {
			return false;
		}

		fd_set fds_read, fds_write;
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&fds_read);
		socket_list_t::fill_set(&fds_read);
		FD_ZERO(&fds_write);
		socket_list_t::fill_set(&fds_write);

		int action = select( FD_SETSIZE, &fds_read, &fds_write, NULL, &tv );
		if(  action < 0  ) {
			// error - connection lost
			return false;
		}
	}
	return true;
}


// send data to all PLAYING clients
// nwc is invalid after the call
void network_send_all(network_command_t* nwc, bool exclude_us )
{
	if (nwc) {
		nwc->prepare_to_send();
		socket_list_t::send_all(nwc, true);
		if(  !exclude_us  &&  network_server_port  ) {
			// I am the server
			nwc->get_packet()->sent_by_server();
			received_command_queue.append(nwc);
		}
		else {
			delete nwc;
		}
	}
}


// send data to server
// nwc is invalid after the call
void network_send_server(network_command_t* nwc )
{
	if (nwc) {
		nwc->prepare_to_send();
		if(  !network_server_port  ) {
			// I am client
			socket_list_t::send_all(nwc, true);
			delete nwc;
		}
		else {
			// I am the server
			nwc->get_packet()->sent_by_server();
			received_command_queue.append(nwc);
		}
	}
}


/**
 * send data to dest
 * @param buf the data
 * @param size length of buffer and number of bytes to be sent
 * @return true if data was completely send, false if an error occurs and connection needs to be closed
 */
bool network_send_data( SOCKET dest, const char *buf, const uint16 size, uint16 &count, const int timeout_ms )
{
	count = 0;
	while (count < size) {
		int sent = send(dest, buf+count, size-count, 0);
		if (sent == -1) {
			int err = GET_LAST_ERROR();
			if (err != EWOULDBLOCK) {
				dbg->warning("network_send_data", "error \"%s\" while sending to [%d]", strerror(err), dest);
				return false;
			}
			if (timeout_ms <= 0) {
				// no timeout, continue sending later
				return true;
			}
			else {
				// try again, test whether sending is possible
				fd_set fds;
				FD_ZERO(&fds);
				FD_SET(dest,&fds);
				struct timeval tv;
				tv.tv_sec = timeout_ms / 1000;
				tv.tv_usec = (timeout_ms % 1000) * 1000ul;
				// can we write?
				if(  select( FD_SETSIZE, NULL, &fds, NULL, &tv )!=1  ) {
					dbg->warning("network_send_data", "could not write to [%s]", dest);
					return false;
				}
			}
			continue;
		}
		if (sent == 0) {
			// connection closed
			dbg->warning("network_send_data", "connection [%d] already closed (sent %d of &d)", dest, count, size );
			return false;
		}
		count += sent;
		DBG_DEBUG4("network_send_data", "sent %d bytes to socket[%d]; size=%d, left=%d", count, dest, size, size-count );
	}
	// we reach here only if data are sent completely
	return true;
}


/**
 * receive data from sender
 * @param dest the destination buffer
 * @param len length of destination buffer and number of bytes to be received
 * @param received number of received bytes is returned here
 * @return true if connection is still valid, false if an error occurs and connection needs to be closed
 */
bool network_receive_data( SOCKET sender, void *dest, const uint16 len, uint16 &received, const int timeout_ms )
{
	received = 0;
	char *ptr = (char *)dest;

	do {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(sender,&fds);
		struct timeval tv;
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000ul;
		// can we read?
		if(  select( FD_SETSIZE, &fds, NULL, NULL, &tv )!=1  ) {
			return true;
		}
		// now receive
		int res = recv( sender, ptr+received, len-received, 0 );
		if (res == -1) {
			int err = GET_LAST_ERROR();
			if (err != EWOULDBLOCK) {
				dbg->warning("network_receive_data", "error %d while receiving from [%d]", err, sender);
				return false;
			}
			// try again later
			return true;
		}
		if (res == 0) {
			// connection closed
			dbg->warning("network_receive_data", "connection [%d] already closed", sender);
			return false;
		}
		received += res;
	} while(  received<len  );

	return true;
}


void network_close_socket( SOCKET sock )
{
	if(  sock != INVALID_SOCKET  ) {
#if USE_WINSOCK || defined __BEOS__
		closesocket( sock );
#else
		close( sock );
#endif

#ifndef NETTOOL
		// reset all the special / static socket variables
		if(  sock==nwc_join_t::pending_join_client  ) {
			nwc_join_t::pending_join_client = INVALID_SOCKET;
			DBG_MESSAGE( "network_close_socket()", "Close pending_join_client [%d]", nwc_join_t::pending_join_client );
		}
		if(  sock==nwc_pakset_info_t::server_receiver) {
			nwc_pakset_info_t::server_receiver = INVALID_SOCKET;
		}
#endif
	}
}


void network_reset_server()
{
	clear_command_queue();
	socket_list_t::reset_clients();
#ifndef NETTOOL
	nwc_ready_t::clear_map_counters();
#endif
}


/**
 * Shuts down the network core (since that is needed for some platforms
 */
void network_core_shutdown()
{
	clear_command_queue();

	socket_list_t::reset();

	if(network_active) {
#if USE_WINSOCK
		WSACleanup();
#endif
	}

	network_active = false;
#ifndef NETTOOL
	env_t::networkmode = false;
#endif
}
