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

