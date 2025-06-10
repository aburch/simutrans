//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for wayobj
//


function test_wayobj_build_straight()
{
	local pl = player_x(0)
	local remover = command_x(tool_remove_wayobj)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local overhead_line = wayobj_desc_x.get_available_wayobjs(wt_rail).filter(@(idx, wobj) wobj.is_overhead_line())[0]
	ASSERT_TRUE(overhead_line != null)

	{
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(2, 1, 0), coord3d(2, 1, 0), overhead_line), "")
		ASSERT_EQUAL(tile_x(2, 1, 0).find_object(mo_wayobj), null)

		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(3, 1, 0), coord3d(2, 1, 0), overhead_line), "")
		ASSERT_EQUAL(tile_x(3, 1, 0).find_object(mo_wayobj), null)
		ASSERT_EQUAL(tile_x(2, 1, 0).find_object(mo_wayobj), null)
	}

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 1, 0), coord3d(5, 1, 0), rail, true), null)

	// now build wayobj
	{
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(2, 1, 0), coord3d(2, 1, 0), overhead_line), null)
		local wo = tile_x(2, 1, 0).find_object(mo_wayobj)
		ASSERT_TRUE(wo != null)
		ASSERT_TRUE(wo.is_valid())
	}

	{
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(3, 1, 0), coord3d(4, 1, 0), overhead_line), null)
		local wo = tile_x(3, 1, 0).find_object(mo_wayobj)
		ASSERT_TRUE(wo != null)
		ASSERT_TRUE(wo.is_valid())
		wo = tile_x(4, 1, 0).find_object(mo_wayobj)
		ASSERT_TRUE(wo != null)
		ASSERT_TRUE(wo.is_valid())

		// todo: check for wayobj ribis ?
	}

	{
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(2, 1, 0), coord3d(4, 1, 0), overhead_line), null)
		local wo = tile_x(3, 1, 0).find_object(mo_wayobj)
		ASSERT_TRUE(wo != null)
		ASSERT_TRUE(wo.is_valid())
	}


	// remove wayobj
	{
		ASSERT_EQUAL(remover.work(pl, coord3d(2, 1, 0), coord3d(2, 2, 0), "" + wt_rail), "")
		ASSERT_EQUAL(remover.work(pl, coord3d(2, 1, 0), coord3d(3, 1, 0), "" + wt_rail), null)
		ASSERT_EQUAL(remover.work(pl, coord3d(2, 1, 0), coord3d(3, 1, 0), "" + wt_rail), null) // returns success even if nothing was removed
		ASSERT_EQUAL(remover.work(pl, coord3d(4, 1, 0), coord3d(4, 1, 0), "" + wt_rail), null)
		ASSERT_EQUAL(remover.work(pl, coord3d(4, 1, 0), coord3d(4, 1, 0), "" + wt_rail), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 1, 0), coord3d(5, 1, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_wayobj_build_disconnected()
{
	local pl = player_x(0)
	local remover = command_x(tool_remove_wayobj)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local overhead_line = wayobj_desc_x.get_available_wayobjs(wt_rail).filter(@(idx, wobj) wobj.is_overhead_line())[0]
	ASSERT_TRUE(overhead_line != null)

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 1, 0), coord3d(5, 1, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 5, 0), coord3d(5, 5, 0), rail, true), null)

	// build disconnected
	{
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(3, 5, 0), coord3d(3, 1, 0), overhead_line), "Ways not connected")
		ASSERT_EQUAL(tile_x(3, 5, 0).find_object(mo_wayobj), null)
		ASSERT_EQUAL(tile_x(3, 1, 0).find_object(mo_wayobj), null)
	}

	// remove disconnected wayobj
	{
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(2, 1, 0), coord3d(5, 1, 0), overhead_line), null)
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(2, 5, 0), coord3d(5, 5, 0), overhead_line), null)

		ASSERT_EQUAL(remover.work(pl, coord3d(3, 1, 0), coord3d(3, 5, 0), "" + wt_rail), "Ways not connected")
		ASSERT_TRUE(tile_x(3, 1, 0).find_object(mo_wayobj) != null)
		ASSERT_TRUE(tile_x(3, 5, 0).find_object(mo_wayobj) != null)
	}

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 1, 0), coord3d(2, 5, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 1, 0), coord3d(5, 5, 0), rail, true), null)

	{
		ASSERT_EQUAL(remover.work(pl, coord3d(3, 1, 0), coord3d(3, 5, 0), "" + wt_rail), null)
		ASSERT_TRUE(tile_x(3, 1, 0).find_object(mo_wayobj) == null)
		ASSERT_TRUE(tile_x(3, 5, 0).find_object(mo_wayobj) == null)
		ASSERT_TRUE(tile_x(4, 1, 0).find_object(mo_wayobj) != null)
		ASSERT_TRUE(tile_x(4, 5, 0).find_object(mo_wayobj) != null)
	}

	// clean up
	ASSERT_EQUAL(remover.work(pl, coord3d(4, 1, 0), coord3d(4, 5, 0), "" + wt_rail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 1, 0), coord3d(5, 5, 0), "" + wt_rail), null)
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(2, 1, 0), coord3d(5, 5, 0), "" + wt_rail), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_wayobj_upgrade_downgrade()
{
	local pl = player_x(0)
	local wobj_remover = command_x(tool_remove_wayobj)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local catenaries = wayobj_desc_x.get_available_wayobjs(wt_rail).filter(@(idx, wobj) wobj.is_overhead_line())
	catenaries.sort(@(a, b) a.get_topspeed() <=> b.get_topspeed())

	// FIXME need at least 2 catenaries
	ASSERT_TRUE(catenaries.len() >= 2)
	local slow_cat = catenaries[0]
	local fast_cat = catenaries[1]

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), rail, true), null)

	// upgrade wayobj
	{
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), slow_cat), null)
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), fast_cat), null)

		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_wayobj) != null)
		ASSERT_TRUE(tile_x(4, 4, 0).find_object(mo_wayobj) != null)
		ASSERT_TRUE(tile_x(5, 4, 0).find_object(mo_wayobj) != null)

		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))
		ASSERT_TRUE(tile_x(4, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))
		ASSERT_TRUE(tile_x(5, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))

		ASSERT_EQUAL(wobj_remover.work(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), "" + wt_rail), null)
	}

	// upgrade wayobj to same type, should incur no cost
	{
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), slow_cat), null)
		local old_cash = pl.get_current_cash()
		local old_maint = pl.get_current_maintenance()
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), slow_cat), null)
		ASSERT_EQUAL(pl.get_current_cash(), old_cash)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)

		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_wayobj) != null)
		ASSERT_TRUE(tile_x(4, 4, 0).find_object(mo_wayobj) != null)
		ASSERT_TRUE(tile_x(5, 4, 0).find_object(mo_wayobj) != null)

		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_wayobj).get_desc().is_equal(slow_cat))
		ASSERT_TRUE(tile_x(4, 4, 0).find_object(mo_wayobj).get_desc().is_equal(slow_cat))
		ASSERT_TRUE(tile_x(5, 4, 0).find_object(mo_wayobj).get_desc().is_equal(slow_cat))

		ASSERT_EQUAL(wobj_remover.work(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), "" + wt_rail), null)
	}

	// upgrade wayobj to faster type
	{
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), slow_cat), null)
		local old_cash = pl.get_current_cash()
		local old_maint = pl.get_current_maintenance()
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), fast_cat), null)
		ASSERT_TRUE(pl.get_current_cash() < old_cash)
		ASSERT_TRUE(pl.get_current_maintenance() > old_maint)

		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_wayobj) != null)
		ASSERT_TRUE(tile_x(4, 4, 0).find_object(mo_wayobj) != null)
		ASSERT_TRUE(tile_x(5, 4, 0).find_object(mo_wayobj) != null)

		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))
		ASSERT_TRUE(tile_x(4, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))
		ASSERT_TRUE(tile_x(5, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))

		ASSERT_EQUAL(wobj_remover.work(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), "" + wt_rail), null)
	}

	// downgrade wayobj without ctrl, should fail
	{
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), fast_cat), null)
		local old_cash = pl.get_current_cash()
		local old_maint = pl.get_current_maintenance()
		ASSERT_EQUAL(command_x.build_wayobj(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), slow_cat), null)
		ASSERT_EQUAL(pl.get_current_cash(), old_cash)
		ASSERT_EQUAL(pl.get_current_maintenance(), old_maint)

		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_wayobj) != null)
		ASSERT_TRUE(tile_x(4, 4, 0).find_object(mo_wayobj) != null)
		ASSERT_TRUE(tile_x(5, 4, 0).find_object(mo_wayobj) != null)

		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))
		ASSERT_TRUE(tile_x(4, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))
		ASSERT_TRUE(tile_x(5, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))

		ASSERT_EQUAL(wobj_remover.work(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), "" + wt_rail), null)
	}

	// downgrade wayobj with ctrl, should succeed
	{
		local builder = command_x(tool_build_wayobj)
		builder.set_flags(2) // enable ctrl
		ASSERT_EQUAL(builder.work(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), fast_cat.get_name()), null)
		ASSERT_EQUAL(builder.work(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), slow_cat.get_name()), null)

		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_wayobj) != null)
		ASSERT_TRUE(tile_x(4, 4, 0).find_object(mo_wayobj) != null)
		ASSERT_TRUE(tile_x(5, 4, 0).find_object(mo_wayobj) != null)

		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))
		ASSERT_TRUE(tile_x(4, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))
		ASSERT_TRUE(tile_x(5, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))

		ASSERT_EQUAL(wobj_remover.work(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), "" + wt_rail), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_wayobj_upgrade_change_owner()
{
	local pl = player_x(0)
	local public_pl = player_x(1)

	local wobj_remover = command_x(tool_remove_wayobj)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local catenaries = wayobj_desc_x.get_available_wayobjs(wt_rail).filter(@(idx, wobj) wobj.is_overhead_line())
	catenaries.sort(@(a, b) a.get_topspeed() <=> b.get_topspeed())

	// FIXME need at least 2 catenaries
	ASSERT_TRUE(catenaries.len() >= 1)
	local slow_cat = catenaries[0]
	local fast_cat = catenaries[1]

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), rail, true), null)

	// public player: replace catenary of normal player
	{
		local builder = command_x(tool_build_wayobj)

		ASSERT_EQUAL(command_x.build_wayobj(pl,        coord3d(3, 4, 0), coord3d(5, 4, 0), slow_cat), null)
		ASSERT_EQUAL(command_x.build_wayobj(public_pl, coord3d(3, 4, 0), coord3d(5, 4, 0), fast_cat), null)

		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))
		ASSERT_EQUAL(tile_x(3, 4, 0).find_object(mo_wayobj).get_owner().get_name(), public_pl.get_name())

		ASSERT_EQUAL(wobj_remover.work(public_pl, coord3d(3, 4, 0), coord3d(5, 4, 0), "" + wt_rail), null)
	}

	// normal player: replace caterary of public player
	{
		local builder = command_x(tool_build_wayobj)

		ASSERT_EQUAL(command_x.build_wayobj(public_pl,  coord3d(3, 4, 0), coord3d(5, 4, 0), slow_cat), null)
		ASSERT_EQUAL(command_x.build_wayobj(pl,         coord3d(3, 4, 0), coord3d(5, 4, 0), fast_cat), null)

		ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_wayobj).get_desc().is_equal(fast_cat))
		ASSERT_EQUAL(tile_x(3, 4, 0).find_object(mo_wayobj).get_owner().get_name(), pl.get_name())

		ASSERT_EQUAL(wobj_remover.work(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), "" + wt_rail), null)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_wayobj_electrify_depot()
{
	local pl = player_x(0)
	local start_pos = coord3d(3, 4, 0)
	local end_pos = coord3d(5, 4, 0)
	local wayremover = command_x(tool_remove_way)
	local wobj_remover = command_x(tool_remove_wayobj)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local catenaries = wayobj_desc_x.get_available_wayobjs(wt_rail).filter(@(idx, wobj) wobj.is_overhead_line())
	catenaries.sort(@(a, b) a.get_topspeed() <=> b.get_topspeed())

	local slow_cat = catenaries[0]
	local fast_cat = catenaries[1]
	local depot = get_depot_by_wt(wt_rail)

	ASSERT_TRUE(slow_cat != null)
	ASSERT_TRUE(fast_cat != null)
	ASSERT_TRUE(depot != null)

	ASSERT_EQUAL(command_x.build_way(pl, start_pos, end_pos, rail, true), null)
	ASSERT_EQUAL(command_x.build_depot(pl, start_pos, depot), null)
	ASSERT_EQUAL(command_x.build_depot(pl, end_pos, depot), null)

	{
		ASSERT_EQUAL(command_x.build_wayobj(pl, start_pos, end_pos, slow_cat), null)
		ASSERT_TRUE(way_x(start_pos.x, start_pos.y, start_pos.z).is_electrified())
		ASSERT_TRUE(way_x(end_pos.x, end_pos.y, end_pos.z).is_electrified())

		// TODO check that depot is really electrified
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(pl, start_pos, "" + mo_depot_rail), null)
	ASSERT_EQUAL(command_x(tool_remover).work(pl, end_pos, "" + mo_depot_rail), null)
	ASSERT_EQUAL(wayremover.work(pl, start_pos, end_pos, "" + wt_rail), null) // also removes way with wayobj

	RESET_ALL_PLAYER_FUNDS()
}
