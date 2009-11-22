/*
 * Copyright (c) 2009 : Knightly
 *
 * A centralised, steppable path searching system using Floyd-Warshall Algorithm
 */


#ifndef path_explorer_h
#define path_explorer_h

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

private:

	class compartment_t
	{
	
	private:

		// structure for storing connexion hashtable and serving transport counter
		struct connexion_list_entry_t
		{
			quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*> *connexion_table;
			uint8 serving_transport;
		};

		// element used during path search and for storing calculated paths
		struct path_element_t
		{
			uint16 aggregate_time;
			halthandle_t next_transfer;

			path_element_t() : aggregate_time(65535) { }
		};

		// element used during path search only for storing best lines/convoys
		struct transport_element_t
		{
			union
			{
				struct
				{
					uint16 first_line;
					uint16 first_convoy;
				};
				uint32 first_transport;
			};

			union
			{
				struct
				{
					uint16 last_line;
					uint16 last_convoy;
				};
				uint32 last_transport;
			};

			transport_element_t() : first_transport(0), last_transport(0) { }
		};

		// structure used for storing indices of halts connected to a transfer, grouped by transport
		class connection_t
		{

		public:

			// element used for storing indices of halts connected to a transfer, together with their common transport
			struct connection_cluster_t
			{
				uint32 transport;
				vector_tpl<uint16> connected_halts;

				connection_cluster_t(const uint32 halt_vector_size) : connected_halts(halt_vector_size) { }

				connection_cluster_t(const uint32 halt_vector_size, const uint32 transport_id, const uint16 halt_id) 
					: transport(transport_id), connected_halts(halt_vector_size)
				{
					connected_halts.append(halt_id);
				}

				connection_cluster_t& operator= (const connection_cluster_t &source)
				{
					// self-assignment --> do nothing and return
					if ( this == &source )
					{
						return *this;
					}

					// reset transport ID
					transport = source.transport;
					
					// clear connected halt list and copy over from source one by one
					connected_halts.clear();
					connected_halts.resize( source.connected_halts.get_size() );
					for ( uint32 i = 0; i < source.connected_halts.get_count(); ++i )
					{
						connected_halts.append( source.connected_halts[i] );
					}
					
					return *this;
				}
			};

		private:

			vector_tpl<connection_cluster_t*> connection_clusters;
			uint32 usage_level;			// number of connection clusters used
			uint32 halt_vector_size;	// size of connected halt vector in connection cluster object

		public:

			connection_t(const uint32 cluster_count, const uint32 working_halt_count) 
				: connection_clusters(cluster_count), usage_level(0), halt_vector_size(working_halt_count)
			{
				// create connection clusters in advance
				for ( uint32 i = 0; i < cluster_count; ++i )
				{
					connection_clusters.append ( new connection_cluster_t(halt_vector_size) );
				}
			}

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
			}

			void register_connection(const uint32 transport_id, const uint16 halt_id)
			{
				// check against each existing cluster
				for ( uint32 i = 0; i < usage_level; ++i )
				{
					if ( connection_clusters[i]->transport == transport_id )
					{
						connection_clusters[i]->connected_halts.append(halt_id);
						return;
					}
				}

				// reaching here means no match is found --> re-use or create a new connection cluster 
				if ( usage_level < connection_clusters.get_count() )
				{
					// case : free connection clusters available for use
					connection_clusters[usage_level]->transport = transport_id;
					connection_clusters[usage_level]->connected_halts.append(halt_id);
				}
				else
				{
					// case : no more available connection cluster for use --> requires allocation
					connection_clusters.append( new connection_cluster_t( halt_vector_size, transport_id, halt_id ) );
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
				if ( element_id < usage_level )
				{
					return *(connection_clusters[element_id]);
				}
				else
				{
					dbg->fatal("connection_t::operator[]()", "Index out of bounds: %i not in 0..%d", element_id, usage_level - 1);
				}
			}
		};

		// data structure for temporarily storing lines and lineless conovys
		struct linkage_t
		{
			linehandle_t line;
			convoihandle_t convoy;
		};

		// store the start time of refresh
		unsigned long refresh_start_time;

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

		// a vector for storing lines and lineless convoys
		vector_tpl<linkage_t> *linkages;
		uint32 linkages_count;

		// set of variables for transfer list
		uint16 *transfer_list;
		uint16 transfer_count;

		uint8 catg;				// category managed by this compartment
		const char *catg_name;	// name of the category
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

		// an array for keeping a list of connexion hash table
		static connexion_list_entry_t connexion_list[65536];

		// iteration representative
		static uint16 representative_halt_count;
		static uint8 representative_category;

		// iteration limits
		static uint32 limit_rebuild_connexions;
		static uint32 limit_filter_eligible;
		static uint32 limit_fill_matrix;
		static uint64 limit_explore_paths;
		static uint32 limit_reroute_goods;

		// back-up iteration limits
		static uint32 backup_rebuild_connexions;
		static uint32 backup_filter_eligible;
		static uint32 backup_fill_matrix;
		static uint64 backup_explore_paths;
		static uint32 backup_reroute_goods;

		// default iteration limits
		static const uint32 default_rebuild_connexions = 4096;
		static const uint32 default_filter_eligible = 4096;
		static const uint32 default_fill_matrix = 4096;
		static const uint64 default_explore_paths = 1048576;
		static const uint32 default_reroute_goods = 4096;

		// maximum limit for full refresh
		static const uint32 maximum_limit_32bit = UINT32_MAX_VALUE;
		static const uint64 maximum_limit_64bit = UINT64_MAX_VALUE;

		// phase indices
		static const uint8 phase_check_flag = 0;
		static const uint8 phase_init_prepare = 1;
		static const uint8 phase_rebuild_connexions = 2;
		static const uint8 phase_filter_eligible = 3;
		static const uint8 phase_fill_matrix = 4;
		static const uint8 phase_explore_paths = 5;
		static const uint8 phase_reroute_goods = 6;

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

		static void initialise();
		static void finalise();
		void step();
		void reset(const bool reset_finished_set);

		bool are_paths_available() { return paths_available; }
		bool is_refresh_completed() { return refresh_completed; }
		bool is_refresh_requested() { return refresh_requested; }

		void set_category(uint8 category);
		void set_refresh() { refresh_requested = true; }

		bool get_path_between(const halthandle_t origin_halt, const halthandle_t target_halt,
							  uint16 &aggregate_time, halthandle_t &next_transfer);

		const char *get_category_name() { return ( catg_name ? catg_name : "" ); }
		const char *get_current_phase_name() { return phase_name[current_phase]; }

		static void initialise_connexion_list();

		static void reset_connexion_entry(const uint16 halt_id);

		static void reset_connexion_list();

		static void finalise_connexion_list();
		
		static void backup_limits()
		{
			backup_rebuild_connexions = limit_rebuild_connexions;
			backup_filter_eligible = limit_filter_eligible;
			backup_fill_matrix = limit_fill_matrix;
			backup_explore_paths = limit_explore_paths;
			backup_reroute_goods = limit_reroute_goods;
		}
		static void restore_limits()
		{
			limit_rebuild_connexions = backup_rebuild_connexions;
			limit_filter_eligible = backup_filter_eligible;
			limit_fill_matrix = backup_fill_matrix;
			limit_explore_paths = backup_explore_paths;
			limit_reroute_goods = backup_reroute_goods;
		}
		static void set_maximum_limits()
		{
			limit_rebuild_connexions = maximum_limit_32bit;
			limit_filter_eligible = maximum_limit_32bit;
			limit_fill_matrix = maximum_limit_32bit;
			limit_explore_paths = maximum_limit_64bit;
			limit_reroute_goods = maximum_limit_32bit;
		}
		static void set_default_limits()
		{
			limit_rebuild_connexions = default_rebuild_connexions;
			limit_filter_eligible = default_filter_eligible;
			limit_fill_matrix = default_fill_matrix;
			limit_explore_paths = default_explore_paths;
			limit_reroute_goods = default_reroute_goods;
		}

		static uint32 get_limit_rebuild_connexions() { return limit_rebuild_connexions; }
		static uint32 get_limit_filter_eligible() { return limit_filter_eligible; }
		static uint32 get_limit_fill_matrix() { return limit_fill_matrix; }
		static uint64 get_limit_explore_paths() { return limit_explore_paths; }
		static uint32 get_limit_reroute_goods() { return limit_reroute_goods; }

	};

	static karte_t *world;
	static uint8 max_categories;
	static uint8 category_empty;
	static compartment_t *goods_compartment;
	static uint8 current_compartment;
	static bool processing;

public:

	static void initialise(karte_t *welt);
	static void finalise();
	static void step();

	static void full_instant_refresh();
	static void refresh_all_categories(const bool reset_working_set);
	static void refresh_category(const uint8 category) { goods_compartment[category].set_refresh(); }
	static bool get_catg_path_between(const uint8 category, const halthandle_t origin_halt, const halthandle_t target_halt,
									  uint16 &aggregate_time, halthandle_t &next_transfer)
	{
		return goods_compartment[category].get_path_between(origin_halt, target_halt, aggregate_time, next_transfer);
	}

	static karte_t *get_world() { return world; }
	static uint32 get_limit_rebuild_connexions() { return compartment_t::get_limit_rebuild_connexions(); }
	static uint32 get_limit_filter_eligible() { return compartment_t::get_limit_filter_eligible(); }
	static uint32 get_limit_fill_matrix() { return compartment_t::get_limit_fill_matrix(); }
	static uint64 get_limit_explore_paths() { return compartment_t::get_limit_explore_paths(); }
	static uint32 get_limit_reroute_goods() { return compartment_t::get_limit_reroute_goods(); }
	static bool is_processing() { return processing; }
	static const char *get_current_category_name() { return goods_compartment[current_compartment].get_category_name(); }
	static const char *get_current_phase_name() { return goods_compartment[current_compartment].get_current_phase_name(); }

};

#endif

