/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "network_file_transfer.h"
#include "../simdebug.h"
#include "../simloadingscreen.h"
#include "../sys/simsys.h"

#include <string.h>
#include <errno.h>
#include "../utils/cbuffer.h"

#ifndef NETTOOL
#include "../dataobj/translator.h"
#else
#define dr_remove remove
#define dr_fopen fopen
#endif
#include "../simversion.h"


/*
 * Functions required by both Simutrans and Nettool
 */

const char *network_receive_file(const SOCKET src_sock, const char *const save_as, sint32 const length, sint32 const timeout )
{
	// ok, we have a socket to connect
	dr_remove(save_as);

	DBG_MESSAGE("network_receive_file", "File size %i", length );

	if(length>0) {
#ifndef NETTOOL // no display, no translator available
		loadingscreen_t ls(translator::translate("Downloading"),length,true,true);
#endif

		// good place to show a progress bar
		char rbuf[4096];
		sint32 length_read = 0;
		if (FILE* const f = dr_fopen(save_as, "wb")) {
			while(length_read < length) {
				if(  timeout > 0  ) {
					/** 10s for 4096 bytes:
					 * As long as you are not connected with less than 1200 Baud that should be fine
					 * otherwise upgrade your acoustic coupler to 56k ...
					 */
					fd_set fds;
					FD_ZERO(&fds);
					FD_SET(src_sock,&fds);
					struct timeval tv; // 10 s timeout
					tv.tv_sec = 10000 / 1000;
					tv.tv_usec = (10000 % 1000) * 1000ul;
					// can we read?
					if(  select( FD_SETSIZE, &fds, NULL, NULL, &tv )!=1  ) {
						dbg->warning("network_receive_file", "Timeout during transfer: %s", strerror(errno) );
						break;
					}
				}
				// ok, now here should be something new to read
				int i = recv(src_sock, rbuf, length_read + 4096 < length ? 4096 : length - length_read, 0);
				if (i > 0) {
					fwrite(rbuf, 1, i, f);
					length_read += i;
#ifndef NETTOOL
					ls.set_progress(length_read);
#endif
				}
				else {
					if (i < 0) {
						dbg->warning("network_receive_file", "recv failed with %i", i);
					}
					break;
				}
			}
			fclose(f);
		}
		if(  length_read<length  ) {
			return "Not enough bytes transferred";
		}
	}
	return NULL;
}

/*
 * Functions not needed by Nettool following below
 */
#ifndef NETTOOL

#include "network_cmd.h"
#include "network_cmd_ingame.h"
#include "network_socket_list.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/gameinfo.h"
#include "../dataobj/environment.h"
#include "../world/simworld.h"
#include "../utils/simstring.h"


// connect to address (cp), receive gameinfo, close
const char *network_gameinfo(const char *cp, gameinfo_t *gi)
{
	// open from network
	const char *err = NULL;
	SOCKET const my_client_socket = network_open_address(cp, err);
	if(  err==NULL  ) {
		network_command_t *nwc;
		nwc_gameinfo_t *nwgi;
		uint32 len;
		char filename[PATH_MAX];
		loadsave_t fd;

		socket_list_t::add_client( my_client_socket );
		{
			nwc_gameinfo_t nwgi;
			nwgi.rdwr();
			if(  !nwgi.send(my_client_socket)  ) {
				err = "send of NWC_GAMEINFO failed";
				goto end;
			}
		}
		// wait for join command (tolerate some wrong commands)
		nwc = network_check_activity( NULL, 10000 ); // 10s should be enough for reply ...
		if (nwc==NULL) {
			err = "Server did not respond!";
			goto end;
		}
		nwgi = dynamic_cast<nwc_gameinfo_t*>(nwc);
		if (nwgi==NULL) {
			err = "Protocol error (expected NWC_GAMEINFO)";
			goto end;
		}
		if (nwgi->len==0) {
			err = "Server busy";
			goto end;
		}
		len = nwgi->len;
		sprintf( filename, "client%i-network.sve", nwgi->len );
		err = network_receive_file( my_client_socket, filename, len );

		if (err == NULL) {
			// now into gameinfo
			const loadsave_t::file_status_t status = fd.rd_open( filename );

			if(  status == loadsave_t::FILE_STATUS_ERR_FUTURE_VERSION  ) {
				err = "Server version too new";
			}
			else if(  status == loadsave_t::FILE_STATUS_ERR_NO_VERSION  ) {
				err = "Unknown server version";
			}
			else if(  status != loadsave_t::FILE_STATUS_OK  ) {
				err = "Server busy";
			}
			else if(  fd.is_version_less(120, 8)  ) {
				// Querying gameinfo of older server versions may crash the client
				// See gameinfo_t::rdwr
				err = "Server version too old";
			}
			else {
				*gi = gameinfo_t( &fd );
			}

			fd.close();
		}
		dr_remove( filename );
end:
		socket_list_t::remove_client( my_client_socket );
	}
	network_close_socket( my_client_socket );
	if(err) {
		dbg->warning("network_gameinfo", err);
	}
	return err;
}


// connect to address (cp), receive game, save to client%i-network.sve
const char *network_connect(const char *cp, karte_t *world)
{
	// open from network
	const char *err = NULL;
	SOCKET const my_client_socket = network_open_address(cp, err);
	if(  err==NULL  ) {
		// want to join
		{
			nwc_join_t nwc_join( env_t::nickname.c_str() );
			nwc_join.rdwr();
			if (!nwc_join.send(my_client_socket)) {
				err = "send of NWC_JOIN failed";
				goto end;
			}
		}
		socket_list_t::reset();
		socket_list_t::add_client(my_client_socket);
		// wait for join command (tolerate some wrong commands)
		network_command_t *nwc = NULL;
		for(uint8 i=0; i<5; i++) {
			nwc = network_check_activity( NULL, 10000 );
			if (nwc  &&  nwc->get_id() == NWC_JOIN) break;
		}
		if (nwc==NULL) {
			err = "Server did not respond!";
			goto end;
		}
		nwc_join_t *nwj = dynamic_cast<nwc_join_t*>(nwc);
		if (nwj==NULL) {
			err = "Protocol error (expected NWC_JOIN)";
			goto end;
		}
		if (nwj->answer!=1) {
			err = "Server busy";
			goto end;
		}
		// set nickname
		env_t::nickname = nwj->nickname.c_str();

		network_set_client_id(nwj->client_id);
		// update map counter
		// wait for sync command (tolerate some wrong commands)
		for(  uint8 i=0;  i<5;  ++i  ) {
			nwc = network_check_activity( NULL, 10000 );
			if(  nwc  &&  nwc->get_id()==NWC_SYNC  ) break;
		}
		if(  nwc==NULL  ||  nwc->get_id()!=NWC_SYNC  ) {
			err = "Protocol error (expected NWC_SYNC)";
			goto end;
		}
		world->set_map_counter( ((nwc_sync_t*)nwc)->get_new_map_counter() );
		// receive nwc_game_t
		{
#ifndef NETTOOL // no display, no translator available
			loadingscreen_t ls(translator::translate("Server preparing game ..."),300,true,true);
#endif
			// wait for game command for 5 min (tolerate some wrong commands) to leave it enough time for saving
			for(int i=0; i<300; i++) {
#ifndef NETTOOL // no display, no translator available
				ls.set_progress(i);
#endif
				nwc = network_check_activity( NULL, 1000 );
				if (nwc  &&  nwc->get_id() == NWC_GAME) {
					break;
				}
			}
		}
		if (nwc == NULL  ||  nwc->get_id()!=NWC_GAME) {
			err = "Protocol error (expected NWC_GAME)";
			goto end;
		}
		int len = ((nwc_game_t*)nwc)->len;
		// guaranteed individual file name ...
		char filename[256];
		sprintf( filename, "client%i-network.sve", network_get_client_id() );
		err = network_receive_file( my_client_socket, filename, len );
	}
end:
	if(err) {
		dbg->warning("network_connect", err);
		if (!socket_list_t::remove_client(my_client_socket)) {
			network_close_socket( my_client_socket );
		}
	}
	else {
		const uint32 id = socket_list_t::get_client_id(my_client_socket);
		socket_list_t::change_state(id, socket_info_t::playing);
	}
	return err;
}


const char *network_send_file( const SOCKET dst_sock, const char *filename )
{
	FILE *fp = dr_fopen(filename,"rb");
	if (fp == NULL) {
		dbg->warning("network_send_file", "could not open file %s", filename);
		return "Could not open file";
	}
	char buffer[1024];

	// find out length
	fseek(fp, 0, SEEK_END);
	long length = (long)ftell(fp);
	rewind(fp);
	uint32 bytes_sent = 0;

	// send size of file
	nwc_game_t nwc(length);
	if (dst_sock==INVALID_SOCKET  ||  !nwc.send(dst_sock)) {
		goto error;
	}

	// good place to show a progress bar
	if(length>0) {
		loadingscreen_t ls( translator::translate("Transferring game ..."), length, true, true );

		while(  !feof(fp)  ) {
			int bytes_read = (int)fread( buffer, 1, sizeof(buffer), fp );
			uint16 dummy;
			if( !network_send_data(dst_sock, buffer, bytes_read, dummy, 250) ) {
				socket_list_t::remove_client(dst_sock);
				goto error;
			}

			bytes_sent += bytes_read;
			ls.set_progress( bytes_sent );
		}
	}

	// ok, new client has savegame
	fclose(fp);
	return NULL;

error:
	// an error occurred: close file
	fclose(fp);
	return "Client closed connection during transfer";
}

/// POST a message (poststr) to an HTTP server at the specified address and relative path (name)
/// Optionally: Receive response to file localname
const char *network_http_post( const char *address, const char *name, const char *poststr, const char *localname )
{
	DBG_MESSAGE("network_http_post", "");
	// Open socket
	const char *err = NULL;
	SOCKET const my_client_socket = network_open_address(address, err);
	if (err==NULL) {
#ifndef REVISION
#	define REVISION 0
#endif
		const char* format = "POST %s HTTP/1.1\r\n"
				"User-Agent: Simutrans/r%s\r\n"
				"Host: %s\r\n"
				"Content-Type: application/x-www-form-urlencoded\r\n"
				"Content-Length: %d\r\n\r\n%s";
		if ((strlen(format) + strlen(name) + strlen(address) + strlen(poststr) + strlen(QUOTEME(REVISION))) > 4060) {
			// We will get a buffer overwrite here if we continue
			dbg->fatal( "network_http_post", "Error: String too long (>4096)" );
		}
		DBG_MESSAGE("network_http_post", "2");
		char request[4096];
		int const len = sprintf(request, format, name, QUOTEME(REVISION), address, strlen(poststr), poststr);
		uint16 dummy;
		if (!network_send_data(my_client_socket, request, len, dummy, 250)) {
			err = "Server did not respond!";
		}


		DBG_MESSAGE("network_http_post", "3");
		// Read the response header
		// line is max length of a header line
		// rbuf is a single char
		char line[1024], rbuf;
		unsigned int pos = 0;
		sint32 length = 0;

		// TODO better handling of error message from listing server // TODO

		while(1) {
			// Returns number of bytes received
			// Receive one char from socket into rbuf
			int i = recv( my_client_socket, &rbuf, 1, 0 );
			if(  i>0  ) {
				// If char is above 32 in ascii table + not going to overflow line[]
				// Note: This will truncate any header to 1023 chars!
				// Add char to line at the next position
				// This ignores non-printable chars, including the CR char
				if(  rbuf>=32  &&  pos<sizeof(line)-1  ) {
					line[pos++] = rbuf;
				}
				// This doesn't follow the HTTP spec! Headers can be split across multiple lines if the next line is whitespace!
				// Should also accept tab char as whitespace (but maybe convert to space?)
				// If char is an LF (\n) then this signifies an EOL
				if(  rbuf==10  ) {
					// If EOL at position 0, blank line (remember we ignore code points less than 32, e.g. the CR)
					// Next line will be data
					if(  pos == 0  ) {
						DBG_MESSAGE("network_http_post", "all headers received, message body follows");
						// this line was empty => now data will follow
						break;
					}
					// NUL at the end of our constructed line string
					line[pos] = 0;
					// we only need the length tag
					// Compare string so far constructed against the header we
					// are seeking (e.g. Content-Length)
					DBG_MESSAGE("network_http_post", "received header: %s", line);
					if(  STRNICMP("Content-Length:",line,15)==0  ) {
						length = atol( line+15 );
					}
					// Begin again to parse the next line
					pos = 0;
				}
			} else {
				break;
			}
		}
		DBG_MESSAGE("network_http_post", "5");
		// for a simple query, just pass an empty filename
		if(  localname  &&  *localname  ) {
			err = network_receive_file( my_client_socket, localname, length );
		}
		network_close_socket( my_client_socket );
	}
	return err;
}

const char* network_http_get(const char* address, const char* name, cbuffer_t& local)
{
	const int REQ_HEADER_LEN = 1024;
	// open from network
	const char* err = NULL;
	SOCKET const my_client_socket = network_open_address(address, err);
	if (err == NULL) {
#ifndef REVISION
#	define REVISION 0
#endif
		const char* format = "GET %s HTTP/1.1\r\n"
			"User-Agent: Simutrans/r%s\r\n"
			"Host: %s\r\n\r\n";
		if ((strlen(format) + strlen(name) + strlen(address) + strlen(QUOTEME(REVISION))) > (REQ_HEADER_LEN - 1)) {
			// We will get a buffer overwrite here if we continue
			return "Error: String too long";
		}
		char request[REQ_HEADER_LEN];
		int const len = sprintf(request, format, name, QUOTEME(REVISION), address);
		uint16 dummy;
		if (!network_send_data(my_client_socket, request, len, dummy, 250)) {
			err = "Server did not respond!";
		}

		// Read the response header and parse arguments as needed
		char line[1024], rbuf;
		unsigned int pos = 0;
		sint32 length = 0;
		while (1) {
			// Receive one character at a time the HTTP headers
			int i = recv(my_client_socket, &rbuf, 1, 0);
			if (i > 0) {
				if (rbuf >= 32 && pos < sizeof(line) - 1) {
					line[pos++] = rbuf;
				}
				if (rbuf == 10) {
					if (pos == 0) {
						// this line was empty => now data will follow
						break;
					}
					line[pos] = 0;
					DBG_MESSAGE("network_http_get", "received header: %s", line);
					// Parse out the length tag to get length of content
					if (STRNICMP("Content-Length:", line, 15) == 0) {
						length = atol(line + 15);
					}
					pos = 0;
				}
			}
			else {
				break;
			}
		}

		// Make buffer to receive data into
		char* buffer = new char[length + 1];
		uint16 bytesreceived = 0;

		if (!network_receive_data(my_client_socket, buffer, length, bytesreceived, 10000)) {
			err = "Error: network_receive_data failed!";
		}
		else if (bytesreceived != length) {
			err = "Error: Bytes received does not match length!";
		}
		else {
			buffer[length] = 0;
			local.append(buffer, length);
		}

		DBG_MESSAGE("network_http_get", "received data length: %i", local.len());

		delete[] buffer;
		network_close_socket(my_client_socket);
	}
	return err;
}

const char *network_http_get_file( const char* address, const char* name, const char *filename )
{
	const int REQ_HEADER_LEN = 1024;
	// open from network
	const char *err = NULL;
	SOCKET const my_client_socket = network_open_address( address, err );
	if (  err==NULL  ) {
#ifndef REVISION
#	define REVISION 0
#endif
		const char* format = "GET %s HTTP/1.1\r\n"
				"User-Agent: Simutrans/r%s\r\n"
				"Host: %s\r\n\r\n";
		if (  (strlen( format ) + strlen( name ) + strlen( address ) + strlen( QUOTEME(REVISION)) ) > ( REQ_HEADER_LEN - 1 )  ) {
			// We will get a buffer overwrite here if we continue
			return "Error: String too long";
		}
		char request[REQ_HEADER_LEN];
		int const len = sprintf( request, format, name, QUOTEME(REVISION), address );
		uint16 dummy;
		if (  !network_send_data( my_client_socket, request, len, dummy, 250 )  ) {
			err = "Server did not respond!";
		}

		// Read the response header and parse arguments as needed
		char line[1024], rbuf;
		unsigned int pos = 0;
		sint32 length = 0;
		while(pos<lengthof(line)) {
			// Receive one character at a time the HTTP headers
			int i = recv( my_client_socket, &rbuf, 1, 0 );
			if (  i > 0  ) {
				if (  rbuf >= 32  &&  pos < sizeof(line) - 1  ) {
					line[pos++] = rbuf;
				}
				if (  rbuf == 10  ) {
					if (  pos > 0  &&  line[pos-1]==10) {
						line[pos++] = 10;
						line[pos++] = 0;
						// this line was empty => now data will follow
						break;
					}
					line[pos++] = 10;
				}
			}
			else {
				break;
			}
		}
		DBG_DEBUG("network_http_get_file()", "%s", line);
		int http_code = atoi(strchr(line, ' '));
		// if not sucessful maybe redirect
		if(http_code==302  ||  http_code==301) {
			network_close_socket(my_client_socket);

			char new_ip[1024];
			char new_path[1024];
			if(char *c=strstr(line,"\nLocation: http://")) {
				tstrncpy(new_ip, c + 18, lengthof(new_ip));
				if(char *c = strchr(new_ip, '/')) {
					tstrncpy(new_path, c, lengthof(new_path));
					strcpy( c, ":80");

					*strchr(new_path, 10) = 0;
					return network_http_get_file(new_ip, new_path, filename);
				}
			}
			if (char* c = strstr(line, "Location: https://")) {
				return "Cannot handole https.";
			}
			return "Unknown redirect.";
		}

		if (http_code == 200) {
			if (char *c=strstr(line,"Content-Length:")) {
				length = atol(c + 15);
			}
			DBG_MESSAGE("network_http_get", "received data length: %i", length);
			err = network_receive_file(my_client_socket, filename, length);
		}
		else {
			static char err_code[64];
			sprintf(err_code,"Cannot handle https", http_code);
			err = err_code;
		}

		network_close_socket( my_client_socket );
	}
	return err;
}

#endif
