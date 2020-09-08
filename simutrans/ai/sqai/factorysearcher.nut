/**
 * Class that searches for usefull unbuild factory connections.
 * Depending on player number uses two different methods:
 * 1) Method that tries to complete a factory tree (taken from the c++ implementation of the freight AI in player/ai_goods.cc
 * 2) A demand-driven method.
 */
class factorysearcher_t extends manager_t
{
	froot = null    // factory_x, complete this tree
	method = -1
	factory_iterator = null

	constructor()
	{
		base.constructor("factorysearcher_t")
		debug = false
		::factorysearcher = this
	}

	function get_next_end_consumer()
	{
		// iterate the factory_iterator, which is a generator
		if (factory_iterator == null  ||  typeof(factory_iterator) != "generator") {
			// this is a generator
			factory_iterator = factory_iteration()
		}
		if (factory_iterator.getstatus() != "dead") {
			return resume factory_iterator
		}
		factory_iterator = null
		return null
	}

	function factory_iteration()
	{
		local list = factory_list_x()
		foreach(factory in list) {
			if (factory.output.len() == 0) {
				yield factory
			}
		}
	}

	function work()
	{
		if (method < 0) {
			method = our_player_nr % 2;
			froot = null
		}

		if (method == 0) {
			// traditional method, taken from C++ implementation
			// of Freight AI

			// root still has missing links?
			if (froot  &&  count_missing_factories(froot) <= 0) {
				froot = null
			}
			// determine new root
			if (froot == null) {
				// find factory with incomplete connected tree
				local min_mfc = 10000;

				local fab
				while(fab = get_next_end_consumer()) {

					local n = count_missing_factories(fab)

					if ((n > 0)  &&  (n < min_mfc)) {
						// TODO add some random here
						min_mfc = n
						froot = fab
					}
				}
				if (froot) {
					local fab = froot
					dbgprint("Choose consumer " + fab.get_name() + " at " + fab.x + "," + fab.y + ", which has " + min_mfc + " missing links")
				}
			}

			// nothing found??
			if (froot==null) return r_t(RT_DONE_NOTHING);

			dbgprint("Connect  " + froot.get_name() + " at " + froot.x + "," + froot.y)

			// find link to connect
			if (!plan_missing_link(froot)) {
				dbgprint(".. no missing link")
				// no missing link found - reset froot
				froot = null
				return r_t(RT_SUCCESS)
			}
			return r_t(RT_PARTIAL_SUCCESS)
		}
		else {
			// demand-driven method

			// plan root tree
			if (froot  &&  plan_increase_consumption(froot) <= 0) {
				froot = null
			}

			// determine new root
			if (froot == null) {
				local fab
				if (fab = get_next_end_consumer()) {
					local n = plan_increase_consumption(fab)

					return r_t( n>0  ? RT_PARTIAL_SUCCESS : RT_SUCCESS)
				}
			}
		}
		return r_t(RT_SUCCESS)
	}

	/**
	 * Creates the planner node
	 */
	static function plan_connection(fsrc, fdest, freight)
	{
		if (industry_manager.get_link_state(fsrc, fdest, freight) != industry_link_t.st_free) {
			dbgprint("Link for " + freight + " from " + fsrc.get_name() + " at " + fsrc.x + "," + fsrc.y + " to "+ fdest.get_name() + " at " + fdest.x + "," + fdest.y + " state is " + industry_manager.get_link_state(fsrc, fdest, freight) );
			return false
		}
		dbgprint("Close link for " + freight + " from " + fsrc.get_name() + " at " + fsrc.x + "," + fsrc.y + " to "+ fdest.get_name() + " at " + fdest.x + "," + fdest.y)

		industry_manager.set_link_state(fsrc, fdest, freight, industry_link_t.st_planned);

		local icp = industry_connection_planner_t(fsrc, fdest, freight);

		append_child(icp)
		return true
	}

	/**
	 * @returns -1 if factory tree is incomplete, otherwise number of missing connections
	 */
	// TODO cache the results per factory
	static function count_missing_factories(fab, indent = "")
	{
		// source of raw material?
		if (fab.input.len() == 0) return 0;

		local end_consumer = fab.output.len() == 0

		// build list of supplying factories
		local suppliers = [];
		foreach(c in fab.get_suppliers()) {
			suppliers.append( factory_x(c.x, c.y) );
		}

		local count = 0;
		local g_atleastone = false;
		// iterate all input goods and search for supply
		foreach(good, islot in fab.input) {

			// test for in-storage or in-transit goods
			local st = islot.get_storage()
			local it = islot.get_in_transit()
			if (st[0] + st[1] + it[0] + it[1] > 0) {
				// something stored/in-transit in last and current month
				// no need to search for more supply
				g_atleastone = true
				continue
			}

			// there is a complete tree to produce this good
			local g_complete = false;
			// minimum of missing links for one input good
			local g_count    = 10000;
			foreach(s in suppliers) {

				if (good in s.output) {
					// check state of connection
					local state = industry_manager.get_link_state(s, fab, good);

					if (state == industry_link_t.st_failed) {
						continue // foreach
					}
					if (state != industry_link_t.st_free) {
						// planned / built -> nothing missing
						g_complete = true
						g_count = 0
						continue
					}

					local n = count_missing_factories(s, indent + "  ");
					if ( n<0) {
						// incomplete tree
					}
					else {
						// complete tree
						g_complete = true;
						g_count = min(g_count, n+1)
					}
				}
			}

			if (!g_complete  &&  !end_consumer) {
				dbgprint(indent + "No supply of " + good + " for " + fab.get_name())
				// no suppliers for this good
				return -1
			}
			g_atleastone = g_atleastone || g_complete

			if (!end_consumer) {
				count += g_count // sum missing links
			}
			else {
				if (g_count > 0  &&  (count == 0  ||  g_count < count)) {
					count = g_count;
				}
			}
			dbgprint(indent + "Supply of " + good + " for " + fab.get_name() + " has " + g_count + " missing links")
		}

		if (end_consumer  &&  !g_atleastone) {
			dbgprint(indent + "No supply for " + fab.get_name())
			count = -1
		}

		dbgprint(indent + "Factory " + fab.get_name() + " at " + fab.x + "," + fab.y + " has " + count + " missing links")
		return count
	}

	/**
	 * find link to connect in tree of factory @p fab.
	 * sets fsrc, fdest, lgood if true was returned
	 * @returns true if link is found
	 */
	function plan_missing_link(fab, indent = "")
	{
		dbgprint(indent + "Missing link for factory " + fab.get_name() + " at " + fab.x + "," + fab.y)
		// source of raw material?
		if (fab.input.len() == 0) return false;

		// build list of supplying factories
		local suppliers = [];
		foreach(c in fab.get_suppliers()) {
			suppliers.append( factory_x(c.x, c.y) );
		}

		local count = 0;
		// iterate all input goods and search for supply
		foreach(good, islot in fab.input) {
			// check for current supply
			if ( 4*(islot.get_storage()[0] + islot.get_in_transit()[0]) > islot.max_storage) {
				dbgprint(indent + ".. enough supply of " + good)
				continue
			}
			// find suitable supplier
			foreach(s in suppliers) {

				if ( !(good in s.output)) continue;

				// connection forbidden? planned? built?
				local state = industry_manager.get_link_state(s, fab, good)
				if (state != industry_link_t.st_free) {
					if (state == industry_link_t.st_built  ||  state == industry_link_t.st_planned) {
						dbgprint(indent + ".. connection for " + good + " from " + s.get_name() + " to " + fab.get_name() + " already "
							   + (state == industry_link_t.st_built ? "built" : "planned") )
						break
					}
					continue // if connection state is 'failed'
				}

				local oslot = s.output[good]

				dbgprint(indent + ".. Factory " + s.get_name() + " at " + s.x + "," + s.y + " supplies " + good)

				if (8*oslot.get_storage()[0] > oslot.max_storage  ||  !plan_missing_link(s, indent + "  ")) {
					// this is our link
					dbgprint(indent + ".. plan this connection")
					plan_connection(s, fab, good)
				}
				return true
			}
		}
		return false // all links are connected
	}

	/**
	 * Estimates additional possible consumption at this end-consumer factory
	 */
	function plan_increase_consumption(fab, indent = "")
	{
		// initialize search
		if (fab.output.len() > 0) {
			return 0
		}

		local planned = 0;
		foreach(good, islot in fab.input) {
			local tree = estimate_consumption(fab, good)

			// now do some greedy selection: for each producer select enough suppliers
			planned += plan_consumption_connection(tree, fab, good)
		}
		return planned
	}

	function plan_consumption_connection(tree, fdest, freight, indent = "")
	{
		local planned = 0;
		local needed = tree.increase
		while (needed > 0) {
			local best = null
			local best_supply = 0
			foreach(supplier in tree.suppliers) {
				local supply = supplier.supply
				dbgprint(indent + "Needed " + needed + " Provided " + supply )
				if (supply > needed  ? (best_supply == 0  ||  supply < best_supply)  :  supply > best_supply) {
					best = supplier
					best_supply = supply
				}
			}
			// go down in tree
			foreach(good, supplier_slot in best.inputs) {
				planned += plan_consumption_connection(supplier_slot, best.supplier, good, indent + "  ")
				dbgprint(indent + "Planned for " + best_supply + " (total = " + planned + ")")
			}
			// plan this connection
			if (planned==0) {
				if (plan_connection(best.supplier, fdest, freight)) {
					dbgprint(indent + "Planned to consumer ")
					planned++
				}
			}
			// disable this tree
			needed -= best_supply
			best.supply = 0
		}
		return planned
	}

	/**
	 * Estimates additional possible consumption of good at this factory
	 *
	 * Returns tree:
	 * tree.increase	- Potential local consumption
	 * tree.supply	- Potential supply
	 * tree.basec	- Base consumption
	 * tree.suppliers	- Array of supplier nodes
	 * 		[].supplier	- Factory
	 * 		[].supply	- Potential production of supplier	} - produced by estimate_production
	 * 		[].inputs	- Table Good -> Consumer tree		}
	 *
	 * @returns estimated consumption increase
	 */
	static function estimate_consumption(fab, prod = null, indent = "")
	{

		dbgprint(indent + "Estimates for consumption of " + prod + " at factory " + fab.get_name() + " at " + fab.x + "," + fab.y)

		// estimate max consumption
		local islot = fab.input[prod]
		local max_c = islot.get_base_consumption()
		// estimate actual consumpion
		local est_c = estimate_actual_consumption(islot)

		local increase = max_c - est_c
		dbgprint(indent + "  potential increase of consumption of " + prod + " is " + increase)

		// iterate suppliers:
		// calculate potential additional supply (and build it)

		local tree = { suppliers = [], basec = max_c }

		// search for supply
		local supply = 0
		foreach(c in fab.get_suppliers()) {
			local s = factory_x(c.x, c.y)

			if (prod in s.output) {

				local state = industry_manager.get_link_state(s, fab, prod)
				if (state == industry_link_t.st_planned  ||  state == industry_link_t.st_failed) {
					// failed / planned -> no improvement possible
					dbgprint(indent + "  transport link for  " + prod + " is failed/planned" )
					continue
				}
				local s_tree = estimate_production(s, prod, state == industry_link_t.st_built, indent + "  ")

				s_tree.supplier <- s

				local more = s_tree.supply
				supply += more

				tree.suppliers.append(s_tree)

				dbgprint(indent + "  potential increase of " + prod + " is " + more + " (state =" + state + ")")
			}
		}
		dbgprint(indent + "  total additional supply of " + prod + " is " + supply)

		tree.increase <- min(increase, supply)
		tree.supply   <- supply

		return tree
	}

	/**
	 * Estimates additional possible production of good at this factory
	 * @returns tree, see estimate_consumption
	 */
	static function estimate_production(fab, prod, exists, indent = "")
	{
		dbgprint(indent + "Estimates for production of " + prod + " at factory " + fab.get_name() + " at " + fab.x + "," + fab.y)

		local oslot = fab.output[prod]
		local fac   = oslot.get_production_factor()
		local est_p = estimate_actual_production(oslot, exists)

		local increase = oslot.get_base_production() - est_p
		dbgprint(indent + "  potential increase of production of " + prod + " is " + increase)

		// build tree for later planning
		local tree = { inputs = {}, supply = 0 }

		if (increase <= 0) {
			return tree
		}

		if (fab.input.len() == 0) {
			// producer of raw materials
			tree.supply = ( increase*fac)/100;
			return tree
		}
		// iterate all input goods and search for supply
		foreach(good, islot in fab.input) {

			local c_tree = estimate_consumption(fab, good, indent + "  ")
			tree.inputs[good] <- c_tree

			local con = c_tree.increase
			local est = (con * fac)/ islot.get_consumption_factor()

			increase = min(increase, est)
		}
		tree.supply = increase
		return tree
	}

	static function estimate_actual_consumption(islot)
	{
		local con = islot.get_consumed()
		local isnew = (con.reduce(sum) - con[0]) == 0;
		if (!isnew) {
			// established connection: report max
			return con.reduce(max)
		}
		else {
			if (con[0] == 0) {
				// non-existing connection
				return 0
			}
			else {
				// new connection: report base
				return islot.get_base_consumption()
			}
		}
	}

	static function estimate_actual_production(oslot, exists)
	{
		local pro = oslot.get_produced()
		local isnew = (pro.reduce(sum) - pro[0]) == 0;

		if (!isnew) {
			// established connection: report max
			return pro.reduce(max)
		}
		else {
			if (pro[0] == 0  &&  !exists) {
				// non-existing connection
				return 0
			}
			else {
				// new connection: report base
				return oslot.get_base_consumption()
			}
		}
	}
}
