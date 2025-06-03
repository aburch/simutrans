//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Test helpers
//
function make_assertion_str(val)
{
	if (typeof val == "string") {
		return "\"" + val + "\""
	}
	else {
		return "" + val
	}
}


function ASSERT_EQUAL(act, exp)
{
	if (!(act == exp)) {
		local err = ttext("Assertion failed, '{act} == {exp}' was not true")
		err.act = make_assertion_str(act)
		err.exp = make_assertion_str(exp)
		throw err.tostring()
	}
}


function ASSERT_TRUE(a)
{
	if (!(a == true)) {
		local err = ttext("Assertion failed, '{a}' was not true")
		err.a = make_assertion_str(a)
		throw err.tostring()
	}
}


function ASSERT_FALSE(a)
{
	if (!(a == false)) {
		local err = ttext("Assertion failed, '{a}' was not false")
		err.a = make_assertion_str(a)
		throw err.tostring()
	}
}


// Set player funds (incl. non-cash assets) to some specific amount (in credit-cents)
function SET_PLAYER_FUNDS(pl, amount)
{
	pl.book_cash(amount - pl.get_current_net_wealth())
}


function RESET_ALL_PLAYER_FUNDS()
{
	local default_cash = 200000 * 100

	for (local i = 0; i < 8; ++i) {
		local pl = player_x(i)
		if (pl.is_valid()) {
			ASSERT_EQUAL(pl.get_current_maintenance(), 0)
			SET_PLAYER_FUNDS(pl, default_cash)
		}
	}
}


function char_to_dir(ch)
{
	switch (ch) {
	case '.': return dir.none // '.' == dontcare
	case '0': return dir.none
	case '1': return dir.north
	case '2': return dir.east
	case '3': return dir.northeast
	case '4': return dir.south
	case '5': return dir.northsouth
	case '6': return dir.southeast
	case '7': return dir.northsoutheast
	case '8': return dir.west
	case '9': return dir.northwest
	case 'A': return dir.eastwest
	case 'B': return dir.northeastwest
	case 'C': return dir.southwest
	case 'D': return dir.northsouthwest
	case 'E': return dir.southeastwest
	case 'F': return dir.all
	}

	ASSERT_TRUE(false) // should not reach here
}


function ASSERT_WAY_PATTERN(waytype, lefttop, pattern)
{
	local z = lefttop.z

	for (local y = 0; y < pattern.len(); ++y) {
		for (local x = 0; x < pattern[y].len(); ++x) {
			local tile = square_x(lefttop.x + x, lefttop.y + y).get_tile_at_height(z)
			local expected_dir = char_to_dir(pattern[y][x])

			if (tile != null) {
				local actual_dir = 0
				if (waytype != wt_power) {
					actual_dir = tile.get_way_dirs(waytype)
				}
				else {
					// powerlines connect to other powerlines automatically.
					local powerline = tile.find_object(mo_powerline)
					actual_dir = 0

					if (powerline) {
						for (local i = 0; i < 4; ++i) {
							local offset = dir.to_coord(1<<i)
							local nb = square_x(lefttop.x + x + offset.x, lefttop.y + y + offset.y)
							if (nb && nb.is_valid() && nb.get_tile_at_height(z) && nb.get_tile_at_height(z).find_object(mo_powerline)) {
								actual_dir = actual_dir | (1<<i)
							}
						}
					}
				}

				ASSERT_EQUAL(actual_dir, expected_dir)
			}
		}
	}
}


function ASSERT_WAY_PATTERN_MASKED(waytype, lefttop, pattern)
{
	if (waytype == wt_power) {
		ASSERT_WAY_PATTERN(waytype, lefttop, pattern)
		return
	}

	local z = lefttop.z

	for (local y = 0; y < pattern.len(); ++y) {
		for (local x = 0; x < pattern[y].len(); ++x) {
			local tile = square_x(lefttop.x + x, lefttop.y + y).get_tile_at_height(z)
			local expected_dir = char_to_dir(pattern[y][x])

			if (tile != null) {
				local actual_dir = tile.get_way_dirs_masked(waytype)
				ASSERT_EQUAL(actual_dir, expected_dir)
			}
		}
	}
}


function get_depot_by_wt(waytype)
{
	local list = building_desc_x.get_building_list(building_desc_x.depot)

	foreach (building in list) {
		if (building.get_type() == building_desc_x.depot && building.get_waytype() == waytype) {
			return building
		}
	}

	return null
}
