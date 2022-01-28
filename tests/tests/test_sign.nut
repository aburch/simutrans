//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for signs/signals
//


function test_sign_build_oneway()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local wayremover = command_x(tool_remove_way)
	local remover = command_x(tool_remover)
	local road = way_desc_x("dirt_road")
	local rail = way_desc_x("sand_track")
	local sign = sign_desc_x.get_available_signs(wt_road).filter(@(idx, sign) sign.is_one_way())[0]

	ASSERT_TRUE(road != null)
	ASSERT_TRUE(sign != null)
	ASSERT_TRUE(sign.is_one_way())

	// on ground without way
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Tool has no effects")
		}
		ASSERT_TRUE(error_caught)
	}

	// in the air
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 1), sign), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Tool has no effects")
		}
		ASSERT_TRUE(error_caught)
	}

	// build way
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 1, 0), coord3d(2, 4, 0), road, true), null)

	{
		ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), null)
		ASSERT_EQUAL(tile_x(2, 3, 0).find_object(mo_signal), null)

		local s = tile_x(2, 3, 0).find_object(mo_roadsign)
		ASSERT_TRUE(s != null)
		ASSERT_TRUE(s.is_valid())
		ASSERT_TRUE(s.can_pass(pl))
		ASSERT_TRUE(s.can_pass(public_pl))

		local w = tile_x(2, 3, 0).find_object(mo_way)
		ASSERT_TRUE(w != null)
		ASSERT_EQUAL(w.get_dirs(), dir.northsouth)
		ASSERT_EQUAL(w.get_dirs_masked(), dir.south)

		// change direction of sign
		ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), null)
		ASSERT_TRUE(s.is_valid())
		ASSERT_TRUE(s.can_pass(pl))
		ASSERT_TRUE(s.can_pass(public_pl))

		ASSERT_EQUAL(w.get_dirs(), dir.northsouth)
		ASSERT_EQUAL(w.get_dirs_masked(), dir.north)

		// change direction again, should have original direction
		ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), null)
		ASSERT_TRUE(s.is_valid())
		ASSERT_TRUE(s.can_pass(pl))
		ASSERT_TRUE(s.can_pass(public_pl))

		ASSERT_EQUAL(w.get_dirs(), dir.northsouth)
		ASSERT_EQUAL(w.get_dirs_masked(), dir.south)
	}

	// build road/rail crossing over sign, should succeed if crossing is possible without sign
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 3, 0), coord3d(3, 3, 0), rail, true), null)

		ASSERT_WAY_PATTERN_MASKED(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"..4.....",
				"..5.....",
				"..4.....",
				"..1.....",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN_MASKED(wt_rail, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				".2A8....",
				"........",
				"........",
				"........",
				"........"
			])

		local w = tile_x(2, 3, 0).find_object(mo_way)
		ASSERT_TRUE(w.is_crossing())

		// change direction of sign on rail-road crossing, should succeed
		ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), null)

		ASSERT_WAY_PATTERN_MASKED(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"..4.....",
				"..5.....",
				"..1.....",
				"..1.....",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN_MASKED(wt_rail, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				".2A8....",
				"........",
				"........",
				"........",
				"........"
			])

		// and remove rail
		ASSERT_EQUAL(wayremover.work(pl, coord3d(1, 3, 0), coord3d(3, 3, 0), "" + wt_rail), null)
	}

	// build road/road crossing over sign, should succeed
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 3, 0), coord3d(3, 3, 0), road, true), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"..4.....",
				"..5.....",
				".2F8....",
				"..1.....",
				"........",
				"........",
				"........"
			])

		local w = tile_x(2, 3, 0).find_object(mo_way)
		ASSERT_FALSE(w.is_crossing())
		ASSERT_WAY_PATTERN_MASKED(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"..4.....",
				"..5.....",
				".2B8....",
				"..1.....",
				"........",
				"........",
				"........"
			])

		// change direction of sign on road crossing, should fail
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), null)
		}
		catch (e) {
			ASSERT_EQUAL(e, "Tool has no effects")
			error_caught = true
		}
		ASSERT_TRUE(error_caught)
	}

	// remove sign, try to build again (should fail because of crossing)
	{
		ASSERT_EQUAL(remover.work(pl, coord3d(2, 3, 0)), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"..4.....",
				"..5.....",
				".2F8....",
				"..1.....",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(tile_x(2, 3, 0).find_object(mo_roadsign), null)

		local error_caught = false
		try {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), null)
		}
		catch (e) {
			ASSERT_EQUAL(e, "Tool has no effects")
			error_caught = true
		}
		ASSERT_TRUE(error_caught)
	}

	ASSERT_EQUAL(wayremover.work(pl, coord3d(1, 3, 0), coord3d(3, 3, 0), "" + wt_road), null)

	// Cannot remove signs of other players (except public player)
	{
		ASSERT_EQUAL(command_x.build_sign_at(public_pl, coord3d(2, 2, 0), sign), null)
		ASSERT_EQUAL(command_x.build_sign_at(pl,        coord3d(2, 3, 0), sign), null)

		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(2, 2, 0)), "Der Besitzer erlaubt das Entfernen nicht")
		ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(2, 2, 0)), null)
		ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(2, 3, 0)), null)
	}

	// remove stuff
	ASSERT_EQUAL(wayremover.work(pl, coord3d(2, 1, 0), coord3d(2, 4, 0), "" + wt_road), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_sign_build_trafficlight()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local wayremover = command_x(tool_remove_way)
	local remover = command_x(tool_remover)
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local trafficlight = sign_desc_x.get_available_signs(wt_road).filter(@(idx, sign) sign.is_traffic_light())[0]

	ASSERT_TRUE(trafficlight != null)

	// on ground
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), trafficlight), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Tool has no effects")
		}
		ASSERT_TRUE(error_caught)
	}

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 1, 0), coord3d(2, 4, 0), road, true), null)

	// end of way
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 1, 0), trafficlight), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Tool has no effects")
		}
		ASSERT_TRUE(error_caught)
	}

	// 2-way
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), trafficlight), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Tool has no effects")
		}
		ASSERT_TRUE(error_caught)
	}

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 3, 0), coord3d(2, 3, 0), road, true), null)

	// 3-way
	{
		ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), trafficlight), null)

		local tl = tile_x(2, 3, 0).find_object(mo_roadsign)
		ASSERT_TRUE(tl != null)
		ASSERT_TRUE(tl.can_pass(pl))
		ASSERT_TRUE(tl.can_pass(public_pl))
		ASSERT_TRUE(tl.get_desc().is_traffic_light())

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"..4.....",
				"..5.....",
				".2D.....",
				"..1.....",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(tile_x(2, 3, 0).get_way_dirs_masked(wt_road), dir.northsouthwest)
	}


	// Make 4-way from 3-way
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 3, 0), coord3d(3, 3, 0), road, true), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"..4.....",
				"..5.....",
				".2F8....",
				"..1.....",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(tile_x(2, 3, 0).get_way_dirs_masked(wt_road), dir.all)
	}

	// remove traffic light on crossing via wayremover
	{
		ASSERT_EQUAL(wayremover.work(pl, coord3d(2, 1, 0), coord3d(2, 4, 0), "" + wt_road), null)
		ASSERT_EQUAL(tile_x(2, 3, 0).find_object(mo_roadsign), null)
	}

	ASSERT_EQUAL(wayremover.work(pl, coord3d(1, 3, 0), coord3d(3, 3, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_sign_remove_trafficlight()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local wayremover = command_x(tool_remove_way)
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local trafficlight = sign_desc_x.get_available_signs(wt_road).filter(@(idx, sign) sign.is_traffic_light())[0]

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 1, 0), coord3d(2, 3, 0), road, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 2, 0), coord3d(3, 2, 0), road, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(5, 4, 0), coord3d(5, 6, 0), road, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 5, 0), coord3d(6, 5, 0), road, true), null)

	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 2, 0), trafficlight), null)
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(5, 5, 0), trafficlight), null)

	// Note that both traffic lights must have the same direction for the test to work
	// So the second traffic light must not change direction between the two wayremover calls
	{
		ASSERT_EQUAL(wayremover.work(pl, coord3d(2, 1, 0), coord3d(2, 2, 0), "" + wt_road), null)
		ASSERT_EQUAL(wayremover.work(pl, coord3d(4, 5, 0), coord3d(5, 5, 0), "" + wt_road), null)

		ASSERT_EQUAL(tile_x(2, 2, 0).find_object(mo_signal), null)
		ASSERT_EQUAL(tile_x(5, 5, 0).find_object(mo_signal), null)
	}

	ASSERT_EQUAL(wayremover.work(pl, coord3d(1, 2, 0), coord3d(3, 2, 0), "" + wt_road), null)
	ASSERT_EQUAL(wayremover.work(pl, coord3d(5, 4, 0), coord3d(5, 6, 0), "" + wt_road), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_sign_build_private_way()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local wayremover = command_x(tool_remove_way)
	local remover = command_x(tool_remover)

	local road = way_desc_x("dirt_road")
	local rail = way_desc_x("sand_track")
	local sign = sign_desc_x.get_available_signs(wt_road).filter(@(idx, sign) sign.is_private_way())[0]

	ASSERT_TRUE(road != null)
	ASSERT_TRUE(sign != null)
	ASSERT_TRUE(sign.is_private_way())

	// on ground
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Tool has no effects")
		}
		ASSERT_TRUE(error_caught)
	}

	// in the air
	{
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 1), sign), "")
		}
		catch (e) {
			error_caught = true
			ASSERT_EQUAL(e, "Tool has no effects")
		}
		ASSERT_TRUE(error_caught)
	}

	// build way
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 1, 0), coord3d(2, 4, 0), road, true), null)

	{
		ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), null)
		ASSERT_EQUAL(tile_x(2, 3, 0).find_object(mo_signal), null)

		local s = tile_x(2, 3, 0).find_object(mo_roadsign)
		ASSERT_TRUE(s != null)
		ASSERT_TRUE(s.is_valid())
		ASSERT_TRUE(s.can_pass(pl))
		ASSERT_FALSE(s.can_pass(public_pl))

		local w = tile_x(2, 3, 0).find_object(mo_way)
		ASSERT_TRUE(w != null)
		ASSERT_EQUAL(w.get_dirs(), dir.northsouth)
		ASSERT_EQUAL(w.get_dirs_masked(), dir.northsouth)

		// change direction of sign
		ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), null)
		ASSERT_TRUE(s.is_valid())
		ASSERT_TRUE(s.can_pass(pl))
		ASSERT_FALSE(s.can_pass(public_pl))

		ASSERT_EQUAL(w.get_dirs(), dir.northsouth)
		ASSERT_EQUAL(w.get_dirs_masked(), dir.northsouth)

		// change direction again, should have original direction
		ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), null)
		ASSERT_TRUE(s.is_valid())
		ASSERT_TRUE(s.can_pass(pl))
		ASSERT_FALSE(s.can_pass(public_pl))

		ASSERT_EQUAL(w.get_dirs(), dir.northsouth)
		ASSERT_EQUAL(w.get_dirs_masked(), dir.northsouth)
	}

	// build road/rail crossing over sign, should succeed if crossing is possible without sign
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 3, 0), coord3d(3, 3, 0), rail, true), null)
		ASSERT_WAY_PATTERN_MASKED(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"..4.....",
				"..5.....",
				"..5.....",
				"..1.....",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN_MASKED(wt_rail, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"........",
				".2A8....",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_TRUE(tile_x(2, 3, 0).find_object(mo_way).is_crossing())

		// and remove rail
		ASSERT_EQUAL(wayremover.work(pl, coord3d(1, 3, 0), coord3d(3, 3, 0), "" + wt_rail), null)
	}

	// build road/road crossing over sign, should succeed
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 3, 0), coord3d(3, 3, 0), road, true), null)

		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"..4.....",
				"..5.....",
				".2F8....",
				"..1.....",
				"........",
				"........",
				"........"
			])

		local w = tile_x(2, 3, 0).find_object(mo_way)
		ASSERT_FALSE(w.is_crossing())
		ASSERT_EQUAL(w.get_dirs_masked(), dir.all)

		// change direction of sign on road crossing, should fail
		local error_caught = false
		try {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), null)
		}
		catch (e) {
			ASSERT_EQUAL(e, "Tool has no effects")
			error_caught = true
		}
		ASSERT_TRUE(error_caught)
	}

	// remove sign, try to build again (should fail because of crossing)
	{
		ASSERT_EQUAL(remover.work(pl, coord3d(2, 3, 0)), null)
		ASSERT_WAY_PATTERN(wt_road, coord3d(0, 0, 0),
			[
				"........",
				"..4.....",
				"..5.....",
				".2F8....",
				"..1.....",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(tile_x(2, 3, 0).find_object(mo_roadsign), null)

		local error_caught = false
		try {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), sign), null)
		}
		catch (e) {
			ASSERT_EQUAL(e, "Tool has no effects")
			error_caught = true
		}
		ASSERT_TRUE(error_caught)
	}

	// remove stuff
	ASSERT_EQUAL(wayremover.work(pl, coord3d(1, 3, 0), coord3d(3, 3, 0), "" + wt_road), null)
	ASSERT_EQUAL(wayremover.work(pl, coord3d(2, 1, 0), coord3d(2, 4, 0), "" + wt_road), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_sign_build_signal()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local wayremover = command_x(tool_remove_way)
	local remover = command_x(tool_remover)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local signal     = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, sign) sign.is_signal())[0]
	local presignal  = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, sign) sign.is_pre_signal())[0]
	local priosignal = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, sign) sign.is_priority_signal())[0]
	local lbsignal   = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, sign) sign.is_longblock_signal())[0]
	local chsignal   = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, sign) sign.is_choose_sign())[0]
	local eocsignal  = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, sign) sign.is_end_choose_signal())[0]

	local all_signals = [ signal, presignal, priosignal, lbsignal, chsignal, eocsignal ]

	foreach (s in all_signals) {
		ASSERT_TRUE(s != null)
	}

	{
		// Build signs on flat ground, should fail
		foreach (s in all_signals) {
			local error_caught = false
			try {
				ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), s), "")
			}
			catch (e) {
				error_caught = true
				ASSERT_EQUAL(e, "Tool has no effects")
			}
			ASSERT_TRUE(error_caught)
			ASSERT_EQUAL(tile_x(2, 3, 0).find_object(mo_signal), null)
		}
	}

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 0, 0), coord3d(2, 7, 0), road, true), null)

	{
		// Rail signals on road, should fail
		foreach (s in all_signals) {
			local error_caught = false
			try {
				ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(2, 3, 0), s), "")
			}
			catch (e) {
				error_caught = true
				ASSERT_EQUAL(e, "Tool has no effects")
			}
			ASSERT_TRUE(error_caught)
			ASSERT_EQUAL(tile_x(2, 3, 0).find_object(mo_signal), null)
		}
	}

	ASSERT_EQUAL(wayremover.work(pl, coord3d(2, 0, 0), coord3d(2, 7, 0), "" + wt_road), null)


	// build signals
	{
		foreach (i, s in all_signals) {
			ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, i, 0), coord3d(2, i, 0), rail, true), null)
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(1, i, 0), s), null)

			if (s == eocsignal) {
				ASSERT_TRUE(tile_x(1, i, 0).find_object(mo_roadsign) != null) // for end-of-choose
			}
			else {
				ASSERT_TRUE(tile_x(1, i, 0).find_object(mo_signal) != null)
			}
		}

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN_MASKED(wt_rail, coord3d(0, 0, 0),
			[
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"........",
				"........"
			])
	}

	// make signals directional
	{
		foreach (i, s in all_signals) {
			ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, i, 0), coord3d(2, i, 0), rail, true), null)
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(1, i, 0), s), null)

			if (s == eocsignal) {
				ASSERT_TRUE(tile_x(1, i, 0).find_object(mo_roadsign) != null) // for end-of-choose
			}
			else {
				ASSERT_TRUE(tile_x(1, i, 0).find_object(mo_signal) != null)
			}
		}

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN_MASKED(wt_rail, coord3d(0, 0, 0),
			[
				"288.....",
				"288.....",
				"288.....",
				"288.....",
				"288.....",
				"2A8.....",
				"........",
				"........"
			])
	}

	// signals into different direction
	{
		foreach (i, s in all_signals) {
			ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, i, 0), coord3d(2, i, 0), rail, true), null)
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(1, i, 0), s), null)

			if (s == eocsignal) {
				ASSERT_TRUE(tile_x(1, i, 0).find_object(mo_roadsign) != null) // for end-of-choose
			}
			else {
				ASSERT_TRUE(tile_x(1, i, 0).find_object(mo_signal) != null)
			}
		}

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"2A8.....",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN_MASKED(wt_rail, coord3d(0, 0, 0),
			[
				"228.....",
				"228.....",
				"228.....",
				"228.....",
				"228.....",
				"2A8.....",
				"........",
				"........"
			])
	}

	// build way across signal
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 0, 0), coord3d(1, all_signals.len()-1, 0), rail, true), null)

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"2E8.....",
				"2F8.....",
				"2F8.....",
				"2F8.....",
				"2F8.....",
				"2B8.....",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN_MASKED(wt_rail, coord3d(0, 0, 0),
			[
				"268.....",
				"278.....",
				"278.....",
				"278.....",
				"278.....",
				"2B8.....",
				"........",
				"........"
			])
	}

	// remove everything
	{
		foreach (i, s in all_signals) {
			ASSERT_EQUAL(wayremover.work(pl, coord3d(0, i, 0), coord3d(2, i, 0), "" + wt_rail), null)
		}

		ASSERT_EQUAL(wayremover.work(pl, coord3d(1, 0, 0), coord3d(1, all_signals.len()-1, 0), "" + wt_rail), null)
	}

	// build signals, dir 2
	{
		foreach (i, s in all_signals) {
			ASSERT_EQUAL(command_x.build_way(pl, coord3d(i, 0, 0), coord3d(i, 2, 0), rail, true), null)
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(i, 1, 0), s), null)

			if (s == eocsignal) {
				ASSERT_TRUE(tile_x(i, 1, 0).find_object(mo_roadsign) != null) // for end-of-choose
			}
			else {
				ASSERT_TRUE(tile_x(i, 1, 0).find_object(mo_signal) != null)
			}
		}

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"444444..",
				"555555..",
				"111111..",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN_MASKED(wt_rail, coord3d(0, 0, 0),
			[
				"444444..",
				"555555..",
				"111111..",
				"........",
				"........",
				"........",
				"........",
				"........"
			])
	}

	// make signals directional, dir 2
	{
		foreach (i, s in all_signals) {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(i, 1, 0), s), null)

			if (s == eocsignal) {
				ASSERT_TRUE(tile_x(i, 1, 0).find_object(mo_roadsign) != null) // for end-of-choose
			}
			else {
				ASSERT_TRUE(tile_x(i, 1, 0).find_object(mo_signal) != null)
			}
		}

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"444444..",
				"555555..",
				"111111..",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN_MASKED(wt_rail, coord3d(0, 0, 0),
			[
				"444444..",
				"444445..",
				"111111..",
				"........",
				"........",
				"........",
				"........",
				"........"
			])
	}

	// signals into different direction, dir 2
	{
		foreach (i, s in all_signals) {
			ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(i, 1, 0), s), null)

			if (s == eocsignal) {
				ASSERT_TRUE(tile_x(i, 1, 0).find_object(mo_roadsign) != null) // for end-of-choose
			}
			else {
				ASSERT_TRUE(tile_x(i, 1, 0).find_object(mo_signal) != null)
			}
		}

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"444444..",
				"555555..",
				"111111..",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN_MASKED(wt_rail, coord3d(0, 0, 0),
			[
				"444444..",
				"111115..",
				"111111..",
				"........",
				"........",
				"........",
				"........",
				"........"
			])
	}

	// build way across signal
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, 1, 0), coord3d(all_signals.len()-1, 1, 0), rail, true), null)

		ASSERT_WAY_PATTERN(wt_rail, coord3d(0, 0, 0),
			[
				"444444..",
				"7FFFFD..",
				"111111..",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_WAY_PATTERN_MASKED(wt_rail, coord3d(0, 0, 0),
			[
				"444444..",
				"3BBBBD..",
				"111111..",
				"........",
				"........",
				"........",
				"........",
				"........"
			])
	}

	// remove everything again
	{
		foreach (i, s in all_signals) {
			ASSERT_EQUAL(wayremover.work(pl, coord3d(i, 2, 0), coord3d(i, 0, 0), "" + wt_rail), null)
		}

		ASSERT_EQUAL(wayremover.work(pl, coord3d(0, 1, 0), coord3d(all_signals.len()-1, 1, 0), "" + wt_rail), null)
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_sign_build_signal_multiple()
{
	local pl = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local signal = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, sign) sign.is_signal())[0]

	// starts on way end, not possible
	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, 2, 0), coord3d(7, 2, 0), rail, true), null)

		ASSERT_EQUAL(command_x(tool_build_roadsign).work(pl, coord3d(0, 2, 0), coord3d(7, 2, 0), signal.get_name()),
			"Hier kann kein\nSignal aufge-\nstellt werden!\n")

		for (local x = 2; x <= 7; ++x) {
			ASSERT_EQUAL(tile_x(x, 2, 0).find_object(mo_signal), null)
		}

		ASSERT_WAY_PATTERN_MASKED(wt_rail, coord3d(0, 0, 0),
			[
				"........",
				"........",
				"2AAAAAA8",
				"........",
				"........",
				"........",
				"........",
				"........"
			])

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(0, 2, 0), coord3d(7, 2, 0), "" + wt_rail), null)
	}

	{
		ASSERT_EQUAL(command_x.build_way(pl, coord3d(0, 2, 0), coord3d(7, 2, 0), rail, true), null)

		// FIXME click-and-drag for tools is not supported
		local error_caught = false
		try {
			command_x(tool_build_roadsign).work(pl, coord3d(1, 2, 0), coord3d(6, 2, 0), signal.get_name())
		}
		catch (e) {
			ASSERT_EQUAL(e, "First click has side effects")
			error_caught = true
		}
		ASSERT_EQUAL(error_caught, true)

		ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(0, 2, 0), coord3d(7, 2, 0), "" + wt_rail), null)
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_sign_replace_signal()
{
	local pl = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local signal = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, sign) sign.is_signal())[0]
	local presignal = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, sign) sign.is_pre_signal())[0]

	// preconditions
	ASSERT_TRUE(signal != null)
	ASSERT_TRUE(presignal != null)

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 3, 0), coord3d(6, 3, 0), rail, true), null)

	// try replacing one-directional signals
	{
		ASSERT_EQUAL(command_x(tool_build_roadsign).work(pl, coord3d(5, 3, 0), signal.get_name()), null)

		local error_caught = false
		try {
			command_x(tool_build_roadsign).work(pl,        coord3d(5, 3, 0), presignal.get_name())
		}
		catch (e) {
			ASSERT_EQUAL(e, "Tool has no effects")
			error_caught = true
		}
		ASSERT_TRUE(error_caught)

		local s = sign_x(5, 3, 0)
		ASSERT_TRUE(s != null)
		ASSERT_EQUAL(s.get_desc().get_name(), signal.get_name())
	}

	// and one-directional signals
	{
		ASSERT_EQUAL(command_x(tool_build_roadsign).work(pl, coord3d(5, 3, 0), signal.get_name()), null)

		local error_caught = false
		try {
			command_x(tool_build_roadsign).work(pl,        coord3d(5, 3, 0), presignal.get_name())
		}
		catch (e) {
			ASSERT_EQUAL(e, "Tool has no effects")
			error_caught = true
		}
		ASSERT_TRUE(error_caught)

		local s = sign_x(5, 3, 0)
		ASSERT_TRUE(s != null)
		ASSERT_EQUAL(s.get_desc().get_name(), signal.get_name())
	}

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(6, 3, 0), coord3d(4, 3, 0), "" + wt_rail), null)

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}


function test_sign_signal_when_player_removed()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local signal = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, sign) sign.is_signal())[0]

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(4, 3, 0), coord3d(6, 3, 0), rail, true), null)
	ASSERT_EQUAL(command_x(tool_build_roadsign).work(public_pl, coord3d(5, 3, 0), signal.get_name()), null)

	{
		world.remove_player(pl)
		ASSERT_EQUAL(tile_x(5, 3, 0).find_object(mo_way), null)
		ASSERT_TRUE(tile_x(5, 3, 0).find_object(mo_signal) != null)
	}

	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(5, 3, 0)), null)
	ASSERT_TRUE(tile_x(5, 3, 0).find_object(mo_signal) == null)

	RESET_ALL_PLAYER_FUNDS()
}
