#include "api.h"

/** @file api_gui.cc exports gui-related functions. */

#include "../api_class.h"
#include "../api_function.h"

#include "../../simmesg.h"
#include "../../simmenu.h"
#include "../../simworld.h"
#include "../../dataobj/scenario.h"
#include "../../player/simplay.h"

using namespace script_api;


void_t add_scenario_message_at(const char* text, koord pos)
{
	if (text) {
		message_t *msg = welt->get_message();
		msg->add_message(text, pos, message_t::scenario, PLAYER_FLAG|welt->get_active_player()->get_player_nr());
	}
	return void_t();
}


call_tool_init add_scenario_message(player_t* player, const char* text)
{
	// build param string (see tool_add_message_t::init)
	cbuffer_t buf;
	buf.printf("%d,%s", message_t::scenario, text);

	return call_tool_init(TOOL_ADD_MESSAGE | SIMPLE_TOOL, (const char*)buf, 0, player ? player : welt->get_active_player());
}

bool open_info_win_result(scenario_t* scen)
{
	return scen->open_info_win();
}

void export_gui(HSQUIRRELVM vm, bool scenario)
{
	/**
	 * Table with methods to access gui functions.
	 */
	begin_class(vm, "gui", 0);

	if (scenario) {
		/**
		 * Opens scenario info window and shows 'result' tab.
		 * @note Only available in scenario mode.
		 */
		STATIC register_method(vm, &open_info_win_result, "open_info_win", true);

		/**
		 * Opens scenario info window with specific tab open.
		 * @param tab possible values are "info", "goal", "rules", "result", "about"
		 * @note Only available in scenario mode.
		 */
		STATIC register_method(vm, &scenario_t::open_info_win, "open_info_win_at");

		/**
		* Adds message to the players mailboxes.
		* Will be shown in ticker or as pop-up window depending on players preferences.
		* Message window has small view of world.
		*
		* @param text Text to be shown. Has to be a translated string or a translatable string.
		* @param position Position of the view on the map. Clicking on the message will center viewport at this position.
		* @warning Message only shown on server, but stored in savegame.
		* @note Only available in scenario mode.
		* @ingroup scen_only
		*/
		STATIC register_method(vm, &add_scenario_message_at, "add_message_at");

		/**
		* Adds message to the players mailboxes.
		* Will be shown in ticker or as pop-up window depending on players preferences.
		*
		* @param text Text to be shown. Has to be a translated string or a translatable string.
		* @note Only available in scenario mode.
		* @ingroup scen_only
		*/
		STATIC register_method(vm, &add_scenario_message, "add_message");
	}

	end_class(vm);
}
