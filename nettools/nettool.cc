#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../dataobj/network.h"
#include "../dataobj/network_cmd.h"
#include "../dataobj/network_packet.h"
#include "../dataobj/network_socket_list.h"
#include "../simtypes.h"
#include "../simversion.h"
#include "../utils/simstring.h"


// declaration of stuff from network.cc needed here.
SOCKET network_open_address( const char *cp, long timeout_ms, const char * &err);

// dummy implementation
// only receive nwc_service_t here
// called from network_check_activity
network_command_t* network_command_t::read_from_packet(packet_t *p)
{
	// check data
	if (p==NULL  ||  p->has_failed()  ||  !p->check_version()) {
		delete p;
		dbg->warning("network_command_t::read_from_packet", "error in packet");
		return NULL;
	}
	network_command_t* nwc = NULL;
	switch (p->get_id()) {
		case NWC_SERVICE:     nwc = new nwc_service_t(); break;
		default:
			dbg->warning("network_command_t::read_from_socket", "received unknown packet id %d", p->get_id());
	}
	if (nwc) {
		if (!nwc->receive(p) ||  p->has_failed()) {
			dbg->warning("network_command_t::read_from_packet", "error while reading cmd from packet");
			delete nwc;
			nwc = NULL;
		}
	}
	return nwc;
}

network_command_t* network_receive_command(uint16 id)
{
	// wait for command with id, ignore other commands
	for(uint8 i=0; i<5; i++) {
		network_command_t* nwc = network_check_activity( NULL, 10000 );
		if (nwc  &&  nwc->get_id() == id) {
			return nwc;
		}
		delete nwc;
	}
	dbg->warning("network_receive_command", "no command with id=%d received", id);
	return NULL;
}


int main(int argc, char* argv[])
{
	argv++, argc--;

	init_logging( "stderr", true, true, NULL );

	if (argc && !STRICMP(argv[0], "quiet")) {
		argv++, argc--;
	} else {
		puts(
			"\nNettool for simutrans " VERSION_NUMBER " and higher\n\n"
		);
	}

	if (argc) {
		const char *server_address = argv[0];
		argv++, argc--;
		const char *error = NULL;
		SOCKET socket = network_open_address( server_address, 100, error);
		if (error) {
			printf("Could not connect to server at %s: %s\n",server_address,error);
			return 1;
		}
		// now we are connected
		socket_list_t::add_client(socket);

		// server should talk to master server
		if (argc && !STRICMP(argv[0], "announce")) {
			argv++, argc--;
			nwc_service_t nwcs;
			nwcs.flag = nwc_service_t::SRVC_ANNOUNCE_SERVER;
			if (!nwcs.send(socket)) {
				return 2;
			}
			return 0;
		}

		// read password
		if (argc) {
			{
				// try to authenticate us
				nwc_service_t nwcs;
				nwcs.flag = nwc_service_t::SRVC_LOGIN_ADMIN;
				nwcs.text = strdup(ltrim(argv[0]));
				argv++, argc--;
				if (!nwcs.send(socket)) {
					printf("Could not send login data!\n");
					return 2;
				}
			}
			// wait for acknowledgement
			nwc_service_t *nws = (nwc_service_t*)network_receive_command(NWC_SERVICE);
			if (nws==NULL  ||  nws->flag != nwc_service_t::SRVC_LOGIN_ADMIN) {
				printf("Authentification failed!\n");
				delete nws;
				return 3;
			}
			if (!nws->number) {
				printf("Wrong password!\n");
				delete nws;
				return 3;
			}
		}

		// get client list from server
		if (argc && !STRICMP(argv[0], "clients")) {
			argv++, argc--;
			nwc_service_t nwcs;
			nwcs.flag = nwc_service_t::SRVC_GET_CLIENT_LIST;
			if (!nwcs.send(socket)) {
				return 2;
			}
			nwc_service_t *nws = (nwc_service_t*)network_receive_command(NWC_SERVICE);
			if (nws==NULL) {
				return 3;
			}
			if (nws->flag != nwc_service_t::SRVC_GET_CLIENT_LIST  ||  nws->socket_info==NULL) {
				delete nws;
				return 3;
			}
			// now get client list from packet
			vector_tpl<socket_info_t> &list = *(nws->socket_info);
			bool head = false;
			for(uint32 i=0; i<list.get_count(); i++) {
				if (list[i].state==socket_info_t::playing) {
					if (!head) {
						printf("List of playing clients:\n");
						head = true;
					}
					uint32 ip = list[i].address.ip;
					printf("  [%3d]  ..   %02d.%02d.%02d.%02d\n", i, (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
				}
			}
			if (!head) {
				printf("No playing clients.\n");
			}
			delete nws;
			return 0;
		}
		// get blacklist from server
		if (argc && !STRICMP(argv[0], "blacklist")) {
			argv++, argc--;
			nwc_service_t nwcs;
			nwcs.flag = nwc_service_t::SRVC_GET_BLACK_LIST;
			if (!nwcs.send(socket)) {
				return 2;
			}
			nwc_service_t *nws = (nwc_service_t*)network_receive_command(NWC_SERVICE);
			if (nws==NULL) {
				return 3;
			}
			if (nws->flag != nwc_service_t::SRVC_GET_BLACK_LIST  ||  nws->address_list==NULL) {
				delete nws;
				return 3;
			}
			// now get list from packet
			address_list_t &list = *(nws->address_list);
			if (list.get_count() == 0) {
				printf("Blacklist empty\n");
			}
			else {
				printf("List of banned addresses:\n");
			}
			for(uint32 i=0; i<list.get_count(); i++) {
				printf("  [%3d]  ..   ", i);
				for(sint8 j=24; j>=0; j-=8) {
					uint32 m = (list[i].mask >> j) & 0xff;
					if (m) {
						printf("%02d", (list[i].ip >> j) & 0xff);
						if ( m != 0xff) {
							printf("[%02x]", m);
						}
						if (j > 0) {
							printf(".");
						}
					}
					else {
						break;
					}
				}
				printf("\n");
			}
			delete nws;
			return 0;
		}
		// kick client from server
		if (argc  &&  (!STRICMP(argv[0], "kick-client")  ||  !STRICMP(argv[0], "ban-client"))) {
			bool ban = !STRICMP(argv[0], "ban-client");
			argv++, argc--;
			int client_nr = 0;
			if (argc) {
				client_nr = atoi(argv[0]);
			}
			if (client_nr<=0) {
				return 3;
			}
			nwc_service_t nwcs;
			nwcs.flag = ban ? nwc_service_t::SRVC_BAN_CLIENT : nwc_service_t::SRVC_KICK_CLIENT;
			nwcs.number = client_nr;
			if (!nwcs.send(socket)) {
				return 2;
			}
		}
		// ban ip range
		if (argc  &&  (!STRICMP(argv[0], "ban")  ||  !STRICMP(argv[0], "unban"))) {
			bool ban = !STRICMP(argv[0], "ban");
			argv++, argc--;
			if (argc  &&  argv[0]) {
				net_address_t address(argv[0]);
				if (address.ip) {
					nwc_service_t nwcs;
					nwcs.flag = ban ? nwc_service_t::SRVC_BAN_IP : nwc_service_t::SRVC_UNBAN_IP;
					nwcs.text = strdup(argv[0]);
					if (!nwcs.send(socket)) {
						return 2;
					}
				}
			}
		}

		// send admin message
		if (argc  &&  !STRICMP(argv[0], "say")) {
			argv++, argc--;
			if (argc  &&  argv[0]) {
				nwc_service_t nwcs;
				nwcs.flag = nwc_service_t::SRVC_ADMIN_MSG;
				nwcs.text = strdup(argv[0]);
				if (!nwcs.send(socket)) {
					return 2;
				}
			}
		}

		// shutdown server
		if (argc  &&  !STRICMP(argv[0], "shut-down")) {
			argv++, argc--;
			nwc_service_t nwcs;
			nwcs.flag = nwc_service_t::SRVC_SHUTDOWN;
			if (!nwcs.send(socket)) {
				return 2;
			}
		}

		// force server to send sync command
		if (argc  &&  !STRICMP(argv[0], "force-sync")) {
			argv++, argc--;
			nwc_service_t nwcs;
			nwcs.flag = nwc_service_t::SRVC_FORCE_SYNC;
			if (!nwcs.send(socket)) {
				return 2;
			}
		}

		return 0;
	}

	puts(
		"\n   Usage:\n"
		"\n"
		"      NetTool <server-url> announce\n"
		"         Tell the server to call back to its masterserver\n"
		"\n"
		"      NetTool <server-url> <passwd> clients\n"
		"         Receive list of playing clients from server\n"
		"\n"
		"      NetTool <server-url> <passwd> kick-client <client-number>\n"
		"      NetTool <server-url> <passwd> ban-client  <client-number>\n"
		"         Kick / ban client\n"
		"\n"
		"      NetTool <server-url> <passwd> ban   <ip-range>\n"
		"      NetTool <server-url> <passwd> unban <ip-range>\n"
		"         Ban / unban ip-range\n"
		"\n"
		"      NetTool <server-url> <passwd> say <message>\n"
		"         Send admin message to all clients\n"
		"\n"
		"      NetTool <server-url> <passwd> shut-down <message>\n"
		"         Shut down server\n"
		"\n"
		"      NetTool <server-url> <passwd> force-sync <message>\n"
		"         Force server to send sync command in order to save & reload the game\n"
		"\n"
		"      with QUIET as first arg copyright message will be omitted\n"
		"\n"
		"      return codes:\n"
		"           0 .. success\n"
		"           1 .. server not reachable\n"
		"           2 .. could not sent message to server\n"
		"           3 .. misc errors\n"
	);

	return 3;
}
