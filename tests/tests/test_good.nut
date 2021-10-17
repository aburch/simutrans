//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for goods / freight
//

function test_good_is_interchangeable()
{
	local pax = good_desc_x.passenger
	local mail = good_desc_x.mail

	local coal = good_desc_x("Kohle")
	local ore  = good_desc_x("Eisenerz")

	ASSERT_FALSE(pax.is_interchangeable(mail))
	ASSERT_FALSE(mail.is_interchangeable(pax))
	ASSERT_TRUE(pax.get_catg_index() != mail.get_catg_index())
	ASSERT_EQUAL(pax.get_catg_index(), 0)
	ASSERT_EQUAL(mail.get_catg_index(), 1)

	ASSERT_TRUE(coal.is_interchangeable(ore))
	ASSERT_TRUE(ore.is_interchangeable(coal))
	ASSERT_EQUAL(coal.get_catg_index(), ore.get_catg_index())
	ASSERT_TRUE(coal.get_catg_index() >= 3)
	ASSERT_TRUE(ore.get_catg_index() >= 3)
}


function test_good_speed_bonus()
{
	local pax = good_desc_x.passenger
	local concrete = good_desc_x("Concrete")

	ASSERT_TRUE(concrete.calc_revenue(wt_road, 0) == concrete.calc_revenue(wt_road, 999)) // this has a speed bonus of 0

	local wtypes = [ wt_road, wt_rail, wt_water, wt_monorail, wt_maglev, wt_narrowgauge ]
	local goods = [ pax, concrete ]

	foreach (g in goods) {
		foreach (wt in wtypes) {
			local last = 0
			for (local speed = 0; speed <= 999; ++speed) {
				local current = g.calc_revenue(wt, speed)
				ASSERT_TRUE(current >= last)
				last = current
			}
		}
	}
}
