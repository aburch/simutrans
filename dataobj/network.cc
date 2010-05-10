/* basic network functionality, borrowed from OpenTTD */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "network.h"
#include "network_cmd.h"

#ifdef __BEOS__
#include <net/netdb.h>
#include <net/sockets.h>
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
#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/slist_tpl.h"

#ifdef WIN32
#define socklen_t int
#endif

static bool network_active = false;
// local server cocket
static SOCKET my_socket = INVALID_SOCKET;
// local client socket
static SOCKET my_client_socket = INVALID_SOCKET;
static slist_tpl<const char *>pending_list;

// to query all open sockets, we maintain this list
static vector_tpl<SOCKET> clients;
static uint32 active_clients;

// list of commands on the server
static slist_tpl<network_command_t *> server_command_queue;

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
#if defined(__BEOS__)
	struct hostent *theHost;
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
		dbg->warning( "network_open_address", err_str );
		return err_str;
	}
	active_clients = 0;
	server_command_queue.clear();

	return NULL;
}


// connect to address (cp), receive game, save to client%i-network.sve
const char *network_connect(const char *cp)
{
	// open from network
	const char *err = network_open_address( cp );
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
		err = network_recieve_file( my_client_socket, filename, len );
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
	client_id = 0;
	server_command_queue.clear();

	return true;
}

// get server socket
SOCKET network_get_server()
{
	return clients.get_count()>0 ? clients[0] : INVALID_SOCKET;
}

void network_add_client( SOCKET sock )
{
	if(  !clients.is_contained(sock)  ) {
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
	if(  clients.is_contained(sock)  &&  active_clients>0) {
		uint32 ind = clients.index_of(sock);
		clients[ind] = INVALID_SOCKET;
		active_clients--;
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
 * - recieve commands
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
		// recieve only one command
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
			dbg->warning( "network_check_activity()", "recieved cmd id=%d %s", nwc->get_id(), nwc->get_name());
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
				if(s==my_socket) {
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



uint16 network_recieve_data( SOCKET sender, void *dest, const uint16 length )
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
#if defined(__HAIKU__)
		// no closesocket() ?!?
#elif defined(WIN32)  ||  defined(__BEOS__)
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
