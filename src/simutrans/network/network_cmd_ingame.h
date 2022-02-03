/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_NETWORK_CMD_INGAME_H
#define NETWORK_NETWORK_CMD_INGAME_H


#include "network_cmd.h"
#include "memory_rw.h"
#include "../world/simworld.h"
#include "../tpl/slist_tpl.h"
#include "../utils/plainstring.h"
#include "../dataobj/koord3d.h"

class connection_info_t;
class packet_t;
class player_t;
class tool_t;

/**
 * nwc_gameinfo_t
 * @from-client: client wants map info
 *      server sends nwc_gameinfo_t to sender
 * @from-server:
 *      @data len of gameinfo
 *      client processes this in network_connect
 */
class nwc_gameinfo_t : public network_command_t {
public:
	nwc_gameinfo_t() : network_command_t(NWC_GAMEINFO) { len = 0; }
	bool execute(karte_t *) OVERRIDE;
	void rdwr() OVERRIDE;
	uint32 client_id;
	uint32 len;
};

/**
 * nwc_nick_t
 * @from-client: client sends new nickname,
 *               server checks if nickname is already taken,
 *               and generates a default one if this is the case
 * @from-server: server sends the checked nickname back to the client
 */
class nwc_nick_t : public network_command_t {
public:
	nwc_nick_t(const char* nick=NULL)
	: network_command_t(NWC_NICK), nickname(nick) {  }

	bool execute(karte_t *) OVERRIDE;
	void rdwr() OVERRIDE;

	plainstring nickname;

	enum {
		WELCOME,
		CHANGE_NICK,
		FAREWELL
	};

	/**
	 * Server-side nickname related stuff:
	 * what = WELCOME     .. new player joined: send welcome message
	 * what = CHANGE_NICK .. change nickname: in socket_list per client, send new nick back to client, tell others as well
	 * what = FAREWELL    .. player has left: send message
	 */
	static void server_tools(karte_t *welt, uint32 client_id, uint8 what, const char* nick);

private:
	nwc_nick_t(const nwc_nick_t&);
	nwc_nick_t& operator=(const nwc_nick_t&);
};

/**
 * nwc_chat_t
 * @from-client: client sends chat message to server
 *               server logs message and sends it to all clients
 * @from-server: server sends a chat message for display on the client
 */
class nwc_chat_t : public network_command_t {
public:
	nwc_chat_t (const char* msg = NULL, sint8 pn = -1, const char* cn = NULL, const char* dn = NULL)
	: network_command_t(NWC_CHAT), message(msg), player_nr(pn), clientname(cn), destination(dn) {}

	bool execute (karte_t *) OVERRIDE;
	void rdwr () OVERRIDE;

	void add_message (karte_t*) const;

	plainstring message;            // Message text
	sint8 player_nr;                // Company number message was sent as
	plainstring clientname;         // Name of client message is from
	plainstring destination;        // Client to send message to (NULL for all)

private:
	nwc_chat_t(const nwc_chat_t&);
	nwc_chat_t& operator=(const nwc_chat_t&);
};

/**
 * nwc_join_t
 * @from-client: client wants to join the server
 *      server sends nwc_join_t to sender, nwc_sync_t to all clients
 * @from-server:
 *      @data answer == 1 (if joining now is ok)
 *      @data client_id
 *      client ignores the following nwc_sync_t, waits for nwc_ready_t
 */
class nwc_join_t : public nwc_nick_t {
public:
	nwc_join_t(const char* nick=NULL)
	: nwc_nick_t(nick), client_id(0), answer(0) { id = NWC_JOIN; }

	bool execute(karte_t *) OVERRIDE;
	void rdwr() OVERRIDE;

	uint32 client_id;
	uint8 answer;

	/**
	 * this clients is in the process of joining
	 */
	static SOCKET pending_join_client;

	static bool is_pending() { return pending_join_client != INVALID_SOCKET; }
private:
	nwc_join_t(const nwc_join_t&);
	nwc_join_t& operator=(const nwc_join_t&);
};


/**
 * nwc_ready_t
 * @from-client:
 *      @data sync_steps at which client will continue
 *      client paused, waits for unpause
 * @from-server:
 *      data is resent to client
 *      map_counter to identify network_commands
 *      unpause client
 */
class nwc_ready_t : public network_command_t {
public:
	nwc_ready_t() : network_command_t(NWC_READY), sync_step(0), map_counter(0) { }
	nwc_ready_t(uint32 sync_step_, uint32 map_counter_, const checklist_t &checklist_) : network_command_t(NWC_READY), sync_step(sync_step_), map_counter(map_counter_), checklist(checklist_) { }

	bool execute(karte_t *) OVERRIDE;
	void rdwr() OVERRIDE;

	uint32 sync_step;
	uint32 map_counter;
	checklist_t checklist;

	static void append_map_counter(uint32 map_counter_);
	static void clear_map_counters();
private:
	static vector_tpl<uint32>all_map_counters;
};

/**
 * nwc_game_t
 * @from-server:
 *      @data len of savegame
 *     client processes this in network_connect
 */
class nwc_game_t : public network_command_t {
public:
	nwc_game_t(uint32 len_=0) : network_command_t(NWC_GAME), len(len_) {}

	void rdwr() OVERRIDE;

	uint32 len;
};

/**
 * commands that have to be executed at a certain sync_step
 */
class network_world_command_t : public network_command_t {
public:
	network_world_command_t() : network_command_t(), sync_step(0), map_counter(0) {}
	network_world_command_t(uint16 /*id*/, uint32 /*sync_step*/, uint32 /*map_counter*/);

	void rdwr() OVERRIDE;

	// put it to the command queue
	bool execute(karte_t *) OVERRIDE;

	// apply it to the world
	virtual void do_command(karte_t*) {}
	uint32 get_sync_step() const { return sync_step; }
	uint32 get_map_counter() const { return map_counter; }
	// ignore events that lie in the past?
	// if false: any cmd with sync_step < world->sync_step forces network disconnect
	virtual bool ignore_old_events() const { return false;}
	// for sorted data structures
	bool operator <= (network_world_command_t c) const { return sync_step <= c.sync_step; }
	static bool cmp(network_world_command_t *nwc1, network_world_command_t *nwc2) { return nwc1->get_sync_step() <= nwc2->get_sync_step(); }
protected:
	uint32 sync_step; // when this has to be executed
	uint32 map_counter; // cmd comes from world at this stage
};

/**
 * nwc_sync_t
 * @from-server:
 *      @data client_id this client wants to receive the game
 *      @data new_map_counter new map counter for the new world after game reloading
 *      clients: pause game, save, load, wait for nwc_ready_t command to unpause
 *      server: pause game, save, load, send game to client, send nwc_ready_t command to client
 */
class nwc_sync_t : public network_world_command_t {
public:
	nwc_sync_t() : network_world_command_t(NWC_SYNC, 0, 0), client_id(0), new_map_counter(0) {}
	nwc_sync_t(uint32 sync_steps, uint32 map_counter, uint32 send_to_client, uint32 _new_map_counter) : network_world_command_t(NWC_SYNC, sync_steps, map_counter), client_id(send_to_client), new_map_counter(_new_map_counter) { }

	void rdwr() OVERRIDE;
	void do_command(karte_t*) OVERRIDE;

	uint32 get_new_map_counter() const { return new_map_counter; }
private:
	uint32 client_id; // this client shall receive the game
	uint32 new_map_counter; // map counter to be applied to the new world after game reloading
};

/**
 * nwc_check_t
 * @from-server:
 *      @data checklist random seed and quickstone next check entries at previous sync_step
 *      clients: check random seed, if check fails disconnect.
 *      the check is done in karte_t::interactive
 */
class nwc_check_t : public network_world_command_t {
public:
	nwc_check_t() : network_world_command_t(NWC_CHECK, 0, 0), server_sync_step(0) { }
	nwc_check_t(uint32 sync_steps, uint32 map_counter, const checklist_t &server_checklist_, uint32 server_sync_step_) : network_world_command_t(NWC_CHECK, sync_steps, map_counter), server_checklist(server_checklist_), server_sync_step(server_sync_step_) {}
	void rdwr() OVERRIDE;
	void do_command(karte_t*) OVERRIDE { }

	checklist_t server_checklist;
	uint32 server_sync_step;
	// no action required -> can be ignored if too old
	bool ignore_old_events() const OVERRIDE { return true; }
};

/**
 * commands that need to be executed at a certain syncstep
 * the command will be cloned at the server and broadcasted to all clients
 */
class network_broadcast_world_command_t : public network_world_command_t {
public:
	network_broadcast_world_command_t(uint16 id, uint32 sync_step=0, uint32 map_counter=0)
	: network_world_command_t(id, sync_step, map_counter), exec(false) { }

	void rdwr() OVERRIDE;
	bool execute(karte_t *) OVERRIDE;

	// clones the command to be broadcasted
	// all validity checks must be done here
	// it must return a new command
	// clone() must return NULL to indicate failure
	virtual network_broadcast_world_command_t* clone(karte_t *) = 0;

	bool is_from_initiator() const { return !exec; }
private:
	// at server: true if command needs to be put in command queue
	// it will be cloned and broadcasted otherwise
	bool exec;
};

/**
 * nwc_chg_player_t
 *      commands that require special authentication checks: toggle freeplay, start AI player
 * @from-server:
 *      @data cmd command to perform (see karte_t::change_player_tool)
 *      @data player_nr affected player
 *      @data param
 */
class nwc_chg_player_t : public network_broadcast_world_command_t {
public:
	nwc_chg_player_t() : network_broadcast_world_command_t(NWC_CHG_PLAYER, 0, 0), pending_company_creator(NULL) { }
	nwc_chg_player_t(uint32 sync_steps, uint32 map_counter, uint8 cmd_=255, uint8 player_nr_=255, uint16 param_=0, bool scripted_call_=false)
	: network_broadcast_world_command_t(NWC_CHG_PLAYER, sync_steps, map_counter),
	  cmd(cmd_), player_nr(player_nr_), param(param_), scripted_call(scripted_call_), pending_company_creator(NULL) {}

	~nwc_chg_player_t();

	void rdwr() OVERRIDE;
	void do_command(karte_t*) OVERRIDE;
	// do some special checks
	network_broadcast_world_command_t* clone(karte_t *) OVERRIDE;

	uint8 cmd;
	uint8 player_nr;
	uint16 param;
	bool scripted_call;
	connection_info_t* pending_company_creator; // this client want to create new company (not sent)

	/// store information about client that created a company
	static connection_info_t* company_creator[PLAYER_UNOWNED];

	/// store information about clients that played with a company
	static slist_tpl<connection_info_t> company_active_clients[PLAYER_UNOWNED];

	/// callback when company was removed
	static void company_removed(uint8 player_nr);
private:
	nwc_chg_player_t(const nwc_chg_player_t&);
	nwc_chg_player_t& operator=(const nwc_chg_player_t&);
};

/**
 * nwc_tool_t
 * @from-client: client sends tool init/work
 * @from-server: server sends nwc_tool_t to all clients with step when tool has to be executed
 *      @data client_id (the client that launched the tool, sent by server)
 *      @data player_nr
 *      @data init (if true call init else work)
 *      @data tool_id
 *      @data pos
 *      @data default_param
 *      @data exec (if true executes, else server sends it to clients)
 */
class nwc_tool_t : public network_broadcast_world_command_t {
public:
	// to detect desync we sent these infos always together (only valid for tools)
	checklist_t last_checklist;
	uint32 last_sync_step;

	nwc_tool_t();
	nwc_tool_t(player_t *player, tool_t *tool, koord3d pos, uint32 sync_steps, uint32 map_counter, bool init);
	nwc_tool_t(const nwc_tool_t&);

	// messages are allowed to arrive at any time
	bool ignore_old_events() const OVERRIDE;

	virtual ~nwc_tool_t();

	void rdwr() OVERRIDE;

	// clone performs authentication checks
	network_broadcast_world_command_t* clone(karte_t *) OVERRIDE;

	// really executes it, here exec should be true
	void do_command(karte_t*) OVERRIDE;

	void init_tool();
private:
	// transfered data
	plainstring default_param;
	uint32 tool_client_id;
	uint32 callback_id;
	uint16 tool_id;
	sint16 wt; // needed for scenario checks
	koord3d pos;
	uint8 flags;
	uint8 player_nr;
	bool init;

	uint8 custom_data_buf[256];
	memory_rw_t custom_data;

	// tool that will be executed
	tool_t* tool;

	// compare default_param's (NULL pointers allowed)
	// @return true if default_param are equal
	static bool cmp_default_param(const char *d1, const char *d2);
};

/**
 * nwc_step_t
 * @from-server:
 *      @data contains the current sync_steps of the server
 *      defining the maximum sync_steps a client can advance to.
 */
class nwc_step_t : public network_world_command_t {
public:
	nwc_step_t() : network_world_command_t(NWC_STEP, 0, 0) { }
	nwc_step_t(uint32 sync_steps, uint32 map_counter) : network_world_command_t(NWC_STEP, sync_steps, map_counter) {}

	bool execute(karte_t *) OVERRIDE { return true;}
};

#endif
