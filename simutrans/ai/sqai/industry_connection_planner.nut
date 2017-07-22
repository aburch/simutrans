
function abs(a) { return a >= 0 ? a : -a }


class industry_connection_planner_t extends node_t
{
	fsrc = null       // factory_x
	fdest = null      // factory_x
	freight = null    // string
	prod = -1   	// integer

	// planned stuff
	planned_way = null      // way_desc_x
	planned_station = null  // building_desc_x
	planned_depot = null    // building_desc_x
	planned_convoy = null   // prototyper_t
	plan_report = null      // report_t

	constructor(s,d,f)
	{
		base.constructor("industry_connection_planner_t");
		fsrc = s; fdest = d; freight = f;
	}

	function step()
	{
		debug = true
		local tic = get_ops_total();

		dbgprint("Plan link for " + freight + " from " + fsrc.get_name() + " at " + fsrc.x + "," + fsrc.y + " to "+ fdest.get_name() + " at " + fdest.x + "," + fdest.y)

		// TODO check if factories are still existing
		// TODO check if connection is plannable

		// compute monthly production
		if (prod < 0) {
			prod = calc_production()
		}
		dbgprint("production = " + prod);

		// plan convoy prototype
		local prototyper = prototyper_t(wt_road, freight)

		prototyper.max_vehicles = 4
		prototyper.min_speed = 1
		prototyper.max_length = 1

		local cnv_valuator = valuator_simple_t()
		cnv_valuator.wt = wt_road
		cnv_valuator.freight = freight
		cnv_valuator.volume = prod
		cnv_valuator.max_cnvs = 200
		cnv_valuator.distance = abs(fsrc.x-fdest.x) + abs(fsrc.y-fdest.y)

		local bound_valuator = valuator_simple_t.valuate_monthly_transport.bindenv(cnv_valuator)
		prototyper.valuate = bound_valuator

		if (planned_convoy == null) {
			if (prototyper.step().has_failed()) {
				dbgprint("Set link for " + freight + " from " + fsrc.get_name() + " at " + fsrc.x + "," + fsrc.y + " to "+ fdest.get_name() + " at " + fdest.x + "," + fdest.y + " to MISSING")

				industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_missing)
				return r_t(RT_TOTAL_FAIL)
			}

			planned_convoy = prototyper.best
		}

		// fill in report when best way is found
		local r = report_t()
		// plan way
		if (planned_way == null) {
			local way_list = way_desc_x.get_available_ways(wt_road, st_flat)
			local best_way = null
			local best = null

			foreach(way in way_list) {
				cnv_valuator.way_maintenance = way.get_maintenance()
				cnv_valuator.way_max_speed   = way.get_topspeed()

				local test = cnv_valuator.valuate_monthly_transport(planned_convoy)
				if (best == null  ||  test > best) {
					best = test
					best_way = way
				}
			}
			dbgprint("Best value = " + best + " way = " + best_way.get_name())

			// valuate again with best way
			cnv_valuator.way_maintenance = 0
			cnv_valuator.way_max_speed   = best_way.get_topspeed()

			r.gain_per_m  = cnv_valuator.valuate_monthly_transport(planned_convoy)
			r.nr_convoys = planned_convoy.nr_convoys

			planned_way = best_way
		}


		// plan station
		if (planned_station == null) {
			local station_list = building_desc_x.get_available_stations(building_desc_x.station, wt_road, good_desc_x(freight))
			select_station(station_list)
		}
		// plan depot
		if (planned_depot == null) {
			local depot_list = building_desc_x.get_available_stations(building_desc_x.depot, wt_road, good_desc_x(freight))

			if (depot_list.len()) {
				planned_depot = depot_list[0]
			}
		}

		if (planned_convoy == null  ||  planned_way == null || planned_station == null || planned_depot == null) {

			dbgprint("Set link for " + freight + " from " + fsrc.get_name() + " at " + fsrc.x + "," + fsrc.y + " to "+ fdest.get_name() + " at " + fdest.x + "," + fdest.y + " to MISSING")

			industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_missing)
			return r_t(RT_TOTAL_FAIL)
		}

		// successfull - complete report
		r.cost_fix     = cnv_valuator.distance * planned_way.get_cost() + 2*planned_station.get_cost() + planned_depot.get_cost()
		r.cost_monthly = cnv_valuator.distance * planned_way.get_maintenance() + 2*planned_station.get_maintenance() + planned_depot.get_maintenance()
		r.gain_per_m  -= r.cost_monthly

		// create action node
		local cn = road_connector_t()
		cn.fsrc = fsrc
		cn.fdest = fdest
		cn.freight = freight
		cn.planned_way = planned_way
		cn.planned_station = planned_station
		cn.planned_depot = planned_depot
		cn.planned_convoy = planned_convoy

		r.action = cn

		// that's the report
		plan_report = r

		dbgprint("Plan: way = " + planned_way.get_name() + ", station = " + planned_station.get_name() + ", depot = " + planned_depot.get_name());
		dbgprint("Report: gain_per_m  = " + r.gain_per_m + ", nr_convoys  = " + r.nr_convoys + ", cost_fix  = " + r.cost_fix + ", cost_monthly  = " + r.cost_monthly)
		dbgprint("Report: dist = " + cnv_valuator.distance+ " way_cost = " + planned_way.get_cost())
		dbgprint("Report: station = " + planned_station.get_cost()+ " depot = " + planned_depot.get_cost())

		// deliver it
		local r = r_t(RT_READY)
		r.report = plan_report

		local toc = get_ops_total();
		print("industry_connection_planner wasted " + (toc-tic) + " ops")
		return r
	}


	function calc_production()
	{
		local src_prod = fsrc.output[freight].get_base_production();
		local dest_con = fdest.input[freight].get_base_consumption();

		// TODO implement production factors

		dbgprint("production = " + src_prod + " / " + dest_con);
		return min(src_prod,dest_con)
	}

	function select_station(list)
	{
		local station_length = (planned_convoy.length + CARUNITS_PER_TILE - 1) / CARUNITS_PER_TILE
		local capacity       =  planned_convoy.capacity

		local station_capacity = 0
		local station_is_terminus = false

		foreach(station in list) {
			local ok = (planned_station == null)
			local s_capacity = station_length * station.get_capacity()

			if (!ok  &&  station_length == 1) {
				// prefer terminus
				ok = station.is_terminus()  &&  !station_is_terminus
				if (!ok) {
					// then prefer stations with enough capacity
					ok = station_capacity < capacity ? station_capacity < s_capacity
					                                 : capacity < s_capacity  &&  s_capacity < station_capacity
				}
			}
			if (station_length >  1) {
				// force non-terminus
				ok = !station_is_terminus
			}
			if (ok) {
				planned_station = station
				station_capacity = station.get_capacity()
				station_is_terminus = station.is_terminus()
			}
		}
	}
}
