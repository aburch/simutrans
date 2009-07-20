/*
 * Copyright (c) 2009 : Knightly
 *
 * A centralised, steppable path searching system using Floyd-Warshall Algorithm
 */


#include "path_explorer.h"

#include "tpl/slist_tpl.h"
#include "dataobj/translator.h"
#include "bauer/warenbauer.h"
#include "besch/ware_besch.h"
#include "simsys.h"
#include "simgraph.h"
#include "./player/simplay.h"


// #define DEBUG_EXPLORER_SPEED
// #define DEBUG_COMPARTMENT_STEP


///////////////////////////////////////////////

// path_explorer_t

karte_t *path_explorer_t::world = NULL;
uint8 path_explorer_t::max_categories = 0;
uint8 path_explorer_t::category_empty = 255;
path_explorer_t::compartment_t *path_explorer_t::goods_compartment = NULL;
uint8 path_explorer_t::current_compartment = 0;
bool path_explorer_t::processing = false;


void path_explorer_t::initialise(karte_t *welt)
{
	if (welt)
	{
		world = welt;
	}
	max_categories = warenbauer_t::get_max_catg_index();
	category_empty = warenbauer_t::nichts->get_catg_index();
	goods_compartment = new compartment_t[max_categories];

	for (uint8 i = 0; i < max_categories; i++)
	{
		goods_compartment[i].set_category(i);
	}

	current_compartment = 0;
	processing = false;

	compartment_t::initialise();
}


void path_explorer_t::destroy()
{
	delete[] goods_compartment;
	goods_compartment = NULL;
	current_compartment = 0;
	category_empty = 255;
	max_categories = 0;
	processing = false;

	compartment_t::destroy();
}


void path_explorer_t::step()
{
	// at most check all goods categories once
	for (uint8 i = 0; i < max_categories; i++)
	{
		if ( current_compartment != category_empty
			 && (!goods_compartment[current_compartment].is_refresh_completed() 
			     || goods_compartment[current_compartment].is_refresh_requested() ) )
		{
			processing = true;	// this step performs something

			// perform step
			goods_compartment[current_compartment].step();

			// if refresh is completed, move on to the next category
			if ( goods_compartment[current_compartment].is_refresh_completed() )
			{
				current_compartment = (current_compartment + 1) % max_categories;
			}

			// each step process at most 1 goods category
			return;
		}

		// advance to the next category only if compartment.step() is not invoked
		current_compartment = (current_compartment + 1) % max_categories;
	}

	processing = false;	// this step performs nothing
}


void path_explorer_t::full_instant_refresh()
{
	// exclude empty goods (nichts)
	uint16 total_steps = (max_categories - 1) * 6;
	uint16 curr_step = 0;

	processing = true;

	// initialize progress bar
	display_set_progress_text(translator::translate("Calculating paths ..."));
	display_progress(curr_step, total_steps);

	compartment_t::backup_limits();

	// change all limits to maximum so that each phase is performed in one step
	compartment_t::set_maximum_limits();

	// clear all connexion hash tables
	compartment_t::clear_all_connexion_tables();

#ifdef DEBUG_EXPLORER_SPEED
	unsigned long start, diff;
	start = dr_time();
#endif

	for (uint8 c = 0; c < max_categories; c++)
	{
		if ( c != category_empty )
		{
			// clear any previous leftovers
			goods_compartment[c].reset(true);

#ifndef DEBUG_EXPLORER_SPEED
			// go through all 6 phases
			for (uint8 p = 0; p < 6; p++)
			{
				// perform step
				goods_compartment[c].step();
				curr_step++;
				display_progress(curr_step, total_steps);
			}
#else
			// one step should perform the compartment phases from the first phase till the path exploration phase
			goods_compartment[c].step();
#endif
		}
	}

#ifdef DEBUG_EXPLORER_SPEED
	diff = dr_time() - start;
	printf("\n\nTotal time taken :  %lu ms \n", diff);
#endif

	// restore limits back to default
	compartment_t::restore_limits();

	// reset current category pointer
	current_compartment = 0;

	processing = false;
}


void path_explorer_t::refresh_all_categories(const bool reset_working_set)
{
	if (reset_working_set)
	{
		for (uint8 c = 0; c < max_categories; c++)
		{
			// do not remove the finished matrix and halt index map
			goods_compartment[c].reset(false);
		}

		// clear all connexion hash tables
		compartment_t::clear_all_connexion_tables();

		// reset current category pointer : refresh will start from passengers
		current_compartment = 0;
	}
	else
	{
		for (uint8 c = 0; c < max_categories; c++)
		{
			// only set flag
			goods_compartment[c].set_refresh();
		}
	}
}

///////////////////////////////////////////////

// compartment_t

quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*> *path_explorer_t::compartment_t::connexion_list[65536];

uint32 path_explorer_t::compartment_t::limit_rebuild_connexions = default_rebuild_connexions;
uint32 path_explorer_t::compartment_t::limit_find_eligible = default_find_eligible;
uint32 path_explorer_t::compartment_t::limit_fill_matrix = default_fill_matrix;
uint32 path_explorer_t::compartment_t::limit_explore_paths = default_explore_paths;
uint32 path_explorer_t::compartment_t::limit_reroute_goods = default_reroute_goods;

uint32 path_explorer_t::compartment_t::backup_rebuild_connexions = default_rebuild_connexions;
uint32 path_explorer_t::compartment_t::backup_find_eligible = default_find_eligible;
uint32 path_explorer_t::compartment_t::backup_fill_matrix = default_fill_matrix;
uint32 path_explorer_t::compartment_t::backup_explore_paths = default_explore_paths;
uint32 path_explorer_t::compartment_t::backup_reroute_goods = default_reroute_goods;

uint16 path_explorer_t::compartment_t::representative_halt_count = 0;
uint8 path_explorer_t::compartment_t::representative_category = 0;

path_explorer_t::compartment_t::compartment_t()
{
	finished_matrix = NULL;
	finished_halt_index_map = NULL;
	finished_halt_count = 0;

	working_matrix = NULL;
	transport_matrix = NULL;
	working_halt_index_map = NULL;
	working_halt_list = NULL;
	working_halt_count = 0;

	all_halts_list = NULL;
	all_halts_count = 0;

	linkages = NULL;
	linkages_count = 0;

	transfer_list = NULL;
	transfer_count = 0;;

	catg = 255;
	step_count = 0;

	paths_available = false;
	refresh_completed = true;
	refresh_requested = true;

	current_phase = phase_gate_sentinel;

	phase_counter = 0;
	iterations = 0;

	via_index = 0;
	origin = 0;

	limit_explore_origins = 0;

	statistic_duration = 0;
	statistic_iteration = 0;
}


path_explorer_t::compartment_t::~compartment_t()
{
	if (finished_matrix)
	{
		for (uint16 i = 0; i < finished_halt_count; i++)
		{
			delete[] finished_matrix[i];
		}
		delete[] finished_matrix;
	}
	if (finished_halt_index_map)
	{
		delete[] finished_halt_index_map;
	}


	if (working_matrix)
	{
		for (uint16 i = 0; i < working_halt_count; i++)
		{
			delete[] working_matrix[i];
		}
		delete[] working_matrix;
	}
	if (transport_matrix)
	{
		for (uint16 i = 0; i < working_halt_count; i++)
		{
			delete[] transport_matrix[i];
		}
		delete[] transport_matrix;
	}
	if (working_halt_index_map)
	{
		delete[] working_halt_index_map;
	}
	if (working_halt_list)
	{
		delete[] working_halt_list;
	}


	if (all_halts_list)
	{
		delete[] all_halts_list;
	}

	if (linkages)
	{
		delete linkages;
	}

	if (transfer_list)
	{
		delete[] transfer_list;
	}
}


void path_explorer_t::compartment_t::reset(const bool reset_finished_set)
{
	if (reset_finished_set)
	{
		if (finished_matrix)
		{
			for (uint16 i = 0; i < finished_halt_count; i++)
			{
				delete[] finished_matrix[i];
			}
			delete[] finished_matrix;
			finished_matrix = NULL;
		}
		if (finished_halt_index_map)
		{
			delete[] finished_halt_index_map;
			finished_halt_index_map = NULL;
		}		
		finished_halt_count = 0;
	}


	if (working_matrix)
	{
		for (uint16 i = 0; i < working_halt_count; i++)
		{
			delete[] working_matrix[i];
		}
		delete[] working_matrix;
		working_matrix = NULL;
	}	
	if (transport_matrix)
	{
		for (uint16 i = 0; i < working_halt_count; i++)
		{
			delete[] transport_matrix[i];
		}
		delete[] transport_matrix;
		transport_matrix = NULL;
	}
	if (working_halt_index_map)
	{
		delete[] working_halt_index_map;
		working_halt_index_map = NULL;
	}
	if (working_halt_list)
	{
		delete[] working_halt_list;
		working_halt_list = NULL;
	}	
	working_halt_count = 0;


	if (all_halts_list)
	{
		delete[] all_halts_list;
		all_halts_list = NULL;
	}	
	all_halts_count = 0;


	if (linkages)
	{
		delete linkages;
		linkages = NULL;
	}
	linkages_count = 0;


	if (transfer_list)
	{
		delete[] transfer_list;
		transfer_list = NULL;
	}	
	transfer_count = 0;

	step_count = 0;

	if (reset_finished_set)
	{
		paths_available = false;
	}
	refresh_completed = true;
	refresh_requested = true;

	current_phase = phase_gate_sentinel;

	phase_counter = 0;
	iterations = 0;

	via_index = 0;
	origin = 0;

	limit_explore_origins = 0;

	statistic_duration = 0;
	statistic_iteration = 0;
}


void path_explorer_t::compartment_t::initialise()
{
	initialise_connexion_list();
}


void path_explorer_t::compartment_t::destroy()
{
	destroy_all_connexion_tables();
}


void path_explorer_t::compartment_t::step()
{

#ifdef DEBUG_COMPARTMENT_STEP
	printf("\n\nCategory :  %s / %s \n", translator::translate( warenbauer_t::get_info_catg_index(catg)->get_catg_name()), translator::translate( warenbauer_t::get_info_catg_index(catg)->get_name()) );
#endif

	// For timing use
	unsigned long start, diff;

	switch (current_phase)
	{
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Phase 0 : Determine if a new refresh should be done, and prepare relevant flags accordingly
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_gate_sentinel :
		{
			if (refresh_requested)
			{
				refresh_requested = false;	// immediately reset it so that we can take new requests
				refresh_completed = false;	// indicate that processing is at work
				current_phase = phase_init_prepare;	// proceed to next phase
				// no return statement here, as we want to fall through to the next phase
			}
			else
			{
#ifdef DEBUG_COMPARTMENT_STEP
				printf("\t\t\tRefresh has not been requested\n");
#endif
				return;
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Phase 1 : Prepare a list of all halts, a halt index map, and a list of linkages for connexions reconstruction
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_init_prepare :  
		{
#ifdef DEBUG_COMPARTMENT_STEP
			step_count++;

			printf("\t\tCurrent Step : %lu \n", step_count);

			start = dr_time();	// start timing
#endif

			slist_iterator_tpl<halthandle_t> halt_iter(haltestelle_t::get_alle_haltestellen());
			all_halts_count = (uint16) haltestelle_t::get_alle_haltestellen().get_count();
			
			// create all halts list
			if (all_halts_count > 0)
			{
				all_halts_list = new halthandle_t[all_halts_count];
			}

			// Save the halt list in an array first to prevent the list from being modified across steps, causing bugs
			for (uint16 i = 0; i < all_halts_count; i++)
			{
				halt_iter.next();
				all_halts_list[i] = halt_iter.get_current();

				// create an empty connexion hash table if the current halt does not already have one
				if ( connexion_list[ all_halts_list[i].get_id() ] == NULL )
				{
					connexion_list[ all_halts_list[i].get_id() ] = new quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*>();
				}
			}

			// create and initlialize a halthandle-entry to matrix-index map (halt index map)
			working_halt_index_map = new uint16[65536];
			for (uint32 i = 0; i < 65536; i++)
			{
				// For quickstone handle, there can at most be 65535 valid entries, plus entry 0 which is reserved for null handle
				// Thus, the range of quickstone entries [1, 65535] is mapped to the range of matrix index [0, 65534]
				// Matrix index 65535 either means null handle or the halt has no connexion of the relevant goods category
				// This is always created regardless
				working_halt_index_map[i] = 65535;
			}

			// create a list of schedules of lines and lineless convoys
			linkages = new vector_tpl<linkage_t>(1024);
			convoihandle_t current_convoy;
			linehandle_t current_line;
			convoihandle_t dummy_convoy;
			linkage_t temp_linkage;


			// loop through all convoys
			for (vector_tpl<convoihandle_t>::const_iterator i = world->convois_begin(), end = world->convois_end(); i != end; i++) 
			{
				current_convoy = *i;
				// only consider lineless convoys which support this compartment's goods catetory
				if ( !current_convoy->get_line().is_bound() && current_convoy->get_goods_catg_index().is_contained(catg) )
				{
					temp_linkage.convoy = current_convoy;
					linkages->append(temp_linkage);
				}
			}

			temp_linkage.convoy = dummy_convoy;	// reset the convoy handle component

			// loop through all lines of all players
			for (int i = 0; i < MAX_PLAYER_COUNT; i++) 
			{
				spieler_t *current_player = world->get_spieler(i);

				if(  current_player == NULL  ) 
				{
					continue;
				}

				for (vector_tpl<linehandle_t>::const_iterator j = current_player->simlinemgmt.get_all_lines().begin(), 
					 end = current_player->simlinemgmt.get_all_lines().end(); j != end; j++) 
				{
					current_line = *j;
					// only consider lines which support this compartment's goods category
					if ( current_line->get_goods_catg_index().is_contained(catg) )
					{
						temp_linkage.line = current_line;
						linkages->append(temp_linkage);
					}
				}
			}

			linkages_count = linkages->get_count();


#ifdef DEBUG_COMPARTMENT_STEP
			diff = dr_time() - start;	// stop timing

			printf("\tTotal Halt Count :  %lu \n", all_halts_count);
			printf("\tTotal Lines/Lineless Convoys Count :  %ul \n", linkages_count);
			printf("\t\t\tInitial prepration takes :  %lu ms \n", diff);
#endif

			current_phase = phase_rebuild_connexions;	// proceed to the next phase

#ifndef DEBUG_EXPLORER_SPEED
			return;
#endif
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Phase 2 : Rebuild connexions for this compartment's goods category
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_rebuild_connexions :
		{
#ifdef DEBUG_COMPARTMENT_STEP
			step_count++;

			printf("\t\tCurrent Step : %lu \n", step_count);
#endif

			const spieler_t *const public_player = world->get_spieler(1); // public player is #1; #0 is human player
			const ware_besch_t *const ware_type = warenbauer_t::get_info_catg_index(catg);
			const float journey_time_adjustment = world->get_einstellungen()->get_journey_time_multiplier() * 600;

			linkage_t current_linkage;
			schedule_t *current_schedule;
			spieler_t *current_owner;
			uint32 current_average_speed;

			uint8 entry_count;
			halthandle_t current_halt;
			float journey_time_factor;

			minivec_tpl<halthandle_t> halt_list(64);
			minivec_tpl<uint16> journey_time_list(64);

			uint16 accumulated_journey_time;
			quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*> *catg_connexions;
			haltestelle_t::connexion *new_connexion;

			start = dr_time();	// start timing

			// for each schedule of line / lineless convoy
			while (phase_counter < linkages_count)
			{
				current_linkage = (*linkages)[phase_counter];

				// determine schedule, owner and average speed
				if ( current_linkage.line.is_bound() && current_linkage.line->get_schedule() && current_linkage.line->count_convoys() )
				{
					// Case : a line
					current_schedule = current_linkage.line->get_schedule();
					current_owner = current_linkage.line->get_besitzer();
					current_average_speed = (uint32) ( current_linkage.line->get_finance_history(1, LINE_AVERAGE_SPEED) > 0 ? 
													   current_linkage.line->get_finance_history(1, LINE_AVERAGE_SPEED) : 
													   ( speed_to_kmh(current_linkage.line->get_convoy(0)->get_min_top_speed()) / 2 ) );
				}
				else if ( current_linkage.convoy.is_bound() && current_linkage.convoy->get_schedule() )
				{
					// Case : a lineless convoy
					current_schedule = current_linkage.convoy->get_schedule();
					current_owner = current_linkage.convoy->get_besitzer();
					current_average_speed = (uint32) ( current_linkage.convoy->get_finance_history(1, CONVOI_AVERAGE_SPEED) > 0 ? 
													   current_linkage.convoy->get_finance_history(1, CONVOI_AVERAGE_SPEED) : 
													   ( speed_to_kmh(current_linkage.convoy->get_min_top_speed()) / 2 ) );
				}
				else
				{
					// Case : nothing is bound -> just ignore
					phase_counter++;
					continue;
				}

				// create a list of reachable halts
				entry_count = current_schedule->get_count();
				halt_list.clear();

				for (uint8 i = 0; i < entry_count; i++)
				{
					current_halt = haltestelle_t::get_halt(world, current_schedule->eintrag[i].pos, current_owner);
					
					if( !current_halt.is_bound() )
					{
						// Try a public player halt
						current_halt = haltestelle_t::get_halt(world, current_schedule->eintrag[i].pos, public_player);
					}

					if ( current_halt.is_bound() && current_halt->is_enabled(ware_type) )
					{
						// Assign to halt list only if current halt supports this compartment's goods category
						halt_list.append(current_halt, 64);
					}
				}

				// precalculate journey times between consecutive halts
				entry_count = halt_list.get_count();
				journey_time_factor = journey_time_adjustment / (float)current_average_speed;
				journey_time_list.clear();
				journey_time_list.append(0);	// reserve the first entry for the last journey time from last halt to first halt

				for (uint8 i = 0; i < entry_count; i++)
				{
					// journey time from halt 0 to halt 1 is stored in journey_time_list[1]
					journey_time_list.append
					(
						(uint16)( (float)accurate_distance( halt_list[i]->get_basis_pos(), 
															halt_list[(i+1)%entry_count]->get_basis_pos() ) * journey_time_factor ),
						64 
					);
				}

				journey_time_list[0] = journey_time_list[entry_count];	// copy the last entry to the first entry
				journey_time_list.remove_at(entry_count);	// remove the last entry

				// rebuild connexions for all halts in halt list
				// for each origin halt
				for (uint8 h = 0; h < entry_count; h++)
				{
					accumulated_journey_time = 0;

					// use hash tables in connexion list, but not the hash tables stored in the halt
					catg_connexions = connexion_list[ halt_list[h].get_id() ];

					// for each target halt (origin halt is excluded)
					for (uint8 i = 1,		t = (h + 1) % entry_count; 
						 i < entry_count; 
						 i++,				t = (t + 1) % entry_count) 
					{

						// Case : origin halt is encountered again
						if ( halt_list[t] == halt_list[h] )
						{
							// reset and process the next
							accumulated_journey_time = 0;
							continue;
						}

						// Case : suitable halt
						accumulated_journey_time += journey_time_list[t];

						// Check the journey times to the connexion
						new_connexion = new haltestelle_t::connexion;
						new_connexion->waiting_time = halt_list[h]->get_average_waiting_time(halt_list[t], catg);
						new_connexion->journey_time = accumulated_journey_time;
						new_connexion->best_convoy = current_linkage.convoy;
						new_connexion->best_line = current_linkage.line;

						// Adapted from haltestelle_t::add_connexion()
						// Check whether this is the best connexion so far, and, if so, add it.
						if( !catg_connexions->put(halt_list[t], new_connexion) )
						{
							// The key exists in the hashtable already - check whether this entry is better.
							haltestelle_t::connexion* existing_connexion = catg_connexions->get(halt_list[t]);
							if( existing_connexion->journey_time > new_connexion->journey_time )
							{
								// The new connexion is better - replace it.
								delete existing_connexion;
								catg_connexions->set(halt_list[t], new_connexion);
							}
							else
							{
								delete new_connexion;
							}
						}
						else
						{
							halt_list[h]->prepare_goods_list(catg);
						}
					}
				}

				phase_counter++;

				// iteration control
				iterations++;
				if (iterations == limit_rebuild_connexions)
				{
					break;
				}
			}

			diff = dr_time() - start;	// stop timing

			// iteration statistics collection
			if ( catg == representative_category )
			{
				statistic_duration += ( diff ? diff : 1 );
				statistic_iteration += iterations;
			}

#ifdef DEBUG_COMPARTMENT_STEP
			printf("\t\t\tRebuilding connexions takes :  %lu ms \n", diff);
#endif

			// check if this phase is finished
			if (phase_counter == linkages_count)
			{
				// iteration limit adjustment
				if ( catg == representative_category )
				{
					const uint32 projected_iterations = statistic_iteration * time_midpoint / statistic_duration;
					if ( projected_iterations > 0 )
					{
						if ( limit_rebuild_connexions == maximum_limit )
						{
							const uint32 percentage = projected_iterations * 100 / backup_rebuild_connexions;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								backup_rebuild_connexions = projected_iterations;
							}
						}
						else
						{
							const uint32 percentage = projected_iterations * 100 / limit_rebuild_connexions;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								limit_rebuild_connexions = projected_iterations;
							}
						}
					}
				}

				// reset statistic variables
				statistic_duration = 0;
				statistic_iteration = 0;

				// delete immediately after use
				if (linkages)
				{
					delete linkages;
					linkages = NULL;
				}
				linkages_count = 0;

				current_phase = phase_find_eligible;	// proceed to the next phase
				phase_counter = 0;	// reset counter

			}

			iterations = 0;	// reset iteration counter

#ifndef DEBUG_EXPLORER_SPEED
			return;
#endif
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Phase 3 : Construct eligible halt list which contains halts supporting current goods type. Also, update halt index map
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_find_eligible :
		{
#ifdef DEBUG_COMPARTMENT_STEP
			step_count++;

			printf("\t\tCurrent Step : %lu \n", step_count);
#endif

			// create working halt list only if we are not resuming and there is at least one halt
			if ( phase_counter == 0 && all_halts_count > 0 )
			{
				working_halt_list = new halthandle_t[all_halts_count];
			}

			start = dr_time();	// start timing

			// add halt to working halt list only if it has connexions that support this compartment's goods category
			while (phase_counter < all_halts_count)
			{
				if ( ! connexion_list[ all_halts_list[phase_counter].get_id() ]->empty() )
				{
					// valid connexion(s) found -> add to working halt list and update halt index map
					working_halt_list[working_halt_count] = all_halts_list[phase_counter];
					working_halt_index_map[ all_halts_list[phase_counter].get_id() ] = working_halt_count;
					working_halt_count++;
				}

				// swap the old connexion hash table with a new one, then clear the old one
				all_halts_list[phase_counter]->swap_connexions( catg, connexion_list[ all_halts_list[phase_counter].get_id() ] );
				clear_connexion_table( all_halts_list[phase_counter].get_id() );

				phase_counter++;
				
				// iteration control
				iterations++;
				if (iterations == limit_find_eligible)
				{
					break;
				}
			}

			diff = dr_time() - start;	// stop timing

			// iteration statistics collection
			if ( catg == representative_category )
			{
				statistic_duration += ( diff ? diff : 1 );
				statistic_iteration += iterations;
			}

#ifdef DEBUG_COMPARTMENT_STEP
			printf("\t\t\tConstructing eligible halt list takes :  %lu ms \n", diff);
#endif

			// check if this phase is finished
			if (phase_counter == all_halts_count)
			{
				// iteration limit adjustment
				if ( catg == representative_category )
				{
					const uint32 projected_iterations = statistic_iteration * time_midpoint / statistic_duration;
					if ( projected_iterations > 0 )
					{
						if ( limit_find_eligible == maximum_limit )
						{
							const uint32 percentage = projected_iterations * 100 / backup_find_eligible;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								backup_find_eligible = projected_iterations;
							}
						}
						else
						{
							const uint32 percentage = projected_iterations * 100 / limit_find_eligible;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								limit_find_eligible = projected_iterations;
							}
						}
					}
				}

				// reset statistic variables
				statistic_duration = 0;
				statistic_iteration = 0;

				// update representative category and halt count where necessary
				if ( working_halt_count > representative_halt_count )
				{
					representative_category = catg;
					representative_halt_count = working_halt_count;
				}

				current_phase = phase_fill_matrix;	// proceed to the next phase
				phase_counter = 0;	// reset counter

#ifdef DEBUG_COMPARTMENT_STEP
				printf("\tEligible Halt Count :  %lu \n", working_halt_count);
#endif
			}

			iterations = 0;	// reset iteration counter

#ifndef DEBUG_EXPLORER_SPEED
			return;
#endif
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Phase 4 : Create and fill working path matrix with data. Determine transfer list at the same time.
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_fill_matrix :
		{
#ifdef DEBUG_COMPARTMENT_STEP
			step_count++;

			printf("\t\tCurrent Step : %lu \n", step_count);
#endif

			// build working matrix and transfer list only if we are not resuming
			if (phase_counter == 0)
			{
				if (working_halt_count > 0)
				{
					// build working matrix
					working_matrix = new path_element_t*[working_halt_count];
					for (uint16 i = 0; i < working_halt_count; i++)
					{
						working_matrix[i] = new path_element_t[working_halt_count];
					}

					// build transport matrix
					transport_matrix = new transport_element_t*[working_halt_count];
					for (uint16 i = 0; i < working_halt_count; i++)
					{
						transport_matrix[i] = new transport_element_t[working_halt_count];
					}

					// build transfer list
					transfer_list = new uint16[working_halt_count];
				}
			}

			// temporary variables
			halthandle_t current_halt;
			halthandle_t reachable_halt;
			uint32 reachable_halt_index;
			haltestelle_t::connexion *current_connexion;

			start = dr_time();	// start timing

			while (phase_counter < working_halt_count)
			{
				current_halt = working_halt_list[phase_counter];

				// temporary variables for determining transfer halt
				bool is_transfer_halt = false;
				linehandle_t previous_line;
				convoihandle_t previous_convoy;

				quickstone_hashtable_iterator_tpl<haltestelle_t, haltestelle_t::connexion*> origin_iter( *(current_halt->get_connexions(catg)) );

				// iterate over the connexions of the current halt
				while (origin_iter.next())
				{
					reachable_halt = origin_iter.get_current_key();

					if (!reachable_halt.is_bound())
					{
						continue;
					}

					current_connexion = origin_iter.get_current_value();

					// determine the matrix index of reachable halt in working halt index map
					reachable_halt_index = working_halt_index_map[reachable_halt.get_id()];

					// update corresponding matrix element
					working_matrix[phase_counter][reachable_halt_index].next_transfer = reachable_halt;
					working_matrix[phase_counter][reachable_halt_index].aggregate_time = current_connexion->waiting_time + current_connexion->journey_time;
					transport_matrix[phase_counter][reachable_halt_index].first_line 
						= transport_matrix[phase_counter][reachable_halt_index].last_line 
						= current_connexion->best_line;
					transport_matrix[phase_counter][reachable_halt_index].first_convoy 
						= transport_matrix[phase_counter][reachable_halt_index].last_convoy 
						= current_connexion->best_convoy;					

					// Debug journey times
					// printf("\n%s -> %s : %lu \n",current_halt->get_name(), reachable_halt->get_name(), working_matrix[phase_counter][reachable_halt_index].journey_time);

					// check if it is already determined to be a transfer halt
					if (!is_transfer_halt)
					{
						if ( !previous_line.is_bound() && !previous_convoy.is_bound() )
						{
							// Case : first processed connexion of the current halt -> initialize line info
							previous_line = current_connexion->best_line;
							previous_convoy = current_connexion->best_convoy;
						}
						else if (previous_line != current_connexion->best_line || previous_convoy != current_connexion->best_convoy)
						{
							// Case : a different line is encountered -> current halt is a transfer halt
							is_transfer_halt = true;	// set flag to avoid checking for transfer again for the current halt
							transfer_list[transfer_count] = phase_counter;
							transfer_count++;
						}
					}
				}

				// Special case
				working_matrix[phase_counter][phase_counter].aggregate_time = 0;

				phase_counter++;
				
				// iteration control
				iterations++;
				if (iterations == limit_fill_matrix)
				{
					break;
				}
			}

			diff = dr_time() - start;	// stop timing

			// iteration statistics collection
			if ( catg == representative_category )
			{
				statistic_duration += ( diff ? diff : 1 );
				statistic_iteration += iterations;
			}

#ifdef DEBUG_COMPARTMENT_STEP
			printf("\t\t\tCreating and filling path matrix takes :  %lu ms \n", diff);
#endif

			if (phase_counter == working_halt_count)
			{
				// iteration limit adjustment
				if ( catg == representative_category )
				{				
					const uint32 projected_iterations = statistic_iteration * time_midpoint / statistic_duration;
					if ( projected_iterations > 0 )
					{
						if ( limit_fill_matrix == maximum_limit )
						{
							const uint32 percentage = projected_iterations * 100 / backup_fill_matrix;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								backup_fill_matrix = projected_iterations;
							}
						}
						else
						{
							const uint32 percentage = projected_iterations * 100 / limit_fill_matrix;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								limit_fill_matrix = projected_iterations;
							}
						}
					}
				}

				// reset statistic variables
				statistic_duration = 0;
				statistic_iteration = 0;

				// delete immediately after use
				if (working_halt_list)
				{
					delete[] working_halt_list;
					working_halt_list = NULL;
				}

				current_phase = phase_explore_paths;	// proceed to the next phase
				phase_counter = 0;	// reset counter

#ifdef DEBUG_COMPARTMENT_STEP
				printf("\tTransfer Count :  %lu \n", transfer_count);
#endif
			}

			iterations = 0;	// reset iteration counter

#ifndef DEBUG_EXPLORER_SPEED
			return;
#endif
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Phase 5 : Path exploration using the matrix.
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_explore_paths :
		{
#ifdef DEBUG_COMPARTMENT_STEP
			step_count++;

			printf("\t\tCurrent Step : %lu \n", step_count);
#endif

			// temporary variables
			uint32 combined_time;
			uint16 via;
			uint16 target;

			// initialize only when not resuming
			if (via_index == 0 && origin == 0)
			{
				if (limit_explore_paths == maximum_limit)
				{
					limit_explore_origins = maximum_limit;
				}
				else
				{
					limit_explore_origins = limit_explore_paths * time_midpoint / ( working_halt_count ? working_halt_count : 1 );
				}
			}

			start = dr_time();	// start timing

			// for each transfer halt
			while ( via_index < transfer_count )
			{
				via = transfer_list[via_index];

				// for each origin
				while ( origin < working_halt_count )
				{
					// for each destination
					for ( target = 0; target < working_halt_count; target++ )
					{
						// The first 2 comparisons may seem redundant, but they cut down search time by 10%
						if ( working_matrix[origin][via].aggregate_time < working_matrix[origin][target].aggregate_time &&
							 working_matrix[via][target].aggregate_time < working_matrix[origin][target].aggregate_time &&
							 ( combined_time = working_matrix[origin][via].aggregate_time + working_matrix[via][target].aggregate_time ) 
									< working_matrix[origin][target].aggregate_time &&
							 ( transport_matrix[origin][via].last_line != transport_matrix[via][target].first_line
							   || transport_matrix[origin][via].last_convoy != transport_matrix[via][target].first_convoy ) )
						{
							working_matrix[origin][target].aggregate_time = combined_time;
							working_matrix[origin][target].next_transfer = working_matrix[origin][via].next_transfer;
							transport_matrix[origin][target].first_line = transport_matrix[origin][via].first_line;
							transport_matrix[origin][target].first_convoy = transport_matrix[origin][via].first_convoy;
							transport_matrix[origin][target].last_line = transport_matrix[via][target].last_line;
							transport_matrix[origin][target].last_convoy = transport_matrix[via][target].last_convoy;
						}
					}

					origin++;

					// iteration control
					iterations++;
					if (iterations == limit_explore_origins)
					{
						goto loop_termination;
					}

				}

				origin = 0;
				via_index++;
			}

		loop_termination :

			diff = dr_time() - start;	// stop timing

			// iterations statistics collection
			if ( catg == representative_category )
			{
				// the variables have different meaning here
				statistic_duration++;	// step count
				statistic_iteration += iterations * working_halt_count / ( diff ? diff : 1 );	// sum of iterations per ms
			}

#ifdef DEBUG_COMPARTMENT_STEP
			printf("\t\t\tPath searching -> %lu iterations takes :  %lu ms \t [ %lu ] \n", iterations, diff, limit_explore_paths);
#endif

			if (via_index == transfer_count)
			{
				// iteration limit adjustment
				if ( catg == representative_category )
				{
					const uint32 average = statistic_iteration / statistic_duration;
					if ( average > 0 )
					{
						if ( limit_explore_paths == maximum_limit )
						{
							const uint32 percentage = average * 100 / backup_explore_paths;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								backup_explore_paths = average;
							}
						}
						else
						{
							const uint32 percentage = average * 100 / limit_explore_paths;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								limit_explore_paths = average;
							}
						}
					}
				}

				// reset statistic variables
				statistic_duration = 0;
				statistic_iteration = 0;


				// path search completed -> delete old path info
				if (finished_matrix)
				{
					for (uint16 i = 0; i < finished_halt_count; i++)
					{
						delete[] finished_matrix[i];
					}
					delete[] finished_matrix;
					finished_matrix = NULL;	
				}											
				if (finished_halt_index_map)
				{
					delete[] finished_halt_index_map;
					finished_halt_index_map = NULL;
				}

				// transfer working to finished
				finished_matrix = working_matrix;
				working_matrix = NULL;
				finished_halt_index_map = working_halt_index_map;
				working_halt_index_map = NULL;
				finished_halt_count = working_halt_count;
				// working_halt_count is reset below after deleting transport matrix								

				// path search completed -> delete auxilliary data structures
				if (transport_matrix)
				{
					for (uint16 i = 0; i < working_halt_count; i++)
					{
						delete[] transport_matrix[i];
					}
					delete[] transport_matrix;
					transport_matrix = NULL;
				}
				working_halt_count = 0;
				if (transfer_list)
				{
					delete[] transfer_list;
					transfer_list = NULL;
				}
				transfer_count = 0;

				// Debug paths : to execute, working_halt_list should not be deleted in the previous phase
				// enumerate_all_paths(finished_matrix, working_halt_list, finished_halt_index_map, finished_halt_count);

				current_phase = phase_reroute_goods;	// proceed to the next phase
				limit_explore_origins = 0;	// reset step limit
				// reset counters
				via_index = 0;
				origin = 0;

				paths_available = true;
			}
			
			iterations = 0;	// reset iteration counter

			return;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Phase 6 : Re-route existing goods in the halts
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_reroute_goods :
		{
#ifdef DEBUG_COMPARTMENT_STEP
			step_count++;

			printf("\t\tCurrent Step : %lu \n", step_count);
#endif

			start = dr_time();	// start timing

			while (phase_counter < all_halts_count)
			{
				if ( all_halts_list[phase_counter]->reroute_goods(catg) )
				{
					// only halts with relevant goods packets are counted
					iterations++;
				}

				phase_counter++;

				// iteration control
				if (iterations == limit_reroute_goods)
				{
					break;
				}
			}

			diff = dr_time() - start;	// stop timing

			// iteration statistics collection
			if ( catg == representative_category )
			{
				statistic_duration += ( diff ? diff : 1 );
				statistic_iteration += iterations;
			}

#ifdef DEBUG_COMPARTMENT_STEP
			printf("\t\t\tRe-routing goods takes :  %lu ms \n", diff);
#endif

			if (phase_counter == all_halts_count)
			{
				// iteration limit adjustment
				if ( catg == representative_category )
				{
					const uint32 projected_iterations = statistic_iteration * time_midpoint / statistic_duration;
					if ( projected_iterations > 0 )
					{
						if ( limit_reroute_goods == maximum_limit )
						{
							const uint32 percentage = projected_iterations * 100 / backup_reroute_goods;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								backup_reroute_goods = projected_iterations;
							}
						}
						else
						{
							const uint32 percentage = projected_iterations * 100 / limit_reroute_goods;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								limit_reroute_goods = projected_iterations;
							}
						}
					}
				}

				// reset statistic variables
				statistic_duration = 0;
				statistic_iteration = 0;

				current_phase = phase_gate_sentinel;	// reset to the 1st phase
				phase_counter = 0;	// reset counter

				// delete immediately after use
				if (all_halts_list)
				{
					delete[] all_halts_list;
					all_halts_list = NULL;
				}
				all_halts_count = 0;

#ifdef DEBUG_COMPARTMENT_STEP
				printf("\tFinished in : %lu Steps \n\n", step_count);
#endif

				step_count = 0;
				refresh_completed = true;
			}

			iterations = 0;	// reset iteration counter

			return;
		}

		default :
		{
			// Should not reach here
			dbg->error("compartment_t::step()", "Invalid step index : %u \n", current_phase);
			return;
		}
	}

}


void path_explorer_t::compartment_t::enumerate_all_paths(const path_element_t *const *const matrix, const halthandle_t *const halt_list, 
														 const uint16 *const halt_map, const uint16 halt_count)
{
	// Debugging code : Enumerate all paths for validation
	halthandle_t transfer_halt;

	for (uint16 x = 0; x < halt_count; x++)
	{
		for (uint16 y = 0; y < halt_count; y++)
		{
			if (x != y)
			{
				// print origin
				printf("\n\nOrigin :  %s\n", halt_list[x]->get_name());
				
				transfer_halt = matrix[x][y].next_transfer;

				if (matrix[x][y].aggregate_time == 65535)
				{
					printf("\t\t\t\t******** No Route ********\n");
				}
				else
				{
					while (transfer_halt != halt_list[y])
					{
						printf("\t\t\t\t%s\n", transfer_halt->get_name());

						if ( halt_map[transfer_halt.get_id()] != 65535 )
						{
							transfer_halt = matrix[ halt_map[transfer_halt.get_id()] ][y].next_transfer;
						}
						else
						{
							printf("\t\t\t\tError!!!");
							break;
						}					
					}
				}

				// print destination
				printf("Target :  %s\n\n", halt_list[y]->get_name());								
			}
		}
	}
}


bool path_explorer_t::compartment_t::get_path_between(const halthandle_t origin_halt, const halthandle_t target_halt, 
													  uint16 &aggregate_time, halthandle_t &next_transfer)
{
	static const halthandle_t dummy_halt;
	uint32 origin_index, target_index;
	
	// check if origin and target halts are both present in matrix; if yes, check the validity of the next transfer
	if ( paths_available && origin_halt.is_bound() && target_halt.is_bound()
			&& ( origin_index = finished_halt_index_map[ origin_halt.get_id() ] ) != 65535
			&& ( target_index = finished_halt_index_map[ target_halt.get_id() ] ) != 65535
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


void path_explorer_t::compartment_t::initialise_connexion_list()
{
	for (uint32 i = 0; i < 63336; i++)
	{
		connexion_list[i] = NULL;
	}
}


void path_explorer_t::compartment_t::clear_connexion_table(const uint16 halt_id)
{
	if ( connexion_list[halt_id] && !connexion_list[halt_id]->empty() )
	{
		quickstone_hashtable_iterator_tpl<haltestelle_t, haltestelle_t::connexion*> iter(*(connexion_list[halt_id]));

		while(iter.next())
		{
			delete iter.get_current_value();
		}
		
		connexion_list[halt_id]->clear();
	}
}


void path_explorer_t::compartment_t::clear_all_connexion_tables()
{
	for (uint32 i = 0; i < 63356; i++)
	{
		if ( connexion_list[i] )
		{
			clear_connexion_table(i);
		}
	}
}


void path_explorer_t::compartment_t::destroy_all_connexion_tables()
{
	for (uint32 i = 0; i < 63356; i++)
	{
		if ( connexion_list[i] )
		{
			clear_connexion_table(i);
			delete connexion_list[i];
			connexion_list[i] = NULL;
		}
	}
}

