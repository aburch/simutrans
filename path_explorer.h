/*
 * Copyright (c) 2009 : Knightly
 *
 * A centralized, steppable path searching system using Floyd-Warshall Algorithm
 */


#ifndef path_explorer_h
#define path_explorer_h

#include <string>

#include "halthandle_t.h"
#include "convoihandle_t.h"
#include "linehandle_t.h"
#include "simtypes.h"
#include "simdebug.h"
#include "simmem.h"

#include "tpl/vector_tpl.h"

class path_explorer_t
{

private:

	class compartment_t
	{
	
	private:

		// element used during path search and for storing calculated paths
		struct path_element_t
		{
			uint16 aggregate_time;
			halthandle_t next_transfer;

			path_element_t() { aggregate_time = 65535; }
		};

		// element used during path search only for storing best lines/convoys
		struct transport_element_t
		{
			linehandle_t first_line;
			linehandle_t last_line;
			convoihandle_t first_convoy;
			convoihandle_t last_convoy;
		};

		// set of variables for finished path data
		path_element_t **finished_matrix;
		uint16 *finished_halt_index_map;
		uint16 finished_halt_count;

		// set of variables for working path data
		path_element_t **working_matrix;
		transport_element_t **transport_matrix;
		uint16 *working_halt_index_map;
		halthandle_t *working_halt_list;
		uint16 working_halt_count;

		// set of variables for full halt list
		halthandle_t *all_halts_list;
		uint16 all_halts_count;

		// set of variables for transfer list
		uint16 *transfer_list;
		uint16 transfer_count;

		uint8 catg;			// category managed by this compartment
		uint16 step_count;	// number of steps done so far for a path refresh request

		// coordination flags
		bool paths_available;
		bool refresh_completed;
		bool refresh_requested;

		// phase indicator
		uint8 current_phase;

		// phase counters
		uint16 phase_counter;
		uint32 iterations;

		// phase counters for path searching
		uint16 via_index;
		uint16 origin;

		// origins limit for path searching
		uint32 limit_origin_explore;

		// statistics for determining limit_path_explore
		uint32 statistic_duration;
		uint32 statistic_iteration;

		// iteration representative
		static uint16 representative_halt_count;
		static uint8 representative_category;

		// iteration limits
		static uint32 limit_find_eligible;
		static uint32 limit_fill_matrix;
		static uint32 limit_path_explore;
		static uint32 limit_ware_reroute;

		// back-up iteration limits
		static uint32 backup_find_eligible;
		static uint32 backup_fill_matrix;
		static uint32 backup_path_explore;
		static uint32 backup_ware_reroute;

		// default iteration limits
		static const uint32 default_find_eligible = 4096;
		static const uint32 default_fill_matrix = 4096;
		static const uint32 default_path_explore = 65536;
		static const uint32 default_ware_reroute = 4096;

		// maximum limit for full refresh
		static const uint32 maximum_limit = 4294967295;

		// phase indices
		static const uint8 phase_gate_sentinel = 0;
		static const uint8 phase_init_prepare = 1;
		static const uint8 phase_find_eligible = 2;
		static const uint8 phase_fill_matrix = 3;
		static const uint8 phase_path_explore = 4;
		static const uint8 phase_ware_reroute = 5;

		// absolute time limits
		static const uint32 time_midpoint = 32;
		static const uint32 time_deviation = 2;
		static const uint32 time_lower_limit = time_midpoint - time_deviation;
		static const uint32 time_upper_limit = time_midpoint + time_deviation;
		static const uint32 time_threshold = time_midpoint / 2;

		// percentage time limits
		static const uint32 percent_deviation = 5;
		static const uint32 percent_lower_limit = 100 - percent_deviation;
		static const uint32 percent_upper_limit = 100 + percent_deviation;

		void enumerate_all_paths(const path_element_t *const *const matrix, const halthandle_t *const halt_list,
								 const uint16 *const halt_map, const uint16 halt_count);

	public:

		compartment_t();
		~compartment_t();

		void step();
		void reset(const bool reset_finished_set);

		bool are_paths_available() { return paths_available; }
		bool is_refresh_completed() { return refresh_completed; }
		bool is_refresh_requested() { return refresh_requested; }

		void set_category(uint8 category) { catg = category; }
		void set_refresh() { refresh_requested = true; }

		bool get_path_between(const halthandle_t origin_halt, const halthandle_t target_halt,
							  uint16 &aggregate_time, halthandle_t &next_transfer)
		{
			static const halthandle_t dummy_halt;
			uint32 origin_index, target_index;
			
			// check if paths are available and if origin halt and target halt are in finished halt heap
			if ( paths_available && origin_halt.is_bound() && target_halt.is_bound()
					&& (origin_index = finished_halt_index_map[origin_halt.get_id()]) != 65535
					&& (target_index = finished_halt_index_map[target_halt.get_id()]) != 65535
					&& finished_matrix[origin_index][target_index].next_transfer.is_bound() )
			{
				aggregate_time = finished_matrix[origin_index][target_index].aggregate_time;
				next_transfer = finished_matrix[origin_index][target_index].next_transfer;
				return true;
			}

			// requested path not found
			aggregate_time = 65535;
			next_transfer = dummy_halt;
			return false;
		}
		
		static void backup_limits()
		{
			backup_find_eligible = limit_find_eligible;
			backup_fill_matrix = limit_fill_matrix;
			backup_path_explore = limit_path_explore;
			backup_ware_reroute = limit_ware_reroute;
		}
		static void restore_limits()
		{
			limit_find_eligible = backup_find_eligible;
			limit_fill_matrix = backup_fill_matrix;
			limit_path_explore = backup_path_explore;
			limit_ware_reroute = backup_ware_reroute;
		}
		static void set_maximum_limits()
		{
			limit_find_eligible = maximum_limit;
			limit_fill_matrix = maximum_limit;
			limit_path_explore = maximum_limit;
			limit_ware_reroute = maximum_limit;
		}
		static void set_default_limits()
		{
			limit_find_eligible = default_find_eligible;
			limit_fill_matrix = default_fill_matrix;
			limit_path_explore = default_path_explore;
			limit_ware_reroute = default_ware_reroute;
		}

		static uint32 get_limit_find_eligible() { return limit_find_eligible; }
		static uint32 get_limit_fill_matrix() { return limit_fill_matrix; }
		static uint32 get_limit_path_explore() { return limit_path_explore; }
		static uint32 get_limit_ware_reroute() { return limit_ware_reroute; }

	};

	static uint8 max_categories;
	static uint8 category_empty;
	static compartment_t *ware_compartment;
	static uint8 current_compartment;
	static bool processing;

public:

	static void initialize();
	static void destroy();
	static void step();

	static void full_instant_refresh();
	static void refresh_all_categories(const bool reset_working_set);
	static void refresh_category(const uint8 category) { ware_compartment[category].set_refresh(); }
	static bool get_catg_path_between(const uint8 category, const halthandle_t origin_halt, const halthandle_t target_halt,
									  uint16 &aggregate_time, halthandle_t &next_transfer)
	{
		return ware_compartment[category].get_path_between(origin_halt, target_halt, aggregate_time, next_transfer);
	}

	static uint32 get_limit_find_eligible() { return compartment_t::get_limit_find_eligible(); }
	static uint32 get_limit_fill_matrix() { return compartment_t::get_limit_fill_matrix(); }
	static uint32 get_limit_path_explore() { return compartment_t::get_limit_path_explore(); }
	static uint32 get_limit_ware_reroute() { return compartment_t::get_limit_ware_reroute(); }
	static bool is_processing() { return processing; }


};

#endif

