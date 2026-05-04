//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//

//
// Tests for adding, removing and deleting cities
//


function test_city_add_invalid()
{
	ASSERT_EQUAL(command_x(tool_add_city).work(player_x(1), coord3d(-1, -1, 0)), "")
}


function test_city_add_cannot_afford()
{
	ASSERT_EQUAL(command_x(tool_add_city).work(player_x(0), coord3d(7, 8, 0)), "Insufficient funds!")

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_city_add_by_public_player()
{
	{
		ASSERT_EQUAL(command_x(tool_add_city).work(player_x(1), coord3d(7, 8, 0)), null)

		ASSERT_TRUE(tile_x(7, 8, 0).find_object(mo_building) != null)
		ASSERT_TRUE(tile_x(6, 9, 0).get_way(wt_road) != null)
		ASSERT_TRUE(tile_x(7, 9, 0).get_way(wt_road) != null)
		ASSERT_TRUE(tile_x(8, 9, 0).get_way(wt_road) != null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(7, 8, 0)), null)
	// street
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(6, 9, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(7, 9, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(8, 9, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_city_add_on_existing_townhall()
{
	ASSERT_EQUAL(command_x(tool_add_city).work(player_x(1), coord3d(7, 8, 0)), null)

	{
		ASSERT_EQUAL(command_x(tool_add_city).work(player_x(1), coord3d(7, 8, 0)), "Cities can only be built on flat empty ground!")
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(7, 8, 0)), null)
	// street
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(6, 9, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(7, 9, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(8, 9, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_city_add_near_map_border()
{
	{
		ASSERT_EQUAL(command_x(tool_add_city).work(player_x(1), coord3d(0, 15, 0)), null)

		ASSERT_TRUE(tile_x(0, 14, 0).find_object(mo_building) != null)
		ASSERT_TRUE(tile_x(1, 13, 0).get_way(wt_road) != null)
		ASSERT_TRUE(tile_x(1, 14, 0).get_way(wt_road) != null)
		ASSERT_TRUE(tile_x(1, 15, 0).get_way(wt_road) != null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(0, 14, 0)), null)
	// street
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(1, 13, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(1, 14, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(1, 15, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_city_change_size_invalid_params()
{
	local pl = player_x(0)

	// not a city
	{
		ASSERT_EQUAL(command_x(tool_change_city_size).work(pl, coord3d(1, 1, 0), "100"), "")
	}

	ASSERT_EQUAL(command_x(tool_add_city).work(player_x(1), coord3d(1, 1, 0)), null)

	// invalid pos
	{
		ASSERT_EQUAL(command_x(tool_change_city_size).work(pl, coord3d(-1,-1,-1), "100"), "")
	}

	// Null default_param
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_change_city_size).work(pl, coord3d(1, 1, 0), null), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)
	}

	// Empty default_param
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_change_city_size).work(pl, coord3d(1, 1, 0), ""), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)
	}

	// Wrong default_param
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_change_city_size).work(pl, coord3d(1, 1, 0), "bla"), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(1, 1, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(1, 2, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


// Exercises the stadt_t::~stadt_t() multi-tile-building path: build a
// city, grow it until check_bau_townhall picks a multi-tile townhall
// (pak64 lands on 04_CITY 2x2 around bev=20000), tear down every
// non-townhall building first to keep the destructor's iteration
// focused on the multi-tile townhall, then remove the townhall.
// Without the destructor fix in this commit, the cascade through
// gebaeude_t::cleanup -> stadt_t::remove_gebaeude_from_stadt asserts
// when it walks back to the already-popped head tile.
function test_city_remove_with_multitile_townhall()
{
	local pl = player_x(1)
	local townhall_pos = coord3d(8, 8, 0)

	ASSERT_EQUAL(command_x(tool_add_city).work(pl, townhall_pos, "0"), null)
	ASSERT_EQUAL(command_x(tool_change_city_size).work(player_x(0), townhall_pos, "20000"), null)

	// Tear down every non-townhall building.  Capture the townhall's
	// position on the way through.
	local size = world.get_size()
	local th_corner = null
	for (local x = 0; x < size.x; x++) {
		for (local y = 0; y < size.y; y++) {
			local b = tile_x(x, y, 0).find_object(mo_building)
			if (b == null) continue
			if (b.is_townhall()) {
				if (th_corner == null) th_corner = coord3d(x, y, 0)
				continue
			}
			command_x(tool_remover).work(pl, coord3d(x, y, 0))
		}
	}

	// Fail loudly if the pakset never upgraded the townhall to multi-
	// tile; otherwise the test would silently pass without exercising
	// the destructor path we care about.
	local th_size = tile_x(th_corner.x, th_corner.y, 0).find_object(mo_building).get_desc().get_size(0)
	ASSERT_TRUE(th_size.x > 1 || th_size.y > 1)

	ASSERT_EQUAL(command_x(tool_remover).work(pl, th_corner), null)

	// ~stadt_t() tears down everything in stadt::buildings (the
	// townhall, plus monuments/attractions added via
	// add_gebaeude_to_stadt) but not the auto-generated road network;
	// sweep what's left so RESET_ALL_PLAYER_FUNDS sees zero
	// maintenance.
	for (local x = 0; x < size.x; x++) {
		for (local y = 0; y < size.y; y++) {
			local tile = tile_x(x, y, 0)
			if (tile.get_way(wt_road) != null) {
				local p = coord3d(x, y, 0)
				command_x(tool_remove_way).work(pl, p, p, "" + wt_road)
			}
		}
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_city_change_size_to_minimum()
{
	ASSERT_EQUAL(command_x(tool_add_city).work(player_x(1), coord3d(1, 1, 0)), null)
	ASSERT_EQUAL(city_x(1, 1).get_citizens()[0], 126)

	{
		ASSERT_EQUAL(command_x(tool_change_city_size).work(player_x(0),  coord3d(1, 1, 0), "-100"), null)
		ASSERT_EQUAL(city_x(1, 1).get_citizens()[0], 126)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(1, 1, 0)), null)
	// street
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(0, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(1, 2, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(2, 2, 0)), null)

	RESET_ALL_PLAYER_FUNDS()
}
