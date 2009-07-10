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
#include "simhalt.h"


// #define DEBUG_EXPLORER_SPEED
// #define DEBUG_COMPARTMENT_STEP


///////////////////////////////////////////////

// path_explorer_t

uint8 path_explorer_t::max_categories = 0;
uint8 path_explorer_t::category_empty = 255;
path_explorer_t::compartment_t *path_explorer_t::goods_compartment = NULL;
uint8 path_explorer_t::current_compartment = 0;
bool path_explorer_t::processing = false;


void path_explorer_t::initialize()
{
	max_categories = warenbauer_t::get_max_catg_index();
	category_empty = warenbauer_t::nichts->get_catg_index();
	goods_compartment = new compartment_t[max_categories];

	for (uint8 i = 0; i < max_categories; i++)
	{
		goods_compartment[i].set_category(i);
	}

	current_compartment = 0;
	processing = false;
}


void path_explorer_t::destroy()
{
	delete[] goods_compartment;
	goods_compartment = NULL;
	current_compartment = 0;
	category_empty = 255;
	max_categories = 0;
	processing = false;
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
	int total_steps = (max_categories - 1) * 5;
	int curr_step = 0;

	processing = true;

	// initialize progress bar
	display_set_progress_text(translator::translate("Calculating paths ..."));
	display_progress(curr_step, total_steps);

	compartment_t::backup_limits();

	// change all limits to maximum so that each phase is performed in one step
	compartment_t::set_maximum_limits();

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
			// go through all 5 phases
			for (uint8 p = 0; p < 5; p++)
			{
				// perform step
				goods_compartment[c].step();
				curr_step++;
				display_progress(curr_step, total_steps);
			}
#else
			// one step should perform the first 4 compartment phases
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
	for (uint8 c = 0; c < max_categories; c++)
	{
		if (reset_working_set)
		{
			// do not remove the finished matrix and heap
			goods_compartment[c].reset(false);
		}
		else
		{
			// only set flag
			goods_compartment[c].set_refresh();
		}
	}

	if (reset_working_set)
	{
		// reset current category pointer : refresh will start from passengers
		current_compartment = 0;
	}
}

///////////////////////////////////////////////

// compartment_t

uint32 path_explorer_t::compartment_t::limit_find_eligible = default_find_eligible;
uint32 path_explorer_t::compartment_t::limit_fill_matrix = default_fill_matrix;
uint32 path_explorer_t::compartment_t::limit_explore_paths = default_explore_paths;
uint32 path_explorer_t::compartment_t::limit_reroute_goods = default_reroute_goods;

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
		// Phase 1 : Prepare a list of all halts and a halt index map
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_init_prepare :  
		{
			step_count++;

#ifdef DEBUG_COMPARTMENT_STEP
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
			uint16 actual_halt_count = 0;

			// Save the halt list in an array first to prevent the list from being modified across steps, causing bugs
			while (halt_iter.next())
			{
				if (halt_iter.get_current().is_bound())
				{
					all_halts_list[actual_halt_count] = halt_iter.get_current();
					actual_halt_count++;
				}
			}
			all_halts_count = actual_halt_count;

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


#ifdef DEBUG_COMPARTMENT_STEP
			diff = dr_time() - start;	// stop timing

			printf("\tTotal Halt Count :  %lu \n", all_halts_count);
			printf("\t\t\tConstructing full halt list takes :  %lu ms \n", diff);
#endif

			current_phase = phase_find_eligible;	// proceed to the next phase

#ifndef DEBUG_EXPLORER_SPEED
			return;
#endif
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Phase 2 : Construct eligible halt list which contains halts supporting current goods type. Also, update halt index map
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_find_eligible :
		{
			step_count++;

#ifdef DEBUG_COMPARTMENT_STEP
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
				// connexions are always rebuilt to ensure that we are working on the most up-to-date data
				all_halts_list[phase_counter]->rebuild_connexions(catg);

				if (!all_halts_list[phase_counter]->get_connexions(catg)->empty())
				{
					// valid connexion(s) found
					working_halt_list[working_halt_count] = all_halts_list[phase_counter];
					working_halt_index_map[ all_halts_list[phase_counter].get_id() ] = working_halt_count;
					working_halt_count++;
				}

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
			printf("\t\t\tConstructing sorted halt list takes :  %lu ms \n", diff);
#endif

			// check if this phase is finished
			if (phase_counter == all_halts_count)
			{
				if ( working_halt_count > representative_halt_count )
				{
					representative_category = catg;
					representative_halt_count = working_halt_count;
				}

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
		// Phase 3 : Create and fill working path matrix with data. Determine transfer list at the same time.
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_fill_matrix :
		{
			step_count++;

#ifdef DEBUG_COMPARTMENT_STEP
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

					// determine the position of reachable halt in working heap
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

				// delete immediately after use
				if (working_halt_list)
				{
					delete[] working_halt_list;
					working_halt_list = NULL;
				}

				// reset statistic variables
				statistic_duration = 0;
				statistic_iteration = 0;

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
		// Phase 4 : Path exploration using the matrix.
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_explore_paths :
		{
			step_count++;

#ifdef DEBUG_COMPARTMENT_STEP
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
		// Phase 5 : Re-route existing goods in the halts
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_reroute_goods :
		{
			step_count++;

#ifdef DEBUG_COMPARTMENT_STEP
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

