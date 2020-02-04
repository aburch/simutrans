#include "vehicle_desc.h"
#include "xref_desc.h"
#include "../network/checksum.h"
#include "../simworld.h"
#include "../bauer/goods_manager.h"

uint32 vehicle_desc_t::calc_running_cost(const karte_t *welt, uint32 base_cost) const
{
	// No cost or no time line --> no obsolescence cost increase.
	if (base_cost == 0 || !welt->use_timeline())
	{
		return base_cost;
	}

	// I am not obsolete --> no obsolescence cost increase.
	uint16 months_after_retire = increase_maintenance_after_years * 12;
	if(months_after_retire == 0)
	{
		months_after_retire = welt->get_settings().get_obsolete_running_cost_increase_phase_years() * 12;
	}
	sint32 months_of_obsolescence = welt->get_current_month() - (get_retire_year_month() + months_after_retire);
	if (months_of_obsolescence <= 0)	
	{
		return base_cost; 
	}

	// I am obsolete --> obsolescence cost increase.
	uint16 running_cost_increase_percent = increase_maintenance_by_percent ? increase_maintenance_by_percent :welt->get_settings().get_obsolete_running_cost_increase_percent();
	uint32 max_cost = (base_cost * running_cost_increase_percent) / 100;
	if (max_cost == base_cost)
	{
		return max_cost;
	}

	// Current month is beyond the months_of_increasing_costs --> maximum increased obsolescence cost.
	uint16 phase_years = years_before_maintenance_max_reached ? years_before_maintenance_max_reached :welt->get_settings().get_obsolete_running_cost_increase_phase_years();
	sint32 months_of_increasing_costs = phase_years * 12;
	if (months_of_obsolescence >= months_of_increasing_costs)
	{
		return max_cost;
	}

	// Current month is within the months_of_increasing_costs --> proportionally increased obsolescence cost.
	return ((max_cost - base_cost) * months_of_obsolescence) / months_of_increasing_costs + base_cost;

	// BG, 22.05.2011: improper de-floating of:	return (uint32)((max_cost - base_cost) * (float)months_of_obsolescence / (float)months_of_increasing_costs) + base_cost;
	//const uint32 percentage = ((months_of_obsolescence * 100) / months_of_increasing_costs) + (base_cost * 100);
	//return (max_cost - base_cost) * (percentage / 100);
}

// Get running costs. Running costs increased if the vehicle is obsolete.
// @author: jamespetts
uint16 vehicle_desc_t::get_running_cost(const karte_t* welt) const
{
	return calc_running_cost(welt, get_running_cost());
}

uint32 vehicle_desc_t::get_fixed_cost(karte_t *welt) const
{
	return calc_running_cost(welt, get_fixed_cost());
}

uint32 vehicle_desc_t::get_adjusted_monthly_fixed_cost(karte_t *welt) const
{
	return welt->calc_adjusted_monthly_figure(calc_running_cost(welt, get_fixed_cost()));
}

/**
 * Get the ratio of power to force from either given power and force or according to given waytype.
 * Will never return 0, promised.
 */
float32e8_t vehicle_desc_t::get_power_force_ratio() const
{
	if (power != 0 && tractive_effort != 0)
	{
		return float32e8_t(power, (uint32) tractive_effort);
	}

	static const float32e8_t half_of_kmh_to_ms(10, 36 * 2);
	switch (get_waytype())
	{
		case track_wt:
		case overheadlines_wt: 
		case monorail_wt:      
		case maglev_wt:
		case tram_wt:
		case narrowgauge_wt:
			if (topspeed && get_engine_type() == steam)
			{
				/** This is a steam engine on tracks. Steam engines on tracks are constant force engines.
				* The force is constant from 0 to about half of maximum speed. Above the power becomes nearly constant due 
				* to steam shortage and economics. See here for details: http://www.railway-technical.com/st-vs-de.shtml
				* We assume, that the given power is meant for the half of the engines allowed maximum speed and get the constant force:
				*/
				// Steamers are constant force machines unless about half of maximum speed, when steam runs short.
				return topspeed * half_of_kmh_to_ms;
			}
			return float32e8_t::ten;

		//case water_wt:
			// Ships are constant force machines at all speeds, but the pak sets are balanced for constant power. 
			//return float32e8_t(get_topspeed() * 10, 36);

		case air_wt: 
			// Aircraft are constant force machines at all speeds, but the pak sets are balanced for constant power. 
			// We recommend for simutrans extended to set the tractive effort manually. The existing aircraft power values are very roughly estimated.
			if (topspeed)
			{
				return topspeed * half_of_kmh_to_ms;
			}
			return float32e8_t::ten;

		default:
			/** Constant power characteristic, but we limit maximum force to a tenth of the power multiplied by gear.
			*
			* We consider a stronger gear factor producing additional force in the start-up process, where a greater gear factor allows a more forceful start.
			* This will enforce the player to make more use of slower freight engines.
			*
			* Example: 
			* The german series 230(130 DR) was a universal engine with 2200 kW, 250 kN starting tractive effort and 140 km/h allowed top speed.
			* The same engine with a freight gear (series 231 / 131 DR) and 2200 kW had 340 kN starting tractive effort and 100 km/h allowed top speed.
			*
			* In simutrans extended these engines can be designed  by setting power to 2200, max speed to 140 resp. 100 and tractive effort to 250 resp. 340. 
			* In simutrans standard		these engines can be simulated by setting power to 2200, max speed to 140 resp. 100 and gear to 1.136 resp. 1.545.
			*/
			return float32e8_t::ten;
	}
}

/**
 * calculate unloaded values after loading.
 * @author Bernd Gabriel, Dec 12, 2009
 */
void vehicle_desc_t::loaded()
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

	static const float32e8_t gear_factor((uint32)GEAR_FACTOR); 
	float32e8_t power_force_ratio = get_power_force_ratio();
	force_threshold_speed = (uint16)(power_force_ratio + float32e8_t::half);
	float32e8_t g_power = float32e8_t(power) * (/*(uint32) 1000L * */ (uint32)gear);
	float32e8_t g_force = float32e8_t(tractive_effort) * (/*(uint32) 1000L * */ (uint32)gear);
	if (g_power != 0)
	{
		if (g_force == 0)
		{
			g_force = max(gear_factor, g_power / power_force_ratio);
		}
	}
	else
	{
		if (g_force != 0)
		{
			g_power = max(gear_factor, g_force * power_force_ratio);
		}
	}

	/**
	 * Speed up getting force or power at a given speed (in m/s).
	 * Use arrays instead of repeatedly calculating the force ersp. power.
	 * ToDo: Add effectiveness, which depends on speed and engine type.
	 */
	if (g_power != 0 || g_force != 0)
	{
		uint32 speed = (uint32)topspeed * kmh2ms + float32e8_t::half;
		max_speed = speed;
		geared_power = new uint32[speed+1];
		geared_force = new uint32[speed+1];

		for (; speed > force_threshold_speed; --speed)
		{
			geared_force[speed] = g_power / speed + float32e8_t::half;
			geared_power[speed] = g_power + float32e8_t::half;
		}
		for (; speed <= force_threshold_speed; --speed)
		{
			geared_force[speed] = g_force + float32e8_t::half;
			geared_power[speed] = g_force * speed + float32e8_t::half;
		}
	}
}

/**
 * Get effective force in N at given speed in m/s: effective_force_index *welt->get_settings().get_global_power_factor() / GEAR_FACTOR
 * @author Bernd Gabriel, Dec 14, 2009
 */
uint32 vehicle_desc_t::get_effective_force_index(sint32 speed /* in m/s */ ) const
{
	if (geared_force == 0) 
	{
		// no force at all
		return 0;
	}
	//return speed <= force_threshold_speed ? geared_force : geared_power / speed;
	return geared_force[min(speed, max_speed)];
}

/**
 * Get effective power in W at given speed in m/s: effective_power_index *welt->get_settings().get_global_power_factor() / GEAR_FACTOR
 * @author Bernd Gabriel, Dec 14, 2009
 */
uint32 vehicle_desc_t::get_effective_power_index(sint32 speed /* in m/s */ ) const
{
	if (geared_power == 0) 
	{
		// no power at all
		return 0;
	}
	///return speed <= force_threshold_speed ? geared_force * speed : geared_power;
	return geared_power[min(speed, max_speed)];
}

uint16 vehicle_desc_t::get_obsolete_year_month(const karte_t *welt) const
{ 
	if(increase_maintenance_after_years)
	{
		return retire_date + (12 * increase_maintenance_after_years); 
	}
	else
	{
		return retire_date + (welt->get_settings().get_default_increase_maintenance_after_years((waytype_t)wtyp) * 12);
	}
}

void vehicle_desc_t::fix_number_of_classes()
{
	fix_basic_constraint();
	// We can call this safely since we fixed the number of classes
	// stored in the good desc earlier when registering it.
	uint8 actual_number_of_classes = get_freight_type()->get_number_of_classes();

	if (actual_number_of_classes == 0)
	{
		classes = 0;
		return;
	}

	while (classes > actual_number_of_classes)
	{
		capacity[classes-2]+=capacity[classes-1];
		classes --;
	}

	if (classes < actual_number_of_classes)
	{
		uint8 *comfort_copy = new uint8[classes];
		uint16 *capacity_copy = new uint16[classes];
		for (uint8 i = 0; i < classes; i++)
		{
			comfort_copy[i] = comfort[i];
			capacity_copy[i] = capacity[i];
		}
		delete[] comfort;
		delete[] capacity;
		comfort = new uint8[actual_number_of_classes];
		capacity = new uint16[actual_number_of_classes];
		for (uint8 i = 0; i < classes; i++)
		{
			comfort[i] = comfort_copy[i];
			capacity[i] = capacity_copy[i];
		}
		for (uint8 i = classes; i < actual_number_of_classes; i++)
		{
			comfort[i] = 0;
			capacity[i] = 0;
		}
		classes = actual_number_of_classes;
		delete[] comfort_copy;
		delete[] capacity_copy;
	}
}


uint16 vehicle_desc_t::get_available_livery_count(karte_t *welt) const
{
	uint16 livery_count = 0;
	const uint16 month_now = welt->get_timeline_year_month();
	if (!welt->use_timeline()) {
		return get_livery_count();
	}

	vector_tpl<livery_scheme_t*>* schemes = welt->get_settings().get_livery_schemes();
	ITERATE_PTR(schemes, i)
	{
		livery_scheme_t* scheme = schemes->get_element(i);
		if (!scheme->is_available(welt->get_timeline_year_month()))
		{
			continue;
		}
		if (scheme->get_latest_available_livery(month_now, this)) {
			livery_count++;
		}
	}

	return livery_count;
}





void vehicle_desc_t::calc_checksum(checksum_t *chk) const
{
	obj_desc_transport_related_t::calc_checksum(chk);
	chk->input(classes);
	for(uint32 i = 0; i < classes; i ++)
	{
		chk->input(capacity[i]); 
	}
	chk->input(weight);
	chk->input(power);
	chk->input(running_cost);
	chk->input(base_fixed_cost);
	chk->input(gear);
	chk->input(len);
	chk->input(leader_count);
	chk->input(trailer_count);
	chk->input(engine_type);
	// freight
	const xref_desc_t *xref = get_child<xref_desc_t>(2);
	chk->input(xref ? xref->get_name() : "NULL");

	// vehicle constraints
	// For some reason, this records false mismatches with a few
	// vehicles when names are used. Use  numbers instead.
	/*for(uint8 i=0; i<leader_count+trailer_count; i++) {
		const xref_desc_t *xref = get_child<xref_desc_t>(6+i);
		chk->input(xref ? xref->get_name() : "NULL");
	}*/

	// Extended settings
	chk->input(base_upgrade_price);
	chk->input(overcrowded_capacity);
	chk->input(base_fixed_cost);
	chk->input(upgrades);
	chk->input(is_tilting ? 1 : 0);
	chk->input(mixed_load_prohibition ? 1 : 0);
	chk->input(basic_constraint_prev);
	chk->input(basic_constraint_next);
	chk->input(way_constraints.get_permissive());
	chk->input(way_constraints.get_prohibitive());
	chk->input(bidirectional ? 1 : 0);
	chk->input(can_lead_from_rear ? 1 : 0); // not used
	chk->input(can_be_at_rear ? 1 : 0);
	for (uint32 i = 0; i < classes; i++)
	{
		chk->input(comfort[i]);
	}
	chk->input(max_loading_time_seconds);
	chk->input(min_loading_time_seconds);
	chk->input(tractive_effort);
	chk->input(brake_force);
	chk->input(minimum_runway_length);
	chk->input(range);
	const uint16 ar = air_resistance * float32e8_t((uint32)100);
	const uint16 rr = rolling_resistance * float32e8_t((uint32)100);
	chk->input(ar);
	chk->input(rr);
}

uint8 vehicle_desc_t::get_interactivity() const
{
	uint8 flags = 0;
	if (bidirectional) { flags |= 1; }
	if (power) { flags |= 2; }
	return flags;
}

uint8 vehicle_desc_t::has_available_upgrade(uint16 month_now, bool show_future) const
{
	uint8 upgrade_state = 0; // 1 = not available yet, 2 = already available
	for (int i = 0; i < upgrades; i++)
	{
		if (show_future) {
			upgrade_state = 1;
		}
		const vehicle_desc_t *upgrade_desc = get_upgrades(i);
		if (upgrade_desc && !upgrade_desc->is_future(month_now) && (!upgrade_desc->is_retired(month_now)))
		{
			return 2;
		}
		else if(!show_future && upgrade_desc && upgrade_desc->is_future(month_now)==2) {
			upgrade_state = 1;
		}
	}
	return upgrade_state;
}


// The old pak doesn't have a basic constraint, so add a value referring to the constraint.
// Note: This is ambiguous because it does not have data of cab and constraint[prev]=any.
void vehicle_desc_t::fix_basic_constraint()
{
	// front side
	if (basic_constraint_prev & unknown_constraint) {
		bool prev_has_none = false;
		for (int i = 0; i < leader_count; i++)
		{
			vehicle_desc_t const* const veh = get_child<vehicle_desc_t>(get_add_to_node() + i);
			if (veh == NULL)
			{
				prev_has_none = true;
				basic_constraint_prev |= can_be_tail;
				break;
			}
		}

		switch (leader_count) {
		case 0:
			if (bidirectional) {
				basic_constraint_prev |= can_be_tail;
			}
			if (!bidirectional || can_lead_from_rear || (bidirectional && power)) {
				// Consider locomotive if it has power
				basic_constraint_prev |= can_be_head;
			}
			break;
		case 1:
			if (prev_has_none) {
				// only set "none"
				basic_constraint_prev |= only_at_front;
				if (can_lead_from_rear) {
					basic_constraint_prev |= can_be_tail;
				}
			}
			else {
				basic_constraint_prev |= intermediate_unique;
			}
			break;
		default:
			if (prev_has_none && ((bidirectional && power) || can_lead_from_rear || !bidirectional)) {
				basic_constraint_prev |= can_be_head;
			}
			break;
		}
		basic_constraint_prev &= ~unknown_constraint;
	}

	// rear side
	if (basic_constraint_next & unknown_constraint) {
		bool next_has_none = false;
		for (int i = 0; i < trailer_count; i++) {
			vehicle_desc_t const* const veh = get_child<vehicle_desc_t>(get_add_to_node() + leader_count + i);
			if (veh == NULL) {
				next_has_none = true;
				basic_constraint_next |= can_be_tail;
				break;
			}
		}

		switch (trailer_count) {
		case 0:
			if (can_be_at_rear == false) {
				basic_constraint_next = 0; // constraint[next]=any
			}
			else {
				basic_constraint_next |= can_be_tail;
				if (can_lead_from_rear || (bidirectional && power)) {
					// Consider locomotive if it has power
					basic_constraint_next |= can_be_head;
				}
			}
			break;
		case 1:
			if (next_has_none) {
				basic_constraint_next |= unconnectable;
				if (can_lead_from_rear) {
					basic_constraint_next |= can_be_head;
				}
			}
			else {
				basic_constraint_next |= intermediate_unique;
			}
			break;
		default:
			break;
		}
		basic_constraint_next &= ~unknown_constraint;
	}
}