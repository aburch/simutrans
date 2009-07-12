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

	// Adapted from sorted_heap_tpl. This version works directly on halthandle_t objects instead of on pointers.
	class halthandle_sorted_heap_t
	{

	private:

		halthandle_t *nodes;
		uint32 node_count;
		uint32 node_size;
		uint32 base_size;

	public:

		halthandle_sorted_heap_t(const uint32 init_size = 4096)
		{
			DBG_MESSAGE("sorted_heap_tpl()","initialized");
			base_size = init_size;
			nodes = MALLOCN(halthandle_t, base_size);
			node_size = base_size;
			node_count = 0;
		}

		~halthandle_sorted_heap_t()
		{
			free( nodes );
		}

		bool insert(const halthandle_t &item, const bool unique = false)
		{
			// uint32 index = node_count;
			uint32 index;
			
			if ( node_count == 0 || !(item <= nodes[0]) )
			{
				// Case : Empty or larger than the largest data
				// Note : Largest data is in nodes[0]
				index = 0;
			}
			else if ( !(nodes[node_count-1] <= item) )
			{
				// Case : Smaller than the smallest data
				index = node_count;
			}
			else
			{
				// Case : Within range : use binary search to locate appropriate insert location;
				//						 abort insertion if same data found and unique is true
				sint32 high = node_count, low = -1, probe;
				while(high - low > 1) 
				{
					probe = ((uint32) (low + high)) >> 1;
					if( nodes[probe] == item ) 
					{
						if (unique)
							return false;
						low = probe;
						break;
					}
					else if( item <= nodes[probe] ) 
					{
						low = probe;
					}
					else 
					{
						high = probe;
					}
				}
				// we want to insert before, so we may add 1
				index = low + 1;	// Knightly : Since we are within range, "low" will always be >= 0 and < node_count

			}

			// need to enlarge?
			if( node_count == node_size ) 
			{
				halthandle_t *tmp=nodes;
				node_size += base_size;
				nodes = MALLOCN(halthandle_t, node_size);
				memcpy( nodes, tmp, sizeof(halthandle_t) * (node_size - base_size) );
				free( tmp );
			}

			if( index < node_count ) 
			{
				memmove( nodes+index+1ul, nodes+index, sizeof(halthandle_t)*(node_count-index) );
			}

			nodes[index] = item;
			node_count ++;
			return true;
		}

		bool contains(const halthandle_t &item) const
		{
			if( node_count > 0  &&  nodes[node_count-1] <= item  &&  item <= nodes[0] ) 
			{
				sint32 high = node_count, low = -1, probe;
				while( high - low > 1) 
				{
					probe = ((uint32) (low + high)) >> 1;
					if( nodes[probe] == item ) 
					{
						return true;
					}
					else if( item <= nodes[probe] ) 
					{
						low = probe;
					}
					else 
					{
						high = probe;
					}
				}
			}
			return false;
		}

		bool get_position(const halthandle_t &item, uint32 &position) const
		{
			if( node_count > 0  &&  nodes[node_count-1] <= item  &&  item <= nodes[0] ) 
			{
				sint32 high = node_count, low = -1, probe;
				while( high - low > 1) 
				{
					probe = ((uint32) (low + high)) >> 1;
					if( nodes[probe] == item ) 
					{
						position = probe;
						return true;
					}
					else if( item <= nodes[probe] ) 
					{
						low = probe;
					}
					else 
					{
						high = probe;
					}
				}
			}
			return false;
		}

		halthandle_t operator[] (const uint32 index) const
		{
			if (index < node_count)
			{
				return nodes[index];
			}
			else
			{
				dbg->error("halthandle_sorted_heap_t::operator[]()", "Accessing beyond the boundary of the heap!");
				return halthandle_t();
			}
		}

		void clear() { node_count = 0; }

		int get_count() const {	return node_count; }

		bool empty() const { return node_count == 0; }
	};

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
		halthandle_sorted_heap_t *finished_heap;
		uint16 finished_halt_count;

		// set of variables for working path data
		path_element_t **working_matrix;
		transport_element_t **transport_matrix;
		halthandle_sorted_heap_t *working_heap;
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
		static uint32 limit_sort_eligible;
		static uint32 limit_fill_matrix;
		static uint32 limit_path_explore;
		static uint32 limit_ware_reroute;

		// back-up iteration limits
		static uint32 backup_sort_eligible;
		static uint32 backup_fill_matrix;
		static uint32 backup_path_explore;
		static uint32 backup_ware_reroute;

		// default iteration limits
		static const uint32 default_sort_eligible = 4096;
		static const uint32 default_fill_matrix = 4096;
		static const uint32 default_path_explore = 65536;
		static const uint32 default_ware_reroute = 4096;

		// maximum limit for full refresh
		static const uint32 maximum_limit = 4294967295;

		// phase indices
		static const uint8 phase_init_prepare = 0;
		static const uint8 phase_full_array = 1;
		static const uint8 phase_sort_eligible = 2;
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

		void enumerate_all_paths(path_element_t **matrix, halthandle_sorted_heap_t &heap);

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
					&& finished_heap->get_position(origin_halt, origin_index)
					&& finished_heap->get_position(target_halt, target_index)
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
			backup_sort_eligible = limit_sort_eligible;
			backup_fill_matrix = limit_fill_matrix;
			backup_path_explore = limit_path_explore;
			backup_ware_reroute = limit_ware_reroute;
		}
		static void restore_limits()
		{
			limit_sort_eligible = backup_sort_eligible;
			limit_fill_matrix = backup_fill_matrix;
			limit_path_explore = backup_path_explore;
			limit_ware_reroute = backup_ware_reroute;
		}
		static void set_maximum_limits()
		{
			limit_sort_eligible = maximum_limit;
			limit_fill_matrix = maximum_limit;
			limit_path_explore = maximum_limit;
			limit_ware_reroute = maximum_limit;
		}
		static void set_default_limits()
		{
			limit_sort_eligible = default_sort_eligible;
			limit_fill_matrix = default_fill_matrix;
			limit_path_explore = default_path_explore;
			limit_ware_reroute = default_ware_reroute;
		}

		static uint32 get_limit_sort_eligible() { return limit_sort_eligible; }
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

	static uint32 get_limit_sort_eligible() { return compartment_t::get_limit_sort_eligible(); }
	static uint32 get_limit_fill_matrix() { return compartment_t::get_limit_fill_matrix(); }
	static uint32 get_limit_path_explore() { return compartment_t::get_limit_path_explore(); }
	static uint32 get_limit_ware_reroute() { return compartment_t::get_limit_ware_reroute(); }
	static bool is_processing() { return processing; }


};

#endif

