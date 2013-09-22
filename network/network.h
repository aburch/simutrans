#ifndef sim_network_h
#define sim_network_h


// windows headers
#ifdef _WIN32
// must be include before all simutrans stuff!

#define NOMINMAX 1

// So that windows.h won't include all kinds of things
#define WIN32_LEAN_AND_MEAN

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
#	ifndef IPV6_V6ONLY
#		define IPV6_V6ONLY (27)
#	endif

#	define GET_LAST_ERROR() WSAGetLastError()
#	include <errno.h>
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
#		include <netinet/tcp.h>
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

void network_close_socket( SOCKET sock );

void network_set_socket_nodelay( SOCKET sock );

// open a socket or give a decent error message
SOCKET network_open_address(char const* cp, char const*& err);

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
 * @return pointer to first received commmand
 * more commands can be obtained by call to network_get_received_command
 */
network_command_t* network_check_activity(karte_t *welt, int timeout);

/**
 * send data to dest:
 * if timeout_ms is positive:
 *    try to send all data, return true if all data are sent otherwise false
 * if timeout_ms is not positive:
 *    try to send as much as possible but return after one send attempt
 *    return true if connection is still open and sending can be continued later
 *
 * @param buf the data
 * @param count length of buffer and number of bytes to be sent
 * @param sent number of bytes sent
 * @param timeout_ms time-out in milli-seconds
 */
bool network_send_data( SOCKET dest, const char *buf, const uint16 size, uint16 &count, const int timeout_ms );

/**
 * receive data from sender
 * @param dest the destination buffer
 * @param len length of destination buffer and number of bytes to be received
 * @param received number of received bytes is returned here
 * @param timeout_ms time-out in milli-seconds
 * @return true if connection is still valid, false if an error occurs and connection needs to be closed
 */
bool network_receive_data( SOCKET sender, void *dest, const uint16 len, uint16 &received, const int timeout_ms );

void network_process_send_queues(int timeout);

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

// get & set our id on the server
uint32 network_get_client_id();
void network_set_client_id(uint32 id);

#endif
