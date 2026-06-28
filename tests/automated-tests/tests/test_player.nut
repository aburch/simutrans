//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for player_x stuff
//


function test_player_cash()
{
	local pl = player_x(0)

	ASSERT_EQUAL(pl.get_current_cash(),        200000)     // get_current_cash is in credits (returns float)
	ASSERT_EQUAL(pl.get_current_net_wealth(),  200000*100) // get_current_net_wealth is in 1/100 credits
	ASSERT_EQUAL(pl.get_current_maintenance(), 0)

	local ok = pl.book_cash(0)
	ASSERT_EQUAL(ok, true)
	ASSERT_EQUAL(pl.get_current_cash(),       200000)
	ASSERT_EQUAL(pl.get_current_net_wealth(), 200000 * 100)
	ASSERT_EQUAL(pl.get_current_maintenance(), 0)

	ok = pl.book_cash(-200000 * 100)               // book_cash is in 1/100 credits
	ASSERT_EQUAL(ok, true)
	ASSERT_EQUAL(pl.get_current_cash(), 0)
	ASSERT_EQUAL(pl.get_current_net_wealth(), 0)
	ASSERT_EQUAL(pl.get_current_maintenance(), 0)

	ok = pl.book_cash(-1)
	ASSERT_EQUAL(ok, true)
	ASSERT_EQUAL(pl.get_current_cash(), -0.01)
	ASSERT_EQUAL(pl.get_current_net_wealth(), -1)
	ASSERT_EQUAL(pl.get_current_maintenance(), 0)

	// clean up
	ok = pl.book_cash(200000 * 100 + 1)
	ASSERT_EQUAL(ok, true)
	ASSERT_EQUAL(pl.get_current_cash(),       200000)
	ASSERT_EQUAL(pl.get_current_net_wealth(), 200000 * 100)
	ASSERT_EQUAL(pl.get_current_maintenance(), 0)

	RESET_ALL_PLAYER_FUNDS()
}


function test_player_isactive()
{
	local pl = player_x(0)
	ASSERT_EQUAL(pl.is_valid(), true)
	ASSERT_EQUAL(pl.is_active(), true)

	local public_pl = player_x(1)
	ASSERT_EQUAL(public_pl.is_valid(), true)
	ASSERT_EQUAL(public_pl.is_active(), true)

	local nonexist = player_x(2)
	ASSERT_EQUAL(nonexist.is_valid(), false)
	ASSERT_EQUAL(nonexist.is_active(), false)

	RESET_ALL_PLAYER_FUNDS()
}


function test_player_create()
{
	local human = player_x(2)
	local goods_ai = player_x(3)
	local passenger_ai = player_x(4)
	local scripted_ai = player_x(5)
	local invalid_type = player_x(6)
	local last_slot = player_x(14)

	ASSERT_FALSE(human.is_valid())
	ASSERT_FALSE(goods_ai.is_valid())
	ASSERT_FALSE(passenger_ai.is_valid())
	ASSERT_FALSE(scripted_ai.is_valid())
	ASSERT_FALSE(invalid_type.is_valid())
	ASSERT_FALSE(last_slot.is_valid())

	ASSERT_TRUE(world.create_player(2, 1))
	ASSERT_TRUE(human.is_valid())
	ASSERT_EQUAL(human.get_type(), 1)

	ASSERT_TRUE(world.create_player(3, 2))
	ASSERT_TRUE(goods_ai.is_valid())
	ASSERT_EQUAL(goods_ai.get_type(), 2)

	ASSERT_TRUE(world.create_player(4, 3))
	ASSERT_TRUE(passenger_ai.is_valid())
	ASSERT_EQUAL(passenger_ai.get_type(), 3)

	ASSERT_TRUE(world.create_player(5, 4))
	ASSERT_TRUE(scripted_ai.is_valid())
	ASSERT_EQUAL(scripted_ai.get_type(), 4)

	ASSERT_TRUE(world.create_player(14, 1))
	ASSERT_TRUE(last_slot.is_valid())
	ASSERT_EQUAL(last_slot.get_type(), 1)

	ASSERT_FALSE(world.create_player(2, 1))
	ASSERT_FALSE(world.create_player(-1, 1))
	ASSERT_FALSE(world.create_player(0, 1))
	ASSERT_FALSE(world.create_player(1, 1))
	ASSERT_FALSE(world.create_player(15, 1))
	ASSERT_FALSE(world.create_player(6, 0))
	ASSERT_FALSE(world.create_player(6, 5))
	ASSERT_FALSE(invalid_type.is_valid())
}


function test_world_open_dialog_tool_invalid()
{
	ASSERT_FALSE(world.open_dialog_tool(-1))
	ASSERT_FALSE(world.open_dialog_tool(9999))
}


function test_player_headquarters()
{
	local pl = player_x(0)
	ASSERT_EQUAL(pl.get_headquarter_level(), 0)
	ASSERT_EQUAL(pl.get_headquarter_pos().tostring(), coord(-1, -1).tostring())

	RESET_ALL_PLAYER_FUNDS()
}


function test_player_name()
{
	local pl = player_x(0)

	ASSERT_EQUAL(pl.get_name(), "Human player")
	ASSERT_TRUE(pl.set_name("Foo"))
	ASSERT_EQUAL(pl.get_name(), "Foo")
	ASSERT_TRUE(pl.set_name("Foo"))
	ASSERT_EQUAL(pl.get_name(), "Foo")

	// clean up
	ASSERT_TRUE(pl.set_name("Human player"))

	RESET_ALL_PLAYER_FUNDS()
}


function test_player_lines()
{
	local pl = player_x(0)

	// empty line list
	{
		local line_list = pl.get_line_list()
		ASSERT_TRUE(line_list != null)
		ASSERT_EQUAL(line_list.get_count(), 0)
	}

	// non-empty line list
	{
		ASSERT_TRUE(pl.create_line(wt_road))

		local line_list = pl.get_line_list()
		ASSERT_TRUE(line_list != null)
		ASSERT_EQUAL(line_list.get_count(), 1)

		foreach (line in line_list) {
			ASSERT_TRUE(line != null)
			ASSERT_EQUAL(line.is_valid(), true)
			ASSERT_EQUAL(line.get_waytype(), wt_road)
		}

		foreach (line in line_list) {
			ASSERT_TRUE(line.destroy(pl))
		}

		ASSERT_EQUAL(pl.get_line_list().get_count(), 0)
	}

	// TODO Destroy lines owned by other players

	// clean up
	RESET_ALL_PLAYER_FUNDS()
}
