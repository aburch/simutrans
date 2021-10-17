//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for dir/coord
//


function test_dir_is_single()
{
	ASSERT_FALSE(dir.is_single(dir.none))

	ASSERT_TRUE(dir.is_single(dir.north))
	ASSERT_TRUE(dir.is_single(dir.east))
	ASSERT_TRUE(dir.is_single(dir.south))
	ASSERT_TRUE(dir.is_single(dir.west))

	ASSERT_FALSE(dir.is_single(dir.northeast))
	ASSERT_FALSE(dir.is_single(dir.northsouth))
	ASSERT_FALSE(dir.is_single(dir.northwest))
	ASSERT_FALSE(dir.is_single(dir.southeast))
	ASSERT_FALSE(dir.is_single(dir.eastwest))
	ASSERT_FALSE(dir.is_single(dir.southwest))

	ASSERT_FALSE(dir.is_single(dir.northsoutheast))
	ASSERT_FALSE(dir.is_single(dir.northeastwest))
	ASSERT_FALSE(dir.is_single(dir.northsouthwest))
	ASSERT_FALSE(dir.is_single(dir.southeastwest))

	ASSERT_FALSE(dir.is_single(dir.all))
}


function test_dir_is_twoway()
{
	ASSERT_FALSE(dir.is_twoway(dir.none))

	ASSERT_FALSE(dir.is_twoway(dir.north))
	ASSERT_FALSE(dir.is_twoway(dir.east))
	ASSERT_FALSE(dir.is_twoway(dir.south))
	ASSERT_FALSE(dir.is_twoway(dir.west))

	ASSERT_TRUE(dir.is_twoway(dir.northeast))
	ASSERT_TRUE(dir.is_twoway(dir.northsouth))
	ASSERT_TRUE(dir.is_twoway(dir.northwest))
	ASSERT_TRUE(dir.is_twoway(dir.southeast))
	ASSERT_TRUE(dir.is_twoway(dir.eastwest))
	ASSERT_TRUE(dir.is_twoway(dir.southwest))

	ASSERT_FALSE(dir.is_twoway(dir.northsoutheast))
	ASSERT_FALSE(dir.is_twoway(dir.northeastwest))
	ASSERT_FALSE(dir.is_twoway(dir.northsouthwest))
	ASSERT_FALSE(dir.is_twoway(dir.southeastwest))

	ASSERT_FALSE(dir.is_twoway(dir.all))
}


function test_dir_is_threeway()
{
	ASSERT_FALSE(dir.is_threeway(dir.none))

	ASSERT_FALSE(dir.is_threeway(dir.north))
	ASSERT_FALSE(dir.is_threeway(dir.east))
	ASSERT_FALSE(dir.is_threeway(dir.south))
	ASSERT_FALSE(dir.is_threeway(dir.west))

	ASSERT_FALSE(dir.is_threeway(dir.northeast))
	ASSERT_FALSE(dir.is_threeway(dir.northsouth))
	ASSERT_FALSE(dir.is_threeway(dir.northwest))
	ASSERT_FALSE(dir.is_threeway(dir.southeast))
	ASSERT_FALSE(dir.is_threeway(dir.eastwest))
	ASSERT_FALSE(dir.is_threeway(dir.southwest))

	ASSERT_TRUE(dir.is_threeway(dir.northsoutheast))
	ASSERT_TRUE(dir.is_threeway(dir.northeastwest))
	ASSERT_TRUE(dir.is_threeway(dir.northsouthwest))
	ASSERT_TRUE(dir.is_threeway(dir.southeastwest))

	ASSERT_TRUE(dir.is_threeway(dir.all))
}


function test_dir_is_curve()
{
	ASSERT_FALSE(dir.is_curve(dir.none))

	ASSERT_FALSE(dir.is_curve(dir.north))
	ASSERT_FALSE(dir.is_curve(dir.east))
	ASSERT_FALSE(dir.is_curve(dir.south))
	ASSERT_FALSE(dir.is_curve(dir.west))

	ASSERT_TRUE(dir.is_curve(dir.northeast))
	ASSERT_FALSE(dir.is_curve(dir.northsouth))
	ASSERT_TRUE(dir.is_curve(dir.northwest))
	ASSERT_TRUE(dir.is_curve(dir.southeast))
	ASSERT_FALSE(dir.is_curve(dir.eastwest))
	ASSERT_TRUE(dir.is_curve(dir.southwest))

	ASSERT_FALSE(dir.is_curve(dir.northsoutheast))
	ASSERT_FALSE(dir.is_curve(dir.northeastwest))
	ASSERT_FALSE(dir.is_curve(dir.northsouthwest))
	ASSERT_FALSE(dir.is_curve(dir.southeastwest))

	ASSERT_FALSE(dir.is_curve(dir.all))
}


function test_dir_is_straight()
{
	ASSERT_FALSE(dir.is_straight(dir.none))

	ASSERT_TRUE(dir.is_straight(dir.north))
	ASSERT_TRUE(dir.is_straight(dir.east))
	ASSERT_TRUE(dir.is_straight(dir.south))
	ASSERT_TRUE(dir.is_straight(dir.west))

	ASSERT_FALSE(dir.is_straight(dir.northeast))
	ASSERT_TRUE(dir.is_straight(dir.northsouth))
	ASSERT_FALSE(dir.is_straight(dir.northwest))
	ASSERT_FALSE(dir.is_straight(dir.southeast))
	ASSERT_TRUE(dir.is_straight(dir.eastwest))
	ASSERT_FALSE(dir.is_straight(dir.southwest))

	ASSERT_FALSE(dir.is_straight(dir.northsoutheast))
	ASSERT_FALSE(dir.is_straight(dir.northeastwest))
	ASSERT_FALSE(dir.is_straight(dir.northsouthwest))
	ASSERT_FALSE(dir.is_straight(dir.southeastwest))

	ASSERT_FALSE(dir.is_straight(dir.all))
}


function test_dir_double()
{
	ASSERT_EQUAL(dir.double(dir.none), dir.none)

	ASSERT_EQUAL(dir.double(dir.north), dir.northsouth)
	ASSERT_EQUAL(dir.double(dir.east), dir.eastwest)
	ASSERT_EQUAL(dir.double(dir.south), dir.northsouth)
	ASSERT_EQUAL(dir.double(dir.west), dir.eastwest)

	ASSERT_EQUAL(dir.double(dir.northeast), dir.none)
	ASSERT_EQUAL(dir.double(dir.northsouth), dir.northsouth)
	ASSERT_EQUAL(dir.double(dir.northwest), dir.none)
	ASSERT_EQUAL(dir.double(dir.southeast), dir.none)
	ASSERT_EQUAL(dir.double(dir.eastwest), dir.eastwest)
	ASSERT_EQUAL(dir.double(dir.southwest), dir.none)

	ASSERT_EQUAL(dir.double(dir.northsoutheast), dir.none)
	ASSERT_EQUAL(dir.double(dir.northeastwest), dir.none)
	ASSERT_EQUAL(dir.double(dir.northsouthwest), dir.none)
	ASSERT_EQUAL(dir.double(dir.southeastwest), dir.none)

	ASSERT_EQUAL(dir.double(dir.all), dir.none)
}


function test_dir_backward()
{
	ASSERT_EQUAL(dir.backward(dir.none), dir.all)

	ASSERT_EQUAL(dir.backward(dir.north), dir.south)
	ASSERT_EQUAL(dir.backward(dir.east),  dir.west)
	ASSERT_EQUAL(dir.backward(dir.south), dir.north)
	ASSERT_EQUAL(dir.backward(dir.west),  dir.east)

	ASSERT_EQUAL(dir.backward(dir.northeast),  dir.southwest)
	ASSERT_EQUAL(dir.backward(dir.northsouth), dir.northsouth)
	ASSERT_EQUAL(dir.backward(dir.northwest),  dir.southeast)
	ASSERT_EQUAL(dir.backward(dir.southeast),  dir.northwest)
	ASSERT_EQUAL(dir.backward(dir.eastwest),   dir.eastwest)
	ASSERT_EQUAL(dir.backward(dir.southwest),  dir.northeast)

	ASSERT_EQUAL(dir.backward(dir.northsoutheast), dir.west)
	ASSERT_EQUAL(dir.backward(dir.northeastwest),  dir.south)
	ASSERT_EQUAL(dir.backward(dir.northsouthwest), dir.east)
	ASSERT_EQUAL(dir.backward(dir.southeastwest),  dir.north)

	ASSERT_EQUAL(dir.backward(dir.all), dir.none)
}


function test_dir_to_slope()
{
	ASSERT_EQUAL(dir.to_slope(dir.none),  slope.flat)

	ASSERT_EQUAL(dir.to_slope(dir.north), slope.south)
	ASSERT_EQUAL(dir.to_slope(dir.east),  slope.west)
	ASSERT_EQUAL(dir.to_slope(dir.south), slope.north)
	ASSERT_EQUAL(dir.to_slope(dir.west),  slope.east)

	ASSERT_EQUAL(dir.to_slope(dir.northeast),  slope.flat)
	ASSERT_EQUAL(dir.to_slope(dir.northsouth), slope.flat)
	ASSERT_EQUAL(dir.to_slope(dir.northwest),  slope.flat)
	ASSERT_EQUAL(dir.to_slope(dir.southeast),  slope.flat)
	ASSERT_EQUAL(dir.to_slope(dir.eastwest),   slope.flat)
	ASSERT_EQUAL(dir.to_slope(dir.southwest),  slope.flat)

	ASSERT_EQUAL(dir.to_slope(dir.northsouthwest), slope.flat)
	ASSERT_EQUAL(dir.to_slope(dir.northeastwest),  slope.flat)
	ASSERT_EQUAL(dir.to_slope(dir.northsouthwest), slope.flat)
	ASSERT_EQUAL(dir.to_slope(dir.southeastwest),  slope.flat)

	ASSERT_EQUAL(dir.to_slope(dir.all), slope.flat)

	local error_raised = false
	try {
		// out-of-range dir: ANDed with dir.all
		ASSERT_EQUAL(dir.to_slope(0x2A), dir.to_slope(0x0A))
	}
	catch (e) {
		error_raised = true;
	}

	ASSERT_FALSE(error_raised)
}


function test_dir_to_coord()
{
	ASSERT_EQUAL(dir.to_coord(dir.none).tostring(),  coord( 0,  0).tostring())

	ASSERT_EQUAL(dir.to_coord(dir.north).tostring(), coord( 0, -1).tostring())
	ASSERT_EQUAL(dir.to_coord(dir.east).tostring(),  coord( 1,  0).tostring())
	ASSERT_EQUAL(dir.to_coord(dir.south).tostring(), coord( 0,  1).tostring())
	ASSERT_EQUAL(dir.to_coord(dir.west).tostring(),  coord(-1,  0).tostring())

	ASSERT_EQUAL(dir.to_coord(dir.northeast).tostring(),  coord( 1, -1).tostring())
	ASSERT_EQUAL(dir.to_coord(dir.northsouth).tostring(), coord( 0,  0).tostring())
	ASSERT_EQUAL(dir.to_coord(dir.southeast).tostring(),  coord( 1,  1).tostring())
	ASSERT_EQUAL(dir.to_coord(dir.northwest).tostring(),  coord(-1, -1).tostring())
	ASSERT_EQUAL(dir.to_coord(dir.eastwest).tostring(),   coord( 0,  0).tostring())
	ASSERT_EQUAL(dir.to_coord(dir.southwest).tostring(),  coord(-1,  1).tostring())

	ASSERT_EQUAL(dir.to_coord(dir.northsouthwest).tostring(), coord(-1,  0).tostring())
	ASSERT_EQUAL(dir.to_coord(dir.southeastwest).tostring(),  coord( 0,  1).tostring())
	ASSERT_EQUAL(dir.to_coord(dir.northsoutheast).tostring(), coord( 1,  0).tostring())
	ASSERT_EQUAL(dir.to_coord(dir.northeastwest).tostring(),  coord( 0, -1).tostring())

	ASSERT_EQUAL(dir.to_coord(dir.all).tostring(), coord( 0,  0).tostring())

	local error_raised = false
	try {
		dir.to_coord(42) // out-of-range dir
	}
	catch (e) {
		ASSERT_EQUAL(e, "Invalid dir 42 (valid values are 0..15)")
		error_raised = true
	}

	ASSERT_TRUE(error_raised)
}
