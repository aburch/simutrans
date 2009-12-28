/* basic network functionality, borrowed from OpenTTD */

#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "network.h"

#ifdef __BEOS__
#include <net/netdb.h>
#endif

// Haiku has select in an additional header
#ifndef FD_SET
#include <sys/select.h>
#endif

#include "../simconst.h"

#include "../simdebug.h"
#include "../simgraph.h"

#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"

#include "../utils/simstring.h"
#include "../tpl/vector_tpl.h"

#ifdef WIN32
#define socklen_t int
#endif

static bool network_active = false;
// local server cocket
static SOCKET my_socket = INVALID_SOCKET;
// local client socket
static SOCKET my_client_socket = INVALID_SOCKET;
static char pending[4096];

// to query all open sockets, we maintain this list
static vector_tpl<SOCKET> clients;
static uint32 our_client_id;
static uint32 active_clients;

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
		if(WSAStartup(MAKEWORD(2, 0), &wsa) != 0) {
			dbg->error("NetworkInitialize()","failed loading windows socket library");
			return false;
		}
#endif /* WIN32 */
	}
	network_active = true;
	return true;
}



// open a socket or give a decent error message
const char *network_open_address( const char *cp)
{
	// Network load. Address format e.g.: "net:128.0.0.1:13353"
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

	struct sockaddr_in server_name;
#ifdef  WIN32
	server_name.sin_addr.s_addr = inet_addr(cp);
	if((int)server_name.sin_addr.s_addr==-1) {// Bad address
#else
#ifdef  __BEOS__
	struct hostent *theHOst;
	theHost = gethostbyname( cp );
	if(theHost) {
		server_name.sin_addr.s_addr = *(ulong *)theHost->h_addr_list[0];
	}
	else {// Bad address
#else
	if(inet_aton(cp,&server_name.sin_addr)==0) { // Bad address
#endif
#endif
		sprintf( err_str, "Bad address %s", cp );
		return err_str;
	}
	server_name.sin_port=htons(port);
	server_name.sin_family=AF_INET;

	// now activate network
	if(  !network_initialize()  ) {
		return "Cannot init network!";
	}

	my_client_socket = socket(PF_INET,SOCK_STREAM,0);
	if(my_client_socket==INVALID_SOCKET) {
		return "Cannot create socket";
	}

	if(connect(my_client_socket,(struct sockaddr *)&server_name,sizeof(server_name))==-1) {
		sprintf( err_str, "Cannot connect to %s", cp );
		return err_str;
	}
	pending[0] = 0;
	active_clients = 0;

	return NULL;
}


// connect to address (cp), receive game, save to (filename)
const char *network_connect(const char *cp, const char *filename)
{
	// open from network
	const char *err = network_open_address( cp );
	if(  err==NULL  ) {
		int len;
		dbg->warning( "network_connect", "send :" NET_TO_SERVER NET_GAME NET_END_CMD );
		len = send( my_client_socket, NET_TO_SERVER NET_GAME NET_END_CMD, 9, 0 );

		len = 64;
		char buf[64];
		network_add_client( my_client_socket );
		if(  network_check_activity( 60000, buf, len )!=INVALID_SOCKET  ) {
			// wait for sync message to finish
			if(  memcmp( NET_FROM_SERVER NET_GAME, buf, 7 )!=0  ) {
				err = "Protocoll error (expecting " NET_GAME ")";
			}
			else {
				int len = 0;
				client_id = 0;
				sscanf(buf+7, " %lu,%li" NET_END_CMD, &client_id, &len);
				dbg->warning( "network_connect", "received: id=%li len=%li", client_id, len);
				err = network_recieve_file( my_client_socket, filename, len );
			}
		}
		else {
			err = "Server did not respond!";
		}
	}
	if(err) {
		network_close_socket( my_client_socket );
	}
	return err;
}


// if sucessful, starts a server on this port
bool network_init_server( int port )
{
	struct sockaddr_in name;

	network_initialize();

	my_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(  my_socket==INVALID_SOCKET  ) {
		dbg->fatal("init_server()", "Fail to open socket!");
		return false;
	}

	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if(  bind(my_socket, (struct sockaddr *)&name, sizeof(name))==-1  ) {
		dbg->fatal("init_server()", "Bind failed!");
		return false;
	}

	if(  listen(my_socket, MAX_PLAYER_COUNT /* max pending connections */)==-1  ) {
		dbg->fatal("init_server()", "Listen failed for %i sockets!", MAX_PLAYER_COUNT );
		return false;
	}

	active_clients = 0;
	network_add_client( my_socket );
	pending[0] = 0;
	client_id = 0;

	return true;
}

void network_add_client( SOCKET sock )
{
	if(  !clients.is_contained(sock)  ) {
		// do not wait to join small (command) packets when sending (may cause 200ms delay!)
		setsockopt( sock, SOL_SOCKET, TCP_NODELAY, NULL, 0 );
		if (active_clients < clients.get_count()) {
			for(uint32 i=0; i<clients.get_count(); i++) {
				if (clients[i]==INVALID_SOCKET) {
					clients[i]=sock;
					active_clients++;
					break;
				}
			}
		}
		else {
			clients.append( sock );
			active_clients++;
		}
	}
}


static void network_remove_client( SOCKET sock )
{
	if(  clients.is_contained(sock)  &&  active_clients>0) {
		uint32 ind = clients.index_of(sock);
		clients[ind] = INVALID_SOCKET;
		active_clients--;
		network_close_socket(sock);
	}
}


// number of currently active clients
int network_get_clients()
{
	// all clients except ourselves
	return active_clients - 1;
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
 * - recieve commands
 */
SOCKET network_check_activity(int timeout, char *buf, int &len )
{
	if(  pending[0]  ) {
		len = strlen(pending);
		tstrncpy( buf, pending, len+1 );
		pending[0] = 0;
		return 1;
	}

	fd_set fds;
	FD_ZERO(&fds);

	int s_max = fill_set(&fds);

	// time out
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = max(0,timeout)*1000;

	int action = select(s_max, &fds, NULL, NULL, &tv );

	if(  action<=0  ) {
		// timeout
		return INVALID_SOCKET;
	}

	if(  my_socket!=INVALID_SOCKET  &&  FD_ISSET(my_socket, &fds)  ) {
		struct sockaddr_in client_name;
		socklen_t size = sizeof(client_name);
		SOCKET s = accept(my_socket, (struct sockaddr *)&client_name, &size);
		if(  s!=INVALID_SOCKET  ) {
#ifdef  __BEOS__
			dbg->message("check_activity()", "Accepted connection from: %lh.\n", client_name.sin_addr.s_addr );
#else
			dbg->message("check_activity()", "Accepted connection from: %s.\n", inet_ntoa(client_name.sin_addr) );
#endif
			network_add_client(s);
		}
		// not a request
		return s;
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
			return INVALID_SOCKET;
		}
		// recieve only one command; they are seperated by ';'
		int bytes = 0;
		do {
			FD_ZERO(&fds);
			FD_SET(sender,&fds);
			tv.tv_usec = 0;
			if(  select((int)sender+1, &fds, NULL, NULL, &tv )!=1  ) {
				break;
			}
			if(  recv((SOCKET)sender, buf+bytes, 1, 0 )==0  ) {
				network_remove_client( sender );
				len = 0;
				return INVALID_SOCKET;
			}
			bytes ++;
		} while(  bytes+1<len  &&  buf[bytes-1]!=*NET_END_CMD  );
		buf[bytes] = 0;
		dbg->warning( "network_check_activity()", "recieved '%s'", buf );
		// read something sucessful
		len = bytes;
		return sender;
	}
	return INVALID_SOCKET;
}


bool network_check_server_connection()
{
	if(  !umgebung_t::server  ) {
		// I am client

		if(  clients.get_count()==0  ) {
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
void network_send_all(char *msg, int len, bool exclude_us )
{
	dbg->warning( "network_send_all()", msg );
	for(uint32 i=0; i<clients.get_count(); i++) {
		if (clients[i]!=INVALID_SOCKET) {
			SOCKET s = clients[i];
			if(exclude_us  &&  s==my_socket) {
				continue;
			}
			send( s, msg, len, 0 );
		}
	}
	if(  !exclude_us  &&  my_socket!=INVALID_SOCKET  ) {
		// I am the server
		tstrncpy( pending, msg, min(4096,len+1) );
	}
}


// send data to server
void network_send_server(char *msg, int len )
{
	dbg->warning( "network_send_server()", msg );
	if(  !umgebung_t::server  ) {
		// I am client
		send( clients[0], msg, len, 0 );
	}
	else {
		// I am the server
		tstrncpy( pending, msg, min(4096,len+1) );
	}
}


const char *network_send_file( SOCKET s, const char *filename )
{
	FILE *fp = fopen(filename,"rb");
	char buffer[1024];

	// find out length
	fseek(fp, 0, SEEK_END);
	long i = (long)ftell(fp);
	rewind(fp);

	// initial: size of file
	char command[128];
	int n = sprintf( command, NET_FROM_SERVER NET_GAME " %lu,%li" NET_END_CMD, clients.index_of(s), i );
	dbg->warning( "network_send_file()", command );
	if(  send(s,command,n,0)==-1  ) {
		network_remove_client(s);
		return "Client closed connection during transfer";
	}

	while(  !feof(fp)  ) {
		size_t bytes_read = fread( buffer, 1, sizeof(buffer), fp );
		if(  send(s,buffer,bytes_read,0)==-1) {
			network_remove_client(s);
			return "Client closed connection during transfer";
		}
	}

	// ok, new client has savegame
	return NULL;
}


const char *network_recieve_file( SOCKET s, const char *save_as, const long length )
{
	// ok, we have a socket to connect
	remove(save_as);

	DBG_MESSAGE("network_recieve_file","Game size %li", length );

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
	return NULL;
}


void network_close_socket( SOCKET sock )
{
	if(  sock != INVALID_SOCKET  ) {
#ifdef WIN32
		closesocket( sock );
#else
		close( sock );
#endif
		if(  sock==my_socket  ) {
			my_socket = INVALID_SOCKET;
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
	network_close_socket( my_socket );
	network_close_socket( my_client_socket );
	if(network_active) {
#if defined(WIN32)
		WSACleanup();
#endif
	}
	network_active = false;
}

