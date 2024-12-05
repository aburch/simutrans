//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Test building/upgrading/removing headquarters
//


function get_hq_by_level(level)
{
	local list = building_desc_x.get_building_list(building_desc_x.headquarter)
	list.sort(@(a, b) a.get_headquarter_level() <=> b.get_headquarter_level())
	return list[level]
}


function test_headquarters_build_flat()
{
	local pl        = player_x(0)
	local public_pl = player_x(1)

	local hq_builder = command_x(tool_headquarter)
	local pos1 = coord3d(2, 3, 0)
	local pos2 = coord3d(4, 1, 0)
	local pos1_2d = coord(2, 3)
	local pos2_2d = coord(4, 1)

	ASSERT_EQUAL(pl.get_headquarter_pos().tostring(), coord(-1, -1).tostring())

	// place normal
	{
		ASSERT_EQUAL(hq_builder.work(pl, pos1, get_hq_by_level(0).get_name()), null)
		ASSERT_EQUAL(pl.get_headquarter_pos().tostring(), pos1_2d.tostring())
	}

	// replace my existing HQ
	{
		ASSERT_EQUAL(hq_builder.work(pl, pos1, get_hq_by_level(1).get_name()), null)
		ASSERT_EQUAL(pl.get_headquarter_pos().tostring(), pos1_2d.tostring())
	}

	{
		// cannot downgrade headquarters FIXME this depends on whether timeline is enabled ?!
		ASSERT_EQUAL(hq_builder.work(pl, pos2, get_hq_by_level(0).get_name()), "Insufficient funds!")
		ASSERT_EQUAL(pl.get_headquarter_pos().tostring(), pos1_2d.tostring())

		pl.book_cash(1*1000*1000*100)
		ASSERT_EQUAL(hq_builder.work(pl, pos2, get_hq_by_level(0).get_name()), null)
		ASSERT_EQUAL(pl.get_headquarter_pos().tostring(), pos2_2d.tostring())
	}

	// cannot replace other people's headquarters (not even public player!)
	{
		ASSERT_EQUAL(hq_builder.work(public_pl, pos2, get_hq_by_level(1).get_name()), "No suitable ground!")
		ASSERT_EQUAL(public_pl.get_headquarter_pos().tostring(), coord(-1, -1).tostring())
		ASSERT_EQUAL(pl.get_headquarter_pos().tostring(), pos2_2d.tostring())

		ASSERT_EQUAL(hq_builder.work(public_pl, pos1, get_hq_by_level(1).get_name()), null)
		ASSERT_EQUAL(public_pl.get_headquarter_pos().tostring(), pos1_2d.tostring())
		ASSERT_EQUAL(pl.get_headquarter_pos().tostring(),        pos2_2d.tostring())
	}

	// remove hq
	local remover = command_x(tool_remover)
	ASSERT_EQUAL(remover.work(pl,        tile_x(2, 3, 0)), "Der Besitzer erlaubt das Entfernen nicht") // cannot remove HQ of other players
	ASSERT_EQUAL(remover.work(public_pl, tile_x(2, 3, 0)), null)
	ASSERT_EQUAL(remover.work(public_pl, tile_x(4, 1, 0)), null)

	ASSERT_EQUAL(pl.get_headquarter_pos().tostring(), coord(-1, -1).tostring())
	ASSERT_EQUAL(public_pl.get_headquarter_pos().tostring(), coord(-1, -1).tostring())

	RESET_ALL_PLAYER_FUNDS()
}
