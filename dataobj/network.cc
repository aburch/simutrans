/* basic network functionality, borrowed from OpenTTD */

#ifdef  __MINGW32__
// warning: IPv6 will only work on XP and up ...
#define USE_IP4_ONLY
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "network.h"
#include "network_socket_list.h"
#include "network_cmd.h"
#include "network_cmp_pakset.h"

#include "loadsave.h"
#include "gameinfo.h"

#include "../simconst.h"

#include "../simdebug.h"
#include "../simgraph.h"
#include "../simworld.h"
#include "../simwerkz.h"
#include "../simmesg.h"

#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"

#include "../utils/simstring.h"
#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/slist_tpl.h"

// forward declaration ..
static char const* network_receive_file(SOCKET s, char const* save_as, long length);


static bool network_active = false;

// list of received commands
static slist_tpl<network_command_t *> received_command_queue;

void clear_command_queue()
{
	while(!received_command_queue.empty()) {
		network_command_t *nwc = received_command_queue.remove_first();
		if (nwc) {
			delete nwc;
		}
	}
}

#ifdef _WIN32
#define RET_ERR_STR { FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,NULL,WSAGetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),err_str,sizeof(err_str),NULL); err = err_str; return INVALID_SOCKET; }
#else
#define RET_ERR_STR { err = err_str; return INVALID_SOCKET; }
#endif



// global client id
static uint32 client_id;
uint32 network_get_client_id()
{
	return client_id;
}

/**
 * Initializes the network core (as that is needed for some platforms
 * @return true if the core has been initialized, false otherwise
 */
bool network_initialize()
{
	if(!network_active) {
		socket_list_t::reset();

#ifdef _WIN32
		/* Let's load the network in windows */
		WSADATA wsa;
		if(int err = WSAStartup( MAKEWORD(2, 2), &wsa)) {
			dbg->error("NetworkInitialize()","failed loading windows socket library");
			return false;
		}
#endif /* _WIN32 */
	}
	network_active = true;
	return true;
}



// open a socket or give a decent error message
SOCKET network_open_address( const char *cp, long timeout_ms, const char * &err)
{
	err = NULL;
#ifdef USE_IP4_ONLY
	// Network load. Address format e.g.: "128.0.0.1:13353"
	char address[32];
	static char err_str[256];
	uint16 port = 13353;
	const char *cp2 = strrchr(cp,':');
	if(cp2!=NULL) {
		port=atoi(cp2+1);
		// Copy the address part
		tstrncpy(address,cp,cp2-cp>31?31:cp2-cp+1);
		cp = address;
	}

	// now activate network
	if(  !network_initialize()  ) {
		err = "Cannot init network!";
		return INVALID_SOCKET;
	}

	struct sockaddr_in server_name;
	memset(&server_name,0,sizeof(server_name));
	server_name.sin_family=AF_INET;
#ifdef  _WIN32
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

#if !defined(__BEOS__)  &&  !defined(__HAIKU__)
	if(  0 &&  timeout_ms>0  ) {
		// use non-blocking sockets to have a shorter timeout
		fd_set fds;
		struct timeval timeout;
#ifdef  _WIN32
		unsigned long opt =1;
		ioctlsocket(my_client_socket, FIONBIO, &opt);
#else
		int opt;
		if(  (opt = fcntl(my_client_socket, F_GETFL, NULL)) < 0  ) {
			err = "fcntl error";
			return INVALID_SOCKET;
		}
		opt |= O_NONBLOCK;
		if( fcntl(my_client_socket, F_SETFL, opt) < 0) {
			err = "fcntl error";
			return INVALID_SOCKET;
		}
#endif
		if(  !connect(my_client_socket, (struct sockaddr*) &server_name, sizeof(server_name))   ) {
#ifdef  _WIN32
			// WSAEWOULDBLOCK indicate, that it may still succeed
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
				FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,NULL,WSAGetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),err_str,sizeof(err_str),NULL);
#else
			// EINPROGRESS indicate, that it may still succeed
			if(  errno != EINPROGRESS  ) {
				sprintf( err_str, "Could not connect to %s", cp );
#endif
				err = err_str;
				return INVALID_SOCKET;
			}
		}

		// no add only this socket to set
		FD_ZERO(&fds);
		FD_SET(my_client_socket, &fds);

		// enter timeout
		timeout.tv_sec = timeout_ms/1000;
		timeout.tv_usec = ((timeout_ms%1000)*1000);

		// and wait ...
		if(  !select(my_client_socket + 1, NULL, &fds, NULL, &timeout)  ) {
			// some other problem?
			err = "Call to select failed";
			return INVALID_SOCKET;
		}

		// is this socket ok?
		if (FD_ISSET(my_client_socket, &fds) == 0) {
			// not in set => timeout
			err = "Server did not respond!";
			return INVALID_SOCKET;
		}

		// make a blocking socket out of it
#ifdef  _WIN32
		opt = 0;
		ioctlsocket(my_client_socket, FIONBIO, &opt);
#else
		opt &= (~O_NONBLOCK);
		fcntl(my_client_socket, F_SETFL, opt);
#endif
	} else
#endif
	{
		if(connect(my_client_socket,(struct sockaddr *)&server_name,sizeof(server_name))==-1) {
			sprintf( err_str, "Could not connect to %s", cp );
			RET_ERR_STR;
		}
	}
#else
	// Address format e.g.: "128.0.0.1:13353" or "[::1]:80"
	char address[1024];
	static char err_str[256];
	uint16 port = 13353;
	const char *cp2 = strrchr(cp,':');
	const char *cp1 = strrchr(cp,']');

	if(cp1!=NULL  ||  cp2!=NULL) {
		if(  cp2!=NULL  &&  cp2>cp1  ) {
			port=atoi(cp2+1);
		}
		else {
			cp2 = cp1+1;
		}
		if(  cp1  ) {
			// IPv6 addresse net:[....]:port
			tstrncpy(address,cp+1,cp1-cp>=sizeof(address)?sizeof(address):cp1-cp);
		}
		else {
			// Copy the address part
			tstrncpy(address,cp,cp2-cp>31?31:cp2-cp+1);
		}
		cp = address;
	}

	// now activate network
	if(  !network_initialize()  ) {
		err = "Cannot init network!";
		return INVALID_SOCKET;
	}

	char port_nr[8];
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo *res;
	int ret;
	sprintf( port_nr, "%u", port );
	if ((ret = getaddrinfo(cp, port_nr, &hints, &res))!=0) {
		sprintf( err_str, "Bad address %s", cp );
		RET_ERR_STR;
	}

	SOCKET my_client_socket = INVALID_SOCKET;
	struct addrinfo *walk;
	for(  walk = res;  walk != NULL;  walk = walk->ai_next  ) {
		my_client_socket = socket( walk->ai_family, walk->ai_socktype, walk->ai_protocol );
		if (my_client_socket < 0){
			/* actually not a valid socket ... */
			continue;
		}
		if(  connect(my_client_socket, walk->ai_addr, walk->ai_addrlen) != 0  ) {
			network_close_socket(my_client_socket);
			my_client_socket = INVALID_SOCKET;
			/* could not connect with this! */
			continue;
		}
		break;
	}

	freeaddrinfo(res);
	if (my_client_socket == INVALID_SOCKET) {
		sprintf( err_str, "Could not connect to %s", cp );
		RET_ERR_STR;
	}
#endif
	return my_client_socket;
}


// download a http file from server address="www.simutrans.com:88" at path "/b/xxx.htm" to file localname="list.txt"
const char *network_download_http( const char *address, const char *name, const char *localname )
{
	// open from network
	const char *err = NULL;
	SOCKET my_client_socket = network_open_address( address, 5000, err );
	if(  err==NULL  ) {
		char uri[1024];
		int const len = sprintf(uri, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", name, address);
		uint16 dummy;
		if(  !network_send_data(my_client_socket, uri, len, dummy, 250)  ) {
			err = "Server did not respond!";
		}
		// read the header
		char line[1024], rbuf;
		int pos = 0;
		long length = 0;
		while(1) {
			int i = recv( my_client_socket, &rbuf, 1, 0 );
			if(  i>0  ) {
				if(  rbuf>=32  &&  pos<sizeof(line)-1  ) {
					line[pos++] = rbuf;
				}
				if(  rbuf==10  ) {
					if(  pos == 0  ) {
						// this line was empty => now data will follow
						break;
					}
					line[pos] = 0;
					// we only need the length tag
					if(  STRNICMP("Content-Length:",line,15)==0  ) {
						length = atol( line+15 );
					}
					pos = 0;
				}
			}
			else {
				break;
			}
		}
		// for a simple query, just pass an empty filename
		if(  localname  &&  *localname  ) {
			err = network_receive_file( my_client_socket, localname, length );
		}
		network_close_socket( my_client_socket );
	}
	return err;
}


// connect to address (cp), receive gameinfo, close
const char *network_gameinfo(const char *cp, gameinfo_t *gi)
{
	// open from network
	const char *err = NULL;
	SOCKET my_client_socket = network_open_address( cp, 5000, err );
	if(  err==NULL  ) {
		{
			nwc_gameinfo_t nwgi;
			nwgi.rdwr();
			if(  !nwgi.send(my_client_socket)  ) {
				err = "send of NWC_GAMEINFO failed";
				goto end;
			}
		}
		socket_list_t::add_client( my_client_socket );
		// wait for join command (tolerate some wrong commands)
		network_command_t *nwc = NULL;
		nwc = network_check_activity( NULL, 10000 );	// 10s should be enough for reply ...
		if (nwc==NULL) {
			err = "Server did not respond!";
			goto end;
		}
		nwc_gameinfo_t *nwgi = dynamic_cast<nwc_gameinfo_t*>(nwc);
		if (nwgi==NULL) {
			err = "Protocoll error (expected NWC_GAMEINFO)";
			goto end;
		}
		if (nwgi->len==0) {
			err = "Server busy";
			goto end;
		}
		uint32 len = nwgi->len;
		char filename[1024];
		sprintf( filename, "client%i-network.sve", nwgi->len );
		err = network_receive_file( my_client_socket, filename, len );
		// now into gameinfo
		loadsave_t fd;
		if(  fd.rd_open( filename )  ) {
			gameinfo_t *pgi = new gameinfo_t( &fd );
			*gi = *pgi;
			delete pgi;
			fd.close();
		}
		remove( filename );
		socket_list_t::remove_client( my_client_socket );
	}
end:
	if(err) {
		dbg->warning("network_gameinfo", err);
	}
	return err;
}


// connect to address (cp), receive game, save to client%i-network.sve
const char *network_connect(const char *cp)
{
	// open from network
	const char *err = NULL;
	SOCKET my_client_socket = network_open_address( cp, 5000, err );
	if(  err==NULL  ) {
		// want to join
		{
			nwc_join_t nwc_join;
			nwc_join.rdwr();
			if (!nwc_join.send(my_client_socket)) {
				err = "send of NWC_JOIN failed";
				goto end;
			}
		}
		socket_list_t::reset();
		socket_list_t::add_client(my_client_socket);
		// wait for join command (tolerate some wrong commands)
		network_command_t *nwc = NULL;
		for(uint8 i=0; i<5; i++) {
			nwc = network_check_activity( NULL, 10000 );
			if (nwc  &&  nwc->get_id() == NWC_JOIN) break;
		}
		if (nwc==NULL) {
			err = "Server did not respond!";
			goto end;
		}
		nwc_join_t *nwj = dynamic_cast<nwc_join_t*>(nwc);
		if (nwj==NULL) {
			err = "Protocoll error (expected NWC_JOIN)";
			goto end;
		}
		if (nwj->answer!=1) {
			err = "Server busy";
			goto end;
		}
		client_id = nwj->client_id;
		// receive nwc_game_t
		// wait for game command (tolerate some wrong commands)
		for(uint8 i=0; i<2; i++) {
			nwc = network_check_activity( NULL, 60000 );
			if (nwc  &&  nwc->get_id() == NWC_GAME) break;
		}
		if (nwc == NULL  ||  nwc->get_id()!=NWC_GAME) {
			err = "Protocoll error (expected NWC_GAME)";
			goto end;
		}
		int len = ((nwc_game_t*)nwc)->len;
		// guaranteed individual file name ...
		char filename[256];
		sprintf( filename, "client%i-network.sve", client_id );
		err = network_receive_file( my_client_socket, filename, len );
	}
end:
	if(err) {
		dbg->warning("network_connect", err);
		if (!socket_list_t::remove_client(my_client_socket)) {
			network_close_socket( my_client_socket );
		}
	}
	else {
		const uint32 id = socket_list_t::get_client_id(my_client_socket);
		socket_list_t::change_state(id, socket_info_t::playing);
	}
	return err;
}


// if sucessful, starts a server on this port
bool network_init_server( int port )
{
	network_initialize();
#ifdef USE_IP4_ONLY
	SOCKET my = socket(PF_INET, SOCK_STREAM, 0);
	if(  my==INVALID_SOCKET  ) {
		dbg->fatal("init_server()", "Fail to open socket!");
		return false;
	}

	struct sockaddr_in name;
	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if(  bind(my, (struct sockaddr *)&name, sizeof(name))==-1  ) {
		dbg->fatal("init_server()", "Bind failed!");
		return false;
	}

	if(  listen(my, MAX_PLAYER_COUNT /* max pending connections */)==-1  ) {
		dbg->fatal("init_server()", "Listen failed for %i sockets!", MAX_PLAYER_COUNT );
		return false;
	}

	socket_list_t::add_server( my );
#else
	struct addrinfo *res;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	int ret;
	char port_nr[16];
	sprintf( port_nr, "%u", port );
	if((ret = getaddrinfo(NULL, port_nr, &hints, &res)) != 0) {
		dbg->fatal("init_server()", "Fail to open socket!");
	}

	SOCKET fd;
	struct addrinfo *walk;
	for (walk = res; walk != NULL; walk = walk->ai_next) {
		fd = socket(walk->ai_family, walk->ai_socktype, walk->ai_protocol);
		if (fd == INVALID_SOCKET) {
			continue;
		}

		if (walk->ai_family == AF_INET6) {
			int on = 1;
			if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&on, sizeof(on)) == -1) {
				// only use real IPv6 sockets for IPv6
				continue;
			}
		}

		if (bind(fd, walk->ai_addr, walk->ai_addrlen)) {
			/* Hier kann eine Fehlermeldung hin, z.B. mit warn() */
			continue;
		}

		if (listen(fd, 32) == -1) {
			/* Hier kann eine Fehlermeldung hin, z.B. mit warn() */
			continue;
		}

		socket_list_t::add_server( fd );
	}
	freeaddrinfo(res);

	dbg->message("network_init_server", "added server %d sockets", socket_list_t::get_server_sockets());
#endif
	client_id = 0;
	clear_command_queue();
	nwc_ready_t::clear_map_counters();
	return true;
}


void network_set_socket_nodelay( SOCKET sock )
{
#if defined(TCP_NODELAY)  &&  !defined(__APPLE__)
	// do not wait to join small (command) packets when sending (may cause 200ms delay!)
	setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, NULL, 0 );
#else
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
network_command_t* network_check_activity(karte_t *, int timeout)
{
	fd_set fds;
	FD_ZERO(&fds);

	int s_max = socket_list_t::fill_set(&fds);

	// time out
	struct timeval tv;
	tv.tv_sec = 0; // seconds
	tv.tv_usec = max(0, timeout) * 1000; // micro-seconds

	int action = select(s_max, &fds, NULL, NULL, &tv );

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
	#ifdef  __BEOS__
				dbg->message("check_activity()", "Accepted connection from: %lh.\n", client_name.sin_addr.s_addr );
	#else
				dbg->message("check_activity()", "Accepted connection from: %s.\n", inet_ntoa(client_name.sin_addr) );
	#endif
				socket_list_t::add_client(s);
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
				dbg->warning( "network_check_activity()", "received cmd id=%d %s from socket[%d]", nwc->get_id(), nwc->get_name(), sender);
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

	int s_max = socket_list_t::fill_set(&fds);

	// time out
	struct timeval tv;
	tv.tv_sec = 0; // seconds
	tv.tv_usec = max(0, timeout) * 1000; // micro-seconds

	int action = select(s_max, NULL, &fds, NULL, &tv );

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
	if(  !umgebung_t::server  ) {
		// I am client

		// If I am playing, playing_clients should be at least one.
		if( socket_list_t::get_playing_clients()==0 ) {
			return false;
		}

		fd_set fds;
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		int s_max = socket_list_t::fill_set(&fds);

		int action = select(s_max, NULL, &fds, NULL, &tv );
		if(  action<=0  ) {
			// timeout
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
		if(  !exclude_us  &&  umgebung_t::server  ) {
			// I am the server
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
		if(  !umgebung_t::server  ) {
			// I am client
			socket_list_t::send_all(nwc, true);
			delete nwc;
		}
		else {
			// I am the server
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
		int sent = ::send(dest, buf+count, size-count, 0);
		if (sent == -1) {
			int err = GET_LAST_ERROR();
			if (err != EWOULDBLOCK) {
				dbg->warning("network_send_data", "error %d while sending to [%d]", err, dest);
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
				tv.tv_sec = 0;
				tv.tv_usec = timeout_ms * 1000ul;
				// can we write?
				if(  select((int)dest+1, NULL, &fds, NULL, &tv )!=1  ) {
					dbg->warning("network_send_data", "could not write to [%s]", dest);
					return false;
				}
			}
			continue;
		}
		if (sent == 0) {
			// connection closed
			dbg->warning("network_send_data", "connection [%d] already closed", dest);
			return false;
		}
		count += sent;
		dbg->message("network_send_data", "sent %d bytes to socket[%d]; size=%d, left=%d", count, dest, size, size-count);
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
		tv.tv_sec = 0;
		tv.tv_usec = timeout_ms * 1000ul;
		// can we read?
		if(  select((int)sender+1, &fds, NULL, NULL, &tv )!=1  ) {
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



const char *network_send_file( uint32 client_id, const char *filename )
{
	FILE *fp = fopen(filename,"rb");
	char buffer[1024];

	// find out length
	fseek(fp, 0, SEEK_END);
	long length = (long)ftell(fp);
	rewind(fp);

	// socket
	SOCKET s = socket_list_t::get_socket(client_id);
	if (s==INVALID_SOCKET) {
		return "Client closed connection during transfer";
	}
	// send size of file
	nwc_game_t nwc(length);
	if (!nwc.send(s)) {
		return "Client closed connection during transfer";
	}

	// good place to show a progress bar
	if(is_display_init()  &&  length>0) {
		display_set_progress_text(translator::translate("Transferring game ..."));
		display_progress(0, length);
	}
	long bytes_sent = 0;
	while(  !feof(fp)  ) {
		int bytes_read = (int)fread( buffer, 1, sizeof(buffer), fp );
		uint16 dummy;
		if( !network_send_data(s, buffer, bytes_read, dummy, 250) ) {
			socket_list_t::remove_client(s);
			return "Client closed connection during transfer";
		}
		bytes_sent += bytes_read;
		display_progress(bytes_sent, length);
	}

	// ok, new client has savegame
	return NULL;
}


static char const* network_receive_file(SOCKET const s, char const* const save_as, long const length)
{
	// ok, we have a socket to connect
	remove(save_as);

	DBG_MESSAGE("network_receive_file","Game size %li", length );

	if(is_display_init()  &&  length>0) {
		display_set_progress_text(translator::translate("Transferring game ..."));
		display_progress(0, length);
	}
	else {
		printf("\n");fflush(NULL);
	}

	// good place to show a progress bar
	char rbuf[4096];
	sint32 length_read = 0;
	if (FILE* const f = fopen(save_as, "wb")) {
		while (length_read < length) {
			int i = recv(s, rbuf, length_read + 4096 < length ? 4096 : length - length_read, 0);
			if (i > 0) {
				fwrite(rbuf, 1, i, f);
				length_read += i;
				display_progress(length_read, length);
			}
			else {
				if (i < 0) {
					dbg->error("loadsave_t::rd_open()", "recv failed with %i", i);
				}
				break;
			}
		}
		fclose(f);
	}
	if(  length_read<length  ) {
		return "Not enough bytes transferred";
	}
	return NULL;
}


void network_close_socket( SOCKET sock )
{
	if(  sock != INVALID_SOCKET  ) {
#if defined(__HAIKU__)
		// no closesocket() ?!?
#elif defined(_WIN32)  ||  defined(__BEOS__)
		closesocket( sock );
#else
		close( sock );
#endif
		// reset all the special / static socket variables
		if(  sock==nwc_join_t::pending_join_client  ) {
			nwc_join_t::pending_join_client = INVALID_SOCKET;
		}
		if(  sock==nwc_pakset_info_t::server_receiver) {
			nwc_pakset_info_t::server_receiver = INVALID_SOCKET;
		}
	}
}


void network_reset_server()
{
	clear_command_queue();
	socket_list_t::reset_clients();
	nwc_ready_t::clear_map_counters();
}


/**
 * Shuts down the network core (since that is needed for some platforms
 */
void network_core_shutdown()
{
	clear_command_queue();

	socket_list_t::reset();

	if(network_active) {
#if defined(_WIN32)
		WSACleanup();
#endif
	}

	network_active = false;
	umgebung_t::networkmode = false;
}
