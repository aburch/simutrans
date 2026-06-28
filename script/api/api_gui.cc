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
#include "../../simevent.h"
#include "../../simworld.h"
#include "../../dataobj/scenario.h"
#include "../../display/viewport.h"
#include "../../gui/simwin.h"
#include "../../gui/gui_frame.h"
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

bool open_dialog_tool(sint32 tool_nr)
{
	if (tool_nr < 0  ||  tool_nr >= DIALOGE_TOOL_COUNT) {
		return false;
	}

	player_t *player = welt->get_active_player();
	if (player == NULL) {
		return false;
	}

	tool_t *tool = create_tool((uint16)tool_nr | DIALOGE_TOOL);
	if (tool == NULL) {
		return false;
	}

	welt->set_tool(tool, player);
	delete tool;
	return true;
}

static void create_bounds_slot(HSQUIRRELVM vm, const char *name, scr_coord pos, scr_size size, SQInteger table_idx = -1)
{
	sq_pushstring(vm, name, -1);
	sq_newtableex(vm, 4);
	create_slot(vm, "x", (SQInteger)pos.x);
	create_slot(vm, "y", (SQInteger)pos.y);
	create_slot(vm, "w", (SQInteger)size.w);
	create_slot(vm, "h", (SQInteger)size.h);
	sq_newslot(vm, table_idx > 0 ? table_idx : -3, false);
}

#ifdef SQAPI_DOC
	/**
	 * Rectangle in screen pixels.
	 */
	class gui_bounds_x {
		public:
			/**
			 * Left coordinate.
			 */
			integer x;
			/**
			 * Top coordinate.
			 */
			integer y;
			/**
			 * Width.
			 */
			integer w;
			/**
			 * Height.
			 */
			integer h;
	};

	/**
	 * Snapshot of an open GUI window returned by @ref gui::get_windows.
	 */
	class gui_window_x {
		public:
			/**
			 * Window id to pass to @ref gui::get_window_components.
			 */
			integer id;
			/**
			 * Stacking order. Higher array indices are closer to the front.
			 */
			integer z_order;
			/**
			 * True if this is the top window.
			 */
			bool top;
			/**
			 * Window title, or an empty string if no title is available.
			 */
			string title;
			/**
			 * True if the window has a title bar.
			 */
			bool has_title;
			/**
			 * Full window bounds in screen pixels.
			 */
			gui_bounds_x bounds;
			/**
			 * Client area bounds in screen pixels.
			 */
			gui_bounds_x client_bounds;
	};

	/**
	 * Snapshot of a GUI component returned by @ref gui::get_window_components.
	 */
	class gui_component_x {
		public:
			/**
			 * Component id, unique within one component snapshot.
			 */
			integer id;
			/**
			 * Component role, such as "button", "label", "textinput", "container", or "scrollpane".
			 */
			string role;
			/**
			 * True if the component is visible.
			 */
			bool visible;
			/**
			 * True if the component can receive focus.
			 */
			bool focusable;
			/**
			 * True if the component currently has focus.
			 */
			bool focused;
			/**
			 * Bounds relative to the parent component.
			 */
			gui_bounds_x bounds;
			/**
			 * Bounds in screen pixels.
			 */
			gui_bounds_x screen_bounds;
			/**
			 * Text exposed by the component. Only present when the component has text.
			 */
			string text;
			/**
			 * Child component snapshots.
			 */
			array<gui_component_x> children;
			/**
			 * Horizontal scroll position. Only present on scrollpane components.
			 */
			integer scroll_x;
			/**
			 * Vertical scroll position. Only present on scrollpane components.
			 */
			integer scroll_y;
	};
#endif

class squirrel_accessibility_property_collector_t : public accessibility_property_collector_t
{
private:
	HSQUIRRELVM vm;
	SQInteger table_idx;

public:
	squirrel_accessibility_property_collector_t(HSQUIRRELVM vm, SQInteger table_idx) : vm(vm), table_idx(table_idx) {}

	void add(const char *key, sint32 value) OVERRIDE { create_slot(vm, key, value, false, table_idx); }
	void add(const char *key, const char *value) OVERRIDE { create_slot(vm, key, value, false, table_idx); }
	void add(const char *key, bool value) OVERRIDE { create_slot(vm, key, value, false, table_idx); }
};

static void push_accessible_component(HSQUIRRELVM vm, gui_component_t *comp, scr_coord offset, uint32 &next_id)
{
	const uint32 id = next_id++;
	const scr_coord pos = comp->get_pos();
	const scr_size size = comp->get_size();
	const scr_coord screen_pos = offset + pos;

	sq_newtable(vm);
	const SQInteger table_idx = sq_gettop(vm);
	create_slot(vm, "id", (SQInteger)id, false, table_idx);
	create_slot(vm, "role", comp->get_accessibility_role(), false, table_idx);
	create_slot(vm, "visible", comp->is_visible(), false, table_idx);
	create_slot(vm, "focusable", comp->is_focusable(), false, table_idx);
	create_slot(vm, "focused", win_get_focus() == comp, false, table_idx);
	create_bounds_slot(vm, "bounds", pos, size, table_idx);
	create_bounds_slot(vm, "screen_bounds", screen_pos, size, table_idx);

	if (const char *text = comp->get_accessibility_text()) {
		create_slot(vm, "text", text, false, table_idx);
	}

	squirrel_accessibility_property_collector_t collector(vm, table_idx);
	comp->add_accessibility_properties(collector);

	sq_pushstring(vm, "children", -1);
	sq_newarray(vm, 0);
	vector_tpl<gui_component_t *> children;
	comp->get_accessibility_children(children);
	for (uint32 i = 0; i < children.get_count(); i++) {
		gui_component_t *child = children[i];
		push_accessible_component(vm, child, offset + comp->get_accessibility_child_screen_offset(child), next_id);
		sq_arrayappend(vm, -2);
	}
	sq_newslot(vm, table_idx, false);

	sq_settop(vm, table_idx);
}

static SQInteger get_windows(HSQUIRRELVM vm)
{
	sq_newarray(vm, 0);
	const uint32 count = win_get_open_count();
	for (uint32 i = 0; i < count; i++) {
		gui_frame_t *win = win_get_index(i);
		if (!win) {
			continue;
		}

		const scr_coord pos = win_get_pos(win);
		const scr_size size = win->get_windowsize();
		const scr_size client_size = win->get_client_windowsize();
		const scr_coord client_pos(pos.x, pos.y + (win->has_title() ? D_TITLEBAR_HEIGHT : 0));

		sq_newtable(vm);
		const SQInteger table_idx = sq_gettop(vm);
		create_slot(vm, "id", (SQInteger)i, false, table_idx);
		create_slot(vm, "z_order", (SQInteger)i, false, table_idx);
		create_slot(vm, "top", win_get_top() == win, false, table_idx);
		create_slot(vm, "title", win->get_name() ? win->get_name() : "", false, table_idx);
		create_slot(vm, "has_title", win->has_title(), false, table_idx);
		create_bounds_slot(vm, "bounds", pos, size, table_idx);
		create_bounds_slot(vm, "client_bounds", client_pos, client_size, table_idx);
		sq_arrayappend(vm, -2);
	}
	return 1;
}

static SQInteger get_window_components(HSQUIRRELVM vm)
{
	SQInteger window_id;
	sq_getinteger(vm, 2, &window_id);
	if (window_id < 0 || (uint32)window_id >= win_get_open_count()) {
		sq_pushnull(vm);
		return 1;
	}

	gui_frame_t *win = win_get_index((uint32)window_id);
	if (!win) {
		sq_pushnull(vm);
		return 1;
	}

	sq_newarray(vm, 0);
	uint32 next_id = 0;
	const scr_coord component_offset = win_get_pos(win) + scr_coord(0, win->has_title() ? D_TITLEBAR_HEIGHT : 0);
	const vector_tpl<gui_component_t *> &components = win->get_components();
	for (uint32 i = 0; i < components.get_count(); i++) {
		push_accessible_component(vm, components[i], component_offset, next_id);
		sq_arrayappend(vm, -2);
	}
	return 1;
}

static unsigned int get_mouse_key_mod(HSQUIRRELVM vm, SQInteger shift_index, SQInteger ctrl_index)
{
	const SQInteger top = sq_gettop(vm);
	const bool shift = top >= shift_index ? param<bool>::get(vm, shift_index) : false;
	const bool ctrl = top >= ctrl_index ? param<bool>::get(vm, ctrl_index) : false;
	return (shift ? SIM_MOD_SHIFT : SIM_MOD_NONE) | (ctrl ? SIM_MOD_CTRL : SIM_MOD_NONE);
}

static void queue_left_mouse_event(event_class_t event_class, sint32 mx, sint32 my, sint32 cx, sint32 cy, int button_state, unsigned int key_mod)
{
	event_t *event = new event_t(event_class);
	event->ev_code = MOUSE_LEFTBUTTON;
	event->mx = mx;
	event->my = my;
	event->cx = cx;
	event->cy = cy;
	event->button_state = button_state;
	event->ev_key_mod = key_mod;
	queue_event(event);
}

static SQInteger click(HSQUIRRELVM vm)
{
	const SQInteger top = sq_gettop(vm);
	if(  top < 3  ||  top > 5  ) {
		return sq_raise_error(vm, "Expected gui.click(x, y, [shift], [ctrl])");
	}

	const sint32 x = param<sint32>::get(vm, 2);
	const sint32 y = param<sint32>::get(vm, 3);
	const unsigned int key_mod = get_mouse_key_mod(vm, 4, 5);

	queue_left_mouse_event(EVENT_CLICK, x, y, x, y, MOUSE_LEFTBUTTON, key_mod);
	queue_left_mouse_event(EVENT_RELEASE, x, y, x, y, 0, key_mod);

	return SQ_OK;
}

static SQInteger drag(HSQUIRRELVM vm)
{
	const SQInteger top = sq_gettop(vm);
	if(  top < 5  ||  top > 7  ) {
		return sq_raise_error(vm, "Expected gui.drag(from_x, from_y, to_x, to_y, [shift], [ctrl])");
	}

	const sint32 from_x = param<sint32>::get(vm, 2);
	const sint32 from_y = param<sint32>::get(vm, 3);
	const sint32 to_x = param<sint32>::get(vm, 4);
	const sint32 to_y = param<sint32>::get(vm, 5);
	const unsigned int key_mod = get_mouse_key_mod(vm, 6, 7);

	queue_left_mouse_event(EVENT_CLICK, from_x, from_y, from_x, from_y, MOUSE_LEFTBUTTON, key_mod);
	queue_left_mouse_event(EVENT_DRAG, to_x, to_y, from_x, from_y, MOUSE_LEFTBUTTON, key_mod);
	queue_left_mouse_event(EVENT_RELEASE, to_x, to_y, from_x, from_y, 0, key_mod);

	return SQ_OK;
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

	/**
	* Opens a dialog tool for the active player.
	*
	* @param tool_nr dialog tool number, for example DIALOG_MINIMAP = 2
	* @returns whether the dialog tool request was accepted
	*/
	STATIC register_method(vm, &open_dialog_tool, "open_dialog_tool");

	/**
	* Get snapshots of currently open GUI windows.
	*
	* Each entry contains id, z_order, top, title, has_title, bounds, and client_bounds.
	* Pass the id to @ref gui::get_window_components to inspect the components in that window.
	* @typemask array<gui_window_x> ()
	*/
	register_function(vm, get_windows, "get_windows", 1, ".");

	/**
	* Get snapshots of components in the given GUI window.
	*
	* Components are returned as a tree. Each component has role, bounds, screen_bounds,
	* visible, focusable, focused, children, and optional text fields.
	* @param window_id id property of a window snapshot returned by @ref gui::get_windows.
	* @returns array of @ref gui_component_x, or null if window_id is invalid.
	* @typemask array<gui_component_x> (integer)
	*/
	register_function(vm, get_window_components, "get_window_components", 2, ".i");

	/**
	* Simulate a left mouse click at the given screen coordinates.
	*
	* @param x x coordinate on screen.
	* @param y y coordinate on screen.
	* @param shift true to click with shift pressed.
	* @param ctrl true to click with ctrl pressed.
	*/
	register_function(vm, click, "click", -3, ".iibb");

	/**
	* Simulate a left mouse drag from one screen coordinate to another.
	*
	* @param from_x start x coordinate on screen.
	* @param from_y start y coordinate on screen.
	* @param to_x end x coordinate on screen.
	* @param to_y end y coordinate on screen.
	* @param shift true to drag with shift pressed.
	* @param ctrl true to drag with ctrl pressed.
	*/
	register_function(vm, drag, "drag", -5, ".iiiibb");
	end_class(vm);
}
