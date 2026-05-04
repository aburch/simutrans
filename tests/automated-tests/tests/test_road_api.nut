//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//

function test_road_api()
{
	local pl = player_x(0)
	local start = coord3d(10, 10, 0)
	local end = coord3d(10, 15, 0)
	local road_desc = way_desc_x.get_available_ways(wt_road, st_flat)[0]

	ASSERT_TRUE(road_desc != null)

	// 1. Build road with oneway_mode
	local err = command_x.build_road(pl, start, end, road_desc, false, {overtaking_mode = oneway_mode})
	ASSERT_EQUAL(err, null)

	for (local y = 10; y <= 15; y++) {
		local tile = tile_x(10, y, 0)
		local way = tile.find_object(mo_way)
		ASSERT_TRUE(way != null)
		ASSERT_TRUE(way instanceof street_x)
		ASSERT_EQUAL(way.get_overtaking_mode(), oneway_mode)
	}

	// 2. Build road with road_avoid_cityroad
	start = coord3d(11, 10, 0)
	end = coord3d(11, 15, 0)
	err = command_x.build_road(pl, start, end, road_desc, false, {overtaking_mode = twoway_mode, street_flag = road_avoid_cityroad})
	ASSERT_EQUAL(err, null)

	for (local y = 10; y <= 15; y++) {
		local tile = tile_x(11, y, 0)
		local way = tile.find_object(mo_way)
		ASSERT_TRUE(way != null)
		ASSERT_TRUE(way instanceof street_x)
		ASSERT_EQUAL(way.get_street_flags(), road_avoid_cityroad)
	}

	// 3. Test combined flags
	start = coord3d(12, 10, 0)
	end = coord3d(12, 15, 0)
	local combined_flags = road_avoid_cityroad | road_citycar_no_entry
	err = command_x.build_road(pl, start, end, road_desc, false, {overtaking_mode = loading_only_mode, street_flag = combined_flags})
	ASSERT_EQUAL(err, null)

	for (local y = 10; y <= 15; y++) {
		local tile = tile_x(12, y, 0)
		local way = tile.find_object(mo_way)
		ASSERT_TRUE(way != null)
		ASSERT_TRUE(way instanceof street_x)
		ASSERT_EQUAL(way.get_overtaking_mode(), loading_only_mode)
		ASSERT_EQUAL(way.get_street_flags(), combined_flags)
	}

	// 4. Test build_way default params
	start = coord3d(13, 10, 0)
	end = coord3d(13, 15, 0)
	err = command_x.build_way(pl, start, end, road_desc, false)
	ASSERT_EQUAL(err, null)

	for (local y = 10; y <= 15; y++) {
		local tile = tile_x(13, y, 0)
		local way = tile.find_object(mo_way)
		ASSERT_TRUE(way != null)
		ASSERT_TRUE(way instanceof street_x)
		ASSERT_EQUAL(way.get_overtaking_mode(), twoway_mode)
		ASSERT_EQUAL(way.get_street_flags(), 0)
	}

	// 5. Test that rail is way_x but not street_x
	local rail_desc = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	ASSERT_TRUE(rail_desc != null)
	start = coord3d(14, 10, 0)
	end = coord3d(14, 15, 0)
	err = command_x.build_way(pl, start, end, rail_desc, false)
	ASSERT_EQUAL(err, null)
	local rail_tile = tile_x(14, 10, 0)
	local rail_way = rail_tile.find_object(mo_way)
	ASSERT_TRUE(rail_way != null)
	ASSERT_TRUE(rail_way instanceof way_x)
	ASSERT_FALSE(rail_way instanceof street_x)
}
