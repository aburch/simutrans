#include "network_cmd_scenario.h"

#include "network_packet.h"
#include "../simworld.h"
#include "../dataobj/scenario.h"
#include "../dataobj/environment.h"


void nwc_scenario_t::rdwr()
{
	network_command_t::rdwr();
	packet->rdwr_short(what);
	packet->rdwr_short(won);
	packet->rdwr_short(lost);
	packet->rdwr_str(function);
	packet->rdwr_str(result);
}


bool nwc_scenario_t::execute(karte_t *welt)
{
	scenario_t *scen = welt->get_scenario();
	if (scen == NULL  ||  !scen->is_scripted()) {
		return true;
	}
	script_vm_t *script = scen->script;

	if (env_t::server) {
		switch (what) {
			case CALL_SCRIPT:
			case CALL_SCRIPT_ANSWER: {
				plainstring res = dynamic_string::fetch_result(function, script, NULL, what==CALL_SCRIPT_ANSWER);

				nwc_scenario_t nwc;
				nwc.what = CALL_SCRIPT_ANSWER;
				nwc.function = function;
				nwc.result = res;
				nwc.send( packet->get_sender() );
				break;
			}
			case UPDATE_WON_LOST:
			default: ;
		}
	}
	else {
		switch (what) {
			case CALL_SCRIPT_ANSWER:
				// store result, call listening string
				dynamic_string::record_result(function, result);
				break;

			case UPDATE_WON_LOST:
				scen->update_won_lost(won, lost);
				break;

			default: ;
		}
	}
	return true;
}


void nwc_scenario_rules_t::do_command(karte_t *welt)
{
	scenario_t *scen = welt->get_scenario();

	scen->intern_forbid(rule, forbid);
	rule = NULL; // pointer is now invalid
}


void nwc_scenario_rules_t::rdwr()
{
	network_world_command_t::rdwr();
	rule->rdwr(packet);
	packet->rdwr_bool(forbid);
}
