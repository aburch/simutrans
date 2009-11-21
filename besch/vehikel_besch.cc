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

// Get running costs. Running costs increased if the vehicle is obsolete.
// Unscaled version for GUI purposes.
// @author: jamespetts
uint16 vehikel_besch_t::get_base_running_costs(karte_t* welt) const
{
	return calc_running_cost(welt, get_base_running_costs());
}

uint32 vehikel_besch_t::get_fixed_maintenance(karte_t *welt) const
{
	return calc_running_cost(welt, get_fixed_maintenance());
}

uint32 vehikel_besch_t::get_adjusted_monthly_fixed_maintenance(karte_t *welt) const
{
	return welt->calc_adjusted_monthly_figure(calc_running_cost(welt, get_fixed_maintenance()));
}


// GEAR_FACTOR: a gear of 1.0 is stored as 64
#define GEAR_FACTOR 64

inline bool is_track_way_type(waytype_t wt)
{
	switch (wt)
	{
		case track_wt:
		case water_wt:         
		case overheadlines_wt: 
		case monorail_wt:      
		case maglev_wt:
		case tram_wt:
		case narrowgauge_wt:
			return true;
	}
	return false;
}

/**
 * Get effective power index. 
 * Steam engine power depends on its speed.
 * Effective power in kW: power_index * welt->get_einstellungen()->get_global_power_factor() / 64
 * (method extracted from sint32 convoi_t::calc_adjusted_power())
 * @author Bernd Gabriel
 */
uint32 vehikel_besch_t::get_effective_power_index(uint16 speed /* in km/h */ ) const
{
	/*
	 * Calculate power according to the force calculation of the overhauled physics model.
	 * In fact this is nearly the *same* calculation due to formula: power = force * speed.
	 * @author: Bernd Gabriel, Nov, 01 2009: steam train related calculations no longer needed with overhauled physics in calc_acceleration()
	 */
	uint32 p = get_leistung(); // p in kW, get_leistung() in kW
	if (p > 0)
	{
		if (get_engine_type() == steam && is_track_way_type(get_waytype()))
		{
			uint16 v = get_geschw() / 2; // half speed in km/h, get_geschw() in km/h
			if (speed < v)
			{
				// the constant force 
				return (p * get_gear() * speed) / v;
			}
			// less force due to steam shortage and economics. Turns over to a constant power machine.
		}
		else 
		{
			// gear works up to full speed:
			uint16 gear10 = 10;
			if (speed < gear10)
			{
				return (uint32)((p * get_gear() * speed) / gear10);
			}
			// gear works at low speed only:
			//double gear10 = (10.0 * GEAR_FACTOR) / get_gear();
			//if (speed < gear10)
			//{
			//	return (uint32)((p * GEAR_FACTOR * speed) / gear10);
			//}
			//return p * GEAR_FACTOR;
		}
	}
	// constant power machine:
	return p * get_gear();

	//uint32 power = get_leistung();
	//uint32 power_index = power * get_gear();
	//if(get_engine_type() != vehikel_besch_t::steam)
	//{
	//	// Not a steam vehicle: effective power == nominal power 
	//	return power_index;
	//}
	//switch (get_waytype()) 
	//{
	//	// Adjustment of power only applies to steam vehicles
	//	// with direct drive (as in steam railway locomotives),
	//	// not steam vehicles with geared driving (as with steam
	//	// road vehicles) or propellor shaft driving (as with
	//	// water craft). Aircraft and maglev, although not
	//	// likely ever to be steam, cannot conceivably have
	//	// direct drive. 
	//case road_wt:
	//case water_wt:
	//case air_wt:
	//case maglev_wt:
	//case invalid_wt:
	//case ignore_wt:
	//	return power_index;
	//}

	//const uint16 max_speed = get_geschw();
	//const uint16 highpoint_speed = (max_speed >= 60) ? max_speed - 30 : 30;
	//
	//// Within 15% of top speed - locomotive less efficient
	//const uint16 high_speed = (max_speed * 85) / 100;
	//
	//if(highpoint_speed < current_speed && current_speed < high_speed)
	//{
	//	// going fast enough that it makes no difference, so the simple formula prevails.
	//	return power_index;
	//}
	//// Reduce the power at higher and lower speeds.
	//// Should be approx (for medium speed locomotive):
	////  40% power at 15kph 
	////  70% power at 25kph
	////  85% power at 32kph 
	//// 100% power at >50kph.
	//// See here for details: http://www.railway-technical.com/st-vs-de.shtml
	//
	//float speed_factor = 1.0F;	
	//if(power <= 500)
	//{
	//	speed_factor += (0.66F / 400.0F) * (450 - (sint32)power);
	//}

	//if (current_speed >= high_speed)
	//{
	//	// Must be within 15% of top speed here.
	//	float factor_modification = 0.15F * (max_speed - current_speed) / (float)(max_speed - high_speed);
	//	return (uint32)(power_index * speed_factor * (factor_modification + 0.8F));
	//}
	//
	////These values are needed to apply different power reduction factors
	////depending on the maximum speed.

	//uint16 lowpoint_speed = highpoint_speed / 3;

	//if(current_speed <= lowpoint_speed)
	//{
	//	return (uint32)(power_index * speed_factor * 0.4F);
	//}

	//uint16 midpoint_speed = lowpoint_speed * 2;
	//if(current_speed <= midpoint_speed)
	//{
	//	float factor_modification = 0.4F * (current_speed - lowpoint_speed) / (float)(midpoint_speed - lowpoint_speed);
	//	return (uint32)(power_index * speed_factor * (factor_modification + 0.4F));
	//}
	//// Not at high speed
	//float factor_modification = 0.15F * (current_speed - midpoint_speed) / (float)(highpoint_speed - lowpoint_speed);
	//return (uint32)(power_index * speed_factor * (factor_modification + 0.8F));
}
