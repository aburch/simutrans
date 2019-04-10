/*
 * Copyright (c) 2009 : Knightly
 *
 * A centralised, steppable path searching system using Floyd-Warshall Algorithm
 */


#ifndef path_explorer_h
#define path_explorer_h

#include "network/memory_rw.h"
#include "simline.h"
#include "simhalt.h"
#include "simworld.h"
#include "halthandle_t.h"
#include "convoihandle_t.h"
#include "linehandle_t.h"
#include "simtypes.h"
#include "simdebug.h"

#include "tpl/vector_tpl.h"
#include "tpl/quickstone_hashtable_tpl.h"


class path_explorer_t
{
public:
	struct limit_set_t
	{
		uint32 rebuild_connexions;
		uint32 filter_eligible;
		uint32 fill_matrix;
		uint64 explore_paths;
		uint32 reroute_goods;

		limit_set_t() : rebuild_connexions(0), filter_eligible(0), fill_matrix(0), explore_paths(0), reroute_goods(0) { }
		limit_set_t(bool) : rebuild_connexions(UINT32_MAX_VALUE), filter_eligible(UINT32_MAX_VALUE), fill_matrix(UINT32_MAX_VALUE), explore_paths(UINT64_MAX_VALUE), reroute_goods(UINT32_MAX_VALUE) { }
		limit_set_t(uint32 c, uint32 e, uint32 m, uint64 p, uint32 g) : rebuild_connexions(c), filter_eligible(e), fill_matrix(m), explore_paths(p), reroute_goods(g) { }
		
		void find_min_with(const limit_set_t &other)
		{
			if( other.rebuild_connexions < rebuild_connexions ) { rebuild_connexions = other.rebuild_connexions; }
			if( other.filter_eligible < filter_eligible ) { filter_eligible = other.filter_eligible; }
			if( other.fill_matrix < fill_matrix ) { fill_matrix = other.fill_matrix; }
			if( other.explore_paths < explore_paths ) { explore_paths = other.explore_paths; }
			if( other.reroute_goods < reroute_goods ) { reroute_goods = other.reroute_goods; }
		}

		bool operator == (const limit_set_t &other) const
		{
			if( rebuild_connexions == other.rebuild_connexions	&&
				filter_eligible == other.filter_eligible		&&
				fill_matrix == other.fill_matrix				&&
				explore_paths == other.explore_paths			&&
				reroute_goods == other.reroute_goods				)
			{
				return true;
			}
			return false;
		}

		bool operator != (const limit_set_t &other) const { return !( *this == other ); }

		void rdwr(memory_rw_t *buffer)
		{
			buffer->rdwr_long( rebuild_connexions );
			buffer->rdwr_long( filter_eligible );
			buffer->rdwr_long( fill_matrix );
			uint32 explore_paths_quotient;
			uint32 explore_paths_remainder;
			if( buffer->is_saving() )
			{
				explore_paths_quotient = (uint32)(explore_paths >> 32);
				explore_paths_remainder = (uint32)(explore_paths & 0xFFFFFFFFull);
			}
			buffer->rdwr_long( explore_paths_quotient );
			buffer->rdwr_long( explore_paths_remainder );
			if( buffer->is_loading() )
			{
				explore_paths = ((uint64)explore_paths_quotient << 32) | (uint64)explore_paths_remainder;
			}
			buffer->rdwr_long( reroute_goods );
		}
	};

	static bool must_refresh_on_loading;

	static void rdwr(loadsave_t* file); 

private:

	class compartment_t
	{
	
		friend class path_explorer_t;

	protected:
		typedef quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*> connexion_table_map;
		// structure for storing connexion hashtable and serving transport counter
		struct connexion_list_entry_t
		{
			connexion_table_map *connexion_table;
			uint8 serving_transport;
		};

	private:

		// element used during path search and for storing calculated paths
		struct path_element_t
		{
			uint32 aggregate_time;
			halthandle_t next_transfer;

			path_element_t() : aggregate_time(UINT32_MAX_VALUE) { }
		};

		// element used during path search only for storing best lines/convoys
		struct transport_element_t
		{
			uint16 first_transport;
			uint16 last_transport;
			
			transport_element_t() : first_transport(0), last_transport(0) { }
		};

		// structure used for storing indices of halts connected to a transfer, grouped by transport
		class connection_t
		{

		public:

			// element used for storing indices of halts connected to a transfer, together with their common transport
			struct connection_cluster_t
			{
				uint16 transport;
				vector_tpl<uint16> connected_halts;

				connection_cluster_t(const uint32 halt_vector_size, const uint16 transport_id, const uint16 halt_id) 
					: transport(transport_id), connected_halts(halt_vector_size)
				{
					connected_halts.append(halt_id);
				}

				connection_cluster_t(loadsave_t* file); 

				void rdwr(loadsave_t* file); 
			};

		private:

			vector_tpl<connection_cluster_t*> connection_clusters;
			uint32 usage_level;			// number of connection clusters used
			uint32 halt_vector_size;	// size of connected halt vector in connection cluster object
			typedef  inthashtable_tpl<uint16, connection_cluster_t*> cluster_map_type;
			cluster_map_type cluster_map;

		public:

			connection_t(const uint32 cluster_count, const uint32 working_halt_count) 
				: connection_clusters(cluster_count), usage_level(0), halt_vector_size(working_halt_count) { }

			~connection_t()
			{
				// deallocate memory for dynamically allocated connection clusters
				for ( uint32 i = 0; i < connection_clusters.get_count(); ++i )
				{
					delete connection_clusters[i];
				}
			}

			void reset()
			{
				// reset only clears the connected halt vectors of used connection clusters
				// connection clusters are not deallocated so that they can be re-used later
				for ( uint32 i = 0; i < usage_level; ++i )
				{
					connection_clusters[i]->connected_halts.clear();
				}
				usage_level = 0;
				cluster_map.clear();
			}

			void register_connection(const uint16 transport_id, const uint16 halt_id)
			{
				// check against each existing cluster
				connection_cluster_t *const existing_cluster = cluster_map.get(transport_id);
				if ( existing_cluster )
				{
					existing_cluster->connected_halts.append(halt_id);
					return;
				}

				// reaching here means no match is found --> re-use or create a new connection cluster 
				if ( usage_level < connection_clusters.get_count() )
				{
					// case : free connection clusters available for use
					connection_clusters[usage_level]->transport = transport_id;
					connection_clusters[usage_level]->connected_halts.append(halt_id);
					cluster_map.put(transport_id, connection_clusters[usage_level]);
				}
				else
				{
					// case : no more available connection cluster for use --> requires allocation
					connection_cluster_t *const new_cluster = new connection_cluster_t( halt_vector_size, transport_id, halt_id );
					connection_clusters.append( new_cluster );
					cluster_map.put(transport_id, new_cluster);
				}
				++usage_level;
			};
			
			uint32 get_cluster_count() const { return usage_level; }

			uint32 get_total_member_count() const
			{
				uint32 total = 0;
				for ( uint32 i = 0; i < usage_level; ++i )
				{
					total += connection_clusters[i]->connected_halts.get_count();
				}
				return total;
			}

			const connection_cluster_t& operator[](const uint32 element_id) const
			{
				if ( element_id >= usage_level )
				{
					dbg->fatal("connection_t::operator[]()", "Index out of bounds: %i not in 0..%i", element_id, (sint32)usage_level - 1);
				}
				return *(connection_clusters[element_id]);
			}

			void rdwr(loadsave_t* file); 
		};

		// data structure for temporarily storing lines and lineless conovys
		struct linkage_t
		{
			linehandle_t line;
			convoihandle_t convoy;
		};

		// store the start time of refresh
		sint64 refresh_start_time;

		// set of variables for finished path data
		path_element_t **finished_matrix;
		uint16 *finished_halt_index_map;
		uint16 finished_halt_count;

		// set of variables for working path data
		path_element_t **working_matrix;
		uint16 *transport_index_map;
		transport_element_t **transport_matrix;
		uint16 *working_halt_index_map;
		halthandle_t *working_halt_list;
		uint16 working_halt_count;

		// set of variables for full halt list
		halthandle_t *all_halts_list;
		uint16 all_halts_count;

		// a vector for storing lines and lineless convoys
		vector_tpl<linkage_t> *linkages;

		// set of variables for transfer list
		uint16 *transfer_list;
		uint16 transfer_count;

		uint8 catg;				// category managed by this compartment
		uint8 g_class;			// Class managed by this compartment
		const char *catg_name;	// Name of the category
		char *class_name;		// Name of the class
		uint16 step_count;		// number of steps done so far for a path refresh request

		// coordination flags
		bool paths_available;
		bool refresh_completed;
		bool refresh_requested;

		// phase indicator
		uint8 current_phase;

		// phase counters
		uint16 phase_counter;
		uint32 iterations;
		uint32 total_iterations; // for desync debug only

		// phase counters for path searching
		uint16 via_index;
		uint32 origin_cluster_index;
		uint32 target_cluster_index;
		uint32 origin_member_index;

		// variables for limiting search around transfers
		connection_t *inbound_connections;		// relative to the current transfer
		connection_t *outbound_connections;		// relative to the current transfer
		bool process_next_transfer;

		// statistics for determining limits
		uint32 statistic_duration;
		uint32 statistic_iteration;

		// an array of names for the various phases
		static const char *const phase_name[];
		
protected:
		// an array for keeping a list of connexion hash table
		static connexion_list_entry_t connexion_list[65536];
		
private:

		// iteration representative
		static uint16 representative_halt_count;
		static uint8 representative_category;

		// indicate whether phase limits are used or not
		// -> it is turned off for initial full instant search
		static bool use_limits;
		
		// iteration limitslinkages
		static uint32 limit_rebuild_connexions;
		static uint32 limit_filter_eligible;
		static uint32 limit_fill_matrix;
		static uint64 limit_explore_paths;
		static uint32 limit_reroute_goods;

		// local iteration limits
		static uint32 local_rebuild_connexions;
		static uint32 local_filter_eligible;
		static uint32 local_fill_matrix;
		static uint64 local_explore_paths;
		static uint32 local_reroute_goods;

		// indicate whether local limits has changed
		static bool local_limits_changed;

		// default iteration limits
		static const uint32 default_rebuild_connexions  = 0x0400;
		static const uint32 default_filter_eligible = 0x00018000;
		static const uint32 default_fill_matrix     = 0x00010000;
		static const uint64 default_explore_paths   = 0x01000000;
		static const uint32 default_reroute_goods       = 0x0100;

		// phase indices
		static const uint8 phase_check_flag = 0;
		static const uint8 phase_init_prepare = 1;
		static const uint8 phase_rebuild_connexions = 2;
		static const uint8 phase_filter_eligible = 3;
		static const uint8 phase_fill_matrix = 4;
		static const uint8 phase_explore_paths = 5;
		static const uint8 phase_reroute_goods = 6;

		// absolute time limits
		// The higher this number, the more processing will be done per step and the more quickly that a refresh will complete, but the more computationally intensive that it will be. 
		// Knightly's original setting was 24. The revised setting was 64.
		// Now set by simuconf.tab.
		static uint32 time_midpoint;
		static const uint32 time_deviation = 2;
		static uint32 time_lower_limit;
		static uint32 time_upper_limit;
		static uint32 time_threshold;

		// percentage time limits
		static const uint32 percent_deviation = 5;
		static const uint32 percent_lower_limit = 100 - percent_deviation;
		static const uint32 percent_upper_limit = 100 + percent_deviation;

		void enumerate_all_paths(const path_element_t *const *const matrix, const halthandle_t *const halt_list,
								 const uint16 *const halt_map, const uint16 halt_count);

	public:

		compartment_t();
		~compartment_t();

		static void initialise();
		static void finalise();

		void rdwr(loadsave_t* file); 

		static void set_absolute_limits();

		void step();
		void reset(const bool reset_finished_set);

		bool are_paths_available() const { return paths_available; }
		bool is_refresh_completed() const { return refresh_completed; }
		bool is_refresh_requested() const { return refresh_requested; }

		// Note that these are only used for the client/server synchronisation checklist for diagnostic purposes.
		uint8 get_current_phase() const { return current_phase; }
		uint16 get_phase_counter() const { return phase_counter; }
		uint16 get_working_halt_count() const { return working_halt_count; }
		uint16 get_all_halt_count() const { return all_halts_count; }
		uint16 get_transfer_count() const { return transfer_count; }
		uint32 get_total_iterations() { const uint32 ti = total_iterations; total_iterations = 0; return ti; }

		void set_category(uint8 category);
		void set_class(uint8 value); 
		void set_refresh() { refresh_requested = true; }

		bool get_path_between(const halthandle_t origin_halt, const halthandle_t target_halt,
							  uint32 &aggregate_time, halthandle_t &next_transfer);

		const char *get_category_name() const { return ( catg_name ? catg_name : "" ); }
		const char *get_class_name() const { return ( class_name ? class_name : "" );  }
		const char *get_current_phase_name() const { return phase_name[current_phase]; }

		static void initialise_connexion_list();

		static void reset_connexion_entry(const uint16 halt_id);

		static void reset_connexion_list();

		static void finalise_connexion_list();

		static void enable_limits(const bool yesno)
		{
			use_limits = yesno;
		}
		
		static limit_set_t get_local_limits()
		{
			return limit_set_t( local_rebuild_connexions, local_filter_eligible, local_fill_matrix, local_explore_paths, local_reroute_goods );
		}

		static limit_set_t get_active_limits()
		{
			return limit_set_t( limit_rebuild_connexions, limit_filter_eligible, limit_fill_matrix, limit_explore_paths, limit_reroute_goods );
		}

		// This is only used in network mode.
		static void set_limits(const limit_set_t &limit_set)
		{
			limit_rebuild_connexions = limit_set.rebuild_connexions;
			limit_filter_eligible = limit_set.filter_eligible;
			limit_fill_matrix = limit_set.fill_matrix;
			limit_explore_paths = limit_set.explore_paths;
			limit_reroute_goods = limit_set.reroute_goods;
		}

		static void set_default_limits()
		{
			limit_rebuild_connexions = default_rebuild_connexions;
			limit_filter_eligible = default_filter_eligible;
			limit_fill_matrix = default_fill_matrix;
			limit_explore_paths = default_explore_paths;
			limit_reroute_goods = default_reroute_goods;
		}

		static bool are_local_limits_changed() { return local_limits_changed; }
		static void reset_local_limits_state() { local_limits_changed = false; }
		static uint32 get_limit_rebuild_connexions() { return limit_rebuild_connexions; }
		static uint32 get_limit_filter_eligible() { return limit_filter_eligible; }
		static uint32 get_limit_fill_matrix() { return limit_fill_matrix; }
		static uint64 get_limit_explore_paths() { return limit_explore_paths; }
		static uint32 get_limit_reroute_goods() { return limit_reroute_goods; }

	};

	static karte_t *world;
	static uint8 max_categories;
	static uint8 max_classes;
	static uint8 category_empty;
public:
	static compartment_t **goods_compartment;
private:
	static uint8 current_compartment_category;
	static uint8 current_compartment_class;
	static bool processing;

public:
#ifdef MULTI_THREAD
	static thread_local bool allow_path_explorer_on_this_thread;
	friend void *path_explorer_threaded(void* args);
#endif
	static void initialise(karte_t *welt);
	static void finalise();
	static void step();
	static void next_compartment();

	static void full_instant_refresh();
	static void refresh_all_categories(const bool reset_working_set);
	static void refresh_category(const uint8 category);
	static void refresh_class_category(const uint8 category, const uint8 g_class);
	static bool get_catg_path_between(const uint8 category, const halthandle_t origin_halt, const halthandle_t target_halt,
									  uint32 &aggregate_time, halthandle_t &next_transfer, uint8 g_class = 0)
	{
		return goods_compartment[category][g_class].get_path_between(origin_halt, target_halt, aggregate_time, next_transfer);
	}

	static karte_t *get_world() { return world; }
	static bool are_local_limits_changed() { return compartment_t::are_local_limits_changed(); }
	static void reset_local_limits_state() { compartment_t::reset_local_limits_state(); }
	static limit_set_t get_local_limits() { return compartment_t::get_local_limits(); }
	static limit_set_t get_active_limits() { return compartment_t::get_active_limits(); }
	static void set_limits(const limit_set_t &limit_set) { compartment_t::set_limits(limit_set); }
	static uint32 get_limit_rebuild_connexions() { return compartment_t::get_limit_rebuild_connexions(); }
	static uint32 get_limit_filter_eligible() { return compartment_t::get_limit_filter_eligible(); }
	static uint32 get_limit_fill_matrix() { return compartment_t::get_limit_fill_matrix(); }
	static uint64 get_limit_explore_paths() { return compartment_t::get_limit_explore_paths(); }
	static uint32 get_limit_reroute_goods() { return compartment_t::get_limit_reroute_goods(); }
	static bool is_processing() { return processing; }
	static const char *get_current_category_name() { return goods_compartment[current_compartment_category][current_compartment_class].get_category_name(); }
	static const char *get_current_class_name() { return  goods_compartment[current_compartment_category][current_compartment_class].get_class_name(); }
	static const char *get_current_phase_name() { return goods_compartment[current_compartment_category][current_compartment_class].get_current_phase_name(); }
	
	// Note that these are only used for the client/server synchronisation checklist for diagnostic purposes.
	static uint8 get_current_compartment_category() { return current_compartment_category; }
	static uint8 get_current_compartment_class() { return current_compartment_class;  }
	static uint8 get_max_categories() { return max_categories; }
	static bool get_paths_available(uint8 catg, uint8 g_class = 0) { return goods_compartment[catg][g_class].are_paths_available(); }
	static bool get_refresh_completed(uint8 catg, uint8 g_class) { return goods_compartment[catg][g_class].is_refresh_completed(); }
	static bool get_refresh_requested(uint8 catg, uint8 g_class) { return goods_compartment[catg][g_class].is_refresh_requested(); }
	static uint8 get_current_phase(uint8 catg, uint8 g_class) { return goods_compartment[catg][g_class].get_current_phase(); }
	static uint16 get_phase_counter(uint8 catg, uint8 g_class) { return goods_compartment[catg][g_class].get_phase_counter(); }
	static uint16 get_working_halt_count(uint8 catg, uint8 g_class) { return goods_compartment[catg][g_class].get_working_halt_count(); }
	static uint16 get_all_halt_count(uint8 catg, uint8 g_class) { return goods_compartment[catg][g_class].get_all_halt_count(); }
	static uint16 get_transfer_count(uint8 catg, uint8 g_class) { return goods_compartment[catg][g_class].get_transfer_count(); }
	static uint32 get_total_iterations(uint8 catg, uint8 g_class) { return goods_compartment[catg][g_class].get_total_iterations(); }

	inline static void set_absolute_limits_external() { compartment_t::set_absolute_limits();  }

	inline static bool get_must_refresh_on_loading() { return must_refresh_on_loading; }
	inline static void set_must_refresh_on_loading() { must_refresh_on_loading = true; }
	inline static void reset_must_refresh_on_loading() { must_refresh_on_loading = false; }
};

#endif

