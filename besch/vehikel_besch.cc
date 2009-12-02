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

/**
 * Get the constant force threshold speed in m/s.
 * Below this threshold the engine works as constant force engine.
 * Above this threshold the engine works as constant power engine.
 * @author Bernd Gabriel, Nov 4, 2009
 */
uint16 vehikel_besch_t::calc_const_force_threshold() const
{
	/** 
	* Vehicles specify their (nominal) power. The formula 
	* 
	* force = power / speed
	*
	* calculates the force, that results from the power at a given speed.
	*
	* Calculating this force we obviously must reduce the maximum result,
	* as vehicles can neither produce nor bear an infinite force.
	*
	* There are different kinds of force transmissions, which lead to different
	* threshold speeds, at which the engine leaves the constant/maximum force range.
	*
	* Below this threshold the engine works as constant force engine.
	* Above this threshold the engine works as constant power engine.
	*/
	switch (get_waytype())
	{
		case water_wt:
			// constant force machines at all speeds.
			//return get_geschw();
			break;

		case track_wt:
		case overheadlines_wt: 
		case monorail_wt:      
		case maglev_wt:
		case tram_wt:
		case narrowgauge_wt:
			if (get_engine_type() != steam)
			{
				break;
			}
			/** This is a steam engine on tracks. Steam engines on tracks are constant force engines.
			* The force is constant from 0 to about half of maximum speed. Above the power becomes nearly constant due 
			* to steam shortage and economics. See here for details: http://www.railway-technical.com/st-vs-de.shtml
			* We assume, that the given power is meant for the half of the engines allowed maximum speed and get the constant force:
			*/
			// steamers are constant force machines unless about half of maximum speed, when the steam runs short.
		case air_wt: // constant force machines at all speeds, but is too weak then.
			return (uint16)(get_geschw() / (3.6 * 2));
	}
	/** Constant power characteristic, but we limit maximum force to a tenth of the power multiplied by gear.
	*
	* We consider a stronger gear factor producing additional force in the start-up process, where a greater gear factor allows a more forceful start.
	* This will enforce the player to make more use of slower freight engines.
	*
	* Example: 
	* The german series 230(130 DR) was a univeral engine with 2200 kW, 250 kN start-up force and 140 km/h allowed top speed.
	* The same engine with a freight gear (series 231 / 131 DR) and 2200 kW had 340 kN start-up force and 100 km/h allowed top speed.
	*
	* In simutrans these engines can be simulated by setting the power to 2200, max speed to 140 resp. 100 and the gear to 1.136 resp. 1.545.
	*/
	return 3; // = rounded 10 / 3.6
}

/**
 * Get effective force in kN at given speed in m/s. 
 * @author Bernd Gabriel
 */
uint32 vehikel_besch_t::get_force(uint16 speed /* in m/s */ ) const
{
	if (geared_power == 0) 
	{
		// no power, no force
		return 0;
	}
	return geared_power / (max(speed, force_threshold_speed) * GEAR_FACTOR);
}


/**
 * Get effective power index. 
 * Steam engine power depends on its speed.
 * Effective power in kW: power_index * welt->get_einstellungen()->get_global_power_factor() / GEAR_FACTOR
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
	if (geared_power == 0) 
	{
		// no power, no force
		return 0;
	}
	if (speed < force_threshold_speed)
	{
		return (geared_power * speed) / force_threshold_speed;
	}
	return geared_power;
}
