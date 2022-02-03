/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simdebug.h"
#include "display/simimg.h"
#include "simcolor.h"
#include "ground/grund.h"
#include "ground/boden.h"
#include "ground/fundament.h"
#include "simfab.h"
#include "world/simcity.h"
#include "simhalt.h"
#include "simware.h"
#include "world/simworld.h"
#include "descriptor/building_desc.h"
#include "descriptor/goods_desc.h"
#include "descriptor/sound_desc.h"
#include "player/simplay.h"

#include "simintr.h"

#include "obj/wolke.h"
#include "obj/gebaeude.h"
#include "obj/field.h"
#include "obj/leitung2.h"

#include "dataobj/settings.h"
#include "dataobj/environment.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"

#include "descriptor/factory_desc.h"
#include "builder/hausbauer.h"
#include "builder/goods_manager.h"
#include "builder/fabrikbauer.h"

#include "gui/fabrik_info.h"

#include "utils/simrandom.h"
#include "utils/cbuffer.h"

#include "gui/simwin.h"
#include "display/simgraph.h"

// Fabrik_t


static const int FAB_MAX_INPUT = 15000;
// Half a display unit (0.5).
static const sint64 FAB_DISPLAY_UNIT_HALF = ((sint64)1 << (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS - 1));
// Half a production factor unit (0.5).
static const sint32 FAB_PRODFACT_UNIT_HALF = ((sint32)1 << (DEFAULT_PRODUCTION_FACTOR_BITS - 1));

karte_ptr_t fabrik_t::welt;


/**
 * Convert internal values to displayed values
 */
sint64 convert_goods(sint64 value) { return ((value + FAB_DISPLAY_UNIT_HALF) >> (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS) ); }
sint64 convert_power(sint64 value) { return ( value >> POWER_TO_MW ); }
sint64 convert_boost(sint64 value) { return ( (value * 100 + (DEFAULT_PRODUCTION_FACTOR>>1)) >> DEFAULT_PRODUCTION_FACTOR_BITS ); }


/**
 * Ordering based on relative distance to a fixed point `origin'.
 */
class RelativeDistanceOrdering
{
private:
	const koord m_origin;
public:
	RelativeDistanceOrdering(const koord& origin)
		: m_origin(origin)
	{ /* nothing */ }

	/**
	 * Returns true if `a' is closer to the origin than `b', otherwise false.
	 */
	bool operator()(const koord& a, const koord& b) const
	{
		return koord_distance(m_origin, a) < koord_distance(m_origin, b);
	}
};

/**
 * Produce a scaled production amount from a production amount and work factor.
 */
sint32 work_scale_production(sint64 prod, sint64 work){
	// compute scaled production, rounding up
	return ((prod * work) + (1 << WORK_BITS) - 1) >> WORK_BITS;
}

/**
 * Produce a work factor from a production amount and scaled production amount.
 */
sint32 work_from_production(sint64 prod, sint64 scaled){
	// compute work, rounding up
	return prod ? ((scaled << WORK_BITS) + prod - 1) / prod : 0;
}

void ware_production_t::init_stats()
{
	for(  int m=0;  m<MAX_MONTH;  ++m  ) {
		for(  int s=0;  s<MAX_FAB_GOODS_STAT;  ++s  ) {
			statistics[m][s] = 0;
		}
	}
	weighted_sum_storage = 0;
}


void ware_production_t::roll_stats(uint32 factor, sint64 aggregate_weight)
{
	// calculate weighted average storage first
	if(  aggregate_weight>0  ) {
		set_stat( weighted_sum_storage / aggregate_weight, FAB_GOODS_STORAGE );
	}

	for(  int s=0;  s<MAX_FAB_GOODS_STAT;  ++s  ) {
		for(  int m=MAX_MONTH-1;  m>0;  --m  ) {
			statistics[m][s] = statistics[m-1][s];
		}
		if(  s==FAB_GOODS_TRANSIT  ) {
			// keep the current amount in transit
			statistics[0][s] = statistics[1][s];
		}
		else {
			statistics[0][s] = 0;
		}
	}
	weighted_sum_storage = 0;

	// restore current storage level
	set_stat( (sint64)menge * (sint64)factor, FAB_GOODS_STORAGE );
}


void ware_production_t::rdwr(loadsave_t *file)
{
	if(  file->is_loading()  ) {
		init_stats();
	}

	// we use a temporary variable to save/load old data correctly
	sint64 statistics_buf[MAX_MONTH][MAX_FAB_GOODS_STAT];
	memcpy( statistics_buf, statistics, sizeof(statistics_buf) );
	if(  file->is_saving()  &&  file->is_version_less(120, 1)  ) {
		for(  int m=0;  m<MAX_MONTH;  ++m  ) {
			statistics_buf[m][0] = (statistics[m][FAB_GOODS_STORAGE] >> DEFAULT_PRODUCTION_FACTOR_BITS);
			statistics_buf[m][2] = (statistics[m][2] >> DEFAULT_PRODUCTION_FACTOR_BITS);
		}
	}

	if(  file->is_version_atleast(112, 1)  ) {
		for(  int s=0;  s<MAX_FAB_GOODS_STAT;  ++s  ) {
			for(  int m=0;  m<MAX_MONTH;  ++m  ) {
				file->rdwr_longlong( statistics_buf[m][s] );
			}
		}
		file->rdwr_longlong( weighted_sum_storage );
	}
	else if(  file->is_version_atleast(110, 5)  ) {
		// save/load statistics
		for(  int s=0;  s<3;  ++s  ) {
			for(  int m=0;  m<MAX_MONTH;  ++m  ) {
				file->rdwr_longlong( statistics_buf[m][s] );
			}
		}
		file->rdwr_longlong( weighted_sum_storage );
	}

	if(  file->is_loading()  ) {
		memcpy( statistics, statistics_buf, sizeof(statistics_buf) );

		// Apply correction for output production graphs which have had their precision changed for factory normalization.
		// Also apply a fix for corrupted in-transit values caused by a logical error.
		if(file->is_version_less(120, 1)){
			for(  int m=0;  m<MAX_MONTH;  ++m  ) {
				statistics[m][0] = (statistics[m][FAB_GOODS_STORAGE] & 0xffffffff) << DEFAULT_PRODUCTION_FACTOR_BITS;
				statistics[m][2] = (statistics[m][2] & 0xffffffff) << DEFAULT_PRODUCTION_FACTOR_BITS;
			}
		}

		// recalc transit always on load
		statistics[0][FAB_GOODS_TRANSIT] = 0;
	}

	if (file->is_version_atleast(122, 1)) {
		file->rdwr_long(index_offset);
	}
	else if (file->is_loading()) {
		index_offset = 0;
	}
}


void ware_production_t::book_weighted_sum_storage(uint32 factor, sint64 delta_time)
{
	const sint64 amount = (sint64)menge * (sint64)factor;
	weighted_sum_storage += amount * delta_time;
	set_stat( amount, FAB_GOODS_STORAGE );
}

sint32 ware_production_t::calculate_output_production_rate() const {
	return fabrik_t::calculate_work_rate_ramp(menge, min_shipment * OUTPUT_SCALE_RAMPDOWN_MULTIPLYER, max);
}

sint32 ware_production_t::calculate_demand_production_rate() const {
	return fabrik_t::calculate_work_rate_ramp(demand_buffer, max / 2, max);
}

void fabrik_t::arrival_statistics_t::init()
{
	for(  uint32 s=0;  s<SLOT_COUNT;  ++s  ) {
		slots[s] = 0;
	}
	current_slot = 0;
	active_slots = 0;
	aggregate_arrival = 0;
	scaled_demand = 0;
}


void fabrik_t::arrival_statistics_t::rdwr(loadsave_t *file)
{
	if(  file->is_version_atleast(110, 5)  ) {
		if(  file->is_loading()  ) {
			aggregate_arrival = 0;
			for(  uint32 s=0;  s<SLOT_COUNT;  ++s  ) {
				file->rdwr_short( slots[s] );
				aggregate_arrival += slots[s];
			}
			scaled_demand = 0;
		}
		else {
			for(  uint32 s=0;  s<SLOT_COUNT;  ++s  ) {
				file->rdwr_short( slots[s] );
			}
		}
		file->rdwr_short( current_slot );
		file->rdwr_short( active_slots );
	}
	else if(  file->is_loading()  ) {
		init();
	}
}


sint32 fabrik_t::arrival_statistics_t::advance_slot()
{
	sint32 result = 0;
	// advance to the next slot
	++current_slot;
	if(  current_slot>=SLOT_COUNT  ) {
		current_slot = 0;
	}
	// handle expiration of past arrivals and reset slot to 0
	if(  slots[current_slot]>0  ) {
		aggregate_arrival -= slots[current_slot];
		slots[current_slot] = 0;
		if(  aggregate_arrival==0  ) {
			// reset slot count to 0 as all previous arrivals have expired
			active_slots = 0;
		}
		result |= ARRIVALS_CHANGED;
	}
	// count the number of slots covered since aggregate arrival last increased from 0 to +ve
	if(  active_slots>0  &&  active_slots<SLOT_COUNT  ) {
		++active_slots;
		result |= ACTIVE_SLOTS_INCREASED;
	}
	return result;
}


void fabrik_t::arrival_statistics_t::book_arrival(const uint16 amount)
{
	if(  aggregate_arrival==0  ) {
		// new arrival after complete inactivity -> start counting slots
		active_slots = 1;
	}
	// increment current slot and aggregate arrival
	slots[current_slot] += amount;
	aggregate_arrival += amount;
}


void fabrik_t::update_transit( const ware_t *ware, bool add )
{
	if(  ware->index > goods_manager_t::INDEX_NONE  ) {
		// only for freights
		fabrik_t *fab = get_fab( ware->get_zielpos() );
		if(  fab  ) {
			fab->update_transit_intern( ware, add );
		}
	}
}

void fabrik_t::apply_transit( const ware_t *ware )
{
	if(  ware->index > goods_manager_t::INDEX_NONE  ) {
		// only for freights
		fabrik_t *fab = get_fab( ware->get_zielpos() );
		if(  fab  ) {
			for(  uint32 input = 0;  input < fab->input.get_count();  input++  ){
				ware_production_t& w = fab->input[input];
				if(  w.get_typ()->get_index() == ware->index  ) {
					// It is now in transit.
					w.book_stat((sint64)ware->menge, FAB_GOODS_TRANSIT );
					// If using JIT2, must decrement demand buffers, activating if required.
					if( welt->get_settings().get_just_in_time() >= 2 ){
						const uint32 prod_factor = fab->desc->get_supplier(input)->get_consumption();
						const sint32 prod_delta = (sint32)((((sint64)(ware->menge) << (DEFAULT_PRODUCTION_FACTOR_BITS + precision_bits)) + (sint64)(prod_factor - 1)) / (sint64)prod_factor);
						const sint32 demand = w.demand_buffer;
						w.demand_buffer -= prod_delta;
						if(  demand >= w.max  &&  w.demand_buffer < w.max  ) {
							fab->inactive_demands --;
						}
					}
					// ours is on its way, no need to handle the other
					return;
				}
			}
		}
	}
}


// just for simplicity ...
void fabrik_t::update_transit_intern( const ware_t *ware, bool add )
{
	FOR(  array_tpl<ware_production_t>,  &w,  input ) {
		if(  w.get_typ()->get_index() == ware->index  ) {
			w.book_stat(add ? (sint64)ware->menge : -(sint64)ware->menge, FAB_GOODS_TRANSIT );
			return;
		}
	}
}


void fabrik_t::init_stats()
{
	for(  int m=0;  m<MAX_MONTH;  ++m  ) {
		for(  int s=0;  s<MAX_FAB_STAT;  ++s  ) {
			statistics[m][s] = 0;
		}
	}
	weighted_sum_production = 0;
	weighted_sum_boost_electric = 0;
	weighted_sum_boost_pax = 0;
	weighted_sum_boost_mail = 0;
	weighted_sum_power = 0;
	aggregate_weight = 0;
}


void fabrik_t::book_weighted_sums(sint64 delta_time)
{
	aggregate_weight += delta_time;

	// storage level of input/output stores
	for( uint32 in = 0;  in < input.get_count();  in++  ){
		input[in].book_weighted_sum_storage(desc->get_supplier(in)->get_consumption(), delta_time);
	}
	for( uint32 out = 0;  out < output.get_count();  out++  ){
		output[out].book_weighted_sum_storage(desc->get_product(out)->get_factor(), delta_time);
	}

	// production level
	const sint32 current_prod = get_current_production();
	weighted_sum_production += current_prod * delta_time;
	set_stat( current_prod, FAB_PRODUCTION );

	// electricity, pax and mail boosts
	weighted_sum_boost_electric += prodfactor_electric * delta_time;
	set_stat( prodfactor_electric, FAB_BOOST_ELECTRIC );
	weighted_sum_boost_pax += prodfactor_pax * delta_time;
	weighted_sum_boost_mail += prodfactor_mail * delta_time;

	// power produced or consumed
	sint64 power = get_power();
	weighted_sum_power += power * delta_time;
	set_stat( power, FAB_POWER );
}


void fabrik_t::update_scaled_electric_demand()
{
	if(  desc->get_electric_demand()==65535  ) {
		// demand not specified in pak, use old fixed demands
		scaled_electric_demand = prodbase * PRODUCTION_DELTA_T;
		if(  desc->is_electricity_producer()  ) {
			scaled_electric_demand *= 4;
		}
		return;
	}

	const sint64 prod = desc->get_productivity();
	scaled_electric_demand = (uint32)( (( (sint64)(desc->get_electric_demand()) * (sint64)prodbase + (prod >> 1) ) / prod) << POWER_TO_MW );

	if(  scaled_electric_demand == 0  ) {
		prodfactor_electric = 0;
	}
}


void fabrik_t::update_scaled_pax_demand()
{
	// first, scaling based on current production base
	const sint64 prod = desc->get_productivity();
	const sint64 desc_pax_demand = ( desc->get_pax_demand()==65535 ? desc->get_pax_level() : desc->get_pax_demand() );
	// formula : desc_pax_demand * (current_production_base / desc_production_base); (prod >> 1) is for rounding
	const uint32 pax_demand = (uint32)( ( desc_pax_demand * (sint64)prodbase + (prod >> 1) ) / prod );
	// then, scaling based on month length
	scaled_pax_demand = (uint32)welt->scale_with_month_length(pax_demand);
	if(  scaled_pax_demand == 0  &&  desc_pax_demand > 0  ) {
		scaled_pax_demand = 1; // since desc pax demand > 0 -> ensure no less than 1
	}
	// pax demand for fixed period length
	arrival_stats_pax.set_scaled_demand( pax_demand );
}


void fabrik_t::update_scaled_mail_demand()
{
	// first, scaling based on current production base
	const sint64 prod = desc->get_productivity();
	const sint64 desc_mail_demand = ( desc->get_mail_demand()==65535 ? (desc->get_pax_level()>>2) : desc->get_mail_demand() );
	// formula : desc_mail_demand * (current_production_base / desc_production_base); (prod >> 1) is for rounding
	const uint32 mail_demand = (uint32)( ( desc_mail_demand * (sint64)prodbase + (prod >> 1) ) / prod );
	// then, scaling based on month length
	scaled_mail_demand = (uint32)welt->scale_with_month_length(mail_demand);
	if(  scaled_mail_demand == 0  &&  desc_mail_demand > 0  ) {
		scaled_mail_demand = 1; // since desc mail demand > 0 -> ensure no less than 1
	}
	// mail demand for fixed period length
	arrival_stats_mail.set_scaled_demand( mail_demand );
}


void fabrik_t::update_prodfactor_pax()
{
	// calculate pax boost based on arrival data and demand of the fixed-length period
	const uint32 periods = welt->get_settings().get_factory_arrival_periods();
	const uint32 slots = arrival_stats_pax.get_active_slots();
	const uint32 pax_demand = ( periods==1 || slots*periods<=(uint32)SLOT_COUNT ?
									arrival_stats_pax.get_scaled_demand() :
									( slots==(uint32)SLOT_COUNT ?
										arrival_stats_pax.get_scaled_demand() * periods :
										(arrival_stats_pax.get_scaled_demand() * periods * slots) >> SLOT_BITS ) );
	const uint32 pax_arrived = arrival_stats_pax.get_aggregate_arrival();
	if(  pax_demand==0  ||  pax_arrived==0  ||  desc->get_pax_boost()==0  ) {
		prodfactor_pax = 0;
	}
	else if(  pax_arrived>=pax_demand  ) {
		// maximum boost
		prodfactor_pax = desc->get_pax_boost();
	}
	else {
		// pro-rata boost : (pax_arrived / pax_demand) * desc_pax_boost; (pax_demand >> 1) is for rounding
		prodfactor_pax = (sint32)( ( (sint64)pax_arrived * (sint64)(desc->get_pax_boost()) + (sint64)(pax_demand >> 1) ) / (sint64)pax_demand );
	}
	set_stat(prodfactor_pax, FAB_BOOST_PAX);
}


void fabrik_t::update_prodfactor_mail()
{
	// calculate mail boost based on arrival data and demand of the fixed-length period
	const uint32 periods = welt->get_settings().get_factory_arrival_periods();
	const uint32 slots = arrival_stats_mail.get_active_slots();
	const uint32 mail_demand = ( periods==1 || slots*periods<=(uint32)SLOT_COUNT ?
									arrival_stats_mail.get_scaled_demand() :
									( slots==(uint32)SLOT_COUNT ?
										arrival_stats_mail.get_scaled_demand() * periods :
										(arrival_stats_mail.get_scaled_demand() * periods * slots) >> SLOT_BITS ) );
	const uint32 mail_arrived = arrival_stats_mail.get_aggregate_arrival();
	if(  mail_demand==0  ||  mail_arrived==0  ||  desc->get_mail_boost()==0  ) {
		prodfactor_mail = 0;
	}
	else if(  mail_arrived>=mail_demand  ) {
		// maximum boost
		prodfactor_mail = desc->get_mail_boost();
	}
	else {
		// pro-rata boost : (mail_arrived / mail_demand) * desc_mail_boost; (mail_demand >> 1) is for rounding
		prodfactor_mail = (sint32)( ( (sint64)mail_arrived * (sint64)(desc->get_mail_boost()) + (sint64)(mail_demand >> 1) ) / (sint64)mail_demand );
	}
	set_stat(prodfactor_mail, FAB_BOOST_MAIL);
}


void fabrik_t::recalc_demands_at_target_cities()
{
	if (!welt->get_settings().get_factory_enforce_demand()) {
		// demand not enforced -> no splitting of demands
		FOR(vector_tpl<stadt_t*>, const c, target_cities) {
			c->access_target_factories_for_pax().update_factory( this, scaled_pax_demand  << DEMAND_BITS);
			c->access_target_factories_for_mail().update_factory(this, scaled_mail_demand << DEMAND_BITS);
		}
		return;
	}
	if (target_cities.empty()) {
		// nothing to do
		return;
	}
	else if(  target_cities.get_count()==1  ) {
		// only 1 target city -> no need to apportion pax/mail demand
		target_cities[0]->access_target_factories_for_pax().update_factory(this, (scaled_pax_demand << DEMAND_BITS));
		target_cities[0]->access_target_factories_for_mail().update_factory(this, (scaled_mail_demand << DEMAND_BITS));
	}
	else {
		// more than 1 target cities -> need to apportion pax/mail demand among the cities
		static vector_tpl<uint32> weights(8);
		weights.clear();
		uint32 sum_of_weights = 0;
		// first, calculate the weights
		for(  uint32 c=0;  c<target_cities.get_count();  ++c  ) {
			weights.append( weight_by_distance( target_cities[c]->get_einwohner(), shortest_distance( get_pos().get_2d(), target_cities[c]->get_center() ) ) );
			sum_of_weights += weights[c];
		}
		// finally, apportion the pax/mail demand; formula : demand * (city_weight / aggregate_city_weight); (sum_of_weights >> 1) is for rounding
		for(  uint32 c=0;  c<target_cities.get_count();  ++c  ) {
			const uint32 pax_amount = (uint32)(( (sint64)(scaled_pax_demand << DEMAND_BITS) * (sint64)weights[c] + (sint64)(sum_of_weights >> 1) ) / (sint64)sum_of_weights);
			target_cities[c]->access_target_factories_for_pax().update_factory(this, pax_amount);
			const uint32 mail_amount = (uint32)(( (sint64)(scaled_mail_demand << DEMAND_BITS) * (sint64)weights[c] + (sint64)(sum_of_weights >> 1) ) / (sint64)sum_of_weights);
			target_cities[c]->access_target_factories_for_mail().update_factory(this, mail_amount);
		}
	}
}


void fabrik_t::recalc_storage_capacities()
{
	if(  desc->get_field_group()  ) {
		// with fields -> calculate based on capacities contributed by fields
		const uint32 ware_types = input.get_count() + output.get_count();
		if(  ware_types>0  ) {
			// calculate total storage capacity contributed by fields
			const field_group_desc_t *const field_group = desc->get_field_group();
			sint32 field_capacities = 0;
			FOR(vector_tpl<field_data_t>, const& f, fields) {
				field_capacities += field_group->get_field_class(f.field_class_index)->get_storage_capacity();
			}
			const sint32 share = (sint32)( ( (sint64)field_capacities << precision_bits ) / (sint64)ware_types );
			// first, for input goods
			FOR(array_tpl<ware_production_t>, & g, input) {
				for(  int b=0;  b<desc->get_supplier_count();  ++b  ) {
					const factory_supplier_desc_t *const input = desc->get_supplier(b);
					if (g.get_typ() == input->get_input_type()) {
						// Inputs are now normalized to factory production.
						uint32 prod_factor = input->get_consumption();
						g.max = (sint32)((((sint64)((input->get_capacity() << precision_bits) + share) << DEFAULT_PRODUCTION_FACTOR_BITS) + (sint64)(prod_factor - 1)) / (sint64)prod_factor);
					}
				}
			}
			// then, for output goods
			FOR(array_tpl<ware_production_t>, & g, output) {
				for(  uint b=0;  b<desc->get_product_count();  ++b  ) {
					const factory_product_desc_t *const output = desc->get_product(b);
					if (g.get_typ() == output->get_output_type()) {
						// Outputs are now normalized to factory production.
						uint32 prod_factor = output->get_factor();
						g.max = (sint32)((((sint64)((output->get_capacity() << precision_bits) + share) << DEFAULT_PRODUCTION_FACTOR_BITS) + (sint64)(prod_factor - 1)) / (sint64)prod_factor);
					}
				}
			}
		}
	}
	else {
		// without fields -> scaling based on prodbase
		// first, for input goods
		FOR(array_tpl<ware_production_t>, & g, input) {
			for(  int b=0;  b<desc->get_supplier_count();  ++b  ) {
				const factory_supplier_desc_t *const input = desc->get_supplier(b);
				if (g.get_typ() == input->get_input_type()) {
					// Inputs are now normalized to factory production.
					uint32 prod_factor = input->get_consumption();
					g.max = (sint32)(((((sint64)input->get_capacity() * (sint64)prodbase) << (precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS)) + (sint64)(prod_factor - 1)) / ((sint64)desc->get_productivity() * (sint64)prod_factor));
				}
			}
		}
		// then, for output goods
		FOR(array_tpl<ware_production_t>, & g, output) {
			for(  uint b=0;  b<desc->get_product_count();  ++b  ) {
				const factory_product_desc_t *const output = desc->get_product(b);
				if (g.get_typ() == output->get_output_type()) {
					// Outputs are now normalized to factory production.
					uint32 prod_factor = output->get_factor();
					g.max = (sint32)(((((sint64)output->get_capacity() * (sint64)prodbase) << (precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS)) + (sint64)(prod_factor - 1)) / ((sint64)desc->get_productivity() * (sint64)prod_factor));
				}
			}
		}
	}

	// Now that the maximum is known, work out the recommended shipment size for outputs in normalized units.
	for(  uint32 out = 0;  out < output.get_count();  out++  ){
		const uint32 prod_factor = desc->get_product(out)->get_factor();

		// Determine the maximum number of whole units the out can store.
		const uint32 unit_size = (uint32)(((sint64)output[out].max * (sint64)prod_factor) >> ( precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS ));

		// Determine the number of units to ship. Prefer 10 units although in future a more dynamic choice may be appropiate.
		uint32 shipment_size;
		// Maximum shipment size.
		if( unit_size >= SHIPMENT_MAX_SIZE * SHIPMENT_NUM_MIN ) {
			shipment_size = SHIPMENT_MAX_SIZE;
		}
		// Dynamic shipment size.
		else if( unit_size > SHIPMENT_NUM_MIN ) {
			shipment_size = unit_size / SHIPMENT_NUM_MIN;
		}
		// Minimum shipment size.
		else {
			shipment_size = 1;
		}

		// Now convert it into the prefered shipment size. Always round up to prevent "off by 1" error.
		output[out].min_shipment = (sint32)((((sint64)shipment_size << (precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS)) + (sint64)(prod_factor - 1)) / (sint64)prod_factor);
	}

	if(  welt->get_settings().get_just_in_time() >= 2  ) {
		rebuild_inactive_cache();
	}
}


void fabrik_t::add_target_city(stadt_t *const city)
{
	if(  target_cities.append_unique(city)  ) {
		recalc_demands_at_target_cities();
	}
}


void fabrik_t::remove_target_city(stadt_t *const city)
{
	if(  target_cities.is_contained(city)  ) {
		target_cities.remove(city);
		city->access_target_factories_for_pax().remove_factory(this);
		city->access_target_factories_for_mail().remove_factory(this);
		recalc_demands_at_target_cities();
	}
}


void fabrik_t::clear_target_cities()
{
	FOR(vector_tpl<stadt_t*>, const c, target_cities) {
		c->access_target_factories_for_pax().remove_factory(this);
		c->access_target_factories_for_mail().remove_factory(this);
	}
	target_cities.clear();
}


void fabrik_t::set_base_production(sint32 p)
{
	prodbase = p;
	recalc_storage_capacities();
	update_scaled_electric_demand();
	update_scaled_pax_demand();
	update_scaled_mail_demand();
	update_prodfactor_pax();
	update_prodfactor_mail();
	recalc_demands_at_target_cities();
}


fabrik_t *fabrik_t::get_fab(const koord &pos)
{
	const grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb) {
			return gb->get_fabrik();
		}
	}
	return NULL;
}


void fabrik_t::link_halt(halthandle_t halt)
{
	welt->access(pos.get_2d())->add_to_haltlist(halt);
}


void fabrik_t::unlink_halt(halthandle_t halt)
{
	planquadrat_t *plan=welt->access(pos.get_2d());
	if(plan) {
		plan->remove_from_haltlist(halt);
	}
}


// returns true, if there is a freight halt serving us
bool fabrik_t::is_within_players_network( const player_t* player ) const
{
	if( const planquadrat_t *plan = welt->access( pos.get_2d() ) ) {
		if( plan->get_haltlist_count() > 0 ) {
			const halthandle_t *const halt_list = plan->get_haltlist();
			for( int h = 0; h < plan->get_haltlist_count(); h++ ) {
				halthandle_t halt = halt_list[h];
				if(  halt.is_bound()  &&  halt->is_enabled(haltestelle_t::WARE)  &&  halt->has_available_network(player)  ) {
					return true;
				}
			}
		}
	}
	return false;
}



void fabrik_t::add_lieferziel(koord ziel)
{
	if(  !lieferziele.is_contained(ziel)  ) {
		lieferziele.insert_ordered( ziel, RelativeDistanceOrdering(pos.get_2d()) );
		// now tell factory too
		fabrik_t * fab = fabrik_t::get_fab(ziel);
		if (fab) {
			fab->add_supplier(get_pos().get_2d());
		}
	}
}


void fabrik_t::rem_lieferziel(koord ziel)
{
	lieferziele.remove(ziel);
}


fabrik_t::fabrik_t(loadsave_t* file)
{
	owner = NULL;
	prodfactor_electric = 0;
	lieferziele_active_last_month = 0;
	pos = koord3d::invalid;
	transformers.clear();

	rdwr(file);

	if(  desc == NULL  ) {
		dbg->warning( "fabrik_t::fabrik_t()", "No pak-file for factory at (%s) - will not be built!", pos_origin.get_str() );
		return;
	}
	else if(  !welt->is_within_limits(pos_origin.get_2d())  ) {
		dbg->warning( "fabrik_t::fabrik_t()", "%s is not a valid position! (Will not be built!)", pos_origin.get_str() );
		desc = NULL; // to get rid of this broken factory later...
	}
	else {
		build(rotate, false, false);
		// now get rid of construction image
		for(  sint16 y=0;  y<desc->get_building()->get_y(rotate);  y++  ) {
			for(  sint16 x=0;  x<desc->get_building()->get_x(rotate);  x++  ) {
				gebaeude_t *gb = welt->lookup_kartenboden( pos_origin.get_2d()+koord(x,y) )->find<gebaeude_t>();
				if(  gb  ) {
					gb->add_alter(10000);
				}
			}
		}
	}

	last_sound_ms = welt->get_ticks();
}


fabrik_t::fabrik_t(koord3d pos_, player_t* owner, const factory_desc_t* factory_desc, sint32 initial_prod_base) :
	desc(factory_desc),
	pos(pos_)
{
	this->pos.z = welt->max_hgt(pos.get_2d());
	pos_origin = pos;

	this->owner = owner;
	prodfactor_electric = 0;
	prodfactor_pax = 0;
	prodfactor_mail = 0;
	if (initial_prod_base < 0) {
		prodbase = desc->get_productivity() + simrand(desc->get_range());
	}
	else {
		prodbase = initial_prod_base;
	}

	delta_sum = 0;
	delta_menge = 0;
	menge_remainder = 0;
	activity_count = 0;
	currently_requiring_power = false;
	currently_producing = false;
	total_input = total_transit = total_output = 0;
	status = STATUS_NOTHING;
	lieferziele_active_last_month = 0;

	// create input information
	input.resize( factory_desc->get_supplier_count() );
	for(  int g=0;  g<factory_desc->get_supplier_count();  ++g  ) {
		const factory_supplier_desc_t *const supp = factory_desc->get_supplier(g);
		input[g].set_typ( supp->get_input_type() );
	}

	// create output information
	output.resize( factory_desc->get_product_count() );
	for(  uint g=0;  g<factory_desc->get_product_count();  ++g  ) {
		const factory_product_desc_t *const product = factory_desc->get_product(g);
		output[g].set_typ( product->get_output_type() );
	}

	recalc_storage_capacities();
	if( welt->get_settings().get_just_in_time() >= 2 ){
		inactive_inputs = inactive_outputs = inactive_demands = 0;
		if(  input.empty()  ){
			// All sources start out with maximum product.
			for(  uint32 out = 0;  out < output.get_count();  out++  ){
				output[out].menge = output[out].max;
				inactive_outputs ++;
			}
		}
		else {
			for(  uint32 out = 0;  out < output.get_count();  out++  ){
				output[out].menge = 0;
			}

			// A consumer of sorts so output and input starts out empty but with a full demand buffer.
			for(  uint32 in = 0;  in < input.get_count();  in++  ){
				input[in].menge = 0;
				input[in].demand_buffer = input[in].max;
				inactive_inputs++;
				inactive_demands++;
			}
		}
	}
	else {
		if(  input.empty()  ) {
			FOR( array_tpl<ware_production_t>, & g, output ) {
				if(  g.max > 0  ) {
					// if source then start with full storage, so that AI will build line(s) immediately
					g.menge = g.max - 1;
				}
			}
		}
	}

	last_sound_ms = welt->get_ticks();
	init_stats();
	arrival_stats_pax.init();
	arrival_stats_mail.init();

	delta_slot = 0;
	times_expanded = 0;
	update_scaled_electric_demand();
	update_scaled_pax_demand();
	update_scaled_mail_demand();
}


fabrik_t::~fabrik_t()
{
	while(!fields.empty()) {
		planquadrat_t *plan = welt->access( fields.back().location );
		// if destructor is called when world is destroyed, plan is already invalid
		if (plan) {
			grund_t *gr = plan->get_kartenboden();
			if (field_t* f = gr->find<field_t>()) {
				delete f; // implicitly removes the field from fields
				plan->boden_ersetzen( gr, new boden_t(gr->get_pos(), slope_t::flat ) );
				plan->get_kartenboden()->calc_image();
				continue;
			}
		}
		fields.pop_back();
	}
	// destroy chart window, if present
	destroy_win((ptrdiff_t)this);
}


void fabrik_t::build(sint32 rotate, bool build_fields, bool force_initial_prodbase)
{
	this->rotate = rotate;
	pos_origin = welt->lookup_kartenboden(pos_origin.get_2d())->get_pos();
	gebaeude_t *gb = hausbauer_t::build(owner, pos_origin.get_2d(), rotate, desc->get_building(), this);
	pos = gb->get_pos();
	pos_origin.z = pos.z;

	if(desc->get_field_group()) {
		// if there are fields
		if(  !fields.empty()  ) {
			for(  uint16 i=0;  i<fields.get_count();  i++   ) {
				const koord k = fields[i].location;
				grund_t *gr=welt->lookup_kartenboden(k);
				if(  gr->ist_natur()  ) {
					// first make foundation below
					grund_t *gr2 = new fundament_t(gr->get_pos(), gr->get_grund_hang());
					welt->access(k)->boden_ersetzen(gr, gr2);
					gr2->obj_add( new field_t(gr2->get_pos(), owner, desc->get_field_group()->get_field_class( fields[i].field_class_index ), this ) );
				}
				else {
					// there was already a building at this position => do not restore!
					fields.remove_at(i);
					i--;
				}
			}
		}
		else if(  build_fields  ) {
			// make sure not to exceed initial prodbase too much
			sint32 org_prodbase = prodbase;
			// we will start with a minimum number and try to get closer to start_fields
			const uint16 spawn_fields = desc->get_field_group()->get_min_fields() + simrand( desc->get_field_group()->get_start_fields()-desc->get_field_group()->get_min_fields() );
			while(  fields.get_count() < spawn_fields  &&  add_random_field(10000u)  ) {
				if (fields.get_count() > desc->get_field_group()->get_min_fields()  &&  prodbase >= 2*org_prodbase) {
					// too much productivity, no more fields needed
					break;
				}
			}
			sint32 field_prod = prodbase - org_prodbase;
			// adjust prodbase
			if (force_initial_prodbase) {
				set_base_production( max(field_prod, org_prodbase) );
			}
		}
	}
	else {
		fields.clear();
	}

	/// Determine control logic
	if( welt->get_settings().get_just_in_time() >= 2 ) {
		// Does it both consume and produce?
		if(  !output.empty() && !input.empty()  ) {
			control_type = CL_FACT_MANY;
		}
		// Does it produce?
		else if( !output.empty() ) {
			control_type = CL_PROD_MANY;
		}
		// Does it consume?
		else if( !input.empty() ) {
			control_type = CL_CONS_MANY;
		}
		// No I/O?
		else {
			control_type = desc->is_electricity_producer() ? CL_ELEC_PROD : CL_NONE;
		}
	}
	else{
		// Classic logic.
		if(  !output.empty() && !input.empty()  ) {
			control_type = CL_FACT_CLASSIC;
		}
		else if( !output.empty() ) {
			control_type = CL_PROD_CLASSIC;
		}
		else if( !input.empty() ) {
			control_type = CL_CONS_CLASSIC;
		}
		else {
			control_type = CL_ELEC_CLASSIC;
		}
	}

	// Boost logic determines what factors boost factory production.
	if( welt->get_settings().get_just_in_time() >= 2 ) {
		if(  !desc->is_electricity_producer()  &&  desc->get_electric_demand() > 0  ) {
			boost_type = BL_POWER;
		}
		else if(  desc->get_pax_demand() ||  desc->get_mail_demand()  ) {
			boost_type = BL_PAXM;
		}
		else {
			boost_type = BL_NONE;
		}
	}
	else {
		boost_type = BL_CLASSIC;
	}

	if(  welt->get_settings().get_just_in_time() >= 2  ) {
		if( input.empty() ) {
			demand_type = DL_NONE;
		}
		else if( output.empty() ) {
			demand_type = DL_ASYNC;
		}
		else {
			demand_type = DL_SYNC;
		}
	}
	else {
		demand_type =  input.empty() ? DL_NONE : DL_OLD;
	}
}


/* field generation code
 */
bool fabrik_t::add_random_field(uint16 probability)
{
	// has fields, and not yet too many?
	const field_group_desc_t *fd = desc->get_field_group();
	if(fd==NULL  ||  fd->get_max_fields() <= fields.get_count()) {
		return false;
	}
	// we are lucky and are allowed to generate a field
	if(  simrand(10000)>=probability  ) {
		return false;
	}

	// we start closest to the factory, and check for valid tiles as we move out
	uint8 radius = 1;

	// pick a coordinate to use - create a list of valid locations and choose a random one
	slist_tpl<grund_t *> build_locations;
	do {
		for(sint32 xoff = -radius; xoff < radius + get_desc()->get_building()->get_size().x ; xoff++) {
			for(sint32 yoff =-radius ; yoff < radius + get_desc()->get_building()->get_size().y; yoff++) {
				// if we can build on this tile then add it to the list
				grund_t *gr = welt->lookup_kartenboden(pos.get_2d()+koord(xoff,yoff));
				if (gr != NULL &&
						gr->get_typ()        == grund_t::boden &&
						get_desc()->get_building()->is_allowed_climate(welt->get_climate(pos.get_2d()+koord(xoff,yoff))) &&
						gr->get_grund_hang() == slope_t::flat &&
						gr->ist_natur() &&
						(gr->find<leitung_t>() || gr->kann_alle_obj_entfernen(NULL) == NULL)) {
					// only on same height => climate will match!
					build_locations.append(gr);
					assert(gr->find<field_t>() == NULL);
				}
				// skip inside of rectangle (already checked earlier)
				if(radius > 1 && yoff == -radius && (xoff > -radius && xoff < radius + get_desc()->get_building()->get_size().x - 1)) {
					yoff = radius + get_desc()->get_building()->get_size().y - 2;
				}
			}
		}
		if (build_locations.empty()) {
			radius++;
		}
	} while (radius < 10 && build_locations.empty());
	// built on one of the positions
	if (!build_locations.empty()) {
		grund_t *gr = build_locations.at(simrand(build_locations.get_count()));
		leitung_t* lt = gr->find<leitung_t>();
		if(lt) {
			gr->obj_remove(lt);
		}
		gr->obj_loesche_alle(NULL);
		// first make foundation below
		const koord k = gr->get_pos().get_2d();
		field_data_t new_field(k);
		assert(!fields.is_contained(new_field));
		// fetch a random field class desc based on spawn weights
		const weighted_vector_tpl<uint16> &field_class_indices = fd->get_field_class_indices();
		new_field.field_class_index = pick_any_weighted(field_class_indices);
		const field_class_desc_t *const field_class = fd->get_field_class( new_field.field_class_index );
		fields.append(new_field);
		grund_t *gr2 = new fundament_t(gr->get_pos(), gr->get_grund_hang());
		welt->access(k)->boden_ersetzen(gr, gr2);
		gr2->obj_add( new field_t(gr2->get_pos(), owner, field_class, this ) );
		// adjust production base and storage capacities
		set_base_production( prodbase + field_class->get_field_production() );
		if(lt) {
			gr2->obj_add( lt );
		}
		gr2->calc_image();
		return true;
	}
	return false;
}


void fabrik_t::remove_field_at(koord pos)
{
	field_data_t field(pos);
	assert(fields.is_contained( field ));
	field = fields[ fields.index_of(field) ];
	const field_class_desc_t *const field_class = desc->get_field_group()->get_field_class( field.field_class_index );
	fields.remove(field);
	// revert the field's effect on production base and storage capacities
	set_base_production( prodbase - field_class->get_field_production() );
}


vector_tpl<fabrik_t *> &fabrik_t::sind_da_welche(koord min_pos, koord max_pos)
{
	static vector_tpl <fabrik_t*> factory_list(16);
	factory_list.clear();

	for(int y=min_pos.y; y<=max_pos.y; y++) {
		for(int x=min_pos.x; x<=max_pos.x; x++) {
			fabrik_t *fab=get_fab(koord(x,y));
			if(fab) {
				if (factory_list.append_unique(fab)) {
//DBG_MESSAGE("fabrik_t::sind_da_welche()","appended factory %s at (%i,%i)",gr->first_obj()->get_fabrik()->get_desc()->get_name(),x,y);
				}
			}
		}
	}
	return factory_list;
}


/**
 * if name==NULL translate desc factory name in game language
 */
char const* fabrik_t::get_name() const
{
	return name ? name.c_str() : translator::translate(desc->get_name(), welt->get_settings().get_name_language_id());
}


void fabrik_t::set_name(const char *new_name)
{
	if(new_name==NULL  ||  strcmp(new_name, translator::translate(desc->get_name(), welt->get_settings().get_name_language_id()))==0) {
		// new name is equal to name given by descriptor/translation -> set name to NULL
		name = NULL;
	}
	else {
		name = new_name;
	}

	fabrik_info_t *win = dynamic_cast<fabrik_info_t*>(win_get_magic((ptrdiff_t)this));
	if (win) {
		win->update_info();
	}
}


void fabrik_t::rdwr(loadsave_t *file)
{
	xml_tag_t f( file, "fabrik_t" );
	sint32 i;
	sint32 owner_n;
	sint32 input_count;
	sint32 output_count;
	sint32 anz_lieferziele;

	if(  file->is_saving()  ) {
		input_count = input.get_count();
		output_count = output.get_count();
		anz_lieferziele = lieferziele.get_count();
		const char *s = desc->get_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s, lengthof(s));
DBG_DEBUG("fabrik_t::rdwr()","loading factory '%s'",s);
		desc = factory_builder_t::get_desc(s);
		if(  desc==NULL  ) {
			//  maybe it was only renamed?
			desc = factory_builder_t::get_desc(translator::compatibility_name(s));
		}
		if(  desc==NULL  ) {
			dbg->warning( "fabrik_t::rdwr()", "Pak-file for factory '%s' missing!", s );
			// we continue loading even if desc==NULL
			welt->add_missing_paks( s, karte_t::MISSING_FACTORY );
		}
	}
	pos_origin.rdwr(file);
	// pos will be assigned after call to hausbauer_t::build
	file->rdwr_byte(rotate);

	if (file->is_version_atleast(122, 1)) {
		file->rdwr_long(delta_sum);
		file->rdwr_long(delta_menge);
		file->rdwr_long(menge_remainder);
		file->rdwr_long(prodfactor_electric);
		file->rdwr_bool(currently_requiring_power);
		file->rdwr_long(total_input);
		file->rdwr_long(total_transit);
		file->rdwr_long(total_output);
		file->rdwr_byte(status);
	}
	else if (file->is_loading()) {
		delta_sum = 0;
		delta_menge = 0;
		menge_remainder = 0;
		prodfactor_electric = 0;
		currently_requiring_power = false;
		total_input = 0;
		total_transit = 0;
		total_output = 0;
		status = STATUS_NOTHING;
	}

	// now rebuild information for received goods
	file->rdwr_long(input_count);
	if(  file->is_loading()  ) {
		input.resize( input_count );
	}
	bool mismatch = false;
	for(  i=0;  i<input_count;  i++  ) {
		ware_production_t &ware = input[i];
		const char *ware_name = NULL;
		sint32 menge = ware.menge;
		if(  file->is_saving()  ) {
			ware_name = ware.get_typ()->get_name();
			if(  file->is_version_less(120, 1)  ) {
				// correct for older saves
				menge = (sint32)(((sint64)ware.menge  * (sint64)desc->get_supplier(i)->get_consumption() )  >> DEFAULT_PRODUCTION_FACTOR_BITS);
			}
		}
		file->rdwr_str(ware_name);
		file->rdwr_long(menge);
		if(  file->is_version_less(110, 5)  ) {
			// max storage is only loaded/saved for older versions
			file->rdwr_long(ware.max);
		}
		//  JIT2 needs to store input demand buffer
		if(  welt->get_settings().get_just_in_time() >= 2  &&  file->is_version_atleast(120, 1)  ){
			file->rdwr_long(ware.demand_buffer);
		}

		ware.rdwr( file );

		if(  file->is_loading()  ) {
			if (!ware_name) {
				dbg->fatal("fabrik_t::rdwr", "Invalid ware at input slot %d of factory at %s", i, pos.get_fullstr());
			}
			ware.set_typ( goods_manager_t::get_info(ware_name) );

			// Maximum in-transit is always 0 on load.
			if(  welt->get_settings().get_just_in_time() < 2  ) {
				ware.max_transit = 0;
			}

			if(  !desc  ||  !desc->get_supplier(i)  ) {
				if (desc) {
					dbg->warning( "fabrik_t::rdwr()", "Factory at %s requested producer for %s but has none!", pos_origin.get_fullstr(), ware_name);
				}
				ware.menge = 0;
			}
			else {
				// Inputs used to be with respect to actual units of production. They now are normalized with respect to factory production so require conversion.
				const uint32 prod_factor = desc ? desc->get_supplier(i)->get_consumption() : 1;
				if(  file->is_version_less(120, 1)  ) {
					ware.menge = (sint32)(((sint64)menge << DEFAULT_PRODUCTION_FACTOR_BITS) / (sint64)prod_factor);
				}
				else {
					ware.menge = menge;
				}

				// repair files that have 'insane' values
				const sint32 max = (sint32)((((sint64)FAB_MAX_INPUT << (precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS)) + (sint64)(prod_factor - 1)) / (sint64)prod_factor);
				if(  ware.menge < 0  ) {
					ware.menge = 0;
				}
				if(  ware.menge > max  ) {
					ware.menge = max;
				}

				if (ware.get_typ() != desc->get_supplier(i)->get_input_type()) {
					mismatch = true;
					dbg->warning("fabrik_t::rdwr", "Factory at %s: producer[%d] mismatch in savegame=%s/%s, in pak=%s",
							 pos_origin.get_fullstr(), i, ware_name, ware.get_typ()->get_name(), desc->get_supplier(i)->get_input_type()->get_name());
				}
			}
			free(const_cast<char *>(ware_name));
		}
	}
	if(  desc  &&  input_count != desc->get_supplier_count() ) {
		dbg->warning("fabrik_t::rdwr", "Mismatch of input slot count for factory %s at %s: savegame = %d, pak = %d", get_name(), pos_origin.get_fullstr(), input_count, desc->get_supplier_count());
		// resize input to match the descriptor
		input.resize( desc->get_supplier_count() );
		mismatch = true;
	}
	if (mismatch) {
		array_tpl<ware_production_t> dummy;
		dummy.resize(desc->get_supplier_count());
		for(uint16 i=0; i<desc->get_supplier_count(); i++) {
			dummy[i] = input[i];
		}
		for(uint16 i=0; i<desc->get_supplier_count(); i++) {
			// search for matching type
			bool missing = true;
			const goods_desc_t* goods = desc->get_supplier(i)->get_input_type();
			for(uint16 j=0; j<desc->get_supplier_count()  &&  missing; j++) {
				if (dummy[j].get_typ() == goods) {
					input[i] = dummy[j];
					dummy[j].set_typ(NULL);
					missing = false;
				}
			}
			if (missing) {
				input[i].set_typ(goods);
			}
		}
	}

	// now rebuilt information for produced goods
	file->rdwr_long(output_count);
	if(  file->is_loading()  ) {
		output.resize( output_count );
	}
	mismatch = false;
	for(  i=0;  i<output_count;  ++i  ) {
		ware_production_t &ware = output[i];
		const char *ware_name = NULL;
		sint32 menge = ware.menge;
		if(  file->is_saving()  ) {
			ware_name = ware.get_typ()->get_name();
			// correct scaling for older saves
			if(  file->is_version_less(120, 1)  ){
				menge = (sint32)(((sint64)ware.menge * desc->get_product(i)->get_factor() ) >> DEFAULT_PRODUCTION_FACTOR_BITS);
			}
		}
		file->rdwr_str(ware_name);
		file->rdwr_long(menge);
		if(  file->is_version_less(110, 5)  ) {
			// max storage is only loaded/saved for older versions
			file->rdwr_long(ware.max);
			// obsolete variables -> statistics already contain records on goods delivered
			sint32 abgabe_sum = (sint32)(ware.get_stat(0, FAB_GOODS_DELIVERED));
			sint32 abgabe_letzt = (sint32)(ware.get_stat(1, FAB_GOODS_DELIVERED));
			file->rdwr_long(abgabe_sum);
			file->rdwr_long(abgabe_letzt);
		}
		ware.rdwr( file );
		if(  file->is_loading()  ) {
			if (ware_name && goods_manager_t::get_info(ware_name)) {
				ware.set_typ( goods_manager_t::get_info(ware_name));
			}
			else {
				dbg->fatal("fabrik_t::rdwr", "Invalid ware %s at output slot %d of factory at %s", ware_name ? ware_name : "", i, pos.get_fullstr());
			}

			if(  !desc  ||  !desc->get_product(i)  ) {
				if (desc) {
					dbg->warning( "fabrik_t::rdwr()", "Factory at %s requested consumer for %s but has none!", pos_origin.get_fullstr(), ware_name );
				}
				ware.menge = 0;
			}
			else {
				// Outputs used to be with respect to actual units of production. They now are normalized with respect to factory production so require conversion.
				if(  file->is_version_less(120, 1)  ){
					const uint32 prod_factor = desc ? desc->get_product(i)->get_factor() : 1;
					ware.menge = (sint32)(((sint64)menge << DEFAULT_PRODUCTION_FACTOR_BITS) / (sint64)prod_factor);
				}
				else {
					ware.menge = menge;
				}

				// repair files that have 'insane' values
				if(  ware.menge < 0  ) {
					ware.menge = 0;
				}

				if (ware.get_typ() != desc->get_product(i)->get_output_type()) {
					mismatch = true;
					dbg->warning("fabrik_t::rdwr", "Factory at %s: consumer[%d] mismatch in savegame=%s/%s, in pak=%s",
							 pos_origin.get_fullstr(), i, ware_name, ware.get_typ()->get_name(), desc->get_product(i)->get_output_type()->get_name());
				}
			}
			free(const_cast<char *>(ware_name));
		}
	}
	if(  desc  &&  output_count != desc->get_product_count()) {
		dbg->warning("fabrik_t::rdwr", "Mismatch of output slot count for factory %s at %s: savegame = %d, pak = %d", get_name(), pos_origin.get_fullstr(), output_count, desc->get_product_count());
		// resize output to match the descriptor
		output.resize( desc->get_product_count() );
		mismatch = true;
	}
	if (mismatch) {
		array_tpl<ware_production_t> dummy;
		dummy.resize(desc->get_product_count());
		for(uint16 i=0; i<desc->get_product_count(); i++) {
			dummy[i] = output[i];
		}
		for(uint16 i=0; i<desc->get_product_count(); i++) {
			// search for matching type
			bool missing = true;
			const goods_desc_t* goods = desc->get_product(i)->get_output_type();
			for(uint16 j=0; j<desc->get_product_count()  &&  missing; j++) {
				if (dummy[j].get_typ() == goods) {
					output[i] = dummy[j];
					dummy[j].set_typ(NULL);
					missing = false;
				}
			}
			if (missing) {
				output[i].set_typ(goods);
			}
		}
	}

	// restore other information
	owner_n = welt->sp2num(owner);
	file->rdwr_long(owner_n);
	file->rdwr_long(prodbase);
	if(  file->is_version_less(110, 5)  ) {
		// prodfactor saving no longer required
		sint32 adjusted_value = (prodfactor_electric / 16) + 16;
		file->rdwr_long(adjusted_value);
	}

	// no longer save power at factories
	if(  file->is_version_atleast(99, 17) && file->is_version_less(120, 4)  ) {
		sint32 power = 0;
		file->rdwr_long(power);
	}
	if(  file->is_version_atleast(120, 1) && file->is_version_less(120, 4)  ) {
		sint32 power_demand = 0;
		file->rdwr_long(power_demand);
	}

	// owner stuff
	if(  file->is_loading()  ) {
		// take care of old files
		if(  file->is_version_less(86, 1)  ) {
			koord k = desc ? desc->get_building()->get_size() : koord(1,1);
			DBG_DEBUG("fabrik_t::rdwr()","correction of production by %i",k.x*k.y);
			// since we step from 86.01 per factory, not per tile!
			prodbase *= k.x*k.y*2;
		}
		// restore factory owner
		// Due to a omission in Volkers changes, there might be savegames
		// in which factories were saved without an owner. In this case
		// set the owner to the default of player 1
		if(owner_n == -1) {
			// Use default
			owner = welt->get_public_player();
		}
		else {
			// Restore owner pointer
			owner = welt->get_player(owner_n);
		}
	}

	file->rdwr_long(anz_lieferziele);

	// connect/save consumer
	for(int i=0; i<anz_lieferziele; i++) {
		if(file->is_loading()) {
			lieferziele.append(koord::invalid);
		}
		lieferziele[i].rdwr(file);
	}

	if(  file->is_version_atleast(112, 2)  ) {
		file->rdwr_long( lieferziele_active_last_month );
	}

	// suppliers / consumers will be recalculated in finish_rd
	if (file->is_loading()  &&  welt->get_settings().is_crossconnect_factories()) {
		lieferziele.clear();
	}

	// information on fields ...
	if(  file->is_version_atleast(99, 10)  ) {
		if(  file->is_saving()  ) {
			uint16 nr=fields.get_count();
			file->rdwr_short(nr);
			if(  file->is_version_atleast(102, 3)  ) {
				// each field stores location and a field class index
				for(  uint16 i=0  ;  i<nr  ;  ++i  ) {
					koord k = fields[i].location;
					k.rdwr(file);
					uint16 idx = fields[i].field_class_index;
					file->rdwr_short(idx);
				}
			}
			else {
				// each field only stores location
				for(  uint16 i=0  ;  i<nr  ;  ++i  ) {
					koord k = fields[i].location;
					k.rdwr(file);
				}
			}
		}
		else {
			uint16 nr=0;
			koord k;
			file->rdwr_short(nr);
			fields.resize(nr);
			if(  file->is_version_atleast(102, 3)  ) {
				// each field stores location and a field class index
				for(  uint16 i=0  ;  i<nr  ;  ++i  ) {
					k.rdwr(file);
					uint16 idx;
					file->rdwr_short(idx);
					if(  desc  &&  desc->get_field_group()  ) {
						// set class index to 0 if it is out of range, if there fields at all
						fields.append( field_data_t(k, idx >= desc->get_field_group()->get_field_class_count() ? 0 : idx ) );
					}
				}
			}
			else {
				// each field only stores location
				for(  uint16 i=0  ;  i<nr  ;  ++i  ) {
					k.rdwr(file);
					if(  desc  &&  desc->get_field_group()  ) {
						// oald add fields if there are any defined
						fields.append( field_data_t(k, 0) );
					}
				}
			}
		}
	}

	// restore city pointer here
	if(  file->is_version_atleast(99, 14)  ) {
		sint32 nr = target_cities.get_count();
		file->rdwr_long(nr);
		for(  int i=0;  i<nr;  i++  ) {
			sint32 city_index = -1;
			if(file->is_saving()) {
				city_index = welt->get_cities().index_of( target_cities[i] );
			}
			file->rdwr_long(city_index);
			if(  file->is_loading()  ) {
				// will also update factory information
				target_cities.append( welt->get_cities()[city_index] );
			}
		}
	}
	else if(  file->is_loading()  ) {
		// will be handled by the city after reloading
		target_cities.clear();
	}

	if(  file->is_version_atleast(110, 5)  ) {
		file->rdwr_short(times_expanded);
		// statistics
		for(  int s=0;  s<MAX_FAB_STAT;  ++s  ) {
			for(  int m=0;  m<MAX_MONTH;  ++m  ) {
				file->rdwr_longlong( statistics[m][s] );
			}
		}
		file->rdwr_longlong( weighted_sum_production );
		file->rdwr_longlong( weighted_sum_boost_electric );
		file->rdwr_longlong( weighted_sum_boost_pax );
		file->rdwr_longlong( weighted_sum_boost_mail );
		file->rdwr_longlong( weighted_sum_power );
		file->rdwr_longlong( aggregate_weight );
		file->rdwr_long( delta_slot );
	}
	else if(  file->is_loading()  ) {
		times_expanded = 0;
		init_stats();
		delta_slot = 0;
	}
	arrival_stats_pax.rdwr( file );
	arrival_stats_mail.rdwr( file );

	if(  file->is_version_atleast(110, 7)  ) {
		file->rdwr_byte(activity_count);
	}
	else if(  file->is_loading()  ) {
		activity_count = 0;
	}

	// save name
	if(  file->is_version_atleast(110, 7)  ) {
		if(  file->is_saving() &&  !name  ) {
			char const* fullname = desc->get_name();
			file->rdwr_str(fullname);
		}
		else {
			file->rdwr_str(name);
			if(  file->is_loading()  &&  desc != NULL  &&  name == desc->get_name()  ) {
				// equal to desc name
				name = 0;
			}
		}
	}
}


/**
 * let the chimney smoke, if there is something to produce
 */
void fabrik_t::smoke()
{
	const smoke_desc_t *rada = desc->get_smoke();
	if(rada) {
		const uint8 rot = (4-rotate)%desc->get_building()->get_all_layouts();
		grund_t *gr = welt->lookup_kartenboden(pos_origin.get_2d()+desc->get_smoketile( rot ));
		const koord offset = ( desc->get_smokeoffset(rot)*OBJECT_OFFSET_STEPS)/16;
		wolke_t *smoke =  new wolke_t(gr->get_pos(), offset.x, 0, offset.y, desc->get_smokelifetime(), desc->get_smokeuplift(), rada->get_images() );
		gr->obj_add(smoke);
		welt->sync_way_eyecandy.add( smoke );
	}

	// maybe sound?
	if(  desc->get_sound()!=NO_SOUND  &&  welt->get_ticks()>last_sound_ms+desc->get_sound_interval_ms()  ) {
		last_sound_ms = welt->get_ticks();
		welt->play_sound_area_clipped( get_pos().get_2d(), desc->get_sound(), FACTORY_SOUND );
	}
}


uint32 fabrik_t::scale_output_production(const uint32 product, uint32 menge) const
{
	// prorate production based upon amount of product in storage
	// but allow full production rate for storage amounts less than twice the minimum shipment size.
	const uint32 maxi = output[product].max;
	const uint32 actu = output[product].menge;
	if(  actu<maxi  ) {
		if(  actu >= (uint32)(2 * output[product].min_shipment) ) {
			if(  menge>(0x7FFFFFFFu/maxi)  ) {
				// avoid overflow
				menge = (((maxi-actu)>>5)*(menge>>5))/(maxi>>10);
			}
			else {
				// and that is the simple formula
				menge = (menge*(maxi-actu)) / maxi;
			}
		}
	}
	else {
		// overfull? No production
		menge = 0;
	}
	return menge;
}


/************** TODO: properly handle more than one transformer! *******************************/

void fabrik_t::set_power_supply(uint32 supply)
{
	if( transformers.empty() ) {
		return;
	}
	pumpe_t *const trans = dynamic_cast<pumpe_t *>(transformers.front());
	if(  trans == NULL  ) {
		return;
	}
	trans->set_power_supply(supply);
}

uint32 fabrik_t::get_power_supply() const
{
	if( transformers.empty() ) {
		return 0;
	}
	pumpe_t *const trans = dynamic_cast<pumpe_t *>(transformers.front());
	if(  trans == NULL  ) {
		return 0;
	}
	return trans->get_power_supply();
}

sint32 fabrik_t::get_power_consumption() const
{
	if( transformers.empty() ) {
		return 0;
	}
	pumpe_t *const trans = dynamic_cast<pumpe_t *>(transformers.front());
	if(  trans == NULL  ) {
		return 0;
	}
	return trans->get_power_consumption();
}

void fabrik_t::set_power_demand(uint32 demand)
{
	if( transformers.empty() ) {
		return;
	}
	senke_t *const trans = dynamic_cast<senke_t *>(transformers.front());
	if(  trans == NULL  ) {
		return;
	}
	trans->set_power_demand(demand);
}

uint32 fabrik_t::get_power_demand() const
{
	if( transformers.empty() ) {
		return 0;
	}
	senke_t *const trans = dynamic_cast<senke_t *>(transformers.front());
	if(  trans == NULL  ) {
		return 0;
	}
	return trans->get_power_demand();
}

sint32 fabrik_t::get_power_satisfaction() const
{
	if( transformers.empty() ) {
		return 0;
	}
	senke_t *const trans = dynamic_cast<senke_t *>(transformers.front());
	if(  trans == NULL  ) {
		return 0;
	}
	return trans->get_power_satisfaction();
}

sint64 fabrik_t::get_power() const
{
	if(  desc->is_electricity_producer()  ) {
		return ((sint64)get_power_supply() * (sint64)get_power_consumption()) >> leitung_t::FRACTION_PRECISION;
	}
	else {
		return -(((sint64)get_power_demand() * (sint64)get_power_satisfaction() + (((sint64)1 << leitung_t::FRACTION_PRECISION) - 1)) >> leitung_t::FRACTION_PRECISION);
	}
}


sint32 fabrik_t::input_vorrat_an(const goods_desc_t *typ)
{
	sint32 menge = -1;

	FOR(array_tpl<ware_production_t>, const& i, input) {
		if (typ == i.get_typ()) {
			menge = i.menge >> precision_bits;
			break;
		}
	}

	return menge;
}


sint32 fabrik_t::vorrat_an(const goods_desc_t *typ)
{
	sint32 menge = -1;

	FOR(array_tpl<ware_production_t>, const& i, output) {
		if (typ == i.get_typ()) {
			menge = i.menge >> precision_bits;
			break;
		}
	}

	return menge;
}


sint32 fabrik_t::liefere_an(const goods_desc_t *typ, sint32 menge)
{
	if(  typ==goods_manager_t::passengers  ) {
		// book pax arrival and recalculate pax boost
		book_stat(menge, FAB_PAX_ARRIVED);
		arrival_stats_pax.book_arrival(menge);
		update_prodfactor_pax();
		return menge;
	}
	else if(  typ==goods_manager_t::mail  ) {
		// book mail arrival and recalculate mail boost
		book_stat(menge, FAB_MAIL_ARRIVED);
		arrival_stats_mail.book_arrival(menge);
		update_prodfactor_mail();
		return menge;
	}
	else {
		// case : freight
		for(  uint32 in = 0;  in < input.get_count();  in++  ){
			ware_production_t& ware = input[in];
			if(  ware.get_typ() == typ  ) {
				ware.book_stat( -menge, FAB_GOODS_TRANSIT );

				// Resolve how much normalized production arrived, rounding up (since we rounded up when we removed it).
				const uint32 prod_factor = desc->get_supplier(in)->get_consumption();
				const sint32 prod_delta = (sint32)((((sint64)menge << (DEFAULT_PRODUCTION_FACTOR_BITS + precision_bits)) + (sint64)(prod_factor - 1)) / (sint64)prod_factor);

				// Activate inactive inputs.
				if( ware.menge <= 0 ) {
					inactive_inputs --;
				}
				ware.menge+= prod_delta;

				if( welt->get_settings().get_just_in_time() >= 2 ){
					// In JIT2 it is illegal to exceed input storage limit. All production that does so is discarded.
					if( ware.menge > ware.max ){
						const sint32 prod_comp = ware.menge - ware.max;
						ware.menge = ware.max;

						// Apply demand penalty. Reactivate inactive demands.
						sint32 demand_old = ware.demand_buffer;
						ware.demand_buffer-= prod_comp;
						if( demand_old >= ware.max && ware.demand_buffer < ware.max ) inactive_demands-= 1;

						// Resolve how many goods we actually used (rounding to nearest)
						menge -= (sint32)(((sint64)prod_comp * (sint64)prod_factor + (1 << (DEFAULT_PRODUCTION_FACTOR_BITS + precision_bits - 1))) >> (DEFAULT_PRODUCTION_FACTOR_BITS + precision_bits));
					}
				}
				else{
					// avoid overflow
					const sint32 max = (sint32)((((sint64)FAB_MAX_INPUT << (precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS)) + (sint64)(prod_factor - 1)) / (sint64)prod_factor);
					if( ware.menge >= max ) {
						menge -= (sint32)(((sint64)menge * (sint64)(ware.menge - max) + (sint64)(prod_delta >> 1)) / (sint64)prod_delta);
						ware.menge = max - 1;
					}
				}
				ware.book_stat(menge, FAB_GOODS_RECEIVED);
				return menge;
			}
		}
	}
	// ware "typ" wird hier nicht verbraucht
	return -1;
}


sint8 fabrik_t::is_needed(const goods_desc_t *typ) const
{
	FOR(array_tpl<ware_production_t>, const& i, input) {
		if(  i.get_typ() == typ  ) {
			// Ordering logic is done in ticks. This means that every supplier is given a chance to fulfill an order (which they do so fairly).
			return i.placing_orders;
		}
	}
	return -1;  // not needed here
}


bool fabrik_t::is_active_lieferziel( koord k ) const
{
	if ( lieferziele.is_contained(k) ) {
		return 0 < ( ( 1 << lieferziele.index_of(k) ) & lieferziele_active_last_month );
	}
	return false;
}


sint32 fabrik_t::get_jit2_power_boost() const
{
	if (!is_transformer_connected()) {
		return 0;
	}

	const sint32 power_satisfaction = get_power_satisfaction();
	if(  power_satisfaction >= ((sint32)1 << leitung_t::FRACTION_PRECISION)  ) {
		// all demand fulfilled
		return (sint32)desc->get_electric_boost();
	}
	else {
		// calculate bonus, rounding down
		return (sint32)(((sint64)desc->get_electric_boost() * (sint64)power_satisfaction) >> leitung_t::FRACTION_PRECISION);
	}
}


void fabrik_t::step(uint32 delta_t)
{
	// Only do something if advancing in time.
	if(  delta_t==0  ) {
		return;
	}

	/// Declare production control variables.

	// The production effort of the factory.
	sint32 prod;

	// Actual production effort done.
	sint32 work = 0;

	// Desired production effort of factory.
	sint32 want = 0;

	/// Boost logic.

	// Solve boosts.
	switch(  boost_type  ) {
		case BL_CLASSIC: {
			// JIT1 implementation for power bonus.
			if(  !desc->is_electricity_producer()  &&  scaled_electric_demand > 0  ) {
				// one may be thinking of linking this to actual production only
				prodfactor_electric = (sint32)(((sint64)desc->get_electric_boost() * (sint64)get_power_satisfaction() + (sint64)((sint64)1 << (leitung_t::FRACTION_PRECISION - 1))) >> leitung_t::FRACTION_PRECISION);
			}
			break;
		}
		case BL_POWER: {
			// get desired power boost amount
			sint32 const prodfactor_want = get_jit2_power_boost();

			// calculate maximum change delta from change rate scaled by time
			const sint32 prodfactor_change = max(BOOST_POWER_CHANGE_RATE * delta_t / PRODUCTION_DELTA_T, 1);

			// limit rate of change of electricity boost (improve stability)
			const sint32 prodfactor_delta = prodfactor_want - prodfactor_electric;
			if(  prodfactor_delta  >  prodfactor_change  ) {
				// limit increase rate
				prodfactor_electric += prodfactor_change;
			}
			// limit decrease rate, off because of possible exploit
//			else if(  prodfactor_delta  <  -prodfactor_change  ) {
//				prodfactor_electric -= prodfactor_change;
//			}
			else {
				// no limit
				prodfactor_electric = prodfactor_want;
			}

			break;
		}
		default: {
			// No boost amounts to solve.
			break;
		}
	};

	// Compute boost amount.
	const sint32 boost = get_prodfactor();

	/// Compute base production.
	{
		// Calculate actual production. A remainder is used for extra precision.
		const uint32 remainder_bits = (PRODUCTION_DELTA_T_BITS + DEFAULT_PRODUCTION_FACTOR_BITS + DEFAULT_PRODUCTION_FACTOR_BITS - fabrik_t::precision_bits);
		const uint64 want_prod_long = (uint64)prodbase * (uint64)boost * (uint64)delta_t + (uint64)menge_remainder;
		prod = (uint32)(want_prod_long >> remainder_bits);
		menge_remainder = (uint32)(want_prod_long & ((1 << remainder_bits) - 1 ));
	}

	/// Perform the control logic.
	{
		// Common logic locals go here.

		// Amount of production change.
		sint32 prod_delta;
		// The amount of production available.
		sint32 prod_comp;
		// The amount of consumption available.
		sint32 cons_comp = 0;

		switch(  control_type  ) {
			case CL_PROD_CLASSIC: {
				// Classic producer logic.
				currently_requiring_power = false;
				currently_producing = false;

				// produces something
				for(  uint32 product = 0;  product < output.get_count();  product++  ) {
					// source producer
					const uint32 menge_out = scale_output_production( product, prod );

					if(  menge_out > 0  ) {
						const sint32 p = (sint32)menge_out;

						// compute work factor
						const sint32 work_fact = work_from_production(prod, p);

						// work done is work done of maximum output
						if(  work_fact  >  work  ) {
							work = work_fact;
						}

						// produce
						if(  output[product].menge < output[product].max  ) {
							// to find out, if storage changed
							delta_menge += p;
							output[product].menge += p;
							output[product].book_stat((sint64)p * (sint64)desc->get_product(product)->get_factor(), FAB_GOODS_PRODUCED);
							// if less than 3/4 filled we neary always consume power
							currently_requiring_power |= (output[product].menge*4 < output[product].max*3);
							currently_producing = true;
						}
						else {
							output[product].book_stat((sint64)(output[product].max - 1 - output[product].menge) * (sint64)desc->get_product(product)->get_factor(), FAB_GOODS_PRODUCED);
							output[product].menge = output[product].max - 1;
						}
					}
				}

				break;
			}
			case CL_PROD_MANY: {
				// A producer with many outputs.
				if(  inactive_outputs == output.get_count()  ) {
					break;
				}

				currently_requiring_power = false;

				for(  uint32 product = 0;  product < output.get_count();  product++  ) {
					prod_comp = output[product].max - output[product].menge;

					// Ignore inactive outputs.
					if(  prod_comp <= 0  ) {
						continue;
					}

					// get desired work factor
					const sint32 work_fact = output[product].calculate_output_production_rate();

					// compute desired production
					prod_delta = work_scale_production(prod, work_fact);

					// Cannot produce more than can be stored.
					if(  prod_delta > prod_comp  ) {
						prod_delta = prod_comp;
					}

					currently_requiring_power = (prod_delta>0);

					delta_menge += prod_delta;
					output[product].menge += prod_delta;
					output[product].book_stat((sint64)prod_delta * (sint64)desc->get_product(product)->get_factor(), FAB_GOODS_PRODUCED);

					work += work_fact;
				}
				currently_producing = currently_requiring_power;

				// normalize work with respect to output number
				work /= output.get_count();

				break;
			}
			case CL_FACT_CLASSIC: {
				// Classic factory logic, work and want determined by the maximum output production rate.
				currently_requiring_power = false;
				currently_producing = false;

				// ok, calulate maximum allowed consumption.
				{
					sint32 min_menge = input[0].menge;
					sint32 consumed_menge = 0;
					for(  uint32 index = 1;  index < input.get_count();  index++  ) {
						if(  input[index].menge < min_menge  ) {
							min_menge = input[index].menge;
						}
					}

					// produces something
					for(  uint32 product = 0;  product < output.get_count();  product++  ) {
						// calculate production
						const sint32 p_menge = (sint32)scale_output_production( product, prod );

						// Want the maximum production rate, this is so correct amount is ordered.
						if(  p_menge > want  ) {
							want = p_menge;
						}

						const sint32 menge_out = p_menge < min_menge ? p_menge : min_menge;  // production smaller than possible due to consumption
						if(  menge_out > consumed_menge  ) {
							consumed_menge = menge_out;
						}

						if(  menge_out > 0  ) {
							const sint32 p = menge_out;

							// produce
							if(  output[product].menge < output[product].max  ) {
								// to find out, if storage changed
								delta_menge += p;
								output[product].menge += p;
								output[product].book_stat((sint64)p * (sint64)desc->get_product(product)->get_factor(), FAB_GOODS_PRODUCED);
								// if less than 3/4 filled we neary always consume power
								currently_requiring_power |= (output[product].menge*4 < output[product].max*3);
								currently_producing = true;
							}
							else {
								output[product].book_stat((sint64)(output[product].max - 1 - output[product].menge) * (sint64)desc->get_product(product)->get_factor(), FAB_GOODS_PRODUCED);
								output[product].menge = output[product].max - 1;
							}
						}
					}

					// and finally consume stock
					for(  uint32 index = 0;  index < input.get_count();  index++  ) {
						const uint32 v = consumed_menge;

						if(  (uint32)input[index].menge > v + 1  ) {
							input[index].menge -= v;
							input[index].book_stat((sint64)v * (sint64)desc->get_supplier(index)->get_consumption(), FAB_GOODS_CONSUMED);
						}
						else {
							input[index].book_stat((sint64)input[index].menge * (sint64)desc->get_supplier(index)->get_consumption(), FAB_GOODS_CONSUMED);
							input[index].menge = 0;
						}
					}

					// work done is consumption rate
					work = work_from_production(prod, consumed_menge);
				}

				break;
			}
			case CL_FACT_MANY: {
				// Logic for a factory with many outputs. Work and want are based on weighted output rate so is more complicated.
				currently_requiring_power = false;
				currently_producing = false;
				if(  inactive_outputs == output.get_count()  ) {
					break;
				}
				{
					const bool no_input = inactive_inputs > 0;

					// Determine minimum input in form of production from all outputs. This limits production.
					// Determine minimum demand work rate from all inputs. This limits production rate.
					sint32 demand_work_limt = input[0].calculate_demand_production_rate();
					cons_comp = input[0].menge;
					for(  uint32 index = 1;  index < input.get_count();  index++  ) {
						if(  input[index].menge < cons_comp  ) {
							cons_comp = input[index].menge;
						}
						sint32 const work_limit = input[index].calculate_demand_production_rate();
						if(  work_limit < demand_work_limt  ) {
							demand_work_limt = work_limit;
						}

					}
					cons_comp *= output.get_count();

					for(  uint32 product = 0;  product < output.get_count();  product++  ) {
						prod_comp = output[product].max - output[product].menge;

						// Ignore inactive outputs.
						if(  prod_comp <= 0  ) {
							continue;
						}

						// get desired work factor
						const sint32 work_fact = min(output[product].calculate_output_production_rate(), demand_work_limt);

						// compute desired production
						prod_delta = work_scale_production(prod, work_fact);

						// limit to maximum storage
						if(  prod_delta > prod_comp  ) {
							prod_delta = prod_comp;
						}

						// credit desired production for want
						want += prod_delta;

						// skip producing anything if no input
						if(  no_input  ) {
							continue;
						}

						// enforce input limits on production
						if(  prod_delta > cons_comp  ) {
							// not enough input
							prod_delta = cons_comp;
							cons_comp = 0;
						}
						else {
							// enough input
							cons_comp -= prod_delta;
						}

						// register inactive outputs
						if(  prod_delta == prod_comp  ) {
							inactive_outputs++;
							if(  inactive_outputs == output.get_count()  ) {
								currently_requiring_power = false;
							}
						}

						// credit output work done
						work += prod_delta;

						currently_requiring_power = true;
						currently_producing = true;

						// Produce output
						delta_menge += prod_delta;
						output[product].menge += prod_delta;
						output[product].book_stat((sint64)prod_delta * (sint64)desc->get_product(product)->get_factor(), FAB_GOODS_PRODUCED);
					}

					// normalize want with respect to output number
					want /= output.get_count();

					// skip consuming anything if no input
					if(  no_input  ) {
						break;
					}

					// compute work done
					const sint32 consumed = work / output.get_count();
					work = work_from_production(prod, consumed);

					// consume inputs
					for(  uint32 index = 0;  index < input.get_count();  index++  ) {
						input[index].menge -= consumed;
						input[index].book_stat((sint64)consumed * (sint64)desc->get_supplier(index)->get_consumption(), FAB_GOODS_CONSUMED);

						// register inactive inputs
						if(  input[index].menge <= 0  ) {
							currently_requiring_power = false;
							input[index].menge = 0;
							inactive_inputs ++;
						}
					}
				}
				break;
			}
			case CL_CONS_CLASSIC: {
				uint32 power = 0;

				// Classic consumer logic.
				currently_requiring_power = false;
				currently_producing = false;

				if(  desc->is_electricity_producer()  ) {
					// power station => start with no production
					power = 0;
				}

				// Always want to consume at rate of prod.
				want = prod;

				// finally consume stock
				for(  uint32 index = 0;  index < input.get_count();  index++  ) {
					const uint32 v = prod;

					if(  (uint32)input[index].menge > v + 1  ) {
						input[index].menge -= v;
						input[index].book_stat((sint64)v * (sint64)desc->get_supplier(index)->get_consumption(), FAB_GOODS_CONSUMED);
						currently_requiring_power = true;
						currently_producing = true;
						if(  desc->is_electricity_producer()  ) {
							// power station => produce power
							power += (uint32)( ((sint64)scaled_electric_demand * (sint64)(DEFAULT_PRODUCTION_FACTOR + prodfactor_pax + prodfactor_mail)) >> DEFAULT_PRODUCTION_FACTOR_BITS );
						}
						work += 1 << WORK_BITS;
						// to find out, if storage changed
						delta_menge += v;
					}
					else {
						if(  desc->is_electricity_producer()  ) {
							// power station => produce power
							power += (uint32)( (((sint64)scaled_electric_demand * (sint64)(DEFAULT_PRODUCTION_FACTOR + prodfactor_pax + prodfactor_mail)) >> DEFAULT_PRODUCTION_FACTOR_BITS) * input[index].menge / (v + 1) );
						}
						delta_menge += input[index].menge;
						work += work_from_production(prod, input[index].menge);
						input[index].book_stat((sint64)input[index].menge * (sint64)desc->get_supplier(index)->get_consumption(), FAB_GOODS_CONSUMED);
						input[index].menge = 0;
					}
				}

				// normalize work with respect to input number
				work /= input.get_count();
				set_power_supply(power);

				break;
			}
			case CL_CONS_MANY: {
				// Consumer logic for many inputs. Work done is based on the average consumption of all inputs.
				currently_requiring_power = false;
				currently_producing = false;

				// always consume prod
				want = prod;

				// Do nothing if we cannot consume anything.
				if(  inactive_inputs == input.get_count()  ) {
					break;
				}

				for(  uint32 index = 0;  index < input.get_count();  index++  ) {

					// Only process active inputs;
					if(  input[index].menge <= 0  ) {
						continue;
					}
					currently_requiring_power = true;
					currently_producing = true;

					sint32 const consumption_prod = work_scale_production(prod, input[index].calculate_demand_production_rate());

					// limit consumption to minimum of production or storage
					prod_delta = input[index].menge > consumption_prod ? consumption_prod : input[index].menge;

					// add to work done
					work += prod_delta;

					// consume input
					delta_menge += prod_delta;
					input[index].menge -= prod_delta;
					input[index].book_stat((sint64)prod_delta * (sint64)desc->get_supplier(index)->get_consumption(), FAB_GOODS_CONSUMED);

					// register inactive input
					if(  input[index].menge <= 0  ) {
						inactive_inputs++;
						if(  inactive_inputs == input.get_count()  ) {
							currently_requiring_power = false;
						}
					}
				}

				// normalise and convert work into work factor
				work = work_from_production(prod, work / input.get_count());

				if(  desc->is_electricity_producer()  ) {
					// compute power production
					uint64 pp = ((uint64)scaled_electric_demand * (uint64)boost * (uint64)work) >> (DEFAULT_PRODUCTION_FACTOR_BITS + WORK_BITS);
					set_power_supply((uint32)pp);
				}

				break;
			}
			case CL_ELEC_PROD: {
				// A simple no input electricity producer, like a solar array.

				// always maximum work
				currently_requiring_power = true;
				currently_producing = true;
				work = 1 << WORK_BITS;
				delta_menge += prod;

				// compute power production
				uint64 pp = ((uint64)scaled_electric_demand * (uint64)boost) >> DEFAULT_PRODUCTION_FACTOR_BITS;
				set_power_supply((uint32)pp);

				break;
			}
			case CL_ELEC_CLASSIC: {
				// Classic no input power producer.
				currently_requiring_power = false;
				work = 1 << WORK_BITS;

				// power station? => produce power
				if(  desc->is_electricity_producer()  ) {
					currently_requiring_power = true;
					currently_producing = true;
					set_power_supply((uint32)( ((sint64)scaled_electric_demand * (sint64)(DEFAULT_PRODUCTION_FACTOR + prodfactor_pax + prodfactor_mail)) >> DEFAULT_PRODUCTION_FACTOR_BITS ));
				}
				break;
			}
			case CL_NONE:
			default: {
				currently_producing = false;
				currently_requiring_power = false;
				// None always produces maximum for whatever reason. Also default.
				work = 1 << WORK_BITS;
				break;
			}
		}
	}

	/// Ordering logic.
	switch(  demand_type  ) {
		case DL_SYNC: {
			// Synchronous ordering. All buffers are filled the same amount in parallel based on desired work factor.

			if(  want == 0  ||  inactive_demands > 0  ) {
				// No ordering if any are overfull or there is no want.
				break;
			}

			for(  uint32 index = 0;  index < input.get_count();  index++  ) {
				input[index].demand_buffer += want;

				// register inactive demand buffer
				if(  input[index].demand_buffer >= input[index].max  ) {
					inactive_demands++;
				}
			}
			break;
		}
		case DL_ASYNC: {
			// Asynchronous ordering. Each buffer tries to order scaled back by the demand work factor.

			if(  want == 0  ||  inactive_demands == input.get_count()  ) {
				// No ordering if all are overfull or no want.
				break;
			}

			for(  uint32 index = 0;  index < input.get_count();  index++  ) {
				// Skip inactive demand buffers.
				if(  input[index].demand_buffer >= input[index].max  ) {
					continue;
				}
				input[index].demand_buffer +=  max(work_scale_production(want, input[index].calculate_demand_production_rate()), 1);

				// register inactive demand buffer
				if(  input[index].demand_buffer >= input[index].max  ) {
					inactive_demands++;
				}
			}
			break;
		}
		default: {
			// Nothing to order.
			break;
		}
	}

	/// Book the weighted sums for statistics.

	book_weighted_sums( delta_t );

	/// Power ordering logic.

	switch(  boost_type  ) {
		case BL_CLASSIC: {
			// draw a fixed amount of power when working sufficiently, otherwise draw no power
			if(  !desc->is_electricity_producer()  ) {
				if(  currently_requiring_power  ) {
					set_power_demand(scaled_electric_demand);
				}
				else {
					set_power_demand(0);
				}
			}
			break;
		}
		case BL_POWER: {
			// compute power demand
			uint64 pd = ((uint64)scaled_electric_demand * (uint64)boost * (uint64)work) >> (DEFAULT_PRODUCTION_FACTOR_BITS + WORK_BITS);
			set_power_demand((uint32)pd);

			break;
		}
		default: {
			// No power is ordered.
			break;
		}
	};

	/// Periodic tasks.

	delta_sum += delta_t;
	if(  delta_sum > PRODUCTION_DELTA_T  ) {
		delta_sum = delta_sum % PRODUCTION_DELTA_T;

		// distribute, if  min shipment waiting.
		for(  uint32 product = 0;  product < output.get_count();  product++  ) {
			// either more than ten or nearly full (if there are less than ten output)
			if(  output[product].menge >= output[product].min_shipment  ) {
				verteile_waren( product );
				INT_CHECK("simfab 636");
			}
		}

		// Order required inputs.
		for(  uint32 index = 0;  index < input.get_count();  index++  ) {
			switch(  demand_type  ) {
				case DL_SYNC:
				case DL_ASYNC: {
					// Orders based on demand buffer.
					input[index].placing_orders = (input[index].demand_buffer > 0);
					break;
				}
				case DL_OLD: {
					// Orders based on storage and maximum transit.
					input[index].placing_orders = (input[index].menge < input[index].max  &&  (input[index].max_transit == 0  ||  input[index].get_in_transit() < input[index].max_transit) );
					break;
				}
				default: {
					// Unknown order logic?
					break;
				}
			}
		}

		recalc_factory_status();

		// rescale delta_menge here: all products should be produced at least once
		// (if consumer only: all supplements should be consumed once)
		const uint32 min_change = output.empty() ? input.get_count() : output.get_count();

		if(  (delta_menge >> fabrik_t::precision_bits) > min_change  ) {
			// we produced some real quantity => smoke
			smoke();

			// chance to expand every 256 rounds of activities, after which activity count will return to 0 (overflow behaviour)
			if(  (++activity_count)==0  ) {
				if(  desc->get_field_group()  ) {
					if(  fields.get_count()<desc->get_field_group()->get_max_fields()  ) {
						// spawn new field with given probability
						add_random_field(desc->get_field_group()->get_probability());
					}
				}
				else {
					if(  times_expanded < desc->get_expand_times()  ) {
						if(  simrand(10000) < desc->get_expand_probability()  ) {
							set_base_production( prodbase + desc->get_expand_minimum() + simrand( desc->get_expand_range() ) );
							++times_expanded;
						}
					}
				}
			}

			INT_CHECK("simfab 558");
			// reset for next cycle
			delta_menge = 0;
		}
	}

	/// advance arrival slot at calculated interval and recalculate boost where necessary
	delta_slot += delta_t;
	const sint32 periods = welt->get_settings().get_factory_arrival_periods();
	const sint32 slot_interval = (1 << (PERIOD_BITS - SLOT_BITS)) * periods;
	while(  delta_slot>slot_interval  ) {
		delta_slot -= slot_interval;
		const sint32 pax_result = arrival_stats_pax.advance_slot();
		if(  pax_result&ARRIVALS_CHANGED  ||  (periods>1  &&  pax_result&ACTIVE_SLOTS_INCREASED  &&  arrival_stats_pax.get_active_slots()*periods>SLOT_COUNT  )  ) {
			update_prodfactor_pax();
		}
		const sint32 mail_result = arrival_stats_mail.advance_slot();
		if(  mail_result&ARRIVALS_CHANGED  ||  (periods>1  &&  mail_result&ACTIVE_SLOTS_INCREASED  &&  arrival_stats_mail.get_active_slots()*periods>SLOT_COUNT  )  ) {
			update_prodfactor_mail();
		}
	}
}


class distribute_ware_t
{
public:
	ware_t ware;             /// goods to be routed to consumer
	halthandle_t halt;       /// potential start halt
	sint32 space_left;       /// free space at halt
	sint32 amount_waiting;   /// waiting goods at halt for same destination as ware
private:
	sint32 ratio_free_space; /// ratio of free space at halt (=0 for overflowing station)

public:
	distribute_ware_t( halthandle_t h, sint32 l, sint32 t, sint32 a, ware_t w )
	{
		halt = h;
		space_left = l;
		amount_waiting = a;
		ware = w;
		// ensure overfull stations compare equal allowing tie breaker clause (amount waiting)
		sint32 space_total = t > 0 ? t : 1;
		ratio_free_space = space_left > 0 ? ((sint64)space_left << fabrik_t::precision_bits) / space_total : 0;
	}
	distribute_ware_t() {}

	static bool compare(const distribute_ware_t &dw1, const distribute_ware_t &dw2)
	{
		return  (dw1.ratio_free_space > dw2.ratio_free_space)
				||  (dw1.ratio_free_space == dw2.ratio_free_space  &&  dw1.amount_waiting <= dw2.amount_waiting);
	}
};


/**
 * distribute stuff to all best destination
 */
void fabrik_t::verteile_waren(const uint32 product)
{
	// wohin liefern ?
	if(  lieferziele.empty()  ) {
		return;
	}

	// not connected?
	const planquadrat_t *plan = welt->access(pos.get_2d());
	if(  plan == NULL  ) {
		dbg->fatal("fabrik_t::verteile_waren", "%s has not distribution target", get_name() );
	}
	if(  plan->get_haltlist_count() == 0  ) {
		return;
	}

	static vector_tpl<distribute_ware_t> dist_list(16);
	dist_list.clear();

	// to distribute to all target equally, we use this counter, for the source hald, and target factory, to try first
	output[product].index_offset++;

	/* distribute goods to factory
	 * that has not an overflowing input storage
	 * also prevent stops from overflowing, if possible
	 * Since we can called with menge>max/2 are at least 10 are there, we must first limit the amount we distribute
	 */
	// We already know the distribution amount. However it has to be converted from factory units into real units.
	const uint32 prod_factor = desc->get_product(product)->get_factor();
	sint32 menge = (sint32)(((sint64)output[product].min_shipment * (sint64)(prod_factor)) >> (DEFAULT_PRODUCTION_FACTOR_BITS + precision_bits));

	// ok, first generate list of possible destinations
	const halthandle_t *haltlist = plan->get_haltlist();
	for(  unsigned i=0;  i<plan->get_haltlist_count();  i++  ) {
		halthandle_t halt = haltlist[(i + output[product].index_offset) % plan->get_haltlist_count()];

		if(  !halt->get_ware_enabled()  ) {
			continue;
		}

		for(  uint32 n=0;  n<lieferziele.get_count();  n++  ) {
			// this way, the halt, that is tried first, will change. As a result, if all destinations are empty, it will be spread evenly
			const koord lieferziel = lieferziele[(n + output[product].index_offset) % lieferziele.get_count()];
			fabrik_t * ziel_fab = get_fab(lieferziel);

			if(  ziel_fab  ) {
				const sint8 needed = ziel_fab->is_needed(output[product].get_typ());
				if(  needed>=0  ) {
					ware_t ware(output[product].get_typ());
					ware.menge = menge;
					ware.to_factory = 1;
					ware.set_zielpos( lieferziel );

					unsigned w;
					// find the index in the target factory
					for(  w = 0;  w < ziel_fab->get_input().get_count()  &&  ziel_fab->get_input()[w].get_typ() != ware.get_desc();  w++  ) {
						// empty
					}

					// if only overflown factories found => deliver to first
					// else deliver to non-overflown factory
					if(  !(welt->get_settings().get_just_in_time() != 0)  ) {
						// without production stop when target overflowing, distribute to least overflow target
						const sint32 fab_left = ziel_fab->get_input()[w].max - ziel_fab->get_input()[w].menge;
						dist_list.insert_ordered( distribute_ware_t( halt, fab_left, ziel_fab->get_input()[w].max, (sint32)halt->get_ware_fuer_zielpos(output[product].get_typ(),ware.get_zielpos()), ware ), distribute_ware_t::compare );

					}
					else if(  needed > 0  ) {
						// we are not overflowing: Station can only store up to a maximum amount of goods per square
						const sint32 halt_left = (sint32)halt->get_capacity(2) - (sint32)halt->get_ware_summe(ware.get_desc());
						dist_list.insert_ordered( distribute_ware_t( halt, halt_left, halt->get_capacity(2), (sint32)halt->get_ware_fuer_zielpos(output[product].get_typ(),ware.get_zielpos()), ware ), distribute_ware_t::compare );
					}
				}
			}
		}
	}

	// Auswertung der Ergebnisse
	if(  !dist_list.empty()  ) {
		distribute_ware_t *best = NULL;
		FOR(vector_tpl<distribute_ware_t>, & i, dist_list) {
			// now search route
			int const result = haltestelle_t::search_route(&i.halt, 1U, welt->get_settings().is_no_routing_over_overcrowding(), i.ware);
			if(  result == haltestelle_t::ROUTE_OK  ||  result == haltestelle_t::ROUTE_WALK  ) {
				// we can deliver to this destination
				best = &i;
				break;
			}
		}

		if(  best == NULL  ) {
			return; // no route for any destination
		}

		halthandle_t &best_halt = best->halt;
		ware_t       &best_ware = best->ware;

		// now process found route
		const sint32 space_left = (welt->get_settings().get_just_in_time() != 0 ) ? best->space_left : (sint32)best_halt->get_capacity(2) - (sint32)best_halt->get_ware_summe(best_ware.get_desc());
		menge = min( menge, 9 + space_left );
		// ensure amount is not negative ...
		if(  menge<0  ) {
			menge = 0;
		}
		// since it is assigned here to an unsigned variable!
		best_ware.menge = menge;

		if(  space_left<0  ) {
			// find, what is most waiting here from us
			ware_t most_waiting(output[product].get_typ());
			most_waiting.menge = 0;
			FOR(vector_tpl<koord>, const& n, lieferziele) {
				uint32 const amount = best_halt->get_ware_fuer_zielpos(output[product].get_typ(), n);
				if(  amount > most_waiting.menge  ) {
					most_waiting.set_zielpos(n);
					most_waiting.menge = amount;
				}
			}

			//  we will reroute some goods
			if(  best->amount_waiting==0  &&  most_waiting.menge>0  ) {
				// remove something from the most waiting goods
				if(  best_halt->recall_ware( most_waiting, min((sint32)(most_waiting.menge/2), 1 - space_left) )  ) {
					best_ware.menge += most_waiting.menge;
				}
				else {
					// overcrowded with other stuff (not from us)
					return;
				}

				// refund JIT2 demand buffers for rerouted goods
				if(  welt->get_settings().get_just_in_time() >= 2  ) {
					// locate destination factory
					fabrik_t *fab = get_fab( most_waiting.get_zielpos() );

					if(  fab  ) {
						for(  uint32 input = 0;  input < fab->input.get_count();  input++  ) {
							ware_production_t& w = fab->input[input];
							if (  w.get_typ()->get_index() == most_waiting.get_index()  ) {
								// correct ware to refund found

								// refund demand buffers, deactivating if required
								const uint32 prod_factor = fab->desc->get_supplier(input)->get_consumption();
								const sint32 prod_delta = (sint32)((((sint64)(most_waiting.menge) << (DEFAULT_PRODUCTION_FACTOR_BITS + precision_bits)) + (sint64)(prod_factor - 1)) / (sint64)prod_factor);
								const sint32 demand = w.demand_buffer;
								w.demand_buffer += prod_delta;
								if(  demand < w.max  &&  w.demand_buffer >= w.max  ) {
									fab->inactive_demands++;
								}

								// refund successful
								break;
							}
						}
					}
				}
			}
			else {
				return;
			}
		}

		// Since menge might have been mutated, it must be converted back. This might introduce some error with some prod factors which is always rounded up.
		const sint32 prod_delta = (sint32)((((sint64)menge << (DEFAULT_PRODUCTION_FACTOR_BITS + precision_bits)) + (sint64)(prod_factor - 1)) / (sint64)prod_factor);

		// If the output is inactive, reactivate it
		if( output[product].menge == output[product].max && prod_delta > 0 ) inactive_outputs-= 1;

		output[product].menge -= prod_delta;

		best_halt->starte_mit_route(best_ware);
		best_halt->recalc_status();
		fabrik_t::apply_transit( &best_ware );
		// add as active destination
		lieferziele_active_last_month |= (1 << lieferziele.index_of(best_ware.get_zielpos()));
		output[product].book_stat(best_ware.menge, FAB_GOODS_DELIVERED);
	}
}


void fabrik_t::new_month()
{
	// calculate weighted averages
	if(  aggregate_weight > 0  ) {
		set_stat( weighted_sum_production / aggregate_weight, FAB_PRODUCTION );
		set_stat( weighted_sum_boost_electric / aggregate_weight, FAB_BOOST_ELECTRIC );
		set_stat( weighted_sum_boost_pax / aggregate_weight, FAB_BOOST_PAX );
		set_stat( weighted_sum_boost_mail / aggregate_weight, FAB_BOOST_MAIL );
		set_stat( weighted_sum_power / aggregate_weight, FAB_POWER );
	}

	// update statistics for input and output goods
	for(  uint32 in = 0;  in < input.get_count();  in++  ){
		input[in].roll_stats( desc->get_supplier(in)->get_consumption(), aggregate_weight );
	}
	for(  uint32 out = 0;  out < output.get_count();  out++  ){
		output[out].roll_stats( desc->get_product(out)->get_factor(), aggregate_weight );
	}
	lieferziele_active_last_month = 0;

	// advance statistics a month
	for(  int s = 0;  s < MAX_FAB_STAT;  ++s  ) {
		for(  int m = MAX_MONTH - 1;  m > 0;  --m  ) {
			statistics[m][s] = statistics[m-1][s];
		}
		statistics[0][s] = 0;
	}

	weighted_sum_production = 0;
	weighted_sum_boost_electric = 0;
	weighted_sum_boost_pax = 0;
	weighted_sum_boost_mail = 0;
	weighted_sum_power = 0;
	aggregate_weight = 0;

	// restore the current values
	set_stat( get_current_production(), FAB_PRODUCTION );
	set_stat( prodfactor_electric, FAB_BOOST_ELECTRIC );
	set_stat( prodfactor_pax, FAB_BOOST_PAX );
	set_stat( prodfactor_mail, FAB_BOOST_MAIL );
	set_stat( get_power(), FAB_POWER );

	// since target cities' population may be increased -> re-apportion pax/mail demand
	recalc_demands_at_target_cities();
}


// static !
uint8 fabrik_t::status_to_color[5] = {COL_RED, COL_ORANGE, COL_GREEN, COL_YELLOW, COL_WHITE };

//#define FL_WARE_NULL         1
#define FL_WARE_ALLENULL       2
#define FL_WARE_LIMIT          4
#define FL_WARE_ALLELIMIT      8
#define FL_WARE_UEBER75        16
#define FL_WARE_ALLEUEBER75    32
#define FL_WARE_FEHLT_WAS      64


/* returns the status of the current factory, as well as output */
void fabrik_t::recalc_factory_status()
{
	uint64 warenlager;
	char status_ein;
	char status_aus;

	int haltcount=welt->access(pos.get_2d())->get_haltlist_count();

	// set bits for input
	warenlager = 0;
	total_transit = 0;
	status_ein = FL_WARE_ALLELIMIT;
	uint32 i = 0;
	FOR( array_tpl<ware_production_t>, const& j, input ) {
		if(  j.menge >= j.max  ) {
			status_ein |= FL_WARE_LIMIT;
		}
		else {
			status_ein &= ~FL_WARE_ALLELIMIT;
		}
		warenlager+= (uint64)j.menge * (uint64)(desc->get_supplier(i++)->get_consumption());
		total_transit += j.get_in_transit();
		if(  (j.menge >> fabrik_t::precision_bits) == 0  ) {
			status_ein |= FL_WARE_FEHLT_WAS;
		}

	}
	warenlager >>= fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS;
	if(  warenlager==0  ) {
		status_ein |= FL_WARE_ALLENULL;
	}
	total_input = (uint32)warenlager;

	// one ware missing, but producing
	if(  status_ein & FL_WARE_FEHLT_WAS  &&  !output.empty()  &&  haltcount > 0  ) {
		status = STATUS_BAD;
		return;
	}

	// set bits for output
	warenlager = 0;
	status_aus = FL_WARE_ALLEUEBER75|FL_WARE_ALLENULL;
	i = 0;
	FOR( array_tpl<ware_production_t>, const& j, output ) {
		if(  j.menge > 0  ) {

			status_aus &= ~FL_WARE_ALLENULL;
			if(  j.menge >= 0.75 * j.max  ) {
				status_aus |= FL_WARE_UEBER75;
			}
			else {
				status_aus &= ~FL_WARE_ALLEUEBER75;
			}
			warenlager += (uint64)j.menge * (uint64)(desc->get_product(i)->get_factor());
			status_aus &= ~FL_WARE_ALLENULL;
		}
		else {
			// menge = 0
			status_aus &= ~FL_WARE_ALLEUEBER75;
		}
	}
	warenlager >>= fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS;
	total_output = (uint32)warenlager;

	// now calculate status bar
	if(  input.empty()  ) {
		// does not consume anything, should just produce

		if(  output.empty()  ) {
			// does also not produce anything
			status = STATUS_NOTHING;
		}
		else if(  status_aus&FL_WARE_ALLEUEBER75  ||  status_aus&FL_WARE_UEBER75  ) {
			status = STATUS_INACTIVE; // not connected?
			if(haltcount>0) {
				if(status_aus&FL_WARE_ALLEUEBER75) {
					status = STATUS_BAD; // connect => needs better service
				}
				else {
					status = STATUS_MEDIUM; // connect => needs better service for at least one product
				}
			}
		}
		else {
			status = STATUS_GOOD;
		}
	}
	else if(  output.empty()  ) {
		// nothing to produce

		if(status_ein&FL_WARE_ALLELIMIT) {
			// we assume not served
			status = STATUS_BAD;
		}
		else if(status_ein&FL_WARE_LIMIT) {
			// served, but still one at limit
			status = STATUS_MEDIUM;
		}
		else if(status_ein&FL_WARE_ALLENULL) {
			status = STATUS_INACTIVE; // assume not served
			if(haltcount>0) {
				// there is a halt => needs better service
				status = STATUS_BAD;
			}
		}
		else {
			status = STATUS_GOOD;
		}
	}
	else {
		// produces and consumes
		if((status_ein&FL_WARE_ALLELIMIT)!=0  &&  (status_aus&FL_WARE_ALLEUEBER75)!=0) {
			status = STATUS_BAD;
		}
		else if((status_ein&FL_WARE_ALLELIMIT)!=0  ||  (status_aus&FL_WARE_ALLEUEBER75)!=0) {
			status = STATUS_MEDIUM;
		}
		else if((status_ein&FL_WARE_ALLENULL)!=0  &&  (status_aus&FL_WARE_ALLENULL)!=0) {
			// not producing
			status = STATUS_INACTIVE;
		}
		else if(haltcount>0  &&  ((status_ein&FL_WARE_ALLENULL)!=0  ||  (status_aus&FL_WARE_ALLENULL)!=0)) {
			// not producing but out of supply
			status = STATUS_MEDIUM;
		}
		else {
			status = STATUS_GOOD;
		}
	}
}


void fabrik_t::open_info_window()
{
	gebaeude_t *gb = welt->lookup(pos)->find<gebaeude_t>();
	create_win(new fabrik_info_t(this, gb), w_info, (ptrdiff_t)this );
}


void fabrik_t::info_prod(cbuffer_t& buf) const
{
	buf.clear();
	buf.append( translator::translate("Durchsatz") );
	buf.append(get_current_production(), 0);
	buf.append( translator::translate("units/day") );
	buf.append("\n");
	buf.printf(translator::translate("Produces: %.1f units/minute"),
		get_production_per_second() * 60.0
	);

	if (!output.empty()) {
		buf.append("\n\n");
		buf.append(translator::translate("Produktion"));

		for (uint32 index = 0; index < output.get_count(); index++) {
			goods_desc_t const *const type = output[index].get_typ();
			sint64 const pfactor = (sint64)desc->get_product(index)->get_factor();

			buf.append("\n - ");
			if(  welt->get_settings().get_just_in_time() >= 2  ) {
				double const pfraction = (double)pfactor / (double)DEFAULT_PRODUCTION_FACTOR;
				double const storage_unit = (double)(1 << fabrik_t::precision_bits);
				buf.printf(translator::translate("%s %.0f%% : %.0f/%.0f @ %.1f%%"),
					translator::translate(type->get_name()),
					pfraction * 100.0,
					(double)output[index].menge * pfraction / storage_unit,
					(double)output[index].max * pfraction / storage_unit,
					(double)output[index].calculate_output_production_rate() * 100.0 / (double)(1 << 16)
				);
			}
			else {
				buf.printf("%s %u/%u%s",
					translator::translate(type->get_name()),
					(uint32) convert_goods( (sint64)output[index].menge * pfactor),
					(uint32) convert_goods( (sint64)output[index].max * pfactor),
					translator::translate(type->get_mass())
				);

				if(type->get_catg() != 0) {
					buf.append(", ");
					buf.append(translator::translate(type->get_catg_name()));
				}

				buf.printf(", %u%%", (uint32)((FAB_PRODFACT_UNIT_HALF + (sint32)pfactor * 100) >> DEFAULT_PRODUCTION_FACTOR_BITS));
			}
		}
	}

	if (!input.empty()) {
		buf.append("\n\n");
		buf.append(translator::translate("Verbrauch"));

		for (uint32 index = 0; index < input.get_count(); index++) {
			sint64 const pfactor = (sint64)desc->get_supplier(index)->get_consumption();

			const char *cat_name = "";
			const char *cat_seperator = "";
			if(  input[index].get_typ()->get_catg() > 0  ) {
				cat_name = translator::translate(input[index].get_typ()->get_catg_name());
				cat_seperator = ", ";
			}

			buf.append("\n - ");
			if(  welt->get_settings().get_just_in_time() >= 2  ) {
				double const pfraction = (double)pfactor / (double)DEFAULT_PRODUCTION_FACTOR;
				double const storage_unit = (double)(1 << fabrik_t::precision_bits);
				buf.printf(translator::translate("%s %.0f%% : %.0f+%u/%.0f%s, %s%s%.1f%%"),
					translator::translate(input[index].get_typ()->get_name()),
					pfraction * 100.0,
					(double)input[index].menge * pfraction / storage_unit,
					(uint32)input[index].get_in_transit(),
					(double)input[index].max * pfraction / storage_unit,
					translator::translate(input[index].get_typ()->get_mass()),
					cat_name, cat_seperator,
					(double)input[index].calculate_demand_production_rate() * 100.0 / (double)(1 << 16)
				);
			}
			else if(  welt->get_settings().get_factory_maximum_intransit_percentage()  ) {
				buf.printf("%s %u/%i(%i)/%u%s, %s%s%u%%",
					translator::translate(input[index].get_typ()->get_name()),
					(uint32) convert_goods( (sint64)input[index].menge * pfactor),
					input[index].get_in_transit(),
					input[index].max_transit,
					(uint32) convert_goods( (sint64)input[index].max * pfactor),
					translator::translate(input[index].get_typ()->get_mass()),
					cat_name, cat_seperator,
					(uint32)((FAB_PRODFACT_UNIT_HALF + (sint32)pfactor * 100) >> DEFAULT_PRODUCTION_FACTOR_BITS)
				);
			}
			else {
				buf.printf("%s %u/%i/%u%s, %s%s%u%%",
					translator::translate(input[index].get_typ()->get_name()),
					(uint32) convert_goods( (sint64)input[index].menge * pfactor),
					input[index].get_in_transit(),
					(uint32) convert_goods( (sint64)input[index].max * pfactor),
					translator::translate(input[index].get_typ()->get_mass()),
					cat_name, cat_seperator,
					(uint32)((FAB_PRODFACT_UNIT_HALF + (sint32)pfactor * 100) >> DEFAULT_PRODUCTION_FACTOR_BITS)
				);
			}
		}
	}
}


void fabrik_t::info_conn(cbuffer_t& buf) const
{
	buf.clear();
	const planquadrat_t *plan = welt->access(get_pos().get_2d());
	if(plan  &&  plan->get_haltlist_count()>0) {
		buf.append(translator::translate("Connected stops"));

		for(  uint i=0;  i<plan->get_haltlist_count();  i++  ) {
			buf.printf("\n - %s", plan->get_haltlist()[i]->get_name() );
		}
	}
}


void fabrik_t::finish_rd()
{
	// adjust production base to be at least as large as fields productivity
	uint32 prodbase_adjust = 1;
	const field_group_desc_t *fd = desc->get_field_group();
	if(fd) {
		for(uint32 i=0; i<fields.get_count(); i++) {
			const field_class_desc_t *fc = fd->get_field_class( fields[i].field_class_index );
			if (fc) {
				prodbase_adjust += fc->get_field_production();
			}
		}
	}

	// set production, update all production related numbers
	set_base_production( max(prodbase, prodbase_adjust) );

	// now we have a valid storage limit
	if(  welt->get_settings().is_crossconnect_factories()  ) {
		FOR(  slist_tpl<fabrik_t*>,  const fab,  welt->get_fab_list()  ) {
			fab->add_supplier(this);
		}
	}
	else {
		// add as supplier to target(s)
		for(uint32 i=0; i<lieferziele.get_count(); i++) {
			fabrik_t * fab2 = fabrik_t::get_fab(lieferziele[i]);
			if(fab2) {
				fab2->add_supplier(pos.get_2d());
				lieferziele[i] = fab2->get_pos().get_2d();
			}
			else {
				// remove this ...
				dbg->warning( "fabrik_t::finish_rd()", "No factory at expected position %s!", lieferziele[i].get_str() );
				lieferziele.remove_at(i);
				i--;
			}
		}
	}

	// Now rebuild input/output activity information.
	if( welt->get_settings().get_just_in_time() >= 2 ){
		inactive_inputs = inactive_outputs = inactive_demands = 0;
		for(  uint32 in = 0;  in < input.get_count();  in++  ){
			if( input[in].menge > input[in].max ) {
				input[in].menge = input[in].max;
			}
			input[in].placing_orders = (input[in].demand_buffer > 0);

			// Also do a sanity check. People might try and force their saves to JIT2 mode via reloading so we need to catch any massive values.
			if( welt->get_settings().get_just_in_time() >= 2 && welt->get_settings().get_factory_maximum_intransit_percentage() != 0 ){
				const sint32 demand_minmax = (sint32)((sint64)input[in].max * (sint64)welt->get_settings().get_factory_maximum_intransit_percentage() / (sint64)100);
				if( input[in].demand_buffer < (-demand_minmax) ) {
					input[in].demand_buffer = -demand_minmax;
				}
				else if( input[in].demand_buffer > (input[in].max + demand_minmax) ) {
					input[in].demand_buffer = input[in].max;
				}
			}
		}
		for( uint32 out = 0 ; out < output.get_count() ; out++ ){
			if( output[out].menge >= output[out].max ) {
				output[out].menge = output[out].max;
			}
		}

		rebuild_inactive_cache();
	}
	else {
		// Classic order logic.
		for(  uint32 in = 0;  in < input.get_count();  in++  ){
			input[in].placing_orders = (input[in].menge < input[in].max  &&  input[in].get_in_transit() < input[in].max_transit);
		}
	}

	// set initial power boost
	if (  boost_type == BL_POWER  ) {
		prodfactor_electric = get_jit2_power_boost();
	}
}


void fabrik_t::rotate90( const sint16 y_size )
{
	rotate = (rotate+3)%desc->get_building()->get_all_layouts();
	pos_origin.rotate90( y_size );
	pos_origin.x -= desc->get_building()->get_x(rotate)-1;
	pos.rotate90( y_size );

	FOR(vector_tpl<koord>, & i, lieferziele) {
		i.rotate90(y_size);
	}
	FOR(vector_tpl<koord>, & i, suppliers) {
		i.rotate90(y_size);
	}
	FOR(vector_tpl<field_data_t>, & i, fields) {
		i.location.rotate90(y_size);
	}
}


void fabrik_t::add_supplier(koord ziel)
{
	// Updated maximum in-transit. Only used for classic support.
	if(  welt->get_settings().get_factory_maximum_intransit_percentage()  &&  !suppliers.is_contained(ziel) && welt->get_settings().get_just_in_time() < 2  ) {
		if(  fabrik_t *fab = get_fab( ziel )  ) {
			for(  uint32 i=0;  i < fab->get_output().get_count();  i++   ) {
				const ware_production_t &w_out = fab->get_output()[i];
				// now update transit limits
				FOR(  array_tpl<ware_production_t>,  &w,  input ) {
					if(  w_out.get_typ() == w.get_typ()  ) {
#ifdef TRANSIT_DISTANCE
						sint64 distance = koord_distance( fab->get_pos(), get_pos() );
						// calculate next mean by the following formula: average + (next - average)/(n+1)
						w.count_suppliers ++;
						sint64 next = 1 + ( distance * welt->get_settings().get_factory_maximum_intransit_percentage() * (w.max >> fabrik_t::precision_bits) ) / 131072;
						w.max_transit += (next - w.max_transit)/(w.count_suppliers);
#else
						const sint32 max_storage = (sint32)(1 + ( ((sint64)w_out.max * (sint64)welt->get_settings().get_factory_maximum_intransit_percentage() ) >> fabrik_t::precision_bits) / 100);
						const sint32 old_max_transit = w.max_transit;
						w.max_transit += max_storage;
						if(  w.max_transit < old_max_transit  ) {
							// we have overflown, so we use the max value
							w.max_transit = 0x7fffffff;
						}
#endif
						break;
					}
				}
			}
			// since there could be more than one good, we have to iterate over all of them
		}
	}
	suppliers.insert_unique_ordered( ziel, RelativeDistanceOrdering(pos.get_2d()) );
}


void fabrik_t::rem_supplier(koord pos)
{
	suppliers.remove(pos);

	// Updated maximum in-transit. Only used for classic support.
	if( welt->get_settings().get_factory_maximum_intransit_percentage() && welt->get_settings().get_just_in_time() < 2 ) {
		// set to zero
		FOR(  array_tpl<ware_production_t>,  &w,  input ) {
			w.max_transit = 0;
		}

		// unfourtunately we have to bite the bullet and recalc the values from scratch ...
		FOR( vector_tpl<koord>, ziel, suppliers ) {
			if(  fabrik_t *fab = get_fab( ziel )  ) {
				for(  uint32 i=0;  i < fab->get_output().get_count();  i++   ) {
					const ware_production_t &w_out = fab->get_output()[i];
					// now update transit limits
					FOR(  array_tpl<ware_production_t>,  &w,  input ) {
						if(  w_out.get_typ() == w.get_typ()  ) {
#ifdef TRANSIT_DISTANCE
							sint64 distance = koord_distance( fab->get_pos(), get_pos() );
							// calculate next mean by the following formula: average + (next - average)/(n+1)
							w.count_suppliers ++;
							sint64 next = 1 + ( distance * welt->get_settings().get_factory_maximum_intransit_percentage() * (w.max >> fabrik_t::precision_bits) ) / 131072;
							w.max_transit += (next - w.max_transit)/(w.count_suppliers);
#else
							const sint32 max_storage = (sint32)(1 + ( ((sint64)w_out.max * (sint64)welt->get_settings().get_factory_maximum_intransit_percentage() ) >> fabrik_t::precision_bits) / 100);
							const sint32 old_max_transit = w.max_transit;
							w.max_transit += max_storage;
							if(  w.max_transit < old_max_transit  ) {
								// we have overflown, so we use the max value
								w.max_transit = 0x7fffffff;
							}
#endif
							break;
						}
					}
				}
				// since there could be more than one good, we have to iterate over all of them
			}
		}
	}
}


/** crossconnect everything possible */
void fabrik_t::add_all_suppliers()
{
	for(int i=0; i < desc->get_supplier_count(); i++) {
		const factory_supplier_desc_t *supplier = desc->get_supplier(i);
		const goods_desc_t *ware = supplier->get_input_type();

		FOR(slist_tpl<fabrik_t*>, const fab, welt->get_fab_list()) {
			// connect to an existing one, if this is an producer
			if(fab!=this  &&  fab->vorrat_an(ware) > -1) {
				// add us to this factory
				// will also add to our suppliers list
				fab->add_lieferziel(pos.get_2d());
			}
		}
	}
}


/* adds a new supplier to this factory
 * fails if no matching goods are there
 */
bool fabrik_t::add_supplier(fabrik_t* fab)
{
	for(int i=0; i < desc->get_supplier_count(); i++) {
		const factory_supplier_desc_t *supplier = desc->get_supplier(i);
		const goods_desc_t *ware = supplier->get_input_type();

			// connect to an existing one, if this is an producer
			if(  fab!=this  &&  fab->vorrat_an(ware) > -1  ) {
				// add us to this factory
				fab->add_lieferziel(pos.get_2d());
				return true;
			}
	}
	return false;
}


void fabrik_t::get_tile_list( vector_tpl<koord> &tile_list ) const
{
	gebaeude_t *gb = welt->lookup( pos )->find<gebaeude_t>();
	koord size = get_desc()->get_building()->get_size( gb->get_tile()->get_layout() );
	const koord3d pos0 = gb->get_pos() - gb->get_tile()->get_offset(); // get origin
	koord k;

	tile_list.clear();
	// add all tiles
	for( k.y = 0; k.y < size.y; k.y++ ) {
		for( k.x = 0; k.x < size.x; k.x++ ) {
			if( grund_t* gr = welt->lookup( pos0+k ) ) {
				if( gebaeude_t* const add_gb = obj_cast<gebaeude_t>(gr->first_obj()) ) {
					if( add_gb->get_fabrik()==this ) {
						tile_list.append( (pos0+k).get_2d() );
					}
				}
			}
		}
	}
}

// Returns a list of goods produced by this factory. The caller must delete
// the list when done
slist_tpl<const goods_desc_t*> *fabrik_t::get_produced_goods() const
{
	slist_tpl<const goods_desc_t*> *goods = new slist_tpl<const goods_desc_t*>();

	FOR(array_tpl<ware_production_t>, const& i, output) {
		goods->append(i.get_typ());
	}

	return goods;
}

void fabrik_t::rebuild_inactive_cache()
{
	inactive_inputs = inactive_outputs = inactive_demands = 0;

	for(  uint32 i = 0;  i < input.get_count();  i++  ){
		if(  input[i].menge <= 0  ) {
			inactive_inputs++;
		}
		if(  input[i].demand_buffer >= input[i].max  ) {
			inactive_demands++;
		}
	}

	for(  uint32 i = 0 ; i < output.get_count() ; i++  ){
		if(  output[i].menge >= output[i].max  ) {
			inactive_outputs++;
		}
	}
}

double fabrik_t::get_production_per_second() const {
	return (double)prodbase * (double)get_prodfactor() / (double)DEFAULT_PRODUCTION_FACTOR / (double)DEFAULT_PRODUCTION_FACTOR;
}

sint32 fabrik_t::calculate_work_rate_ramp(sint32 const amount, sint32 const minimum, sint32 const maximum, uint32 const precision)
{
	sint32 production_rate;
	if(  minimum >= maximum  ||  minimum >= amount  ) {
		// full production
		production_rate = 1 << precision;
	}
	else if(  amount < maximum  ) {
		// reduction production
		production_rate = (sint32)(((sint64)(maximum - amount) << precision) / (sint64)(maximum - minimum));
		// apply work factor minimum limit
		if(  production_rate < WORK_SCALE_MINIMUM_FRACTION  ){
			production_rate = WORK_SCALE_MINIMUM_FRACTION;
		}
	}
	else {
		// no production
		production_rate = 0;
	}
	return production_rate;
}


/**
 * Draws some nice colored bars giving some status information
 */
void fabrik_t::display_status(sint16 xpos, sint16 ypos)
{
	const sint16 count = input.get_count()+output.get_count();

	ypos += -D_WAITINGBAR_WIDTH - LINESPACE/6;
	xpos -= (count * D_WAITINGBAR_WIDTH - get_tile_raster_width()) / 2;

	if( input.get_count() ) {
		if(  currently_producing  ) {
			display_ddd_box_clip_rgb(xpos-2,  ypos-1, D_WAITINGBAR_WIDTH*input.get_count()+4, 6, color_idx_to_rgb(10), color_idx_to_rgb(12));
		}
		display_fillbox_wh_clip_rgb(xpos-1, ypos, D_WAITINGBAR_WIDTH*input.get_count()+2, 4, color_idx_to_rgb(150), true);

		int i = 0;
		FORX(array_tpl<ware_production_t>, const& goods, input, i++) {
			if (!desc->get_supplier(i)) {
				continue;
			}
			//const sint64 max_transit      = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)goods.max_transit * pfactor) >> (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));
			const sint64 pfactor = desc->get_supplier(i) ? (sint64)desc->get_supplier(i)->get_consumption() : 1ll;
			const uint32 storage_capacity = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)goods.max * pfactor) >> (precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));

			if (storage_capacity) {
				const uint32 stock_quantity = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)goods.menge * pfactor) >> (precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));
				const PIXVAL goods_color = goods.get_typ()->get_color();
				const uint16 v = min(25, (uint16)(25 * stock_quantity / storage_capacity)) + 2;

				if (currently_producing) {
					display_fillbox_wh_clip_rgb(xpos, ypos - v - 1, 1, v, color_idx_to_rgb(COL_GREY4), true);
					display_fillbox_wh_clip_rgb(xpos + 1, ypos - v - 1, D_WAITINGBAR_WIDTH - 2, v, goods_color, true);
					display_fillbox_wh_clip_rgb(xpos + D_WAITINGBAR_WIDTH - 1, ypos - v - 1, 1, v, color_idx_to_rgb(COL_GREY1), true);
				}
				else {
					display_blend_wh_rgb(xpos + 1, ypos - v - 1, D_WAITINGBAR_WIDTH - 2, v, goods_color, 60);
					mark_rect_dirty_wc(xpos + 1, ypos - v - 1, xpos + D_WAITINGBAR_WIDTH - 1, ypos - 1);
				}
			}

			xpos += D_WAITINGBAR_WIDTH;
		}
		xpos += 4;
	}

	if( output.get_count() ) {
		if(  currently_producing  ) {
			display_ddd_box_clip_rgb(xpos-2,  ypos-1, D_WAITINGBAR_WIDTH*output.get_count()+4, 6, color_idx_to_rgb(10), color_idx_to_rgb(12));
		}
		display_fillbox_wh_clip_rgb(xpos-1, ypos, D_WAITINGBAR_WIDTH*output.get_count()+2, 4, color_idx_to_rgb(COL_ORANGE), true);

		int i = 0;
		FORX(array_tpl<ware_production_t>, const& goods, output, i++) {

			const sint64 pfactor = (sint64)desc->get_product(i)->get_factor();
			const uint32 storage_capacity = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)goods.max * pfactor) >> (precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));

			if (storage_capacity) {
				const uint32 stock_quantity = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)goods.menge * pfactor) >> (precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));
				const PIXVAL goods_color = goods.get_typ()->get_color();

				const uint16 v = min(25, (uint16)(25 * stock_quantity / storage_capacity)) + 2;

				// the blended bars are too faint for me
				if (currently_producing) {
					display_fillbox_wh_clip_rgb(xpos, ypos - v - 1, 1, v, color_idx_to_rgb(COL_GREY4), true);
					display_fillbox_wh_clip_rgb(xpos + 1, ypos - v - 1, D_WAITINGBAR_WIDTH - 2, v, goods_color, true);
					display_fillbox_wh_clip_rgb(xpos + D_WAITINGBAR_WIDTH - 1, ypos - v - 1, 1, v, color_idx_to_rgb(COL_GREY1), true);
				}
				else {
					display_blend_wh_rgb(xpos + 1, ypos - v - 1, D_WAITINGBAR_WIDTH - 2, v, goods_color, 60);
					mark_rect_dirty_wc(xpos + 1, ypos - v - 1, xpos + D_WAITINGBAR_WIDTH - 1, ypos - 1);
				}
			}

			xpos += D_WAITINGBAR_WIDTH;
		}
	}
}

