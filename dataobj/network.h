#ifndef sim_network_h
#define sim_network_h


// windows headers
#ifdef WIN32
// must be include before all simutrans stuff!

// first: we must find out version number
#ifndef WINVER
#	define _WINSOCKAPI_
#	include <windows.h>
#	undef _WINSOCKAPI_
#endif

// then we know which winsock version to use
#if WINVER < 0x0500
#	include <winsock.h>
#	define USE_IP4_ONLY
#	define socklen_t int
#else
#	include <Windows.h>
#	include <WinSock2.h>
#	include <ws2tcpip.h>
#endif
#	undef min
#	undef max
#	ifndef IPV6_V6ONLY
#		define IPV6_V6ONLY (27)
#	endif

#	define GET_LAST_ERROR() WSAGetLastError()
#	undef  EWOULDBLOCK
#	define EWOULDBLOCK WSAEWOULDBLOCK
#else
	// beos specific headers
#	ifdef  __BEOS__
#		include <net/netdb.h>
#		include <net/sockets.h>

	// non-beos / non-windows
#	else
#		include <sys/types.h>
#		include <sys/socket.h>
#		include <netdb.h>
#		include <unistd.h>
#		include <arpa/inet.h>
#		include <netinet/in.h>
#	endif
#   ifdef  __HAIKU__
#		include <sys/select.h>
#   endif

// all non-windows
#	include <fcntl.h>
#	include <errno.h>
	// to keep compatibility to MS windows
	typedef int SOCKET;
#	define INVALID_SOCKET -1
#	define GET_LAST_ERROR() (errno)
#endif

#include "../simtypes.h"
// version of network protocol code
#define NETWORK_VERSION (1)

class network_command_t;
class gameinfo_t;
class karte_t;

bool network_initialize();

// connect to address with patch name receive to localname, close
const char *network_download_http( const char *address, const char *name, const char *localname );

// connect to address (cp), receive gameinfo, close
const char *network_gameinfo(const char *cp, gameinfo_t *gi);

// connects to server at (cp), receives game, save to client%i-network.sve
const char* network_connect(const char *cp);

void network_close_socket( SOCKET sock );

void network_set_socket_nodelay( SOCKET sock );

// if sucessful, starts a server on this port
bool network_init_server( int port );

/**
 * returns pointer to commmand or NULL
 */
network_command_t* network_get_received_command();

/**
 * do appropriate action for network games:
 * - server: accept connection to a new client
 * - all: receive commands and puts them to the received_command_queue
 *
 * @param timeout in milliseconds
 * @returns pointer to first received commmand
 * more commands can be obtained by call to network_get_received_command
 */
network_command_t* network_check_activity(karte_t *welt, int timeout);

/**
 * receive data from sender
 * @param dest the destination buffer
 * @param len length of destination buffer and number of bytes to be received
 * @param received number of received bytes is returned here
 * @returns true if connection is still valid, false if an error occurs and connection needs to be closed
 */
bool network_receive_data( SOCKET sender, void *dest, const uint16 len, uint16 &received );

void network_process_send_queues(int timeout);

// before calling this, the server should have saved the current game as "server-network.sve"
const char *network_send_file( uint32 client_id, const char *filename );

// true, if I can wrinte on the server connection
bool network_check_server_connection();

// send data to all clients (even us)
// nwc is invalid after the call
void network_send_all(network_command_t* nwc, bool exclude_us );

// send data to server only
// nwc is invalid after the call
void network_send_server(network_command_t* nwc );

void network_reset_server();

void network_core_shutdown();

// get our id on the server
uint32 network_get_client_id();

#endif
