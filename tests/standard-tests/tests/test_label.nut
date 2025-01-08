//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for labels
//


function test_label()
{
	local remover = command_x(tool_remover)
	local pl = player_x(0)
	local public_pl = player_x(1)

	// build label on ground
	{
		ASSERT_EQUAL(label_x.create(coord3d(4, 1,  0), pl, "Foo2"), null)

		// test label list
		local count = 0
		foreach(label in world.get_label_list()) {
			ASSERT_EQUAL(label.get_text(), "Foo2")
			count ++
		}
		ASSERT_EQUAL(count, 1)

		ASSERT_EQUAL(remover.work(pl, coord3d(4, 1, 0)), null)
		ASSERT_EQUAL(world.get_label_list().get_count(), 0)
	}

	// Build label on invalid ground, same as building on map ground
	{
		ASSERT_EQUAL(label_x.create(coord3d(3, 1, -2), pl, "Foo1"), null)

		local label = tile_x(3, 1, 0).find_object(mo_label)
		ASSERT_TRUE(label != null)
		ASSERT_TRUE(label.is_valid())

		ASSERT_EQUAL(label.get_owner().get_name(), pl.get_name()) // TODO More secure way to check for player equality
		ASSERT_EQUAL(label.get_waytype(), wt_invalid)

		ASSERT_EQUAL(label.get_text(), "Foo1")
		ASSERT_EQUAL(label.set_text("Bar"), true)
		ASSERT_EQUAL(label.get_text(), "Bar")

		ASSERT_EQUAL(remover.work(pl, coord3d(3, 1, 0)), null)
		ASSERT_FALSE(label.is_valid())
	}

	// build label on existing label, should fail
	{
		ASSERT_EQUAL(label_x.create(coord3d(5, 1,  0), pl, "Foo3"), null)
		ASSERT_EQUAL(label_x.create(coord3d(5, 1,  0), pl, "Foo4"), "Das Feld gehoert\neinem anderen Spieler\n")
		ASSERT_EQUAL(remover.work(pl, coord3d(5, 1, 0)), null)
	}

	// players cannot remove labels of other players (except public player)
	{
		ASSERT_EQUAL(label_x.create(coord3d(5, 1, 0), pl,        "Foo1"), null)
		ASSERT_EQUAL(label_x.create(coord3d(5, 2, 0), public_pl, "Foo2"), null)

		ASSERT_EQUAL(remover.work(pl, coord3d(5, 2, 0)), "Der Besitzer erlaubt das Entfernen nicht")
		ASSERT_TRUE(tile_x(5, 2, 0).find_object(mo_label) != null)

		ASSERT_EQUAL(remover.work(public_pl, coord3d(5, 2, 0)), null)
		ASSERT_EQUAL(remover.work(public_pl, coord3d(5, 1, 0)), null)
	}

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}
