/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_gui.cc exports gui-related functions. */

#include "../api_class.h"
#include "../api_function.h"

#include "../../simmesg.h"
#include "../../simmenu.h"
#include "../../simworld.h"
#include "../../dataobj/scenario.h"
#include "../../display/viewport.h"
#include "../../gui/simwin.h"
#include "../../player/simplay.h"

#include "../../dataobj/environment.h"
#include "../../network/network.h"
#include "../../network/network_cmd_scenario.h"

using namespace script_api;


call_tool_work add_scenario_message_at(const char* text, koord pos)
{
	// build param string (see tool_add_message_t::init)
	cbuffer_t buf;
	buf.printf("%d,%s", message_t::scenario, text);

	return call_tool_work(TOOL_ADD_MESSAGE | GENERAL_TOOL, (const char*)buf, 0, welt->get_active_player(), koord3d(pos, 0));
}

call_tool_work add_ai_message_at(player_t *player, const char* text, koord pos)
{
	// build param string (see tool_add_message_t::init)
	cbuffer_t buf;
	buf.printf("%d,%s", message_t::ai, text);

	return call_tool_work(TOOL_ADD_MESSAGE | GENERAL_TOOL, (const char*)buf, 0, player, koord3d(pos, 0));
}

call_tool_work add_scenario_message(player_t* player, const char* text)
{
	// build param string (see tool_add_message_t::init)
	cbuffer_t buf;
	buf.printf("%d,%s", message_t::scenario, text);

	return call_tool_work(TOOL_ADD_MESSAGE | GENERAL_TOOL, (const char*)buf, 0, player ? player : welt->get_active_player(), koord3d::invalid);
}

void_t open_info_win_client(const char* tab, uint8 player_nr)
{
	if (env_t::server) {
		// void network_send_all(network_command_t* nwc, bool exclude_us )
		nwc_scenario_t *nwc = new nwc_scenario_t();
		nwc->what = nwc_scenario_t::OPEN_SCEN_WIN;
		nwc->function = tab;
		network_send_all(nwc, false, player_nr);
	}
	else {
		welt->get_scenario()->open_info_win(tab);
	}
	return void_t();
}

void_t open_info_win_at(const char* tab)
{
	return open_info_win_client(tab, PLAYER_UNOWNED);
}

void_t open_info_win()
{
	return open_info_win_client("", PLAYER_UNOWNED);
}

bool jump(koord pos)
{
	if(welt->is_within_limits(pos)) {
		welt->get_viewport()->change_world_position(koord3d(pos,welt->min_hgt(pos)));
		return true;
	}
	return false;
}

void_t close_all_windows()
{
	destroy_all_win(true);
	return void_t();
}

void_t take_screenshot()
{
	display_snapshot( scr_rect(0, 0, display_get_width(), display_get_height()) );
	return void_t();
}

void_t set_zoom(uint8 val)
{
	set_zoom_factor_safe(val);
	welt->get_viewport()->metrics_updated();
	return void_t();
}

void export_gui(HSQUIRRELVM vm, bool scenario)
{
	/**
	 * Table with methods to access gui functions.
	 */
	begin_class(vm, "gui", 0);

	if (scenario) {
		/**
		 * Opens scenario info window and shows 'info' tab.
		 * In network mode, opens window on all clients and server.
		 * @note Only available in scenario mode.
		 */
		STATIC register_method(vm, open_info_win, "open_info_win", true);

		/**
		 * Opens scenario info window with specific tab open.
		 * In network mode, opens window on all clients and server.
		 * @param tab possible values are "info", "goal", "rules", "result", "about", "debug"
		 * @note Only available in scenario mode.
		 */
		STATIC register_method(vm, open_info_win_at, "open_info_win_at");

		/**
		 * Opens scenario info window for certain clients (and the server),
		 * with specific tab open.
		 * @param tab possible values are "info", "goal", "rules", "result", "about", "debug"
		 * @param player_nr opens scenario info window on all clients that have this player unlocked.
		 * @note Window is always opened on server.
		 */
		STATIC register_method(vm, open_info_win_client, "open_info_win_client");
		/**
		* Adds message to the players mailboxes.
		* Will be shown in ticker or as pop-up window depending on players preferences.
		* Message window has small view of world.
		*
		* @param text Text to be shown. Has to be a translated string or a translatable string.
		* @param position Position of the view on the map. Clicking on the message will center viewport at this position.
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
	else {
		/**
		* Adds message to the players mailboxes.
		* Will be shown in ticker or as pop-up window depending on players preferences.
		* Message window has small view of world.
		*
		* @param player sending this message
		* @param text Text to be shown. Has to be a translated string or a translatable string.
		* @param position Position of the view on the map. Clicking on the message will center viewport at this position.
		* @ingroup ai_only
		*/
		STATIC register_method(vm, &add_ai_message_at, "add_message_at");
	}
	
	/**
	* Jump view to given position.
	* This function succeeds only when the pos is within the map.
	*
	* @param position Position of the view on the map to jump to.
	* @return true if succeeded.
	*/
	STATIC register_method(vm, &jump, "jump");
	
	/**
	* Close all windows on the view.
	*/
	STATIC register_method(vm, &close_all_windows, "close_all_windows");
	
	/**
	* Take a screen shot.
	*/
	STATIC register_method(vm, &take_screenshot, "take_screenshot");
	
	/**
	* Set zoom factor.
	*
	* @param zoom Zoom factor to set.
	*/
	STATIC register_method(vm, &set_zoom, "set_zoom");
	end_class(vm);
}
