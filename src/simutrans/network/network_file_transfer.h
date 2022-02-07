/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_NETWORK_FILE_TRANSFER_H
#define NETWORK_NETWORK_FILE_TRANSFER_H


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
const char *network_connect(const char *cp, karte_t *world);

/// Send file over network
const char *network_send_file(const SOCKET dst_sock, const char *filename);

/// Receive file (directly to disk)
const char *network_receive_file(const SOCKET src_sock, const char *const save_as, const sint32 length, const sint32 timeout=10000);

/**
 * Use HTTP POST request to submit poststr to an HTTP server
 * Any response is saved to the file given by localname (pass NULL to ignore response)
 * Connection is closed after request is completed
 */
const char *network_http_post ( const char *address, const char *name, const char *poststr, const char *localname );

/**
 * Use HTTP to retrieve a file into the cbuffer_t object provided
 */
const char *network_http_get ( const char *address, const char *name, cbuffer_t& local );

/**
 * Use HTTP to retrieve a file into the FILE
 */
const char* network_http_get_file( const char* address, const char* name, const char *filename );


#endif
