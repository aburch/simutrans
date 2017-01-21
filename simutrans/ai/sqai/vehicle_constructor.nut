class vehicle_constructor_t extends node_t
{
	// input data
	p_depot  = null // depot_x
	p_line   = null // line_x
	p_convoy = null // prototyper_t
	p_count  = 0
	p_withdraw = false

	// generated data
	c_cnv = null
	c_wt  = 0
	// step-by-step construct convoys
	phase = 0

	constructor()
	{
		base.constructor("vehicle_constructor_t")
		debug = true
	}

	function step()
	{
		local pl = player_x(our_player)
		local tic = get_ops_total();

		c_wt = p_convoy.veh[0].get_waytype()

		switch(phase) {
			case 0: // create the convoy (and the first vehicles)
				{
					p_depot.append_vehicle(pl, convoy_x(0), p_convoy.veh[0])

					// find the newly created convoy
					// it should be the last in the list
					local cnv_list = p_depot.get_convoy_list()

					local trythis = cnv_list[cnv_list.len()-1]
					if (check_convoy(trythis)) {
						c_cnv = trythis
					}

					if (c_cnv == null) {
						foreach(cnv in cnv_list) {
							if (check_convoy(cnv)) {
								c_cnv = cnv
								break
							}
						}
					}
					phase ++
				}
			case 1: // complete the convoy
				{
					local vlist = c_cnv.get_vehicles()
					while (vlist.len() < p_convoy.veh.len())
					{
						p_depot.append_vehicle(pl, c_cnv, p_convoy.veh[ vlist.len() ])
						vlist = c_cnv.get_vehicles()
					}

					phase ++
				}
			case 2: // set line
				{
					c_cnv.set_line(pl, p_line)
					phase ++
				}
			case 3: // withdraw old vehicles
				{
					if (p_withdraw) {
						local cnv_list = p_line.get_convoy_list()
						foreach(o_cnv in cnv_list) {
							if (o_cnv.id != c_cnv.id  &&  !o_cnv.is_withdrawn()) {
								o_cnv.toggle_withdraw(pl)
							}
						}
						p_withdraw = false
					}
					phase ++
				}
			case 4: // start
				{
					p_depot.start_convoy(pl, c_cnv)

					p_count --
					if (p_count > 0) {
						phase = 0
						return r_t(RT_PARTIAL_SUCCESS)
					}
					else {
						phase ++
					}
				}
		}
		return r_t(RT_TOTAL_SUCCESS)
	}

	function check_convoy(cnv)
	{
		// check whether this convoy is for our purpose
		if (cnv.get_line() == null  &&  cnv.get_waytype() == c_wt) {
			// now test for equal vehicles
			local vlist = cnv.get_vehicles()
			local len = vlist.len()
			if (len <= p_convoy.veh.len()) {
				local equal = true;

				for (local i=0; equal  &&  i<len; i++) {
					equal = vlist[i].is_equal(p_convoy.veh[i])
				}
				if (equal) {
					return true
				}
			}
		}
		return false
	}
}