#ifndef NETWORK_FILE_TRANSFER_H
#define NETWORK_FILE_TRANSFER_H

/**
 * Contains functions to send & receive files over network
 * .. and to connect to a running simutrans server
 */

#include "network.h"

class cbuffer_t;
class karte_t;
class gameinfo_t;

// connect to address (cp), receive gameinfo, close
const char *network_gameinfo(const char *cp, gameinfo_t *gi);

// connects to server at (cp), receives game, save to client%i-network.sve
const char* network_connect(const char *cp, karte_t *world);

// sending file over network
const char *network_send_file( uint32 client_id, const char *filename );

// receive file (directly to disk)
char const* network_receive_file(SOCKET const s, char const* const save_as, const long length, const long timeout=10000 );

/*
 * Use HTTP POST request to submit poststr to an HTTP server
 * Any response is saved to the file given by localname (pass NULL to ignore response)
 * Connection is closed after request is completed
 * @author Timothy Baldock <tb@entropy.me.uk>
 */
const char *network_http_post ( const char *address, const char *name, const char *poststr, const char *localname );

/*
 * Use HTTP to retrieve a file into the cbuffer_t object provided
 * @author Timothy Baldock <tb@entropy.me.uk>
 */
const char *network_http_get ( const char *address, const char *name, cbuffer_t& local );

#endif
