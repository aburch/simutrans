#ifndef _NETWORK_CMD_SCENARIO_H_
#define _NETWORK_CMD_SCENARIO_H_

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
	virtual bool execute(karte_t *);
	virtual void rdwr();
	virtual const char* get_name() { return "nwc_scenario_t";}

	enum {
		UNKNOWN,
		CALL_SCRIPT,        /// client asks for an update
		CALL_SCRIPT_ANSWER, /// client wants string, server sends string
		UPDATE_WON_LOST     /// update win/lose flags of the scenario
	};
	uint16 what;

	uint16 won, lost;

	plainstring function;
	plainstring result;
private:
	nwc_scenario_t(const nwc_scenario_t&);
	nwc_scenario_t& operator=(const nwc_scenario_t&);
};


/**
 * nwc_scenario_rules_t:
 * scenario script runs on server only, use this command to send new rules to client
 */
class nwc_scenario_rules_t : public network_world_command_t {
public:
	nwc_scenario_rules_t(uint32 sync_step=0, uint32 map_counter=0) : network_world_command_t(NWC_SCENARIO_RULES, sync_step, map_counter), rule( new scenario_t::forbidden_t()), forbid(true) { }
	~nwc_scenario_rules_t() { delete rule; }
	virtual void do_command(karte_t *);
	virtual void rdwr();
	virtual const char* get_name() { return "nwc_scenario_rules_t";}

	scenario_t::forbidden_t *rule;
	bool forbid;

private:
	nwc_scenario_rules_t(const nwc_scenario_rules_t&);
	nwc_scenario_rules_t& operator=(const nwc_scenario_rules_t&);
};
#endif
