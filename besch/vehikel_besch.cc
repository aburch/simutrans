#include "vehikel_besch.h"

uint32 vehikel_besch_t::calc_running_cost(const karte_t *welt, uint32 base_cost) const
{
	// No cost or no time line --> no obsolescence cost increase.
	if (base_cost == 0 || !welt->use_timeline())
	{
		return base_cost;
	}

	// I am not obsolete --> no obsolescence cost increase.
	sint32 months_of_obsolescence = welt->get_current_month() - get_retire_year_month();
	if (months_of_obsolescence <= 0)	
	{
		return base_cost;
	}

	// I am obsolete --> obsolescence cost increase.
	sint32 max_cost = base_cost * (welt->get_einstellungen()->get_obsolete_running_cost_increase_percent() / 100);
	if (max_cost == base_cost)
	{
		return max_cost;
	}

	// Current month is beyond the months_of_increasing_costs --> maximum increased obsolescence cost.
	sint32 months_of_increasing_costs = welt->get_einstellungen()->get_obsolete_running_cost_increase_phase_years() * 12;
	if (months_of_obsolescence >= months_of_increasing_costs)
	{
		return max_cost;
	}

	// Current month is within the months_of_increasing_costs --> proportionally increased obsolescence cost.
	return (uint32)((max_cost - base_cost) * (float)months_of_obsolescence / (float)months_of_increasing_costs) + base_cost;
}

// Get running costs. Running costs increased if the vehicle is obsolete.
// @author: jamespetts
uint16 vehikel_besch_t::get_betriebskosten(karte_t* welt) const
{
	return calc_running_cost(welt, get_betriebskosten());
}

uint32 vehikel_besch_t::get_fixed_maintenance(karte_t *welt) const
{
	return calc_running_cost(welt, get_fixed_maintenance());
}

uint32 vehikel_besch_t::get_adjusted_monthly_fixed_maintenance(karte_t *welt) const
{
	return welt->calc_adjusted_monthly_figure(calc_running_cost(welt, get_fixed_maintenance()));
}

