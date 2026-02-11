//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for building trees and forests
//

function build_tree(pl, pos, desc, ignore_climate = false, random_age = false)
{
	return command_x(tool_plant_tree).work(pl, pos, "" + ignore_climate.tointeger() + random_age.tointeger() + "," + desc.get_name())
}


function test_trees_plant_single_invalid_pos()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local tree_desc = tree_desc_x("Ahorn-1")

	ASSERT_TRUE(tree_desc != null)

	// build outside of map borders, should fail (obv)
	{
		ASSERT_EQUAL(build_tree(public_pl, coord3d(100, 100, 0), tree_desc), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), 0)
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_trees_plant_single_invalid_param()
{
	local public_pl = player_x(1)

	// invalid default_param -> don't crash
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_plant_tree).work(public_pl, coord3d(4, 2, 0), "1"), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}

		ASSERT_TRUE(error_caught)
			ASSERT_TRUE(tile_x(4, 2, 0).find_object(mo_tree) == null)
	}

	// invalid default_param -> don't crash
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_plant_tree).work(public_pl, coord3d(4, 2, 0), "11"), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}

		ASSERT_TRUE(error_caught)
		ASSERT_TRUE(tile_x(4, 2, 0).find_object(mo_tree) == null)
	}

	// invalid default_param -> don't crash
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_plant_tree).work(public_pl, coord3d(4, 2, 0), "11,"), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}

		ASSERT_TRUE(error_caught)

		ASSERT_TRUE(tile_x(4, 2, 0).find_object(mo_tree) == null)
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_trees_plant_single_null_param()
{
	local public_pl = player_x(1)

	// null default_param -> random tree
	{
		ASSERT_EQUAL(command_x(tool_plant_tree).work(public_pl, coord3d(4, 2, 0), null), null)
		ASSERT_TRUE(tile_x(4, 2, 0).find_object(mo_tree) != null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(4, 2, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}

function test_trees_plant_single_empty_param()
{
	local public_pl = player_x(1)

	// empty default_param -> random tree
	{
		ASSERT_EQUAL(command_x(tool_plant_tree).work(public_pl, coord3d(4, 2, 0), ""), null)
		ASSERT_TRUE(tile_x(4, 2, 0).find_object(mo_tree) != null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(4, 2, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_trees_plant_single_invalid_desc()
{
	local public_pl = player_x(1)

	// invalid desc name -> random tree
	{
		ASSERT_EQUAL(command_x(tool_plant_tree).work(public_pl, coord3d(4, 2, 0), "11,bla"), null)
		ASSERT_TRUE(tile_x(4, 2, 0).find_object(mo_tree) != null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(4, 2, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_trees_plant_single_when_disabled()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local tree_desc = tree_desc_x("Ahorn-1")

	// build single tree, should fail because trees are disallowed
	{
		ASSERT_EQUAL(build_tree(pl, coord3d(4, 3, 0), tree_desc), "Trees disabled!")
		ASSERT_EQUAL(tile_x(4, 3, 0).find_object(mo_tree), null)
		ASSERT_EQUAL(pl.get_current_maintenance(), 0)
	}

	// public player can build trees anyway
	{
		ASSERT_EQUAL(build_tree(public_pl, coord3d(4, 3, 0), tree_desc), null)
		local tree = tile_x(4, 3, 0).find_object(mo_tree)
		ASSERT_TRUE(tree != null)
		ASSERT_EQUAL(tree.get_desc().get_name(), tree_desc.get_name())
		ASSERT_EQUAL(public_pl.get_current_maintenance(), 0)

		ASSERT_TRUE(tree.get_owner() != null) // tree is owned by everybody (player_all) but there is no way to check for player == player_all
		ASSERT_EQUAL(tree.get_age(), 0)

	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(4, 3, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_trees_plant_single_random_age()
{
	local public_pl = player_x(1)
	local tree_desc = tree_desc_x("Ahorn-1")

	// check planting with random age
	{
		ASSERT_EQUAL(build_tree(public_pl, coord3d(4, 3, 0), tree_desc, false, true), null)
		local tree = tile_x(4, 3, 0).find_object(mo_tree)
		ASSERT_TRUE(tree != null)
		ASSERT_EQUAL(tree.get_desc().get_name(), tree_desc.get_name())
		ASSERT_EQUAL(public_pl.get_current_maintenance(), 0)

		ASSERT_TRUE(tree.get_owner() != null) // tree is owned by everybody (player_all) but there is no way to check for player == player_all
		ASSERT_TRUE(tree.get_age() >= 0)
		ASSERT_TRUE(tree.get_age() < (1<<12))

	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(4, 3, 0)), null)
	RESET_ALL_PLAYER_FUNDS();
}


function test_trees_plant_single_ignore_climate()
{
	local public_pl = player_x(1)
	local tree_desc = tree_desc_x("Ahorn-1")

	ASSERT_EQUAL(command_x(tool_set_climate).work(public_pl, coord3d(4, 3, 0), coord3d(4, 3, 0), "" + cl_arctic), null)

	// Check planting with ignore_climate = true
	{
		ASSERT_EQUAL(build_tree(public_pl, coord3d(4, 3, 0), tree_desc, false, false), "")
		ASSERT_EQUAL(tile_x(4, 3, 0).find_object(mo_tree), null)

		ASSERT_EQUAL(build_tree(public_pl, coord3d(4, 3, 0), tree_desc, true, false), null)
		ASSERT_TRUE(tile_x(4, 3, 0).find_object(mo_tree) != null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(command_x(tool_set_climate).work(public_pl, coord3d(4, 3, 0), coord3d(4, 3, 0), "" + cl_mediterran), null)
	RESET_ALL_PLAYER_FUNDS();
}


function test_trees_plant_single_max_per_square()
{
	local public_pl = player_x(1)
	local tree_desc = tree_desc_x("Ahorn-1")

	ASSERT_EQUAL(build_tree(public_pl, coord3d(4, 3, 0), tree_desc), null)
	ASSERT_EQUAL(build_tree(public_pl, coord3d(4, 3, 0), tree_desc), null)
	ASSERT_EQUAL(build_tree(public_pl, coord3d(4, 3, 0), tree_desc), null)
	ASSERT_EQUAL(build_tree(public_pl, coord3d(4, 3, 0), tree_desc), null)
	ASSERT_EQUAL(build_tree(public_pl, coord3d(4, 3, 0), tree_desc), null)

	// Check building over max_no_of_trees_on_square limit (= 5)
	{
		ASSERT_EQUAL(build_tree(public_pl, coord3d(4, 3, 0), tree_desc), "")
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(4, 3, 0)), null) // removes all trees at once
	ASSERT_EQUAL(tile_x(4, 3, 0).find_object(mo_tree), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_trees_plant_single_occupied()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local tree_desc = tree_desc_x("Ahorn-1")

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 3, 0), coord3d(4, 5, 0), road, true), null)

	// Cannot build trees on ways
	{
		ASSERT_EQUAL(build_tree(public_pl, coord3d(4, 4, 0), tree_desc), "")
		ASSERT_EQUAL(tile_x(4, 4, 0).find_object(mo_tree), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(4, 3, 0), coord3d(4, 5, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_trees_plant_forest()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local planter = command_x(tool_forest)
	local tree_desc = tree_desc_x("Busch_1")

	local topleft = coord3d(1, 1, 0)
	local bottomright = coord3d(6, 6, 0)

	// 1x1
	{
		ASSERT_EQUAL(planter.work(public_pl, topleft, topleft, ""), null)
		ASSERT_EQUAL(tile_x(topleft.x, topleft.y, 0).find_object(mo_tree), null)
	}

	// 2x2
	{
		ASSERT_EQUAL(planter.work(public_pl, topleft, topleft + coord(1, 1), ""), null)
		ASSERT_EQUAL(tile_x(topleft.x+0, topleft.y+0, 0).find_object(mo_tree), null)
		ASSERT_EQUAL(tile_x(topleft.x+1, topleft.y+0, 0).find_object(mo_tree), null)
		ASSERT_EQUAL(tile_x(topleft.x+0, topleft.y+1, 0).find_object(mo_tree), null)
		ASSERT_TRUE(tile_x(topleft.x+1, topleft.y+1, 0).find_object(mo_tree) != null)
	}

	// this works with player 0 even though trees are disabled
	{
		ASSERT_EQUAL(planter.work(pl, topleft, bottomright, ""), null)

		for (local y = 0; y < 8; ++y) {
			for (local x = 0; x < 8; ++x) {
				local outside = ((y+1)%8 < 3) || ((x+1)%8 < 3) // FIXME: this should fail for == 2

				if (outside) {
					ASSERT_EQUAL(tile_x(x, y, 0).find_object(mo_tree), null)
				}
			}
		}

		ASSERT_EQUAL(pl.get_current_maintenance(), 0)
	}

	// clean up
	for (local y = 0; y < 8; ++y) {
		for (local x = 0; x < 8; ++x) {
			command_x(tool_remover).work(pl, coord3d(x, y, 0))
		}
	}

	RESET_ALL_PLAYER_FUNDS()
}
