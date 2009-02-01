#include "vehikel_besch.h"

// Get running costs. Running costs increased if the vehicle is obsolete.
// @author: jamespetts
uint16
vehikel_besch_t::get_betriebskosten(karte_t* welt) const
{
	uint16 month_now = welt->get_current_month();
	int normal_running_costs = get_betriebskosten();
	if(!welt->use_timeline() || !is_retired(month_now))	
	{
		return normal_running_costs;
	}
	//Else
	uint16 base_obsolete_running_costs = normal_running_costs * (welt->get_einstellungen()->get_obsolete_running_cost_increase_percent() / 100);
	uint16 obsolete_date = get_retire_year_month();
	if((month_now - obsolete_date) >= (welt->get_einstellungen()->get_obsolete_running_cost_increase_phase_years() * 12))
	{
		return base_obsolete_running_costs;
	}
	//Else
	//If not at end of phasing period, apply only a proportion of the obsolescence cost increase.
	float tmp1 = base_obsolete_running_costs - normal_running_costs;
	float tmp2 = month_now - obsolete_date;
	float proportion = tmp2 / (welt->get_einstellungen()->get_obsolete_running_cost_increase_phase_years() * 12);
	return (tmp1 * proportion) + normal_running_costs;
}