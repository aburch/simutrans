/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_NETWORK_CMD_SCENARIO_H
#define NETWORK_NETWORK_CMD_SCENARIO_H


#include "network_cmd.h"
#include "network_cmd_ingame.h"

#include "../dataobj/scenario.h"
#include "../utils/plainstring.h"

/**
 * nwc_scenario_t:
 * scenario script runs on server only, to communicate results this command is used
 */
class nwc_scenario_t : public network_command_t {
public:
	nwc_scenario_t() : network_command_t(NWC_SCENARIO), what(UNKNOWN), won(0), lost(0), function(NULL), result(NULL)  { }
	bool execute(karte_t *) OVERRIDE;
	void rdwr() OVERRIDE;

	enum {
		UNKNOWN,
		CALL_SCRIPT,        /// client asks for an update
		CALL_SCRIPT_ANSWER, /// client wants string, server sends string
		UPDATE_WON_LOST,    /// update win/lose flags of the scenario
		OPEN_SCEN_WIN       /// open scenario info window
	};
	uint16 what;

	uint16 won, lost;

	plainstring function;
	plainstring result;

	/// register the callback to the script engine
	static void init(script_vm_t *script);

	/**
	 * Callback method: sends answer back to client.
	 * @param function result of this function is returned
	 * @param client_id to send result to
	 * @returns dummy boolean value
	 */
	static bool record_result(const char* function, plainstring result, uint32 client_id);

private:
	nwc_scenario_t(const nwc_scenario_t&);
	nwc_scenario_t& operator=(const nwc_scenario_t&);
};


/**
 * nwc_scenario_rules_t:
 * scenario script runs on server only, use this command to send new rules to client
 */
class nwc_scenario_rules_t : public network_broadcast_world_command_t {
public:
	nwc_scenario_rules_t(uint32 sync_step=0, uint32 map_counter=0) : network_broadcast_world_command_t(NWC_SCENARIO_RULES, sync_step, map_counter), rule( new scenario_t::forbidden_t()), forbid(true) { }
	~nwc_scenario_rules_t() { delete rule; }

	void do_command(karte_t *) OVERRIDE;
	void rdwr() OVERRIDE;
	network_broadcast_world_command_t* clone(karte_t *) OVERRIDE;

	scenario_t::forbidden_t *rule;
	bool forbid;

private:
	nwc_scenario_rules_t(const nwc_scenario_rules_t&);
	nwc_scenario_rules_t& operator=(const nwc_scenario_rules_t&);
};
#endif
