//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for factories / factory links
//

function build_factory(pl, pos, ignore_climates, rotation, base_prod, name)
{
	local factory_builder = command_x(tool_build_factory)
	return factory_builder.work(pl, pos, "" + ignore_climates.tointeger() + rotation + base_prod + "," + name)
}


function test_factory_build_pp()
{
	local pl = player_x(0)
	local public_pl = player_x(1)

	{
		// FIXME Causes signed integer overflow even though < 2^30
		// local production = 1024 * 1000 * 1000
		local production = 1024 // 1/s

		ASSERT_EQUAL(build_factory(pl, coord3d(3, 4, 0), 0, 1, production, "Aufwindkraftwerk"), null)
		local factory = factory_x(3, 4)
		ASSERT_TRUE(factory_x(5, 6) != null)
		ASSERT_EQUAL(factory_list_x().get_count(), 1)

		local desc = factory.get_desc();
		ASSERT_TRUE(desc != null)
		ASSERT_EQUAL(desc.get_name(), "Aufwindkraftwerk")

		// FIXME Max electricity production for PPs not queryable via script?

		ASSERT_EQUAL(factory.get_consumers().len(), 0)
		ASSERT_EQUAL(factory.get_suppliers().len(), 0)
		ASSERT_EQUAL(factory.get_production()[0], 0)
		ASSERT_EQUAL(factory.get_power()[0], 0)
		ASSERT_EQUAL(factory.get_boost_electric()[0], 0)
		ASSERT_EQUAL(factory.get_boost_pax()[0], 0)
		ASSERT_EQUAL(factory.get_boost_mail()[0], 0)
		ASSERT_EQUAL(factory.get_pax_generated()[0], 0)
		ASSERT_EQUAL(factory.get_pax_departed()[0], 0)
		ASSERT_EQUAL(factory.get_pax_arrived()[0], 0)
		ASSERT_EQUAL(factory.get_mail_generated()[0], 0)
		ASSERT_EQUAL(factory.get_tile_list().len(), 3 * 3)
		ASSERT_EQUAL(factory.get_halt_list().len(), 0)
		ASSERT_FALSE(factory.is_transformer_connected())
		ASSERT_EQUAL(factory.get_transformer(), null)
		ASSERT_EQUAL(factory.get_field_count(), 0)
		ASSERT_EQUAL(factory.get_min_field_count(), 0)

		ASSERT_TRUE(factory.input.len() == 0)
		ASSERT_TRUE(factory.output.len() == 0)
	}

	{
		// and remove factory; only public player can remove them
		ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(3, 4, 0)), "Der Besitzer erlaubt das Entfernen nicht")
		ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(3, 4, 0)), null)
	}

	RESET_ALL_PLAYER_FUNDS()
}


function test_factory_build_climate()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local setclimate = command_x(tool_set_climate)

	local allowed_climates = [                                                cl_temperate, cl_tundra, cl_rocky, cl_arctic ]
	local all_climates     = [ cl_water, cl_desert, cl_tropic, cl_mediterran, cl_temperate, cl_tundra, cl_rocky, cl_arctic ]

	{
		foreach (cl in all_climates) {
			ASSERT_EQUAL(setclimate.work(pl, coord3d(0, 0, 0), coord3d(7, 7, 0), "" + cl), null)

			local err = build_factory(pl, coord3d(3, 4, 0), false /*don't ignore climates*/, 1, 1024, "Kohlegrube")

			if (allowed_climates.find(cl) != null) {
				ASSERT_EQUAL(err, null)
				ASSERT_EQUAL(factory_list_x().get_count(), 1)

				// and remove it
				ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(3, 4, 0)), null)
			}
			else {
				ASSERT_EQUAL(err, "No suitable ground!")
				ASSERT_EQUAL(factory_list_x().get_count(), 0)
			}

			RESET_ALL_PLAYER_FUNDS()
		}
	}

	// and ignore climates
	{
		foreach (cl in all_climates) {
			ASSERT_EQUAL(setclimate.work(pl, coord3d(0, 0, 0), coord3d(7, 7, 0), "" + cl), null)

			local err = build_factory(pl, coord3d(3, 4, 0), 1 /*ignore climates*/, 1, 1024, "Kohlegrube")

			if (cl != cl_water) {
				ASSERT_EQUAL(err, null)
				ASSERT_EQUAL(factory_list_x().get_count(), 1)

				// and remove it
				ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(3, 4, 0)), null)
			}
			else {
				ASSERT_EQUAL(err, "No suitable ground!")
				ASSERT_EQUAL(factory_list_x().get_count(), 0)

			}

			RESET_ALL_PLAYER_FUNDS()
		}

		ASSERT_EQUAL(setclimate.work(pl, coord3d(0, 0, 0), coord3d(7, 7, 0), "" + cl_mediterran), null)
		RESET_ALL_PLAYER_FUNDS()
	}

	// partially on disallowed climate
	{
		ASSERT_EQUAL(setclimate.work(pl, coord3d(0, 0, 0), coord3d(3, 7, 0), "" + cl_temperate), null)
		ASSERT_EQUAL(setclimate.work(pl, coord3d(4, 0, 0), coord3d(7, 7, 0), "" + cl_mediterran), null)

		ASSERT_EQUAL(build_factory(pl, coord3d(3, 4, 0), false /*don't ignore climates*/, 1, 1024, "Kohlegrube"), "No suitable ground!")
		ASSERT_EQUAL(build_factory(pl, coord3d(3, 4, 0), true  /*ignore climates*/,       1, 1024, "Kohlegrube"), null)

		ASSERT_EQUAL(factory_list_x().get_count(), 1)

		// and remove it
		ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(3, 4, 0)), null)
	}

	// clean up
	ASSERT_EQUAL(setclimate.work(pl, coord3d(0, 0, 0), coord3d(7, 7, 0), "" + cl_mediterran), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_factory_build_on_water_occupied()
{
	local dims = factory_desc_x("Oelbohrinsel").building_desc.get_size(0);
	ASSERT_TRUE(dims.x > 1)
	ASSERT_TRUE(dims.y > 1)

	// make water
	ASSERT_EQUAL(command_x(tool_set_climate).work(player_x(1), coord3d(0,0,0), coord3d(8,8,0), "" + cl_water), null);

	// make factory
	ASSERT_EQUAL(build_factory(player_x(1), coord3d(4,4,0), 1, 1, 1000, "Oelbohrinsel"), null)

	{
		// check all 4 corners
		ASSERT_EQUAL(build_factory(player_x(1), coord3d(3,3,0), 1, 1, 1000, "Oelbohrinsel"), "No suitable ground!")
		ASSERT_EQUAL(build_factory(player_x(1), coord3d(5,3,0), 1, 1, 1000, "Oelbohrinsel"), "No suitable ground!")
		ASSERT_EQUAL(build_factory(player_x(1), coord3d(5,5,0), 1, 1, 1000, "Oelbohrinsel"), "No suitable ground!")
		ASSERT_EQUAL(build_factory(player_x(1), coord3d(3,5,0), 1, 1, 1000, "Oelbohrinsel"), "No suitable ground!")
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(player_x(1), coord3d(4,4,0)), null)
	ASSERT_EQUAL(command_x(tool_set_climate).work(player_x(1), coord3d(0,0,0), coord3d(8,8,0), "" + cl_mediterran), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_factory_build_with_fields()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local setclimate = command_x(tool_set_climate)

	// build normal
	{
		ASSERT_EQUAL(build_factory(pl, coord3d(3, 4, 0), 0, 1, 1024, "PVkraftwerk"), null)
		ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(3, 4, 0)), null)
	}

	// build in non-suitable climate
	{
		ASSERT_EQUAL(setclimate.work(pl, coord3d(0, 0, 0), coord3d(7, 7, 0), "" + cl_arctic), null)
		ASSERT_EQUAL(build_factory(pl, coord3d(3, 4, 0), 0, 1, 1024, "PVkraftwerk"), "No suitable ground!")
	}

	// Force building in non-suitable climate, factory should have no fields
	{
		ASSERT_EQUAL(build_factory(pl, coord3d(3, 4, 0), 1, 1, 1024, "PVkraftwerk"), null)

		for (local y = 0; y < 8; ++y) {
			for (local x = 0; x < 8; ++x) {
				ASSERT_EQUAL(tile_x(x, y, 0).find_object(mo_field), null)
			}
		}

		ASSERT_EQUAL(factory_list_x().get_count(), 1)

		// and remove it
		ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(3, 4, 0)), null)
	}

	// clean up
	ASSERT_EQUAL(setclimate.work(pl, coord3d(0, 0, 0), coord3d(7, 7, 0), "" + cl_mediterran), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_factory_link()
{
	local pl = player_x(0)
	local public_pl = player_x(1)

	local production = 1024 // 1/s

	// build coal mine + coal power plant
	ASSERT_EQUAL(build_factory(public_pl, coord3d(0, 0, 0), 1, 1, production, "Kohlegrube"), null)
	ASSERT_EQUAL(build_factory(public_pl, coord3d(6, 6, 0), 1, 1, production, "Kohlekraftwerk"), null)

	local mine = factory_x(0, 0)
	local pp = factory_x(6, 6)

	ASSERT_TRUE(mine != null)
	ASSERT_TRUE(pp != null)

	// test pre-conditions
	{
		ASSERT_EQUAL(mine.get_consumers().len(), 0)
		ASSERT_EQUAL(mine.get_suppliers().len(), 0)
// 		ASSERT_EQUAL(mine.get_production()[0], 0) // FIXME this is 1024 when using the -scenario switch
		ASSERT_EQUAL(mine.get_power()[0], 0)
		ASSERT_TRUE(mine.input.len() == 0)
		ASSERT_TRUE(mine.output.len() == 1)

		local out = mine.output.Kohle
		ASSERT_TRUE(out != null)
// 		ASSERT_EQUAL(out.get_storage()[0], 0) // FIXME this is 122 when using the -scenario switch
		ASSERT_EQUAL(out.get_received()[0], 0)
		ASSERT_EQUAL(out.get_consumed()[0], 0)
		ASSERT_EQUAL(out.get_in_transit()[0], 0)
		ASSERT_EQUAL(out.get_delivered()[0], 0)
		ASSERT_EQUAL(out.get_produced()[0], 0)
		ASSERT_EQUAL(out.get_base_production(), production)
		ASSERT_EQUAL(out.get_base_consumption(), production)
		ASSERT_EQUAL(out.get_production_factor(), 100)
		ASSERT_EQUAL(out.max_storage, 122)

		{
			local error_caught = false
			try {
				ASSERT_EQUAL(out.get_consumption_factor(), 0)
			}
			catch (e) {
				ASSERT_EQUAL(e, "No input slot [0] in factory at (0,0,0)")
				error_caught = true
			}
			ASSERT_TRUE(error_caught)
		}

		ASSERT_EQUAL(pp.get_consumers().len(), 0)
		ASSERT_EQUAL(pp.get_suppliers().len(), 0)
		ASSERT_EQUAL(pp.get_production()[0], 0)
		ASSERT_EQUAL(pp.get_power()[0], 0)
		ASSERT_TRUE(pp.input.len() == 1)
		ASSERT_TRUE(pp.output.len() == 0)

		local inp = pp.input.Kohle
		ASSERT_TRUE(inp != null)
		ASSERT_EQUAL(inp.get_storage()[0], 0)
		ASSERT_EQUAL(inp.get_received()[0], 0)
		ASSERT_EQUAL(inp.get_consumed()[0], 0)
		ASSERT_EQUAL(inp.get_in_transit()[0], 0)
		ASSERT_EQUAL(inp.get_delivered()[0], 0)
		ASSERT_EQUAL(inp.get_produced()[0], 0)
		ASSERT_EQUAL(inp.get_base_production(), production)
		ASSERT_EQUAL(inp.get_base_consumption(), production)
		ASSERT_EQUAL(inp.get_consumption_factor(), 100)
		ASSERT_EQUAL(inp.max_storage, 1153)

		{
			local error_caught = false
			try {
				ASSERT_EQUAL(inp.get_production_factor(), 0)
			}
			catch (e) {
				ASSERT_EQUAL(e, "No output slot [-1] in factory at (6,6,0)")
				error_caught = true
			}
			ASSERT_TRUE(error_caught)
		}
	}

	// link first factory with nothing - should fail
	{
		ASSERT_EQUAL(command_x(tool_link_factory).work(pl, coord3d(0, 0, 0), coord3d(15, 15, 0), ""), "")

		ASSERT_EQUAL(mine.get_consumers().len(), 0)
		ASSERT_EQUAL(mine.get_suppliers().len(), 0)

		ASSERT_EQUAL(pp.get_consumers().len(), 0)
		ASSERT_EQUAL(pp.get_suppliers().len(), 0)
	}

	// link factory with itself - should fail
	{
		ASSERT_EQUAL(command_x(tool_link_factory).work(pl, coord3d(0, 0, 0), coord3d(0, 0, 0), ""), "")

		ASSERT_EQUAL(mine.get_consumers().len(), 0)
		ASSERT_EQUAL(mine.get_suppliers().len(), 0)

		ASSERT_EQUAL(pp.get_consumers().len(), 0)
		ASSERT_EQUAL(pp.get_suppliers().len(), 0)
	}

	// link factories
	{
		// default_param is necessary even though it is not used
		ASSERT_EQUAL(command_x(tool_link_factory).work(pl, coord3d(0, 0, 0), coord3d(6, 6, 0), ""), null)

		ASSERT_EQUAL(mine.get_consumers().len(), 1)
		ASSERT_EQUAL(mine.get_suppliers().len(), 0)

		ASSERT_EQUAL(mine.get_consumers()[0].x, pp.x)
		ASSERT_EQUAL(mine.get_consumers()[0].y, pp.y)

		ASSERT_EQUAL(pp.get_consumers().len(), 0)
		ASSERT_EQUAL(pp.get_suppliers().len(), 1)

		ASSERT_EQUAL(pp.get_suppliers()[0].x, mine.x)
		ASSERT_EQUAL(pp.get_suppliers()[0].y, mine.y)
	}

	// second link should have no effect
	{
		ASSERT_EQUAL(command_x(tool_link_factory).work(pl, coord3d(0, 0, 0), coord3d(6, 6, 0), ""), null)

		ASSERT_EQUAL(mine.get_consumers().len(), 1)
		ASSERT_EQUAL(mine.get_suppliers().len(), 0)

		ASSERT_EQUAL(mine.get_consumers()[0].x, pp.x)
		ASSERT_EQUAL(mine.get_consumers()[0].y, pp.y)

		ASSERT_EQUAL(pp.get_consumers().len(), 0)
		ASSERT_EQUAL(pp.get_suppliers().len(), 1)

		ASSERT_EQUAL(pp.get_suppliers()[0].x, mine.x)
		ASSERT_EQUAL(pp.get_suppliers()[0].y, mine.y)
	}

	// unlink factories
	{
		local tool = command_x(tool_link_factory)
		tool.set_flags(2) // ctrl == unlink
		ASSERT_EQUAL(tool.work(pl, coord3d(0, 0, 0), coord3d(7, 7, 0), ""), null)

		ASSERT_EQUAL(mine.get_consumers().len(), 0)
		ASSERT_EQUAL(mine.get_suppliers().len(), 0)

		ASSERT_EQUAL(pp.get_consumers().len(), 0)
		ASSERT_EQUAL(pp.get_suppliers().len(), 0)
	}

	// second unlink should have no effect
	{
		local tool = command_x(tool_link_factory)
		tool.set_flags(2) // ctrl == unlink
		ASSERT_EQUAL(tool.work(pl, coord3d(0, 0, 0), coord3d(7, 7, 0), ""), null)

		ASSERT_EQUAL(mine.get_consumers().len(), 0)
		ASSERT_EQUAL(mine.get_suppliers().len(), 0)

		ASSERT_EQUAL(pp.get_consumers().len(), 0)
		ASSERT_EQUAL(pp.get_suppliers().len(), 0)
	}

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(0, 0, 0)), null)
	ASSERT_EQUAL(command_x(tool_remover).work(public_pl, coord3d(7, 7, 0)), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_factory_desc()
{
	local desc = factory_desc_x("Raffinerie")
	local inp  = desc.get_inputs()
	local out  = desc.get_outputs()

	ASSERT_EQUAL(desc.get_name(), "Raffinerie")
	ASSERT_EQUAL(desc.is_electricity_producer(), false)
	ASSERT_EQUAL(inp.len(), 1)
	ASSERT_EQUAL(out.len(), 3)

	{
		local inp_goods = []
		for(local i=0; i<inp.len(); i++) {
			inp_goods.append(inp[i].good.get_name())
		}
		ASSERT_TRUE(inp_goods.find("Oel")  != null)
		ASSERT_TRUE(inp_goods.find("Holz") == null)
	}

	{
		local out_goods = []
		for(local i=0; i<out.len(); i++) {
			out_goods.append(out[i].good.get_name())
		}
		ASSERT_TRUE(out_goods.find("Chemicals")   != null)
		ASSERT_TRUE(out_goods.find("Plastik")     != null)
		ASSERT_TRUE(out_goods.find("PrintersInk") != null)
		ASSERT_TRUE(out_goods.find("Holz") == null)
	}

	{
		local list = factory_desc_x.get_list()
		ASSERT_TRUE("Aufwindkraftwerk" in list)
	}
}
