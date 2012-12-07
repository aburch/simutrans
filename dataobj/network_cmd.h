#ifndef _NETWORK_CMD_H_
#define _NETWORK_CMD_H_

#include "../simtypes.h"
#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"
#include "network.h"

class address_list_t;
class karte_t;
class packet_t;
class socket_info_t;

// actual commands
enum {
	NWC_INVALID   = 0,
	NWC_GAMEINFO,
	NWC_NICK,
	NWC_CHAT,
	NWC_JOIN,
	NWC_SYNC,
	NWC_GAME,
	NWC_READY,
	NWC_TOOL,
	NWC_CHECK,
	NWC_PAKSETINFO,
	NWC_SERVICE,
	NWC_AUTH_PLAYER,
	NWC_CHG_PLAYER,
	NWC_SCENARIO,
	NWC_SCENARIO_RULES,
	NWC_COUNT
};

class network_command_t {
protected:
	packet_t *packet;
	// always send:
	uint16 id;
	uint32 our_client_id;
	// ready for sending
	bool   ready;
public:
	network_command_t(uint16 /*id*/);
	network_command_t();
	virtual ~network_command_t();
	// receive: calls rdwr from packet
	// return true on success
	bool receive(packet_t *p);
	// calls rdwr if packet is empty
	void prepare_to_send();
	/**
	 * sends to a client
	 * sends complete command-packet
	 * @return whether send was succesfull
	 */
	bool send(SOCKET s);

	// write our data to the packet
	virtual void rdwr();

	// executes the command
	// (see network_world_command_t::execute() )
	// if returns true this can be deleted afterwards
	virtual bool execute(karte_t *) { return true;}

	virtual const char* get_name() { return "network_command_t";}

	bool is_local_cmd();

	uint16 get_id() { return id;}

	SOCKET get_sender();

	packet_t *get_packet() const { return packet; }
	/**
	 * returns ptr to a copy of the packet
	 */
	packet_t *copy_packet() const;

	// creates an instance:
	// gets the nwc-id from the packet, and reads its data
	static network_command_t* read_from_packet(packet_t *p);
};


/**
 * base class for any service commands
 * limited to one execution per 1 minute real-time
 */
class nwc_service_t : public network_command_t {
public:
	uint32 flag;

	enum {
		SRVC_LOGIN_ADMIN     = 0,
		SRVC_ANNOUNCE_SERVER = 1,
		SRVC_GET_CLIENT_LIST = 2,
		SRVC_KICK_CLIENT     = 3,
		SRVC_BAN_CLIENT      = 4,
		SRVC_GET_BLACK_LIST  = 5,
		SRVC_BAN_IP          = 6,
		SRVC_UNBAN_IP        = 7,
		SRVC_ADMIN_MSG       = 8,
		SRVC_SHUTDOWN        = 9,
		SRVC_FORCE_SYNC      = 10,
		SRVC_MAX
	};

	uint32 number;
	char *text;
	vector_tpl<socket_info_t*> *socket_info;
	address_list_t *address_list;

	nwc_service_t() : network_command_t(NWC_SERVICE), text(NULL), socket_info(NULL), address_list(NULL) { }
	~nwc_service_t();

#ifndef NETTOOL
	virtual bool execute(karte_t *);
#endif

	virtual void rdwr();

	virtual const char* get_name() { return "nwc_service_t";}

	// static list of execution times
	static vector_tpl<long> exec_time;
};

#endif
