#ifndef sim_network_h
#define sim_network_h

#ifdef WIN32
// must be include before all simutrans stuff!
#include <winsock.h>
#undef min
#undef max
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#ifndef __BEOS__
#include <arpa/inet.h>
#else
#define PF_INET AF_INET
#define socklen_t int
#endif

// to keep compatibility to MS windows
typedef int SOCKET;
#define INVALID_SOCKET -1
#endif

#include "../simtypes.h"

// prefiexes
#define NET_FROM_SERVER "do:"
#define NET_TO_SERVER "ask:"
#define NET_END_CMD ";"

// actual commands
#define NET_SYNC "sync"	/* saving and reloading game */
#define NET_INFO "info"
#define NET_GAME "game"
#define NET_READY "ready"	/* saving and reloading game */
#define NET_WKZ_INIT "init"	/* call a tool init */
#define NET_WKZ_WORK "work"	/* call a tool to work */
#define NET_CHECK "check"   /* check random counter */



bool network_initialize();

// connects to server at (cp), receives game, saves it to (filename)
const char* network_connect(const char *cp, const char *filename);

void network_close_socket( SOCKET sock );

void network_add_client( SOCKET sock );

// if sucessful, starts a server on this port
bool network_init_server( int port );

/* do appropriate action for network server:
 * - either connect to a new client
 * - recieve commands
 * returns len of recieved bytes, or when negative a negative socket number
 */
SOCKET network_check_activity(int timeout, char *buf, int &len );

// before calling this, the server should have saved the current game as "server-network.sve"
const char *network_send_file( SOCKET s, const char *filename );

// this saves the game from network under "client-network.sve"
const char *network_recieve_file( SOCKET s, const char *name, const long len );

// number of currently active clients
int network_get_clients();

// true, if I can wrinte on the server connection
bool network_check_server_connection();

// send data to all clients (even us)
void network_send_all(char *msg, int len, bool exclude_us );

// send data to server only
void network_send_server(char *msg, int len );

void network_core_shutdown();

// get our id on the server
uint32 network_get_client_id();
#endif
