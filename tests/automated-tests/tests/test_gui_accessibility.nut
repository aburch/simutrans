//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for GUI accessibility snapshots.
//

function test_gui_accessibility_windows_and_components()
{
	gui.close_all_windows()
	gui.open_info_win()

	local windows = gui.get_windows()
	ASSERT_TRUE(windows.len() > 0)

	local target_window = null
	foreach (w in windows) {
		ASSERT_TRUE("id" in w)
		ASSERT_TRUE("bounds" in w)
		ASSERT_TRUE("client_bounds" in w)
		ASSERT_TRUE("title" in w)
		ASSERT_TRUE("x" in w.bounds)
		ASSERT_TRUE("y" in w.bounds)
		ASSERT_TRUE("w" in w.bounds)
		ASSERT_TRUE("h" in w.bounds)

		ASSERT_EQUAL(typeof(w.id), "integer")
		ASSERT_EQUAL(typeof(w.z_order), "integer")
		ASSERT_EQUAL(typeof(w.top), "bool")
		ASSERT_EQUAL(typeof(w.title), "string")
		ASSERT_EQUAL(typeof(w.has_title), "bool")
		ASSERT_EQUAL(typeof(w.bounds.x), "integer")
		ASSERT_EQUAL(typeof(w.bounds.y), "integer")
		ASSERT_EQUAL(typeof(w.bounds.w), "integer")
		ASSERT_EQUAL(typeof(w.bounds.h), "integer")
		ASSERT_EQUAL(typeof(w.client_bounds.x), "integer")
		ASSERT_EQUAL(typeof(w.client_bounds.y), "integer")
		ASSERT_EQUAL(typeof(w.client_bounds.w), "integer")
		ASSERT_EQUAL(typeof(w.client_bounds.h), "integer")

		if (w.top) {
			target_window = w
		}
	}
	ASSERT_TRUE(target_window != null)

	local components = gui.get_window_components(target_window.id)
	ASSERT_TRUE(components != null)
	ASSERT_TRUE(components.len() > 0)

	foreach (c in components) {
		ASSERT_TRUE("id" in c)
		ASSERT_TRUE("role" in c)
		ASSERT_TRUE("screen_bounds" in c)
		ASSERT_TRUE("children" in c)
		ASSERT_TRUE("x" in c.screen_bounds)
		ASSERT_TRUE("y" in c.screen_bounds)
		ASSERT_TRUE("w" in c.screen_bounds)
		ASSERT_TRUE("h" in c.screen_bounds)

		ASSERT_EQUAL(typeof(c.id), "integer")
		ASSERT_EQUAL(typeof(c.role), "string")
		ASSERT_EQUAL(typeof(c.visible), "bool")
		ASSERT_EQUAL(typeof(c.focusable), "bool")
		ASSERT_EQUAL(typeof(c.focused), "bool")
		ASSERT_EQUAL(typeof(c.screen_bounds.x), "integer")
		ASSERT_EQUAL(typeof(c.screen_bounds.y), "integer")
		ASSERT_EQUAL(typeof(c.screen_bounds.w), "integer")
		ASSERT_EQUAL(typeof(c.screen_bounds.h), "integer")
		ASSERT_EQUAL(typeof(c.children), "array")
	}

	ASSERT_EQUAL(gui.get_window_components(-1), null)

	gui.close_all_windows()
}
