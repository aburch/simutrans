#ifndef sim_network_h
#define sim_network_h

// windows headers
#ifdef WIN32
#	include <WinSock2.h>
#	include <ws2tcpip.h>
#	undef min
#	undef max

#	ifndef IPV6_V6ONLY
#		define IPV6_V6ONLY (27)
#	endif
#else
	// beos specific headers
#	ifdef __BEOS__
#		include <net/netdb.h>
#		include <net/sockets.h>
#		include <sys/select.h>
	// non-beos / non-windows
#	else
#		include <sys/types.h>
#		include <sys/socket.h>
#		include <netdb.h>
#		include <unistd.h>
#		include <arpa/inet.h>
#	endif
// non-windows
#	include <fcntl.h>
#	include <errno.h>
	// to keep compatibility to MS windows
	typedef int SOCKET;
#	define INVALID_SOCKET -1
#endif

#if 0
#ifdef WIN32
// must be include before all simutrans stuff!
#include <winsock.h>
#undef min
#undef max
#else
#include <sys/types.h>
#include <sys/socket.h>
#ifdef __BEOS__
#include <net/socket.h>
#define PF_INET AF_INET
#define socklen_t int
#else
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif
#endif
#endif

#include "../simtypes.h"
// version of network protocol code
#define NETWORK_VERSION (1)

class network_command_t;
class gameinfo_t;

bool network_initialize();

// connect to address with patch name receive to localname, close
const char *network_download_http( const char *address, const char *name, const char *localname );

// connect to address (cp), receive gameinfo, close
const char *network_gameinfo(const char *cp, gameinfo_t *gi);

// connects to server at (cp), receives game, save to client%i-network.sve
const char* network_connect(const char *cp);

void network_close_socket( SOCKET sock );

void network_add_client( SOCKET sock );
void network_remove_client( SOCKET sock  );
uint32 network_get_client_id( SOCKET sock );
SOCKET network_get_socket( uint32 client_id );

// if sucessful, starts a server on this port
bool network_init_server( int port );

/* do appropriate action for network server:
 * - either connect to a new client
 * - receive commands
 * returns pointer to commmand or NULL
 * timeout in milliseconds
 */
network_command_t* network_check_activity(int timeout);

// receives x bytes from socket sender
uint16 network_receive_data( SOCKET sender, void *dest, const uint16 length );

// before calling this, the server should have saved the current game as "server-network.sve"
const char *network_send_file( uint32 client_id, const char *filename );

// this saves the game from network under "client-network.sve"
const char *network_receive_file( SOCKET s, const char *name, const long len );

// number of currently active clients
int network_get_clients();

// true, if I can wrinte on the server connection
bool network_check_server_connection();

// send data to all clients (even us)
// nwc is invalid after the call
void network_send_all(network_command_t* nwc, bool exclude_us );

// send data to server only
// nwc is invalid after the call
void network_send_server(network_command_t* nwc );

void network_core_shutdown();

// get our id on the server
uint32 network_get_client_id();

#endif
