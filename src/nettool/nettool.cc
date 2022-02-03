/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Network server control tool for Simutrans
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Needed for interaction with console
// <windows.h> also needed, but this is included by networking code
#ifndef _WIN32
#include <termios.h>
#endif

#include "../simutrans/network/network.h"
#include "../simutrans/network/network_cmd.h"
#include "../simutrans/network/network_packet.h"
#include "../simutrans/network/network_socket_list.h"
#include "../simutrans/simmem.h"
#include "../simutrans/simtypes.h"
#include "../simutrans/simversion.h"
#include "../simutrans/utils/simstring.h"
#include "../simutrans/utils/fetchopt.h"
#include "../simutrans/utils/sha1.h"


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

// echo() used to set whether terminal should echo user input or not
// Used for more secure password entry
#ifdef _WIN32
void echo(bool on) {
	DWORD mode;
	HANDLE hConIn = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(hConIn, &mode);
	mode = on ? (mode | ENABLE_ECHO_INPUT) : (mode & ~(ENABLE_ECHO_INPUT));
	SetConsoleMode(hConIn, mode);
}
#else
// Should be good for POSIX platforms
void echo(bool on) {
	struct termios settings;
	tcgetattr(STDIN_FILENO, &settings);
	settings.c_lflag = on ? (settings.c_lflag | ECHO) : (settings.c_lflag & ~(ECHO));
	tcsetattr(STDIN_FILENO, TCSANOW, &settings);
}
#endif

/**
 * Sets nwcs.flag = command_id, text to argv[1], number to argv[0].
 * @returns true upon success
 */
bool fill_nwc_service(nwc_service_t &nwcs, uint32 command_id, int argc, char **argv)
{
	nwcs.flag = command_id;
	switch (argc) {
		case 2: // second argument is a string
			if (argv[1]) {
				nwcs.text = strdup( argv[1] );
			}

		case 1: // first argument is a positive number
			{
				int _n = atoi(argv[0]);
				if (_n >=0) {
					nwcs.number = _n;
				}
				else {
					return false;
				}
			}
		default: break;
	}
	return true;
}

// Simple commands specify only a command ID to perform an action on the server
int simple_command(SOCKET socket, uint32 command_id, int argc, char **argv) {
	nwc_service_t nwcs;
	if (  !fill_nwc_service(nwcs, command_id, argc, argv)  ) {
		return 3;
	}

	if (!nwcs.send(socket)) {
		fprintf(stderr, "Could not send request!\n");
		return 2;
	}
	return 0;
}

// Simple command to send command ID to server and receive and print a text buffer
int simple_gettext_command(SOCKET socket, uint32 command_id, int argc, char **argv) {
	nwc_service_t nwcs;
	if (  !fill_nwc_service(nwcs, command_id, argc, argv)  ) {
		return 3;
	}
	if (!nwcs.send(socket)) {
		fprintf(stderr, "Could not send request!\n");
		return 2;
	}
	nwc_service_t *nws = (nwc_service_t*)network_receive_command(NWC_SERVICE);
	if (nws==NULL) {
		return 3;
	}
	if (nws->flag != command_id) {
		delete nws;
		return 3;
	}

	if (nws->text) {
		printf("%s", nws->text);
	}
	else {
		printf("Nothing received.\n");
	}
	delete nws;
	return 0;
}

int get_client_list(SOCKET socket, uint32 command_id, int, char **) {
	nwc_service_t nwcs;
	nwcs.flag = command_id;
	if (!nwcs.send(socket)) {
		fprintf(stderr, "Could not send request!\n");
		return 2;
	}
	nwc_service_t *nws = (nwc_service_t*)network_receive_command(NWC_SERVICE);
	if (nws==NULL) {
		return 3;
	}
	if (nws->flag != command_id || nws->socket_info==NULL) {
		delete nws;
		return 3;
	}
	// now get client list from packet
	vector_tpl<socket_info_t*> &list = *(nws->socket_info);
	bool head = false;
	for(uint32 i=0; i<list.get_count(); i++) {
		if (list[i]->state==socket_info_t::playing) {
			if (!head) {
				printf("List of playing clients:\n");
				head = true;
			}
			uint32 ip = list[i]->address.ip;
			printf("  [%3d]  ..   %02d.%02d.%02d.%02d\n", i, (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
		}
	}
	if (!head) {
		printf("No playing clients.\n");
	}
	delete nws;
	return 0;
}

int kick_client(SOCKET socket, uint32 command_id, int, char **argv) {
	int client_nr = 0;
	client_nr = atoi(argv[0]);
	if (client_nr<=0) {
		return 3;
	}
	nwc_service_t nwcs;
	nwcs.flag = command_id;
	nwcs.number = client_nr;
	if (!nwcs.send(socket)) {
		fprintf(stderr, "Could not send request!\n");
		return 2;
	}
	return 0;
}

int ban_client(SOCKET socket, uint32 command_id, int, char **argv) {
	int client_nr = 0;
	client_nr = atoi(argv[0]);
	if (client_nr<=0) {
		return 3;
	}
	nwc_service_t nwcs;
	nwcs.flag = command_id;
	nwcs.number = client_nr;
	if (!nwcs.send(socket)) {
		fprintf(stderr, "Could not send request!\n");
		return 2;
	}
	return 0;
}

int get_blacklist(SOCKET socket, uint32 command_id, int, char **) {
	nwc_service_t nwcs;
	nwcs.flag = command_id;
	if (!nwcs.send(socket)) {
		fprintf(stderr, "Could not send request!\n");
		return 2;
	}
	nwc_service_t *nws = (nwc_service_t*)network_receive_command(NWC_SERVICE);
	if (nws==NULL) {
		return 3;
	}
	if (nws->flag != command_id || nws->address_list==NULL) {
		delete nws;
		return 3;
	}
	// now get list from packet
	address_list_t &list = *(nws->address_list);
	if (list.empty()) {
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

int ban_ip(SOCKET socket, uint32 command_id, int, char **argv) {
	net_address_t address(argv[0]);
	if (address.ip) {
		nwc_service_t nwcs;
		nwcs.flag = command_id;
		nwcs.text = strdup(argv[0]);
		if (!nwcs.send(socket)) {
			fprintf(stderr, "Could not send request!\n");
			return 2;
		}
	}
	return 0;
}

int unban_ip(SOCKET socket, uint32 command_id, int, char **argv) {
	net_address_t address(argv[0]);
	if (address.ip) {
		nwc_service_t nwcs;
		nwcs.flag = command_id;
		nwcs.text = strdup(argv[0]);
		if (!nwcs.send(socket)) {
			fprintf(stderr, "Could not send request!\n");
			return 2;
		}
	}
	return 0;
}

int say(SOCKET socket, uint32 command_id, int argc, char **argv) {
	nwc_service_t nwcs;
	nwcs.flag = command_id;
	// Cat all arguments together into message to send
	const int maxlen = 513;
	int remaining = maxlen - 1;
	char msg[maxlen];
	int ind = 0;
	tstrncpy(msg, argv[ind], remaining);
	remaining -= strlen(argv[ind]);
	ind++;
	while (ind < argc && remaining > 1) {
		strcat(msg, " ");
		remaining -= 1;
		strncat(msg, argv[ind], remaining);
		remaining -= strlen(argv[ind]);
		ind++;
	}
	nwcs.text = strdup(msg);
	if (!nwcs.send(socket)) {
		fprintf(stderr, "Could not send request!\n");
		return 2;
	}
	return 0;
}

int lock_company(SOCKET socket, uint32, int argc, char **argv)
{
	// player number
	int _n = atoi(argv[0]);
	if (_n < 0  ||  _n >= 15 /*PLAYER_UNOWNED*/ ) {
		return 3;
	}
	uint8 player_nr = _n;
	// password source
	bool pwd_from_cl = strcmp(argv[1], "-F") != 0;
	if (!pwd_from_cl  &&  argc<3) {
		return 3;
	}
	const char* password = NULL;
	// password
	if (pwd_from_cl) {
		password = argv[1];
	}
	else {
		if (strcmp(argv[2], "-")==0) {
			// Read password from stdin
			char* password_in = MALLOCN(char, 256);
			fprintf(stderr, "Password: ");
			echo(false);
			scanf("%255s", password_in);
			echo(true);
			printf("\n");
			password = password_in;
		}
		else if (FILE* const fd = fopen(argv[2], "r")) {
			// malloc ok here as utility is short-lived so no need to free()
			size_t const size = 256;
			char* password_in = MALLOCN(char, size);
			fgets(password_in, size, fd);
			password_in[strcspn(password_in, "\n")] = '\0';
			fclose(fd);
			password = password_in;
		}
		else {
			return 3;
		}
	}
	printf("new password: %s\n",password);

	pwd_hash_t pwd_hash;
	{
		SHA1 sha1;
		sha1.Input(password, strlen(password));
		sha1.Result(pwd_hash);
	}

	// now send nwc_auth_t command
	nwc_auth_player_t nwca(player_nr, pwd_hash);
	if (!nwca.send(socket)) {
		fprintf(stderr, "Could not send request!\n");
		return 2;
	}
	return 0;
}

// Print usage and exit
void usage()
{
	fprintf(stderr,
		"nettool for Simutrans " VERSION_NUMBER " and higher\n"
		"\n"
		"  Usage:\n"
		"\n"
		"      nettool [options] <command> [command argument]\n"
		"\n"
		"    Options:\n"
		"      -s <server[:port]> : Specify server to connect to (default is localhost:13353)\n"
		"      -p <password>      : Set password on command line\n"
		"      -P <filename>      : Read password from file specified (use '-' to read from stdin)\n"
		"      -q                 : Quiet mode, copyright message will be omitted\n"
		"\n"
		"    Commands:\n"
		"      announce\n"
		"        Request server announce itself to the central listing server\n"
		"\n"
		"      clients\n"
		"        Receive list of playing clients from server\n"
		"\n"
		"      companies\n"
		"        Receive list of running companies from server\n"
		"\n"
		"      info-company <company number>\n"
		"        Show detailed info for company\n"
		"\n"
		"      lock-company <company number> <new password>\n"
		"      lock-company <company number> -F <filename>\n"
		"        Set password, read from file if specified (use '-' to read from stdin)\n"
		"\n"
		"      unlock-company <company number>\n"
		"        Clear password of company, effectively unlocking it for all clients\n"
		"\n"
		"      remove-company <company number>\n"
		"        Immediately remove company and all its belongings\n"
		"\n"
		"      kick-client <client number>\n"
		"      ban-client  <client number>\n"
		"        Kick / ban client (use clients command to get client number)\n"
		"\n"
		"      ban-ip   <ip address>\n"
		"      unban-ip <ip address>\n"
		"        Ban / unban ip address\n"
		"\n"
		"      blacklist\n"
		"        Display list of banned clients\n"
		"\n"
		"      say <message>\n"
		"        Send admin message to all clients (maximum of 512 characters)\n"
		"\n"
		"      shutdown\n"
		"        Shut down server\n"
		"\n"
		"      force-sync\n"
		"        Force server to send sync command in order to save & reload the game\n"
		"\n"
		"    Return codes:\n"
		"      0 .. success\n"
		"      1 .. server not reachable\n"
		"      2 .. could not send message to server\n"
		"      3 .. misc errors\n"
		"\n"
	);
	exit(3);
}


// For each command, set:
// name       - char* - name of command as specified on command line
// needs_auth - bool  - specifies whether authentication is required for command
// command_id - int   - nwc_service_t command ID for the command in question
// arguments  - int   - does this command require arguments, and if so how many?
// function   - function* - function pointer to function to evaluate for this command
struct command_t {
	const char *name;
	bool needs_auth;
	uint32 command_id;
	int numargs;
	int (*func)(SOCKET, uint32, int, char**);
};


int main(int argc, char* argv[]) {
	init_logging("stderr", true, true, NULL, "nettool");

	// Use Fetchopt to parse option flags
	Fetchopt_t fetchopt(argc, argv, "hp:P:qs:");

	bool opt_q = false;
	char default_server_address[] = "localhost:13353";
	char *server_address = default_server_address;
	char *password = NULL;

	int ch;
	while ((ch = fetchopt.next()) != -1) {
		switch (ch) {
			case 'p':
				// Password specified on command line
				password = fetchopt.get_optarg();
				break;
			case 'P':
				// Read password in from file specified
				// if filename is '-', read in from stdin
				if (!strcmp(fetchopt.get_optarg(), "-")) {
					// Password will be asked for later
				} else if (FILE* const fd = fopen(fetchopt.get_optarg(), "r")) {
					// malloc ok here as utility is short-lived so no need to free()
					size_t const size = 256;
					password = MALLOCN(char, size);
					fgets(password, size, fd);
					password[strcspn(password, "\n")] = '\0';
					fclose(fd);
				} else {
					// Failure, file empty
					fprintf(stderr, "Unable to open file \"%s\" to read password\n", fetchopt.get_optarg());
				}
				break;
			case 'q':
				opt_q = true;
				break;
			case 's':
				server_address = fetchopt.get_optarg();
				break;
			case '?':
			case 'h':
			default:
				usage();
				/* NOTREACHED */
		}
	}

	// argc is length of argv, so argv[0] is first arg (name of program)
	// and argv[argc-1] is the last argument
	// After parsing arguments fetchopt.get_optind() will return the index
	// of the next argv after the last option
	// If this is equal to argc then there are no more arguments, this means
	// that the command has been omitted (which is a failure condition so usage
	// should be printed)
	// For commands which should take an argument, check that fetchopt.get_optind()+1
	// is less than argc as well
	if (fetchopt.get_optind() >= argc) {
		usage();
	}


	command_t commands[] = {
		{"announce",    false, nwc_service_t::SRVC_ANNOUNCE_SERVER, 0, &simple_command},
		{"clients",     true,  nwc_service_t::SRVC_GET_CLIENT_LIST, 0, &get_client_list},
		{"kick-client", true,  nwc_service_t::SRVC_KICK_CLIENT,     1, &kick_client},
		{"ban-client",  true,  nwc_service_t::SRVC_BAN_CLIENT,      1, &ban_client},
		{"blacklist",   true,  nwc_service_t::SRVC_GET_BLACK_LIST,  0, &get_blacklist},
		{"ban-ip",      true,  nwc_service_t::SRVC_BAN_IP,          1, &ban_ip},
		{"unban-ip",    true,  nwc_service_t::SRVC_UNBAN_IP,        1, &unban_ip},
		{"say",         true,  nwc_service_t::SRVC_ADMIN_MSG,       1, &say},
		{"shutdown",    true,  nwc_service_t::SRVC_SHUTDOWN,        0, &simple_command},
		{"force-sync",  true,  nwc_service_t::SRVC_FORCE_SYNC,      0, &simple_command},
		{"companies",      true,  nwc_service_t::SRVC_GET_COMPANY_LIST, 0, &simple_gettext_command},
		{"info-company",   true,  nwc_service_t::SRVC_GET_COMPANY_INFO, 1, &simple_gettext_command},
		{"unlock-company", true,  nwc_service_t::SRVC_UNLOCK_COMPANY,   1, &simple_command},
		{"remove-company", true,  nwc_service_t::SRVC_REMOVE_COMPANY,   1, &simple_command},
		{"lock-company",   true,  nwc_service_t::SRVC_LOCK_COMPANY,     2, &lock_company}
	};
	int numcommands = lengthof(commands);

	// Determine which command we're running & validate any arguments
	int cmdindex;
	for (cmdindex = 0; cmdindex < numcommands; cmdindex++) {
		if (strcmp(commands[cmdindex].name, argv[fetchopt.get_optind()]) == 0) {
			// Validate number of parameters
			if (fetchopt.get_optind() + commands[cmdindex].numargs >= argc) {
				usage();
			}
			break;
		}
	}
	// Unknown command
	if (cmdindex == numcommands) {
		usage();
	}

	// Print copyright notice unless quiet flag set
	if (!opt_q) {
		fprintf(stderr,
			"nettool for Simutrans " VERSION_NUMBER " and higher\n"
		);
	}

	// If command requires authentication and password is not set then
	// ask for password from stdin (interactive)
	if(  commands[cmdindex].needs_auth  &&  password == NULL  ) {
		// Read password from stdin
		// malloc ok here as utility is short-lived so no need to free()
		password = MALLOCN(char, 256);
		fprintf(stderr, "Password: ");
		echo(false);
		scanf("%255s", password);
		echo(true);
		printf("\n");
	}

	// This is done whether we're executing a password protected command or not...
	const char *error = NULL;
	SOCKET const socket = network_open_address(server_address, error);
	if (error) {
		fprintf(stderr, "Could not connect to server at %s: %s\n", server_address, error);
		return 1;
	}
	// now we are connected
	socket_list_t::add_client(socket);

	// If authentication required, perform authentication
	if(  commands[cmdindex].needs_auth  ) {
		// try to authenticate us
		nwc_service_t nwcs;
		nwcs.flag = nwc_service_t::SRVC_LOGIN_ADMIN;
		nwcs.text = strdup(password);
		if (!nwcs.send(socket)) {
			fprintf(stderr, "Could not send login data!\n");
			return 2;
		}
		// wait for acknowledgement
		nwc_service_t *nws = (nwc_service_t*)network_receive_command(NWC_SERVICE);
		if(  nws==NULL  ||  nws->flag != nwc_service_t::SRVC_LOGIN_ADMIN  ) {
			fprintf(stderr, "Authentication failed!\n");
			delete nws;
			return 3;
		}
		if(  nws->number == 0  ) {
			fprintf(stderr, "Wrong password!\n");
			delete nws;
			return 3;
		}
		network_set_client_id( nws->number );
		delete nws;
	}

	// Execute command function and exit with return code
	const int first_arg = fetchopt.get_optind() + 1;
	char **argv_in;
	int argc_in;
	if (first_arg >= argc) {
		argv_in = NULL;
		argc_in = 0;
	} else {
		argv_in = argv + first_arg;
		argc_in = argc - first_arg;
	}
	return commands[cmdindex].func(socket, commands[cmdindex].command_id, argc_in, argv_in);
}
