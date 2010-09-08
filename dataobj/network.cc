/* basic network functionality, borrowed from OpenTTD */

#ifdef  __MINGW32__
// warning: IPv6 will only work on XP and up ...
#define WINVER 0x0501
#endif
#define USE_IP4_ONLY

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "network.h"
#include "network_cmd.h"

#include "loadsave.h"
#include "gameinfo.h"

#include "../simconst.h"

#include "../simdebug.h"
#include "../simgraph.h"

#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"

#include "../utils/simstring.h"
#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/slist_tpl.h"

static bool network_active = false;
// local server cocket
static vector_tpl<SOCKET> my_socket;
// local client socket
static SOCKET my_client_socket = INVALID_SOCKET;
static slist_tpl<const char *>pending_list;

// to query all open sockets, we maintain this list
static vector_tpl<SOCKET> clients;
static uint32 active_clients;

// list of commands on the server
static slist_tpl<network_command_t *> server_command_queue;

#ifdef WIN32
#define RET_ERR_STR { FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,NULL,WSAGetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),err_str,sizeof(err_str),NULL); return err_str; }
#else
#define RET_ERR_STR return err_str
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
#ifdef WIN32
		/* Let's load the network in windows */
		WSADATA wsa;
		if(int err = WSAStartup( MAKEWORD(2, 2), &wsa)) {
			dbg->error("NetworkInitialize()","failed loading windows socket library");
			return false;
		}
#endif /* WIN32 */
	}
	network_active = true;
	return true;
}



// open a socket or give a decent error message
const char *network_open_address( const char *cp, long timeout_ms )
{
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
		return "Cannot init network!";
	}

	struct sockaddr_in server_name;
	memset(&server_name,0,sizeof(server_name));
	server_name.sin_family=AF_INET;
#ifdef  WIN32
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
		return err_str;
	}
	server_name.sin_port=htons(port);

	my_client_socket = socket(AF_INET,SOCK_STREAM,0);
	if(my_client_socket==INVALID_SOCKET) {
		return "Cannot create socket";
	}

#if !defined(__BEOS__)  &&  !defined(__HAIKU__)
	if(  0 &&  timeout_ms>0  ) {
		// use non-blocking sockets to have a shorter timeout
		fd_set fds;
		struct timeval timeout;
#ifdef  WIN32
		unsigned long opt =1;
		ioctlsocket(my_client_socket, FIONBIO, &opt);
#else
		int opt;
		if(  (opt = fcntl(my_client_socket, F_GETFL, NULL)) < 0  ) {
			return "fcntl error";
		}
		opt |= O_NONBLOCK;
		if( fcntl(my_client_socket, F_SETFL, opt) < 0) {
			return "fcntl error";
		}
#endif
		if(  !connect(my_client_socket, (struct sockaddr*) &server_name, sizeof(server_name))   ) {
#ifdef  WIN32
			// WSAEWOULDBLOCK indicate, that it may still succeed
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
				FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,NULL,WSAGetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),err_str,sizeof(err_str),NULL);
#else
			// EINPROGRESS indicate, that it may still succeed
			if(  errno != EINPROGRESS  ) {
				sprintf( err_str, "Could not connect to %s", cp );
#endif
				return err_str;
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
			return "Call to select failed";
		}

		// is this socket ok?
		if (FD_ISSET(my_client_socket, &fds) == 0) {
			// not in set => timeout
			return "Server did not respond!";
		}

		// make a blocking socket out of it
#ifdef  WIN32
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
#ifdef  WIN32
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,NULL,WSAGetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),err_str,sizeof(err_str),NULL);
#else
			sprintf( err_str, "Could not connect to %s", cp );
#endif
			dbg->warning( "network_open_address", err_str );
			return err_str;
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
		return "Cannot init network!";
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

	my_client_socket = INVALID_SOCKET;
	struct addrinfo *walk;
	for(  walk = res;  walk != NULL;  walk = walk->ai_next  ) {
		my_client_socket = socket( walk->ai_family, walk->ai_socktype, walk->ai_protocol );
		if (my_client_socket < 0){
			/* actually not a valid socket ... */
			continue;
		}
		if(  connect(my_client_socket, walk->ai_addr, walk->ai_addrlen) != 0  ) {
#ifdef WIN32
			closesocket(my_client_socket);
#else
			close(my_client_socket);
#endif
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
	active_clients = 0;
	server_command_queue.clear();

	return NULL;
}


// download a http file from server address="www.simutrans.com:88" at path "/b/xxx.htm" to file localname="list.txt"
const char *network_download_http( const char *address, const char *name, const char *localname )
{
	SOCKET old_my = my_client_socket;
	// open from network
	const char *err = network_open_address( address, 5000 );
	if(  err==NULL  ) {
		char uri[1024];
		int len = sprintf( uri, "GET %s HTTP/1.1\nHost: %s\n\n", name, address );
		send( my_client_socket, uri, len, 0 );
		// read the header
		char line[1024], rbuf;
		int pos = 0;
		long length = 0;
		bool prev_empty = false;
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
		err = network_receive_file( my_client_socket, localname, length );
		network_close_socket( my_client_socket );
	}
	my_client_socket = old_my;
	return err;
}


// connect to address (cp), receive gameinfo, close
const char *network_gameinfo(const char *cp, gameinfo_t *gi)
{
	// open from network
	SOCKET old_my_client_socket = my_client_socket;
	const char *err = network_open_address( cp, 5000 );
	if(  err==NULL  ) {
		{
			nwc_gameinfo_t nwgi;
			nwgi.rdwr();
			nwgi.send(my_client_socket);
		}
		network_add_client( my_client_socket );
		// wait for join command (tolerate some wrong commands)
		network_command_t *nwc = NULL;
		nwc = network_check_activity( 10000 );	// 10s should be enough for reply ...
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
		network_remove_client( my_client_socket );
	}
end:
	my_client_socket = old_my_client_socket;
	if(err) {
		dbg->warning("network_connect", err);
	}
	return err;
}


// connect to address (cp), receive game, save to client%i-network.sve
const char *network_connect(const char *cp)
{
	// open from network
	const char *err = network_open_address( cp, 0 );
	if(  err==NULL  ) {
		// want to join
		{
			nwc_join_t nwc_join;
			nwc_join.rdwr();
			nwc_join.send(my_client_socket);
		}
		network_add_client( my_client_socket );
		// wait for join command (tolerate some wrong commands)
		network_command_t *nwc = NULL;
		for(uint8 i=0; i<5; i++) {
			nwc = network_check_activity( 10000 );
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
		// ignore the next (nwc_sync_t) command
		// wait for sync command (tolerate some wrong commands)
		for(uint8 i=0; i<5; i++) {
			nwc = network_check_activity( 10000 );
			if (nwc  &&  nwc->get_id() == NWC_SYNC) break;
		}
		if (nwc == NULL  ||  nwc->get_id()!=NWC_SYNC  ||  ((nwc_sync_t*)nwc)->client_id!= client_id) {
			err = "Protocoll error (expected NWC_SYNC)";
			goto end;
		}
		// receive nwc_game_t
		// wait for sync command (tolerate some wrong commands)
		for(uint8 i=0; i<2; i++) {
			nwc = network_check_activity( 60000 );
			if (nwc  &&  nwc->get_id() == NWC_GAME) break;
		}
		if (nwc == NULL  ||  nwc->get_id()!=NWC_GAME) {
			err = "Protocoll error (expected NWC_GAME)";
			goto end;
		}
		int len = ((nwc_game_t*)nwc)->len;
		// garanteed individual file name ...
		char filename[256];
		sprintf( filename, "client%i-network.sve", client_id );
		err = network_receive_file( my_client_socket, filename, len );
	}
end:
	if(err) {
		dbg->warning("network_connect", err);
		network_close_socket( my_client_socket );
	}
	return err;
}


// if sucessful, starts a server on this port
bool network_init_server( int port )
{
	struct sockaddr_in name;

	network_initialize();
#ifdef USE_IP4_ONLY
	SOCKET my = socket(PF_INET, SOCK_STREAM, 0);
	if(  my==INVALID_SOCKET  ) {
		dbg->fatal("init_server()", "Fail to open socket!");
		return false;
	}

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

	active_clients = 0;
	my_socket.append( my );
	network_add_client( my );
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

		my_socket.append( fd );
	}
	freeaddrinfo(res);

	for(  uint32 i=0;  i<my_socket.get_count();  i++  ) {
		network_add_client( my_socket[i] );
	}
	active_clients = 1;
#endif
	client_id = 0;
	server_command_queue.clear();
	return true;
}


void network_add_client( SOCKET sock )
{
	if(  !clients.is_contained(sock)  ) {
		// purge it first
		while(  !clients.empty()  &&  clients.back() == INVALID_SOCKET  ) {
			clients.remove_at(clients.get_count()-1);
		}
#ifdef TCP_NODELAY
		// do not wait to join small (command) packets when sending (may cause 200ms delay!)
		setsockopt( sock, SOL_SOCKET, TCP_NODELAY, NULL, 0 );
#endif
		/* do not give old client ids as long as possible
		if (active_clients < clients.get_count()) {
			for(uint32 i=0; i<clients.get_count(); i++) {
				if (clients[i]==INVALID_SOCKET) {
					clients[i]=sock;
					active_clients++;
					break;
				}
			}
		}
		else */ {
			clients.append( sock );
			active_clients++;
		}
	}
}


void network_remove_client( SOCKET sock )
{
	if(  clients.is_contained(sock)  ) {
		assert(active_clients>0);
		uint32 ind = clients.index_of(sock);
		clients[ind] = INVALID_SOCKET;
		// just decrease count
		active_clients--;
		// and maybe purge it
		while(  !clients.empty()  &&  clients.back() == INVALID_SOCKET  ) {
			clients.remove_at(clients.get_count()-1);
		}
		network_close_socket(sock);
	}
}


uint32 network_get_client_id( SOCKET sock )
{
	if(  clients.is_contained(sock)  ) {
		return clients.index_of(sock);
	}
	return 0; // 0 is the index of the server
}


// number of currently active clients
int network_get_clients()
{
	// all clients except ourselves
	return active_clients - 1;
}


SOCKET network_get_socket( uint32 client_id )
{
	if (client_id < clients.get_count()) {
		return clients[client_id];
	}
	else {
		return INVALID_SOCKET;
	}
}


static int fill_set(fd_set *fds)
{
	int s_max = 0;
	for(uint32 i=0; i<clients.get_count(); i++) {
		if (clients[i]!=INVALID_SOCKET) {
			SOCKET s = clients[i];
			s_max = max( (int)s, (int)s_max );
			FD_SET( s, fds );
		}
	}
	return s_max+1;
}


/* do appropriate action for network server:
 * - either connect to a new client
 * - receive commands
 */
network_command_t* network_check_activity(int timeout)
{
	if (umgebung_t::server  &&  !server_command_queue.empty()) {
		return server_command_queue.remove_first();
	}

	fd_set fds;
	FD_ZERO(&fds);

	int s_max = fill_set(&fds);

	// time out
	struct timeval tv;
	tv.tv_sec = 0; // seconds
	tv.tv_usec = max(0, timeout) * 1000; // micro-seconds

	int action = select(s_max, &fds, NULL, NULL, &tv );

	if(  action<=0  ) {
		// timeout
		return NULL;
	}

	// accept new connection
	int nr = -1;
	for(  int i=0;  i<my_socket.get_count();  i++  ) {
		if(  FD_ISSET(my_socket[i], &fds)  ) {
			nr = i;
			break;
		}
	}
	if(  nr>=0  ) {
		struct sockaddr_in client_name;
		socklen_t size = sizeof(client_name);
		SOCKET s = accept(my_socket[nr], (struct sockaddr *)&client_name, &size);
		if(  s!=INVALID_SOCKET  ) {
#ifdef  __BEOS__
			dbg->message("check_activity()", "Accepted connection from: %lh.\n", client_name.sin_addr.s_addr );
#else
			dbg->message("check_activity()", "Accepted connection from: %s.\n", inet_ntoa(client_name.sin_addr) );
#endif
			network_add_client(s);
		}
		// not a request
		return NULL;
	}
	else {
		SOCKET sender = INVALID_SOCKET;
		for(uint32 i=0; i<clients.get_count(); i++) {
			if (clients[i]!=INVALID_SOCKET) {
				if(  FD_ISSET(clients[i], &fds )  ) {
					sender = clients[i];
					break;
				}
			}
		}
		if(  sender==INVALID_SOCKET  ) {
			return NULL;
		}
		// receive only one command
		FD_ZERO(&fds);
		FD_SET(sender,&fds);
		tv.tv_usec = 0;
		if(  select((int)sender+1, &fds, NULL, NULL, &tv )!=1  ) {
			return NULL;
		}
		network_command_t *nwc = network_command_t::read_from_socket(sender);
		// something failed
		if (nwc == NULL) {
			network_remove_client(sender);
		}
		else {
			dbg->warning( "network_check_activity()", "received cmd id=%d %s", nwc->get_id(), nwc->get_name());
		}
		// read something sucessful
		return nwc;
	}
	return NULL;
}


bool network_check_server_connection()
{
	if(  !umgebung_t::server  ) {
		// I am client

		if(  clients.get_count()==0  ||  clients[0]==INVALID_SOCKET) {
			return false;
		}

		fd_set fds;
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(clients[0],&fds);

		int action = select((int)clients[0]+1, NULL, &fds, NULL, &tv );
		if(  action<=0  ) {
			// timeout
			return false;
		}
	}
	return true;
}



// send data to all clients
// nwc is invalid after the call
void network_send_all(network_command_t* nwc, bool exclude_us )
{
	if (nwc) {
		nwc->prepare_to_send();
		for(uint32 i=0; i<clients.get_count(); i++) {
			if (clients[i]!=INVALID_SOCKET) {
				SOCKET s = clients[i];
				// if we are server do not send to ourselves
				if(my_socket.is_contained(s)) {
					continue;
				}
				nwc->send(s);
			}
		}
		if(  !exclude_us  &&  umgebung_t::server  ) {
			// I am the server
			server_command_queue.append(nwc);
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
			nwc->send(clients[0]);
			delete nwc;
		}
		else {
			// I am the server
			server_command_queue.append(nwc);
		}
	}
}



uint16 network_receive_data( SOCKET sender, void *dest, const uint16 length )
{
	fd_set fds;
	uint16 bytes = 0;
	char *ptr = (char *)dest;
	// time out
	struct timeval tv;

	do {
		FD_ZERO(&fds);
		FD_SET(sender,&fds);
		tv.tv_sec = 0;
		tv.tv_usec = 250000ul;	// maximum 250ms timeout
		if(  select((int)sender+1, &fds, NULL, NULL, &tv )!=1  ) {
			return bytes;
		}
		if(  recv( sender, ptr+bytes, 1, 0 )==0  ) {
			network_remove_client( sender );
			return 0;
		}
		bytes ++;
	} while(  bytes<length  );

	return length;
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
	SOCKET s = INVALID_SOCKET;
	if (client_id < clients.get_count()) {
		s = clients[client_id];
	}
	if (s==INVALID_SOCKET) {
		return "Client closed connection during transfer";
	}
	// send size of file
	nwc_game_t nwc(length);
	nwc.send(s);

	// good place to show a progress bar
	if(is_display_init()  &&  length>0) {
		display_set_progress_text(translator::translate("Transferring game ..."));
		display_progress(0, length);
	}
	long bytes_sent = 0;
	while(  !feof(fp)  ) {
		int bytes_read = (int)fread( buffer, 1, sizeof(buffer), fp );
		if(  send(s,buffer,bytes_read,0)==-1) {
			network_remove_client(s);
			return "Client closed connection during transfer";
		}
		bytes_sent += bytes_read;
		display_progress(bytes_sent, length);
	}

	// ok, new client has savegame
	return NULL;
}


const char *network_receive_file( SOCKET s, const char *save_as, const long length )
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
	FILE *f = fopen( save_as, "wb" );
	while(  f!=NULL  &&  length_read<length  ) {
		int i = recv( s, rbuf, length_read+4096<length?4096:length-length_read, 0 );
		if(  i>0  ) {
			fwrite( rbuf, i, 1, f );
			display_progress( length_read, length);
			length_read += i;
		}
		else {
			if(  i<0  ) {
				dbg->error("loadsave_t::rd_open()","recv failed with %i",i);
			}
			break;
		}
	}
	fclose( f );
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
#elif defined(WIN32)  ||  defined(__BEOS__)
		closesocket( sock );
#else
		close( sock );
#endif
		if(  my_socket.is_contained(sock)  ) {
			my_socket.remove( sock );
		}
		if(  sock==my_client_socket  ) {
			my_client_socket = INVALID_SOCKET;
		}
		clients.remove( sock );
	}
}


/**
 * Shuts down the network core (since that is needed for some platforms
 */
void network_core_shutdown()
{
	while(  my_socket.get_count()!=0  ) {
		network_close_socket( my_socket.back() );
	}
	network_close_socket( my_client_socket );
	while(  !pending_list.empty()  ) {
		free( (void *)pending_list.remove_first() );
	}
	if(network_active) {
#if defined(WIN32)
		WSACleanup();
#endif
	}
	network_active = false;
}
