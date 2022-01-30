/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "network_cmd_scenario.h"

#include "network_packet.h"
#include "network_socket_list.h"
#include "../simworld.h"
#include "../dataobj/scenario.h"
#include "../dataobj/environment.h"
#include "../script/script.h"


void nwc_scenario_t::init(script_vm_t *script)
{
	script->register_callback(&nwc_scenario_t::record_result, "nwc_scenario_t_record_result");
}


bool nwc_scenario_t::record_result(const char* function, plainstring result, uint32 client_id)
{
	SOCKET sock = socket_list_t::get_socket(client_id);
	if (sock == INVALID_SOCKET) {
		return false;
	}
	nwc_scenario_t nwc;
	nwc.what = CALL_SCRIPT_ANSWER;
	nwc.function = function;
	nwc.result = result;
	nwc.send( sock );
	return true;
}

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

	if (what == OPEN_SCEN_WIN) {
		// open window on server and clients
		scen->open_info_win(function);
		return true;
	}

	if (env_t::server) {
		switch (what) {
			case CALL_SCRIPT:
			case CALL_SCRIPT_ANSWER: {
				// register callback to send result back to client if script is delayed.
				script->prepare_callback("nwc_scenario_t_record_result", 2, function, (const char*)"", socket_list_t::get_client_id( packet->get_sender() ) );
				plainstring res = dynamic_string::fetch_result(function, script, NULL, what==CALL_SCRIPT_ANSWER);
				// clear callback, in case function call was successfull
				script->clear_pending_callback();

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
	network_broadcast_world_command_t::rdwr();
	rule->rdwr(packet);
	packet->rdwr_bool(forbid);
}


nwc_scenario_rules_t::nwc_scenario_rules_t(const nwc_scenario_rules_t& nwr)
: network_broadcast_world_command_t(NWC_SCENARIO_RULES, nwr.get_sync_step(), nwr.get_map_counter())
{
	forbid = nwr.forbid;
	rule = new scenario_t::forbidden_t(*nwr.rule);
}


network_broadcast_world_command_t* nwc_scenario_rules_t::clone(karte_t *)
{
	// scenario scripts only run on server
	if (socket_list_t::get_client_id(packet->get_sender()) != 0) {
		// not sent by server
		return NULL;
	}
	return new nwc_scenario_rules_t(*this);
}
