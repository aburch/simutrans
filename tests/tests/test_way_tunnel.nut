//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for building and removal of tunnels
//


function test_way_tunnel_build_straight()
{
	local digger = command_x(tool_build_tunnel)
	local raise = command_x(tool_raise_land)
	local lower = command_x(tool_lower_land)
	local remover = command_x(tool_remover)
	local default_tunnel = tunnel_desc_x.get_available_tunnels(wt_road)[0]
	local pl = player_x(0)

	ASSERT_TRUE(default_tunnel != null)

	{
		ASSERT_EQUAL(raise.work(pl, coord3d(3, 2, 0)), null)
		ASSERT_EQUAL(raise.work(pl, coord3d(4, 2, 0)), null)

		digger.set_flags(2)
		ASSERT_EQUAL(digger.work(pl, tile_x(3, 1, 0), default_tunnel.get_name()), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"...0....",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(digger.work(pl, tile_x(3, 2, 0), default_tunnel.get_name()), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"...0....",
				"...0....",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(remover.work(pl, coord3d(3, 1, 0)), null)
		ASSERT_EQUAL(remover.work(pl, coord3d(3, 2, 0)), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		digger.set_flags(0)
	}

	{
		ASSERT_EQUAL(raise.work(pl, coord3d(3, 3, 0)), null)
		ASSERT_EQUAL(raise.work(pl, coord3d(4, 3, 0)), null)

		ASSERT_EQUAL(digger.work(pl, tile_x(3, 1, 0), default_tunnel.get_name()), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"...4....",
				"...5....",
				"...1....",
				"........",
				"........",
				"........",
				"........"
			])
	}

	{
		ASSERT_EQUAL(digger.work(pl, tile_x(2, 2, 0), default_tunnel.get_name()), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"...4....",
				"..2D....",
				"...1....",
				"........",
				"........",
				"........",
				"........"
			])

		// building with ctrl
		digger.set_flags(2)
		ASSERT_EQUAL(digger.work(pl, tile_x(4, 2, 0), default_tunnel.get_name()), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"...4....",
				"..2D0...",
				"...1....",
				"........",
				"........",
				"........",
				"........"
			])

		digger.set_flags(0)

		// remove the single tunnel entrance
		ASSERT_EQUAL(remover.work(pl, coord3d(4, 2, 0)), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"...4....",
				"..2D....",
				"...1....",
				"........",
				"........",
				"........",
				"........"
			])

		// remove tunnel network (more than 2 entrances)
		// should fail without ctrl
		local err = remover.work(pl, coord3d(3, 3, 0))
		ASSERT_EQUAL(err, "This tunnel branches. You can try Control+Click to remove.")
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"...4....",
				"..2D....",
				"...1....",
				"........",
				"........",
				"........",
				"........"
			])

		remover.set_flags(2) // activate ctrl
		ASSERT_EQUAL(remover.work(pl, coord3d(3, 3, 0)), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		remover.set_flags(0) // deactivate ctrl
	}

	// clean up
	ASSERT_EQUAL(lower.work(pl, coord3d(3, 2, 0)), null)
	ASSERT_EQUAL(lower.work(pl, coord3d(3, 3, 0)), null)
	ASSERT_EQUAL(lower.work(pl, coord3d(4, 2, 0)), null)
	ASSERT_EQUAL(lower.work(pl, coord3d(4, 3, 0)), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_way_tunnel_build_up_down()
{
	local digger = command_x(tool_build_tunnel)
	local raise = command_x(tool_raise_land)
	local lower = command_x(tool_lower_land)
	local setslope = command_x.set_slope
	local remover = command_x(tool_remover)
	local default_tunnel = tunnel_desc_x.get_available_tunnels(wt_rail)[0]
	local pl = player_x(0)

	ASSERT_TRUE(default_tunnel != null)

	ASSERT_EQUAL(raise.work(pl, coord3d(1, 1, 0)), null)
	ASSERT_EQUAL(raise.work(pl, coord3d(2, 1, 0)), null)
	ASSERT_EQUAL(raise.work(pl, coord3d(1, 2, 0)), null)
	ASSERT_EQUAL(raise.work(pl, coord3d(2, 2, 0)), null)
	ASSERT_EQUAL(raise.work(pl, coord3d(1, 3, 0)), null)
	ASSERT_EQUAL(raise.work(pl, coord3d(2, 3, 0)), null)

	digger.set_flags(2) // ctrl
	ASSERT_EQUAL(digger.work(pl, coord3d(1, 0, 0), default_tunnel.get_name()), null)
	ASSERT_EQUAL(digger.work(pl, coord3d(1, 0, 0), coord3d(1, 1, 0), default_tunnel.get_name()), null)


	// invalid param
	{
		ASSERT_EQUAL(setslope(pl, coord3d(1, 1, 0), 42), "Only up and down movement in the underground!")

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0), // no change
			[
				".4......",
				".1......",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])
	}

	// Build up: Does not work: surface in the way
	{
		ASSERT_EQUAL(setslope(pl, coord3d(1, 1, 0), slope.all_up_slope), "Tile not empty.")
		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0), // no change
			[
				".4......",
				".1......",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])
	}

	// Build down
	{
		local old_maint = pl.get_current_maintenance()
		ASSERT_EQUAL(setslope(pl, coord3d(1, 1, 0), slope.all_down_slope), null)
		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				".4......",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, -1),
			[
				"........",
				".1......",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)
	}

	// try building duble slope down, rail does not support double slopes
	{
		local old_maint = pl.get_current_maintenance()
		ASSERT_EQUAL(setslope(pl, coord3d(1, 1, -1), slope.all_down_slope), "Tile not empty.")
		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				".4......",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, -1),
			[
				"........",
				".1......",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)
	}

	ASSERT_EQUAL(digger.work(pl, coord3d(1, 1, -1), coord3d(1, 2, -1), default_tunnel.get_name()), null)

	// Build up
	{
		local old_maint = pl.get_current_maintenance()

		ASSERT_EQUAL(setslope(pl, coord3d(1, 2, -1), slope.all_up_slope), null)

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				".4......",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, -1),
			[
				"........",
				".5......",
				".1......",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)
	}

	// try building double slope up, rail does not support double slopes
	{
		ASSERT_EQUAL(raise.work(pl, coord3d(1, 2, 1)), null)
		ASSERT_EQUAL(raise.work(pl, coord3d(2, 2, 1)), null)
		ASSERT_EQUAL(raise.work(pl, coord3d(1, 3, 1)), null)
		ASSERT_EQUAL(raise.work(pl, coord3d(2, 3, 1)), null)

		local old_maint = pl.get_current_maintenance()
		ASSERT_EQUAL(setslope(pl, coord3d(1, 1, 0), slope.all_up_slope), "")
		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				".4......",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, -1),
			[
				"........",
				".5......",
				".1......",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)

		ASSERT_EQUAL(lower.work(pl, coord3d(1, 2, 2)), null)
		ASSERT_EQUAL(lower.work(pl, coord3d(2, 2, 2)), null)
		ASSERT_EQUAL(lower.work(pl, coord3d(1, 3, 2)), null)
		ASSERT_EQUAL(lower.work(pl, coord3d(2, 3, 2)), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(1, 0, 0)), null)
	ASSERT_EQUAL(lower.work(pl, coord3d(1, 1, 1)), null)
	ASSERT_EQUAL(lower.work(pl, coord3d(2, 1, 1)), null)
	ASSERT_EQUAL(lower.work(pl, coord3d(1, 2, 1)), null)
	ASSERT_EQUAL(lower.work(pl, coord3d(2, 2, 1)), null)
	ASSERT_EQUAL(lower.work(pl, coord3d(1, 3, 1)), null)
	ASSERT_EQUAL(lower.work(pl, coord3d(2, 3, 1)), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_way_tunnel_make_public()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local tunnel_desc = tunnel_desc_x.get_available_tunnels(wt_road)[0]
	local makepublic  = command_x(tool_make_stop_public)
	local raise       = command_x(tool_raise_land)
	local lower       = command_x(tool_lower_land)

	ASSERT_EQUAL(raise.work(public_pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(raise.work(public_pl, coord3d(5, 3, 0)), null)
	ASSERT_EQUAL(raise.work(public_pl, coord3d(4, 4, 0)), null)
	ASSERT_EQUAL(raise.work(public_pl, coord3d(5, 4, 0)), null)

	ASSERT_EQUAL(command_x(tool_build_tunnel).work(pl, coord3d(4, 2, 0), tunnel_desc.get_name()), null)

	// make tunnel portal public
	{
		local old_pl_cash = pl.get_current_cash()
		local old_pl_maint = pl.get_current_maintenance()
		local old_public_cash = public_pl.get_current_cash()
		local old_public_maint = public_pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(pl, coord3d(4, 2, 0)), null)

		ASSERT_TRUE(way_x(4, 2, 0).get_owner() != null)
		ASSERT_EQUAL(way_x(4, 2, 0).get_owner().get_name(), public_pl.get_name())

		ASSERT_EQUAL(pl.get_current_cash()*100, old_pl_cash*100 - 60 * tunnel_desc.get_maintenance()) // 60 == cst_make_public_months
		ASSERT_EQUAL(pl.get_current_maintenance(), old_pl_maint - tunnel_desc.get_maintenance())

		ASSERT_EQUAL(public_pl.get_current_maintenance(), old_public_maint + tunnel_desc.get_maintenance())
		ASSERT_EQUAL(public_pl.get_current_cash(), old_public_cash)
	}

	// make tunnel inside public
	{
		local old_pl_cash = pl.get_current_cash()
		local old_pl_maint = pl.get_current_maintenance()
		local old_public_cash = public_pl.get_current_cash()
		local old_public_maint = public_pl.get_current_maintenance()

		ASSERT_EQUAL(makepublic.work(pl, coord3d(4, 3, 0)), null)

		ASSERT_TRUE(way_x(4, 3, 0).get_owner() != null)
		ASSERT_EQUAL(way_x(4, 3, 0).get_owner().get_name(), public_pl.get_name())

		ASSERT_EQUAL(pl.get_current_cash()*100, old_pl_cash*100 - 60 * tunnel_desc.get_maintenance()) // 60 == cst_make_public_months
		ASSERT_EQUAL(pl.get_current_maintenance(), old_pl_maint - tunnel_desc.get_maintenance())

		ASSERT_EQUAL(public_pl.get_current_maintenance(), old_public_maint + tunnel_desc.get_maintenance())
		ASSERT_EQUAL(public_pl.get_current_cash(), old_public_cash)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remove_way).work(public_pl, coord3d(4, 2, 0), coord3d(4, 4, 0), "" + wt_road), null)

	ASSERT_EQUAL(lower.work(public_pl, coord3d(4, 3, 1)), null)
	ASSERT_EQUAL(lower.work(public_pl, coord3d(5, 3, 1)), null)
	ASSERT_EQUAL(lower.work(public_pl, coord3d(4, 4, 1)), null)
	ASSERT_EQUAL(lower.work(public_pl, coord3d(5, 4, 1)), null)
	RESET_ALL_PLAYER_FUNDS()
}
