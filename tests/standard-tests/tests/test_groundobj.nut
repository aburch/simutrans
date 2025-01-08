//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for groundobj/movingobj
//


function test_groundobj_build_invalid_param()
{
	// not long enough
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(0, 0, 0), "0"), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)
	}

	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(0, 0, 0), "0a"), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)
	}

	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(0, 0, 0), "0ab"), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)
	}

	// invalid default_param: first char not '0' or '1'
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(0, 0, 0), "afoo"), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Error during initializing tool")
		}
		ASSERT_TRUE(error_caught)
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_groundobj_build_invalid_pos()
{
	// invalid pos
	{
		ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(-1, -1, 0), ""), null)
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_groundobj_build_random()
{
	{
		ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(4, 2, 0), null), null)
		local go = tile_x(4, 2, 0).find_object(mo_groundobj)
		ASSERT_TRUE(go != null)
		ASSERT_EQUAL(go.type, mo_groundobj)
		ASSERT_EQUAL(go.owner.nr, 16)
		ASSERT_EQUAL(go.is_removable(player_x(0)), null)
	}

	{
		ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(2, 4, 0), ""), null)
		local go = tile_x(2, 4, 0).find_object(mo_groundobj)
		ASSERT_TRUE(go != null)
		ASSERT_EQUAL(go.type, mo_groundobj)
		ASSERT_EQUAL(go.owner.nr, 16)
		ASSERT_EQUAL(go.is_removable(player_x(0)), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(2, 4, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 2, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_groundobj_build_specific()
{
	{
		ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(4, 2, 0), "0__See1"), null)
		local go = tile_x(4, 2, 0).find_object(mo_groundobj)
		ASSERT_TRUE(go != null)
		ASSERT_EQUAL(go.type, mo_groundobj)
		ASSERT_EQUAL(go.owner.nr, 16)
		ASSERT_EQUAL(go.is_removable(player_x(0)), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 2, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_groundobj_build_invalid_climate()
{
	{
		ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(4, 2, 0), "0__Hochsitz1"), "")
		ASSERT_EQUAL(tile_x(4, 2, 0).find_object(mo_groundobj), null)
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_groundobj_build_ignore_climate()
{
	{
		ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(4, 2, 0), "1__Hochsitz1"), null)
		local go = tile_x(4, 2, 0).find_object(mo_groundobj)
		ASSERT_TRUE(go != null)
		ASSERT_EQUAL(go.type, mo_groundobj)
		ASSERT_EQUAL(go.owner.nr, 16)
		ASSERT_EQUAL(go.is_removable(player_x(0)), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 2, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_groundobj_build_occupied()
{
	ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(4, 2, 0), "0__See1"), null)

	{
		ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(4, 2, 0), "1__Hochsitz1"), "")
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 2, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_groundobj_build_on_trees()
{
	ASSERT_EQUAL(command_x(tool_plant_tree).work(player_x(1), coord3d(4, 2, 0), "11,Ahorn-1"), null)

	// trees_on_top = 0 -> fail
	{
		ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(4, 2, 0), "1__See1"), "")
		ASSERT_EQUAL(tile_x(4, 2, 0).find_object(mo_groundobj), null)
	}

	// trees_on_top = 1 -> success
	{
		ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(4, 2, 0), "0__moles"), null)
		local go = tile_x(4, 2, 0).find_object(mo_groundobj)
		ASSERT_TRUE(go != null)
		ASSERT_EQUAL(go.type, mo_groundobj)
		ASSERT_EQUAL(go.owner.nr, 16)
		ASSERT_EQUAL(go.is_removable(player_x(0)), null)

		local tree = tile_x(4, 2, 0).find_object(mo_tree)
		ASSERT_TRUE(tree != null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(0), coord3d(4, 2, 0)), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_groundobj_build_on_slope()
{
	ASSERT_EQUAL(command_x.set_slope(player_x(0), coord3d(4, 2, 0), slope.south), null)

	{
		ASSERT_EQUAL(command_x(tool_build_groundobj).work(player_x(0), coord3d(4, 2, 0), "1__See1"), null)
		ASSERT_EQUAL(tile_x(4, 2, 0).find_object(mo_groundobj), null)
	}

	// clean up
	ASSERT_EQUAL(command_x.set_slope(player_x(0), coord3d(4, 2, 0), slope.flat), null)
	RESET_ALL_PLAYER_FUNDS()
}
