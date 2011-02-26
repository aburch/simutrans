/**
 * Contains functions to send & receive files over network
 * .. and to connect to a running simutrans server
 */
#include "network_file_transfer.h"

#include "network_cmd.h"
#include "network_socket_list.h"

#include "loadsave.h"
#include "gameinfo.h"

#include "../simconst.h"

#include "../simdebug.h"
#include "../simgraph.h"
#include "../simworld.h"

#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"

#include "../utils/simstring.h"
#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"

// global client id
extern uint32 client_id;

// forward declaration ..
char const* network_receive_file(SOCKET s, char const* save_as, long length);

// connect to address (cp), receive gameinfo, close
const char *network_gameinfo(const char *cp, gameinfo_t *gi)
{
	// open from network
	const char *err = NULL;
	SOCKET my_client_socket = network_open_address( cp, 5000, err );
	if(  err==NULL  ) {
		{
			nwc_gameinfo_t nwgi;
			nwgi.rdwr();
			if(  !nwgi.send(my_client_socket)  ) {
				err = "send of NWC_GAMEINFO failed";
				goto end;
			}
		}
		socket_list_t::add_client( my_client_socket );
		// wait for join command (tolerate some wrong commands)
		network_command_t *nwc = NULL;
		nwc = network_check_activity( NULL, 10000 );	// 10s should be enough for reply ...
		if (nwc==NULL) {
			err = "Server did not respond!";
			goto end;
		}
		nwc_gameinfo_t *nwgi = dynamic_cast<nwc_gameinfo_t*>(nwc);
		if (nwgi==NULL) {
			err = "Protocoll error (expected NWC_GAMEINFO)";
			goto end;
		}
		if (nwgi->len==0) {
			err = "Server busy";
			goto end;
		}
		uint32 len = nwgi->len;
		char filename[1024];
		sprintf( filename, "client%i-network.sve", nwgi->len );
		err = network_receive_file( my_client_socket, filename, len );
		// now into gameinfo
		loadsave_t fd;
		if(  fd.rd_open( filename )  ) {
			gameinfo_t *pgi = new gameinfo_t( &fd );
			*gi = *pgi;
			delete pgi;
			fd.close();
		}
		remove( filename );
		socket_list_t::remove_client( my_client_socket );
	}
end:
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
	SOCKET my_client_socket = network_open_address( cp, 5000, err );
	if(  err==NULL  ) {
		// want to join
		{
			nwc_join_t nwc_join;
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
		client_id = nwj->client_id;
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
		// wait for game command (tolerate some wrong commands)
		for(uint8 i=0; i<2; i++) {
			nwc = network_check_activity( NULL, 60000 );
			if (nwc  &&  nwc->get_id() == NWC_GAME) break;
		}
		if (nwc == NULL  ||  nwc->get_id()!=NWC_GAME) {
			err = "Protocol error (expected NWC_GAME)";
			goto end;
		}
		int len = ((nwc_game_t*)nwc)->len;
		// guaranteed individual file name ...
		char filename[256];
		sprintf( filename, "client%i-network.sve", client_id );
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


const char *network_send_file( uint32 client_id, const char *filename )
{
	FILE *fp = fopen(filename,"rb");
	char buffer[1024];

	// find out length
	fseek(fp, 0, SEEK_END);
	long length = (long)ftell(fp);
	rewind(fp);

	// socket
	SOCKET s = socket_list_t::get_socket(client_id);
	if (s==INVALID_SOCKET) {
		return "Client closed connection during transfer";
	}
	// send size of file
	nwc_game_t nwc(length);
	if (!nwc.send(s)) {
		return "Client closed connection during transfer";
	}

	// good place to show a progress bar
	if(is_display_init()  &&  length>0) {
		display_set_progress_text(translator::translate("Transferring game ..."));
		display_progress(0, length);
	}
	long bytes_sent = 0;
	while(  !feof(fp)  ) {
		int bytes_read = (int)fread( buffer, 1, sizeof(buffer), fp );
		uint16 dummy;
		if( !network_send_data(s, buffer, bytes_read, dummy, 250) ) {
			socket_list_t::remove_client(s);
			return "Client closed connection during transfer";
		}
		bytes_sent += bytes_read;
		display_progress(bytes_sent, length);
	}

	// ok, new client has savegame
	return NULL;
}


char const* network_receive_file(SOCKET const s, char const* const save_as, long const length)
{
	// ok, we have a socket to connect
	remove(save_as);

	DBG_MESSAGE("network_receive_file","Game size %li", length );

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
	if (FILE* const f = fopen(save_as, "wb")) {
		while (length_read < length) {
			int i = recv(s, rbuf, length_read + 4096 < length ? 4096 : length - length_read, 0);
			if (i > 0) {
				fwrite(rbuf, 1, i, f);
				length_read += i;
				display_progress(length_read, length);
			}
			else {
				if (i < 0) {
					dbg->error("loadsave_t::rd_open()", "recv failed with %i", i);
				}
				break;
			}
		}
		fclose(f);
	}
	if(  length_read<length  ) {
		return "Not enough bytes transferred";
	}
	return NULL;
}
