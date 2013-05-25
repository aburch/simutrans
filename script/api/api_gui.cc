#include "api.h"

/** @file api_gui.cc exports gui-related functions. */

#include "../api_class.h"
#include "../api_function.h"

#include "../../simmesg.h"
#include "../../simworld.h"
#include "../../dataobj/scenario.h"
#include "../../player/simplay.h"

using namespace script_api;

#define STATIC

void_t add_scenario_message_at(const char* text, koord pos)
{
	message_t *msg = welt->get_message();
	msg->add_message(text, pos, message_t::scenario, PLAYER_FLAG|welt->get_active_player()->get_player_nr());
	return void_t();
}


void export_gui(HSQUIRRELVM vm)
{
	/**
	 * Table with methods to access gui functions.
	 */
	begin_class(vm, "gui", 0);

	/**
	 * Opens scenario info window.
	 */
	STATIC register_method(vm, &scenario_t::open_info_win, "open_info_win");

	/**
	 * Adds message to the players mailboxes.
	 * Will be shown in ticker or as pop-up window depending on players preferences.
	 * Message window has small view of world.
	 *
	 * @param text Text to be shown. Has to be a translated string or a translatable string.
	 * @param position Position of the view on the map. Clicking on the message will center viewport at this position.
	 * @warning Message only shown on server, but stored in savegame.
	 */
	STATIC register_method(vm, &add_scenario_message_at, "add_message_at");

	/**
	 * Adds message to the players mailboxes.
	 * Will be shown in ticker or as pop-up window depending on players preferences.
	 *
	 * @param text Text to be shown. Has to be a translated string or a translatable string.
	 * @warning Message only shown on server, but stored in savegame.
	 */
	STATIC register_method_fv(vm, &add_scenario_message_at, "add_message", freevariable<koord>(koord::invalid) );

	end_class(vm);
}
