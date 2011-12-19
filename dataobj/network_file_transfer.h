#ifndef NETWORK_FILE_TRANSFER_H
#define NETWORK_FILE_TRANSFER_H

/**
 * Contains functions to send & receive files over network
 * .. and to connect to a running simutrans server
 */

#include "network.h"

class karte_t;
class gameinfo_t;

// connect to address (cp), receive gameinfo, close
const char *network_gameinfo(const char *cp, gameinfo_t *gi);

// connects to server at (cp), receives game, save to client%i-network.sve
const char* network_connect(const char *cp, karte_t *world);

// sending file over network
const char *network_send_file( uint32 client_id, const char *filename );

// receive file
char const* network_receive_file(SOCKET const s, char const* const save_as, long const length);

// POST message data in poststr to an HTTP server and then receive the response
// The response is saved to file given by localname, closes the connection afterwards
const char *network_http_post( const char *address, const char *name, const char *poststr, const char *localname );

// connect to address with path name, receive to localname, close
const char *network_download_http( const char *address, const char *name, const char *localname );

#endif
