#ifndef _NETWORK_CMD_H_
#define _NETWORK_CMD_H_

#include "../simtypes.h"
#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"
#include "network.h"
#include "pwd_hash.h"

class address_list_t;
class world_t;
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
	 * @return whether send was successful
	 */
	bool send(SOCKET s);

	// write our data to the packet
	virtual void rdwr();

	// executes the command
	// (see network_world_command_t::execute() )
	// if returns true this can be deleted afterwards
	virtual bool execute(world_t *) { return true;}

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
		SRVC_GET_COMPANY_LIST = 11,
		SRVC_GET_COMPANY_INFO = 12,
		SRVC_UNLOCK_COMPANY   = 13,
		SRVC_REMOVE_COMPANY   = 14,
		SRVC_LOCK_COMPANY     = 15,
		SRVC_MAX
	};

	uint32 number;
	char *text;
	vector_tpl<socket_info_t*> *socket_info;
	address_list_t *address_list;

	nwc_service_t() : network_command_t(NWC_SERVICE), text(NULL), socket_info(NULL), address_list(NULL) { }
	~nwc_service_t();

#ifndef NETTOOL
	virtual bool execute(world_t *);
#endif

	virtual void rdwr();

	virtual const char* get_name() { return "nwc_service_t";}
};


/**
 * nwc_auth_player_t
 * @from-client: client sends password hash to unlock player / set player password
 *		 server sends nwc_auth_player_t to sender
 * @from-server:
 *		 information whether players are locked / unlocked
 */
class nwc_auth_player_t : public network_command_t {
public:
	nwc_auth_player_t() : network_command_t(NWC_AUTH_PLAYER), hash(), player_unlocked(0), player_nr(255)  { }
	nwc_auth_player_t(uint8 nr, const pwd_hash_t& hash_) : network_command_t(NWC_AUTH_PLAYER), hash(hash_), player_unlocked(0), player_nr(nr)  { }

#ifndef NETTOOL
	virtual bool execute(world_t *);
#endif
	virtual void rdwr();
	virtual const char* get_name() { return "nwc_auth_player_t";}
	pwd_hash_t hash;
	uint16 player_unlocked;
	uint8  player_nr;

	/**
	 * sets unlocked flags for playing at server
	 */
	static void init_player_lock_server(world_t *);
};

#endif
