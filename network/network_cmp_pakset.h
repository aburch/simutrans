/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_NETWORK_CMP_PAKSET_H
#define NETWORK_NETWORK_CMP_PAKSET_H


#include "network_cmd.h"
#include "pakset_info.h"
#include <string>
/**
 * Compare paksets on server and client
 * client side of communication
 * @param cp url of server
 * @param msg contains html-text of differences to be displayed in a help_frame window
 */
void network_compare_pakset_with_server(const char* cp, std::string &msg);

/**
 * nwc_pakset_info_t
 * @from-client: client wants to get pakset info from server
 * @from-server: server sends pakset info back to client
 */
class nwc_pakset_info_t : public network_command_t {
public:
	nwc_pakset_info_t(uint8 flag_=UNDEFINED) : network_command_t(NWC_PAKSETINFO), flag(flag_), name(NULL), chk(NULL) {}
	~nwc_pakset_info_t();

	bool execute(karte_t *) OVERRIDE;
	void rdwr() OVERRIDE;

	enum {
		CL_INIT       = 0, // client want pakset info
		CL_WANT_NEXT  = 1, // client received one info packet, wants next
		CL_QUIT       = 2, // client ends this negotiation
		SV_ERROR      = 10, // server busy etc
		SV_PAKSET     = 11, // server sends pakset checksum
		SV_DATA       = 12, // server sends data
		SV_LAST       = 19, // server sends last info packet
		UNDEFINED     = 255
	};
	uint8 flag;
	// name of and info about descriptor
	char *name;
	checksum_t *chk;
	void clear() { name = NULL; chk = NULL; }

	// for the communication of the server with the client
	static stringhashtable_tpl<checksum_t*>::iterator server_iterator;
	static SOCKET server_receiver;
};

#endif
