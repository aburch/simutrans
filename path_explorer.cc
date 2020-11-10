/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "path_explorer.h"

#include "tpl/slist_tpl.h"
#include "dataobj/translator.h"
#include "bauer/goods_manager.h"
#include "descriptor/goods_desc.h"
#include "sys/simsys.h"
#include "display/simgraph.h"
#include "player/simplay.h"
#include "dataobj/environment.h"
#include "dataobj/schedule.h"
#include "simconvoi.h"
#include "simloadingscreen.h"

typedef quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*> connexions_map_single_remote;


// #define DEBUG_EXPLORER_SPEED
// #define DEBUG_COMPARTMENT_STEP


///////////////////////////////////////////////

// path_explorer_t

karte_t *path_explorer_t::world = NULL;
uint8 path_explorer_t::max_categories = 0;
uint8 path_explorer_t::max_classes = 0;
uint8 path_explorer_t::category_empty = 255;
path_explorer_t::compartment_t **path_explorer_t::goods_compartment = NULL;
uint8 path_explorer_t::current_compartment_category = 0;
uint8 path_explorer_t::current_compartment_class = 0;
bool path_explorer_t::processing = false;
uint32 path_explorer_t::compartment_t::time_midpoint;
uint32 path_explorer_t::compartment_t::time_lower_limit;
uint32 path_explorer_t::compartment_t::time_upper_limit;
uint32 path_explorer_t::compartment_t::time_threshold;
bool path_explorer_t::must_refresh_on_loading;

#ifdef MULTI_THREAD
bool thread_local path_explorer_t::allow_path_explorer_on_this_thread = false;
#endif

void path_explorer_t::initialise(karte_t *welt)
{
	if (welt)
	{
		world = welt;
	}
	compartment_t::set_absolute_limits();
	max_categories = goods_manager_t::get_max_catg_index();
	max_classes = max(goods_manager_t::passengers->get_number_of_classes(), goods_manager_t::mail->get_number_of_classes());
	category_empty = goods_manager_t::none->get_catg_index();
	// See here for an explanation of the below: http://stackoverflow.com/questions/29375797/copy-2d-array-using-memcpy/29375830#29375830
	// This is not a true 2d array as the number of categories is not fixed.
	goods_compartment = new compartment_t*[max_categories];
	goods_compartment[0] = new compartment_t[max_categories * max_classes];
	for (uint8 i = 1; i < max_categories; i++)
	{
		goods_compartment[i] = goods_compartment[i - 1] + max_classes;
	}

	for (uint8 i = 0; i < max_categories; ++i)
	{
		for (uint8 j = 0; j < max_classes; j++)
		{
			goods_compartment[i][j].set_category(i);
			goods_compartment[i][j].set_class(j);
		}
	}

	current_compartment_category = 0;
	current_compartment_class = 0;
	processing = false;

	compartment_t::initialise();
}


void path_explorer_t::finalise()
{
	// See here for an explanation of the below: http://stackoverflow.com/questions/29375797/copy-2d-array-using-memcpy/29375830#29375830
	// This is not a true 2d array as the number of categories is not fixed.
	delete[] goods_compartment[0];
	delete[] goods_compartment;
	goods_compartment = NULL;
	current_compartment_category = 0;
	current_compartment_class = 0;
	category_empty = 255;
	max_categories = 0;
	max_classes = 0;
	processing = false;

	compartment_t::finalise();
}

void path_explorer_t::rdwr(loadsave_t* file)
{
	if (file->get_extended_version() < 14 || (file->get_extended_version() == 14 && file->get_extended_revision() < 10))
	{
		// Iterate through the compartments and load/save these
		for (uint8 ca = 0; ca < max_categories; ++ca)
		{
			for (uint8 cl = 0; cl < max_classes; ++cl)
			{
				if (ca != category_empty)
				{
					goods_compartment[ca][cl].rdwr(file);
				}
			}
		}
	}
	else
	{
		uint8 file_max_categories = max_categories;
		file->rdwr_byte(file_max_categories);

		uint8 file_passenger_classes = goods_manager_t::passengers->get_number_of_classes();
		file->rdwr_byte(file_passenger_classes);
		for (uint8 cl = 0; cl < file_passenger_classes; cl++)
		{
			if (cl < max_classes)
			{
				goods_compartment[goods_manager_t::INDEX_PAS][cl].rdwr(file);
			}
			else
			{
				compartment_t* dummy_goods_compartment = new compartment_t;
				dummy_goods_compartment->rdwr(file);
				delete dummy_goods_compartment;
			}
		}

		uint8 file_mail_classes = goods_manager_t::mail->get_number_of_classes();
		file->rdwr_byte(file_mail_classes);
		for (uint8 cl = 0; cl < file_mail_classes; cl++)
		{
			if (cl < max_classes)
			{
				goods_compartment[goods_manager_t::INDEX_MAIL][cl].rdwr(file);
			}
			else
			{
				compartment_t* dummy_goods_compartment = new compartment_t;
				dummy_goods_compartment->rdwr(file);
				delete dummy_goods_compartment;
			}
		}

		for (uint8 ca = 0; ca < file_max_categories; ca++)
		{
			if (ca != category_empty && ca != goods_manager_t::INDEX_PAS && ca != goods_manager_t::INDEX_MAIL)
			{
				if (ca < max_categories)
				{
					goods_compartment[ca][0].rdwr(file);
				}
				else
				{
					compartment_t* dummy_goods_compartment = new compartment_t;
					dummy_goods_compartment->rdwr(file);
					delete dummy_goods_compartment;
				}
			}
		}

		file->rdwr_byte(current_compartment_category);
		file->rdwr_byte(current_compartment_class);
	}

	// Load/save the connexion_list, which is static
	uint32 connexion_list_size = 65536;
	if (file->get_extended_version() < 14 || (file->get_extended_version() == 14 && file->get_extended_revision() < 11))
	{
		// Wrong number was used in original code
		connexion_list_size = 63336;
	}

	for (uint32 i = 0; i < connexion_list_size; ++i)
	{
		file->rdwr_byte(compartment_t::connexion_list[i].serving_transport);

		if (file->is_saving())
		{
			uint16 tmp_idx;

			uint32 tmp_journey_time;
			uint32 tmp_waiting_time;
			uint32 tmp_transfer_time;
			uint16 tmp_best_line_idx;
			uint16 tmp_best_convoy_idx;
			uint16 tmp_alternative_seats;
			// TODO: Consider whether to add comfort

			uint32 connexion_table_count = compartment_t::connexion_list[i].connexion_table->get_count();
			file->rdwr_long(connexion_table_count);

			FOR(compartment_t::connexion_table_map, iter, *compartment_t::connexion_list[i].connexion_table)
			{
				tmp_idx = iter.key.get_id();
				file->rdwr_short(tmp_idx);

				tmp_journey_time = iter.value->journey_time;
				tmp_waiting_time = iter.value->waiting_time;
				tmp_transfer_time = iter.value->transfer_time;
				tmp_best_line_idx = iter.value->best_line.get_id();
				tmp_best_convoy_idx = iter.value->best_convoy.get_id();
				tmp_alternative_seats = iter.value->alternative_seats;

				file->rdwr_long(tmp_journey_time);
				file->rdwr_long(tmp_waiting_time);
				file->rdwr_long(tmp_transfer_time);
				file->rdwr_short(tmp_best_line_idx);
				file->rdwr_short(tmp_best_convoy_idx);
				file->rdwr_short(tmp_alternative_seats);
			}
		}

		if (file->is_loading())
		{
			uint32 connexion_table_count;
			file->rdwr_long(connexion_table_count);

			halthandle_t tmp_halt;

			for (uint32 j = 0; j < connexion_table_count; j++)
			{
				uint16 tmp_idx;
				file->rdwr_short(tmp_idx);
				tmp_halt.set_id(tmp_idx);

				haltestelle_t::connexion* tmp_cnx = new haltestelle_t::connexion();

				uint32 tmp_journey_time;
				uint32 tmp_waiting_time;
				uint32 tmp_transfer_time;
				uint16 tmp_best_line_idx;
				uint16 tmp_best_convoy_idx;
				uint16 tmp_alternative_seats;
				// TODO: Consider whether to add comfort

				file->rdwr_long(tmp_journey_time);
				file->rdwr_long(tmp_waiting_time);
				file->rdwr_long(tmp_transfer_time);
				file->rdwr_short(tmp_best_line_idx);
				file->rdwr_short(tmp_best_convoy_idx);
				file->rdwr_short(tmp_alternative_seats);

				tmp_cnx->journey_time = tmp_journey_time;
				tmp_cnx->waiting_time = tmp_waiting_time;
				tmp_cnx->transfer_time = tmp_transfer_time;
				tmp_cnx->best_line.set_id(tmp_best_line_idx);
				tmp_cnx->best_convoy.set_id(tmp_best_convoy_idx);
				tmp_cnx->alternative_seats = tmp_alternative_seats;

				compartment_t::connexion_list[i].connexion_table->put(tmp_halt, tmp_cnx);
			}
		}
	}
}

void path_explorer_t::step()
{
#ifdef MULTI_THREAD
	if (!allow_path_explorer_on_this_thread)
	{
		return;
	}
#endif
	// at most check all goods categories once
	const uint8 max_runs = (max_categories - 2) + goods_manager_t::passengers->get_number_of_classes() + goods_manager_t::mail->get_number_of_classes();
	for (uint8 i = 0; i < max_runs; ++i)
	{
		if ( current_compartment_category != category_empty
			 && (!goods_compartment[current_compartment_category][current_compartment_class].is_refresh_completed()
			     || goods_compartment[current_compartment_category][current_compartment_class].is_refresh_requested() ) )
		{
			processing = true;	// this step performs something
			// perform step
			goods_compartment[current_compartment_category][current_compartment_class].step();

			// if refresh is completed, move on to the next category or class as appropriate
			if ( goods_compartment[current_compartment_category][current_compartment_class].is_refresh_completed() )
			{
				next_compartment();
			}
			// each step process at most 1 goods category
			return;
		}

		// advance to the next category or class only if compartment.step() is not invoked
		next_compartment();
	}

	processing = false;	// this step performs nothing
}

void path_explorer_t::next_compartment()
{
	if (current_compartment_class < goods_manager_t::get_classes_catg_index(current_compartment_category) - 1)
	{
		current_compartment_class += 1;
	}
	else
	{
		current_compartment_class = 0;
		current_compartment_category = (current_compartment_category + 1) % max_categories;
	}
}


void path_explorer_t::full_instant_refresh()
{
#ifdef MULTI_THREAD
	path_explorer_t::allow_path_explorer_on_this_thread = true;
#endif
	uint16 curr_step = 0;
	// exclude empty goods (none)
	uint16 total_steps = (max_categories - 1) * max_classes * 6;

	processing = true;

	// disable the iteration limits
	compartment_t::enable_limits(false);

	// clear all connexion hash tables and reset serving transport counters
	compartment_t::reset_connexion_list();

#ifdef DEBUG_EXPLORER_SPEED
	unsigned long start, diff;
	start = dr_time();
#endif

	// initialize progress bar
	loadingscreen_t ls(translator::translate("Calculating paths ..."), total_steps, true, true);
	ls.set_progress(curr_step);

	for (uint8 ca = 0; ca < max_categories; ++ca)
	{
		for(uint8 cl = 0; cl < goods_manager_t::get_classes_catg_index(ca); ++cl)
		{
			if (ca != category_empty)
			{
				// clear any previous leftovers
				goods_compartment[ca][cl].reset(true);

#ifndef DEBUG_EXPLORER_SPEED
				// go through all 6 phases
				for (uint8 p = 0; p < 6; ++p)
				{
					// perform step
					goods_compartment[ca][cl].step();
					++curr_step;
					ls.set_progress(curr_step);
				}
#else
				// one step should perform the compartment phases from the first phase till the path exploration phase
				goods_compartment[c].step();
				curr_step += 6;
				ls.set_progress(curr_step);
#endif
			}
		}
	}

#ifdef DEBUG_EXPLORER_SPEED
	diff = dr_time() - start;
	printf("\n\nTotal time taken :  %lu ms \n", diff);
#endif

	// enable iteration limits again
	compartment_t::enable_limits(true);

	// reset current category pointer
	current_compartment_category = 0;
	current_compartment_class = 0;

	processing = false;
#ifdef MULTI_THREAD
	path_explorer_t::allow_path_explorer_on_this_thread = false;
#endif
}


void path_explorer_t::refresh_all_categories(const bool reset_working_set)
{
	if (reset_working_set)
	{
		for (uint8 ca = 0; ca < max_categories; ++ca)
		{
			for (uint8 cl = 0; cl < max_classes; ++cl)
			{
				// do not remove the finished matrix and halt index map
				goods_compartment[ca][cl].reset(false);
			}
		}

		// clear all connexion hash tables and reset serving transport counters
		compartment_t::reset_connexion_list();

		// reset current category pointer : refresh will start from passengers
		current_compartment_category = 0;
		current_compartment_class = 0;
	}
	else
	{
		for (uint8 ca = 0; ca < max_categories; ++ca)
		{
			for (uint8 cl = 0; cl < max_classes; ++cl)
			{
				// only set flag
				goods_compartment[ca][cl].set_refresh();
			}
		}
	}
}

void path_explorer_t::refresh_category(uint8 category)
{
	uint8 number_of_classes = goods_manager_t::get_classes_catg_index(category);
	for (uint8 i = 0; i < number_of_classes; i++)
	{
		goods_compartment[category][i].set_refresh();
	}
}

void path_explorer_t::refresh_class_category(uint8 category, uint8 g_class)
{
	goods_compartment[category][g_class].set_refresh();
}

///////////////////////////////////////////////

// compartment_t

const char *const path_explorer_t::compartment_t::phase_name[] =
{
	"flag",
	"prepare",
	"rebuild",
	"filter",
	"matrix",
	"explore",
	"reroute"
};

path_explorer_t::compartment_t::connexion_list_entry_t path_explorer_t::compartment_t::connexion_list[65536];

bool path_explorer_t::compartment_t::use_limits = true;

uint32 path_explorer_t::compartment_t::limit_rebuild_connexions = default_rebuild_connexions;
uint32 path_explorer_t::compartment_t::limit_filter_eligible = default_filter_eligible;
uint32 path_explorer_t::compartment_t::limit_fill_matrix = default_fill_matrix;
uint64 path_explorer_t::compartment_t::limit_explore_paths = default_explore_paths;
uint32 path_explorer_t::compartment_t::limit_reroute_goods = default_reroute_goods;

uint32 path_explorer_t::compartment_t::local_rebuild_connexions = default_rebuild_connexions;
uint32 path_explorer_t::compartment_t::local_filter_eligible = default_filter_eligible;
uint32 path_explorer_t::compartment_t::local_fill_matrix = default_fill_matrix;
uint64 path_explorer_t::compartment_t::local_explore_paths = default_explore_paths;
uint32 path_explorer_t::compartment_t::local_reroute_goods = default_reroute_goods;

bool path_explorer_t::compartment_t::local_limits_changed = false;

uint16 path_explorer_t::compartment_t::representative_halt_count = 0;
uint8 path_explorer_t::compartment_t::representative_category = 0;

path_explorer_t::compartment_t::compartment_t()
{
	refresh_start_time = 0;

	finished_matrix = NULL;
	finished_halt_index_map = NULL;
	finished_halt_count = 0;

	working_matrix = NULL;
	transport_index_map = NULL;
	transport_matrix = NULL;
	working_halt_index_map = NULL;
	working_halt_list = NULL;
	working_halt_count = 0;

	all_halts_list = NULL;
	all_halts_count = 0;

	linkages = NULL;

	transfer_list = NULL;
	transfer_count = 0;

	catg = 255;
	g_class = 255;
	catg_name = NULL;
	class_name = NULL;
	step_count = 0;

	paths_available = false;
	refresh_completed = true;
	refresh_requested = true;

	current_phase = phase_check_flag;

	phase_counter = 0;
	iterations = 0;
	total_iterations = 0;

	via_index = 0;
	origin_cluster_index = 0;
	target_cluster_index = 0;
	origin_member_index = 0;

	inbound_connections = NULL;
	outbound_connections = NULL;
	process_next_transfer = true;

	statistic_duration = 0;
	statistic_iteration = 0;
}


path_explorer_t::compartment_t::~compartment_t()
{
	if (finished_matrix)
	{
		for (uint16 i = 0; i < finished_halt_count; ++i)
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
		for (uint16 i = 0; i < working_halt_count; ++i)
		{
			delete[] working_matrix[i];
		}
		delete[] working_matrix;
	}
	if (transport_index_map)
	{
		delete[] transport_index_map;
	}
	if (transport_matrix)
	{
		for (uint16 i = 0; i < working_halt_count; ++i)
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

	if (inbound_connections)
	{
		delete inbound_connections;
	}

	if (outbound_connections)
	{
		delete outbound_connections;
	}

	if (class_name)
	{
		delete [] class_name;
	}
}


void path_explorer_t::compartment_t::reset(const bool reset_finished_set)
{
	refresh_start_time = 0;

	if (reset_finished_set)
	{
		if (finished_matrix)
		{
			for (uint16 i = 0; i < finished_halt_count; ++i)
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
		for (uint16 i = 0; i < working_halt_count; ++i)
		{
			delete[] working_matrix[i];
		}
		delete[] working_matrix;
		working_matrix = NULL;
	}
	if (transport_index_map)
	{
		delete[] transport_index_map;
		transport_index_map = NULL;
	}
	if (transport_matrix)
	{
		for (uint16 i = 0; i < working_halt_count; ++i)
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


	if (transfer_list)
	{
		delete[] transfer_list;
		transfer_list = NULL;
	}
	transfer_count = 0;

	if (inbound_connections)
	{
		delete inbound_connections;
		inbound_connections = NULL;
	}
	if (outbound_connections)
	{
		delete outbound_connections;
		outbound_connections = NULL;
	}
	process_next_transfer = true;

#ifdef DEBUG_COMPARTMENT_STEP
	step_count = 0;
#endif

	if (reset_finished_set)
	{
		paths_available = false;
	}
	refresh_completed = true;
	refresh_requested = true;

	current_phase = phase_check_flag;

	phase_counter = 0;
	iterations = 0;
	total_iterations = 0;

	via_index = 0;
	origin_cluster_index = 0;
	target_cluster_index = 0;
	origin_member_index = 0;

	statistic_duration = 0;
	statistic_iteration = 0;
}


void path_explorer_t::compartment_t::initialise()
{
	initialise_connexion_list();
}


void path_explorer_t::compartment_t::finalise()
{
	finalise_connexion_list();
}

void path_explorer_t::compartment_t::set_absolute_limits()
{
	time_midpoint = get_world()->get_settings().get_path_explorer_time_midpoint();
	time_lower_limit = time_midpoint - time_deviation;
	time_upper_limit = time_midpoint + time_deviation;
	time_threshold = time_midpoint / 2;
}

void path_explorer_t::compartment_t::step()
{
#ifdef MULTI_THREAD
	if (!allow_path_explorer_on_this_thread)
	{
		return;
	}
#endif

#ifdef DEBUG_COMPARTMENT_STEP
	printf("\n\nCategory :  %s; Class : %s \n", translator::translate(catg_name), translator::translate(class_name));
#endif

	// For timing use
	uint32 start, diff;

	switch (current_phase)
	{
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Phase 0 : Determine if a new refresh should be done, and prepare relevant flags accordingly
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_check_flag :
		{
			if (refresh_requested)
			{
				refresh_requested = false;	// immediately reset it so that we can take new requests
				refresh_completed = false;	// indicate that processing is at work
				//refresh_start_time = dr_time();
				refresh_start_time = world->get_ticks(); // Possibly more network safe than the original (commented out above)
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
			++step_count;

			printf("\t\tCurrent Step : %lu \n", step_count);

			start = dr_time();	// start timing
#endif
			vector_tpl<halthandle_t>::const_iterator halt_iter = haltestelle_t::get_alle_haltestellen().begin();
			all_halts_count = (uint16) haltestelle_t::get_alle_haltestellen().get_count();

			// create all halts list
			if (all_halts_count > 0)
			{
				all_halts_list = new halthandle_t[all_halts_count];
			}

			const bool no_walking_connexions = !world->get_settings().get_allow_routing_on_foot() || catg != goods_manager_t::passengers->get_catg_index();

			// Save the halt list in an array first to prevent the list from being modified across steps, causing bugs
			for (uint16 i = 0; i < all_halts_count; ++i)
			{
				all_halts_list[i] = *halt_iter;
				++halt_iter;

				// Connect halts within walking distance of each other (for passengers only)
				// @author: jamespetts, July 2011

				if ( no_walking_connexions || !all_halts_list[i]->is_enabled(goods_manager_t::passengers) )
				{
					continue;
				}

				const uint32 halts_within_walking_distance = all_halts_list[i]->get_number_of_halts_within_walking_distance();

				halthandle_t walking_distance_halt;
				haltestelle_t::connexion *new_connexion;

				for ( uint32 x = 0; x < halts_within_walking_distance; ++x )
				{
					walking_distance_halt = all_halts_list[i]->get_halt_within_walking_distance(x);

					if(!walking_distance_halt.is_bound() || !walking_distance_halt->is_enabled(goods_manager_t::passengers))
					{
						continue;
					}

					const uint32 walking_journey_distance = shortest_distance(
						all_halts_list[i]->get_next_pos(walking_distance_halt->get_basis_pos()),
						walking_distance_halt->get_next_pos(all_halts_list[i]->get_basis_pos())
						);

					const uint32 journey_time = world->walking_time_tenths_from_distance(walking_journey_distance);

					// Check the journey times to the connexion
					new_connexion = new haltestelle_t::connexion;
					new_connexion->waiting_time = 0; // People do not need to wait to walk.
					new_connexion->transfer_time = walking_distance_halt->get_transfer_time();
					new_connexion->best_convoy = convoihandle_t();
					new_connexion->best_line = linehandle_t();
					new_connexion->journey_time = journey_time;
					new_connexion->alternative_seats = 0;

					// These are walking connexions only. There will not be multiple possible connexions, so no need
					// to check for existing connexions here.
					connexion_list[ all_halts_list[i].get_id() ].connexion_table->put(walking_distance_halt, new_connexion);
					connexion_list[ all_halts_list[i].get_id() ].serving_transport = 1u;	// will become an interchange if served by additional transport(s)
					all_halts_list[i]->prepare_goods_list(catg);
				}
			}

			// create and initlialize a halthandle-entry to matrix-index map (halt index map)
			working_halt_index_map = new uint16[65536];
			for (uint32 i = 0; i < 65536; ++i)
			{
				// For quickstone handle, there can at most be 65535 valid entries, plus entry 0 which is reserved for null handle
				// Thus, the range of quickstone entries [1, 65535] is mapped to the range of matrix index [0, 65534]
				// Matrix index 65535 either means null handle or the halt has no connexion of the relevant goods category
				// This is always created regardless
				working_halt_index_map[i] = 65535;
			}

			transport_index_map = new uint16[131072]();		// initialise all elements to zero

			// create a list of schedules of lines and lineless convoys
			linkages = new vector_tpl<linkage_t>(1024);
			convoihandle_t current_convoy;
			linehandle_t current_line;
			linkage_t temp_linkage;


			// loop through all convoys
			for (vector_tpl<convoihandle_t>::const_iterator i = world->convoys().begin(), end = world->convoys().end(); i != end; i++)
			{
				current_convoy = *i;
				// only consider lineless convoys which support this compartment's goods catetory which are not in the depot
				if (!current_convoy->in_depot() && !current_convoy->get_line().is_bound() && (catg >= 2 || current_convoy->carries_this_or_lower_class(catg, g_class)) && current_convoy->get_goods_catg_index().is_contained(catg) )
				{
					temp_linkage.convoy = current_convoy;
					linkages->append(temp_linkage);
					transport_index_map[ 65536u + current_convoy.get_id() ] = linkages->get_count();
				}
			}

			temp_linkage.convoy = convoihandle_t();	// reset the convoy handle component

			// loop through all lines of all players
			for (int i = 0; i < MAX_PLAYER_COUNT; ++i)
			{
				player_t *current_player = world->get_player(i);

				if(  current_player == NULL  )
				{
					continue;
				}

				for (vector_tpl<linehandle_t>::const_iterator j = current_player->simlinemgmt.get_all_lines().begin(),
					 end = current_player->simlinemgmt.get_all_lines().end(); j != end; j++)
				{
					current_line = *j;
					// only consider lines which support this compartment's goods category and, where applicable, class
					if ( current_line->get_goods_catg_index().is_contained(catg) && (catg >= 2 || current_line->carries_this_or_lower_class(catg, g_class)) && current_line->count_convoys() > 0)
					{
						temp_linkage.line = current_line;
						linkages->append(temp_linkage);
						transport_index_map[ current_line.get_id() ] = linkages->get_count();
					}
				}
			}

			// can have at most 65535 different lines and lineless convoys; passing this limit should be extremely unlikely
			assert( linkages->get_count() <= 65535u );

#ifdef DEBUG_COMPARTMENT_STEP
			diff = dr_time() - start;	// stop timing

			printf("\tTotal Halt Count :  %lu \n", all_halts_count);
			printf("\tTotal Lines/Lineless Convoys Count :  %ul \n", linkages->get_count());
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
			++step_count;

			printf("\t\tCurrent Step : %lu \n", step_count);
#endif
			const goods_desc_t *const ware_type = goods_manager_t::get_info_catg_index(catg);

			linkage_t current_linkage;
			schedule_t *current_schedule;
			player_t *current_owner;
			uint32 current_average_speed;

			uint8 entry_count;
			halthandle_t current_halt;

			minivec_tpl<halthandle_t> halt_list(64);
			minivec_tpl<uint32> journey_time_list(64);
			minivec_tpl<bool> recurrence_list(64);		// an array indicating whether certain halts have been processed already

			uint32 accumulated_journey_time;
			quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*> *catg_connexions;
			haltestelle_t::connexion *new_connexion;

			start = dr_time();	// start timing. Note that this was originally dr_time()

			// for each schedule of line / lineless convoy
			while (phase_counter < linkages->get_count())
			{
				current_linkage = (*linkages)[phase_counter];

				// determine schedule, owner and average speed
				if ( current_linkage.line.is_bound() && current_linkage.line->get_schedule() && current_linkage.line->count_convoys() )
				{
					// Case : a line
					current_schedule = current_linkage.line->get_schedule();
					current_owner = current_linkage.line->get_owner();
					current_average_speed = (uint32) ( current_linkage.line->get_finance_history(1, LINE_AVERAGE_SPEED) > 0 ?
													   current_linkage.line->get_finance_history(1, LINE_AVERAGE_SPEED) :
													   ( speed_to_kmh(current_linkage.line->get_convoy(0)->get_min_top_speed()) >> 1 ) );
				}
				else if ( current_linkage.convoy.is_bound() && current_linkage.convoy->get_schedule() )
				{
					// Case : a lineless convoy
					current_schedule = current_linkage.convoy->get_schedule();
					current_owner = current_linkage.convoy->get_owner();
					current_average_speed = (uint32) ( current_linkage.convoy->get_finance_history(1, convoi_t::CONVOI_AVERAGE_SPEED) > 0 ?
													   current_linkage.convoy->get_finance_history(1, convoi_t::CONVOI_AVERAGE_SPEED) :
													   ( speed_to_kmh(current_linkage.convoy->get_min_top_speed()) >> 1 ) );
				}
				else
				{
					// Case : nothing is bound -> just ignore
					++phase_counter;
					continue;
				}

				// create a list of reachable halts
				bool reverse = false;
				entry_count = current_schedule->is_mirrored() ? (current_schedule->get_count() * 2) - 2 : current_schedule->get_count();
				halt_list.clear();
				recurrence_list.clear();

				uint8 index = 0;

				while (entry_count-- && index < current_schedule->get_count())
				{
					current_halt = haltestelle_t::get_halt(current_schedule->entries[index].pos, current_owner);

					// Make sure that the halt found was built before refresh started and that it supports current goods category
					if ( current_halt.is_bound() && current_halt->get_inauguration_time() < refresh_start_time && current_halt->is_enabled(ware_type) )
					{
						// Assign to halt list only if current halt supports this compartment's goods category
						halt_list.append(current_halt, 64);
						// Initialise the corresponding recurrence list entry to false
						recurrence_list.append(false, 64);
					}

					current_schedule->increment_index(&index, &reverse);
				}

				// precalculate journey times between consecutive halts
				// This is now only a fallback in case the point to point journey time data are not available.
				entry_count = halt_list.get_count();
				uint32 journey_time = 0;
				journey_time_list.clear();
				journey_time_list.append(0);	// reserve the first entry for the last journey time from last halt to first halt


				for (uint8 i = 0; i < entry_count; ++i)
				{
					journey_time = 0;
					const id_pair pair(halt_list[i].get_id(), halt_list[(i+1)%entry_count].get_id());

					if ( current_linkage.line.is_bound() && current_linkage.line->get_average_journey_times().is_contained(pair) )
					{
						if(!halt_list[i].is_bound() || ! halt_list[(i+1)%entry_count].is_bound())
						{
							current_linkage.line->get_average_journey_times().remove(pair);
							continue;
						}
						else
						{
							journey_time = current_linkage.line->get_average_journey_times().access(pair)->reduce();
						}
					}
					else if ( current_linkage.convoy.is_bound() && current_linkage.convoy->get_average_journey_times().is_contained(pair) )
					{
						if(!halt_list[i].is_bound() || ! halt_list[(i+1)%entry_count].is_bound())
						{
							current_linkage.convoy->get_average_journey_times().remove(pair);
							continue;
						}
						else
						{
							journey_time = current_linkage.convoy->get_average_journey_times().access(pair)->reduce();
						}
					}

					if(journey_time == 0)
					{
						// Zero here means that there are no journey time data even if the hashtable entry exists.
						// Fallback to convoy's general average speed if a point-to-point average is not available.
						const uint32 distance = shortest_distance(halt_list[i]->get_basis_pos(), halt_list[(i+1)%entry_count]->get_basis_pos());
						journey_time = world->travel_time_tenths_from_distance(distance, current_average_speed);
					}

					// journey time from halt 0 to halt 1 is stored in journey_time_list[1]
					journey_time_list.append(journey_time, 64);

				}

				journey_time_list[0] = journey_time_list[entry_count];	// copy the last entry to the first entry
				journey_time_list.remove_at(entry_count);	// remove the last entry


				// rebuild connexions for all halts in halt list
				// for each origin halt
				for (uint8 h = 0; h < entry_count; ++h)
				{
					if ( recurrence_list[h] )
					{
						// skip this halt if it has already been processed
						continue;
					}

					accumulated_journey_time = 0;

					// use hash tables in connexion list, but not hash tables stored in the halt
					catg_connexions = connexion_list[ halt_list[h].get_id() ].connexion_table;
					// any serving line/lineless convoy increments serving transport count
					++connexion_list[ halt_list[h].get_id() ].serving_transport;

					// for each target halt (origin halt is excluded)
					for (uint8 i = 1,		t = (h + 1) % entry_count;
						 i < entry_count;
						 ++i,				t = (t + 1) % entry_count)
					{

						// Case : origin halt is encountered again
						if ( halt_list[t] == halt_list[h] )
						{
							// reset and process the next
							accumulated_journey_time = 0;
							// mark this halt in the recurrence list to avoid duplicated processing
							recurrence_list[t] = true;
							continue;
						}

						// Case : suitable halt
						accumulated_journey_time += journey_time_list[t];

						// Check the journey times to the connexion
						id_pair halt_pair(halt_list[h].get_id(), halt_list[t].get_id());
						new_connexion = new haltestelle_t::connexion;
						new_connexion->waiting_time = halt_list[h]->get_average_waiting_time(halt_list[t], catg, g_class);
						new_connexion->transfer_time = catg != goods_manager_t::passengers->get_catg_index() ? halt_list[h]->get_transshipment_time() : halt_list[h]->get_transfer_time();
						if(current_linkage.line.is_bound())
						{
							average_tpl<uint32>* ave = current_linkage.line->get_average_journey_times().access(halt_pair);
							if(ave && ave->count > 0)
							{
								new_connexion->journey_time = ave->reduce();
							}
							else
							{
								// Fallback - use the old method. This will be an estimate, and a somewhat generous one at that.
								new_connexion->journey_time = accumulated_journey_time;
							}
						}
						else if(current_linkage.convoy.is_bound())
						{
							average_tpl<uint32>* ave = current_linkage.convoy->get_average_journey_times().access(halt_pair);
							if(ave && ave->count > 0)
							{
								new_connexion->journey_time = ave->reduce();
							}
							else
							{
								// Fallback - use the old method. This will be an estimate, and a somewhat generous one at that.
								new_connexion->journey_time = accumulated_journey_time;
							}
						}
						new_connexion->best_convoy = current_linkage.convoy;
						new_connexion->best_line = current_linkage.line;
						new_connexion->alternative_seats = 0;

						// Check whether this is the best connexion so far, and, if so, add it.
						if( !catg_connexions->put(halt_list[t], new_connexion) )
						{
							// The key exists in the hashtable already - check whether this entry is better.
							haltestelle_t::connexion* existing_connexion = catg_connexions->get(halt_list[t]);
							if( existing_connexion->journey_time > new_connexion->journey_time )
							{
								// The new connexion is better - replace it.
								new_connexion->alternative_seats = existing_connexion->alternative_seats;
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

				++phase_counter;

				// iteration control
				++total_iterations;
				if ( use_limits && iterations == limit_rebuild_connexions)
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
			if (phase_counter == linkages->get_count())
			{
				// iteration limit adjustment
				if ( catg == representative_category )
				{
					const uint32 projected_iterations = statistic_iteration * time_midpoint / statistic_duration;
					if ( projected_iterations > 0 )
					{
						if ( env_t::networkmode )
						{
							const uint32 percentage = projected_iterations * 100 / local_rebuild_connexions;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								local_rebuild_connexions = projected_iterations;
								local_limits_changed = true;
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

				current_phase = phase_filter_eligible;	// proceed to the next phase
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
		case phase_filter_eligible :
		{
#ifdef DEBUG_COMPARTMENT_STEP
			++step_count;

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
				const halthandle_t &current_halt = all_halts_list[phase_counter];

				// halts may be removed during the process of refresh
				if ( ! current_halt.is_bound() )
				{
					reset_connexion_entry( current_halt.get_id() );
					++phase_counter;
					continue;
				}

				if (!connexion_list[ current_halt.get_id() ].connexion_table->empty())
				{
					// valid connexion(s) found -> add to working halt list and update halt index map
					working_halt_list[working_halt_count] = current_halt;
					working_halt_index_map[ current_halt.get_id() ] = working_halt_count;
					++working_halt_count;
				}

				// swap the old connexion hash table with a new one
				current_halt->swap_connexions(catg, g_class, connexion_list[current_halt.get_id()].connexion_table);

				// transfer the value of the serving transport counter
				current_halt->set_schedule_count( catg, g_class, max_classes, connexion_list[ current_halt.get_id() ].serving_transport );

				// Delete the contents of the old connexion_table; we then reuse the empty table for the next catg/class
				reset_connexion_entry( current_halt.get_id() );

				++phase_counter;

				// iteration control
				++iterations;
				++total_iterations;
				if ( use_limits && iterations == limit_filter_eligible )
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
						if ( env_t::networkmode )
						{
							const uint32 percentage = projected_iterations * 100 / local_filter_eligible;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								local_filter_eligible = projected_iterations;
								local_limits_changed = true;
							}
						}
						else
						{
							const uint32 percentage = projected_iterations * 100 / limit_filter_eligible;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								limit_filter_eligible = projected_iterations;
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
			++step_count;

			printf("\t\tCurrent Step : %lu \n", step_count);
#endif
			// build working matrix and transfer list only if we are not resuming
			if (phase_counter == 0)
			{
				if (working_halt_count > 0)
				{
					// build working matrix
					working_matrix = new path_element_t*[working_halt_count];
					for (uint16 i = 0; i < working_halt_count; ++i)
					{
						working_matrix[i] = new path_element_t[working_halt_count];
					}

					// build transport matrix
					transport_matrix = new transport_element_t*[working_halt_count];
					for (uint16 i = 0; i < working_halt_count; ++i)
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

				// halts may be removed during the process of refresh
				if ( ! current_halt.is_bound() )
				{
					++phase_counter;
					continue;
				}

				// determine if this halt is a transfer halt
				if ( current_halt->get_schedule_count(catg, g_class, max_classes) > 1 )
				{
					transfer_list[transfer_count] = phase_counter;
					++transfer_count;
				}

				// iterate over the connexions of the current halt
				FOR(connexions_map_single_remote, const& connexions_iter, *(current_halt->get_connexions(catg, g_class)))
				{
					reachable_halt = connexions_iter.key;

					// halts may be removed during the process of refresh
					if (!reachable_halt.is_bound())
					{
						continue;
					}

					current_connexion = connexions_iter.value;

					// validate transport and determine transport index
					uint16 transport_idx;
					if ( current_connexion->best_line.is_null() && current_connexion->best_convoy.is_null() )
					{
						// passengers walking between 2 halts
						transport_idx = 0;
					}
					else if ( current_connexion->best_line.is_bound() )
					{
						// valid line
						transport_idx = transport_index_map[ current_connexion->best_line.get_id() ];
					}
					else if ( current_connexion->best_convoy.is_bound() )
					{
						// valid lineless convoy
						transport_idx = transport_index_map[ 65536u + current_connexion->best_convoy.get_id() ];
					}
					else
					{
						// neither walking nor having valid transport -> skip this connection
						continue;
					}

					// determine the matrix index of reachable halt in working halt index map
					reachable_halt_index = working_halt_index_map[reachable_halt.get_id()];

					if(reachable_halt_index == 65535)
					{
						continue;
					}

					// update corresponding matrix element
					working_matrix[phase_counter][reachable_halt_index].next_transfer = reachable_halt;
					working_matrix[phase_counter][reachable_halt_index].aggregate_time = current_connexion->waiting_time + current_connexion->journey_time + current_connexion->transfer_time;
					transport_matrix[phase_counter][reachable_halt_index].first_transport
						= transport_matrix[phase_counter][reachable_halt_index].last_transport
						= transport_idx;

					// Debug journey times
					// printf("\n%s -> %s : %lu \n",current_halt->get_name(), reachable_halt->get_name(), working_matrix[phase_counter][reachable_halt_index].journey_time);
				}

				// Special case
				working_matrix[phase_counter][phase_counter].aggregate_time = 0;

				++phase_counter;

				// iteration control
				++iterations;
				++total_iterations;
				if ( use_limits && iterations == limit_fill_matrix )
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
						if ( env_t::networkmode )
						{
							const uint32 percentage = projected_iterations * 100 / local_fill_matrix;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								local_fill_matrix = projected_iterations;
								local_limits_changed = true;
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
				if (transport_index_map)
				{
					delete[] transport_index_map;
					transport_index_map = NULL;
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
			++step_count;

			printf("\t\tCurrent Step : %lu \n", step_count);
#endif
			// temporary variables
			uint32 combined_time;
			uint32 target_member_index;
			uint64 iterations_processed = 0;

			// initialize only when not resuming
			if ( via_index == 0 && origin_cluster_index == 0 && target_cluster_index == 0 && origin_member_index == 0 )
			{
				// build data structures for inbound/outbound connections to/from transfer halts
				inbound_connections = new connection_t(64u, working_halt_count);
				outbound_connections = new connection_t(64u, working_halt_count);
			}

			start = dr_time();	// start timing

			// for each transfer
			while ( via_index < transfer_count )
			{
				const uint16 via = transfer_list[via_index];

				if ( process_next_transfer )
				{
					// prevent reconstruction of connected halt list while resuming in subsequent steps
					process_next_transfer = false;

					// identify halts which are connected with the current transfer halt
					for ( uint16 idx = 0; idx < working_halt_count; ++idx )
					{
						if ( working_matrix[via][idx].aggregate_time != UINT32_MAX_VALUE && via != idx )
						{
							inbound_connections->register_connection( transport_matrix[idx][via].last_transport, idx );
							outbound_connections->register_connection( transport_matrix[via][idx].first_transport, idx );
						}
					}

					// should take into account the iterations above
					iterations_processed += (uint32)working_halt_count + ( inbound_connections->get_total_member_count() << 1 );
					total_iterations += (uint32)working_halt_count + ( inbound_connections->get_total_member_count() << 1 );
				}

				// for each origin cluster
				while ( origin_cluster_index < inbound_connections->get_cluster_count() )
				{
					const connection_t::connection_cluster_t &origin_cluster = (*inbound_connections)[origin_cluster_index];
					const uint16 inbound_transport = origin_cluster.transport;
					const vector_tpl<uint16> &origin_halt_list = origin_cluster.connected_halts;

					// for each target cluster
					while ( target_cluster_index < outbound_connections->get_cluster_count() )
					{
						const connection_t::connection_cluster_t &target_cluster = (*outbound_connections)[target_cluster_index];
						const uint16 outbound_transport = target_cluster.transport;
						if ( inbound_transport == outbound_transport && inbound_transport != 0u )
						{
							++target_cluster_index;
							continue;
						}
						const vector_tpl<uint16> &target_halt_list = target_cluster.connected_halts;

						// for each origin cluster member
						while ( origin_member_index < origin_halt_list.get_count() )
						{
							const uint16 origin = origin_halt_list[origin_member_index];

							// for each target cluster member
							for ( target_member_index = 0; target_member_index < target_halt_list.get_count(); ++target_member_index )
							{
								const uint16 target = target_halt_list[target_member_index];

								if ( ( combined_time = working_matrix[origin][via].aggregate_time
													 + working_matrix[via][target].aggregate_time )
											< working_matrix[origin][target].aggregate_time			   )
								{
									working_matrix[origin][target].aggregate_time = combined_time;
									working_matrix[origin][target].next_transfer = working_matrix[origin][via].next_transfer;
									transport_matrix[origin][target].first_transport = transport_matrix[origin][via].first_transport;
									transport_matrix[origin][target].last_transport = transport_matrix[via][target].last_transport;
								}
							}	// loop : target cluster member

							++origin_member_index;

							// iteration control
							iterations_processed += target_halt_list.get_count();
							total_iterations += target_halt_list.get_count();
							if ( use_limits && iterations_processed >= limit_explore_paths )
							{
								goto loop_termination;
							}

						}	// loop : origin cluster member

						origin_member_index = 0;

						++target_cluster_index;

					}	// loop : target cluster

					target_cluster_index = 0;

					++origin_cluster_index;

				}	// loop : origin cluster

				origin_cluster_index = 0;

				// clear the inbound/outbound connections
				inbound_connections->reset();
				outbound_connections->reset();
				process_next_transfer = true;

				++via_index;
			}	// loop : transfer

		loop_termination :

			diff = dr_time() - start;	// stop timing

			// iterations statistics collection
			if ( catg == representative_category )
			{
				// the variables have different meaning here
				++statistic_duration;	// step count
				statistic_iteration += static_cast<uint32>( iterations_processed / ( diff ? diff : 1 ) );	// sum of iterations per ms
			}

#ifdef DEBUG_COMPARTMENT_STEP
			printf("\t\t\tPath searching -> %lu iterations takes :  %lu ms \n", static_cast<unsigned long>(iterations_processed), diff);
#endif

			if (via_index == transfer_count)
			{
				// iteration limit adjustment
				if ( catg == representative_category )
				{
					const uint64 projected_iterations = static_cast<uint64>( statistic_iteration / statistic_duration ) * static_cast<uint64>( time_midpoint );
					if ( projected_iterations > 0 )
					{
						if ( env_t::networkmode )
						{
							const uint32 percentage = static_cast<uint32>( projected_iterations * 100 / local_explore_paths );
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								local_explore_paths = projected_iterations;
								local_limits_changed = true;
							}
						}
						else
						{
							const uint32 percentage = static_cast<uint32>( projected_iterations * 100 / limit_explore_paths );
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								limit_explore_paths = projected_iterations;
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
					for (uint16 i = 0; i < finished_halt_count; ++i)
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
					for (uint16 i = 0; i < working_halt_count; ++i)
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

				if (inbound_connections)
				{
					delete inbound_connections;
					inbound_connections = NULL;
				}
				if (outbound_connections)
				{
					delete outbound_connections;
					outbound_connections = NULL;
				}
				process_next_transfer = true;

				// Debug paths : to execute, working_halt_list should not be deleted in the previous phase
				// enumerate_all_paths(finished_matrix, working_halt_list, finished_halt_index_map, finished_halt_count);

				current_phase = phase_reroute_goods;	// proceed to the next phase

				// reset counters
				via_index = 0;
				origin_cluster_index = 0;
				target_cluster_index = 0;
				origin_member_index = 0;

				paths_available = true;
			}

			iterations = 0;	// reset iteration counter // desync debug

			return;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Phase 6 : Re-route existing goods in the halts
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case phase_reroute_goods :
		{
#ifdef DEBUG_COMPARTMENT_STEP
			++step_count;

			printf("\t\tCurrent Step : %lu \n", step_count);
#endif
			start = dr_time();	// start timing

			while (phase_counter < all_halts_count)
			{
				// halts may be removed during the process of refresh
				if ( ! all_halts_list[phase_counter].is_bound() )
				{
					++phase_counter;
					continue;
				}

				all_halts_list[phase_counter]->set_reroute_goods_next_step(catg);
				++iterations;
				++total_iterations;

				// The below is not compatible with multi-threading.
				/*if ( all_halts_list[phase_counter]->reroute_goods(catg) )
				{
					// only halts with relevant goods packets are counted
					++iterations;
					++total_iterations;
				}*/

				++phase_counter;

				// iteration control
				if ( use_limits && iterations == limit_reroute_goods )
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
						if ( env_t::networkmode )
						{
							const uint32 percentage = projected_iterations * 100 / local_reroute_goods;
							if ( percentage < percent_lower_limit || percentage > percent_upper_limit )
							{
								local_reroute_goods = projected_iterations;
								local_limits_changed = true;
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

				current_phase = phase_check_flag;	// reset to the 1st phase
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
				step_count = 0;
#endif

				refresh_start_time = 0;
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

	for (uint16 x = 0; x < halt_count; ++x)
	{
		for (uint16 y = 0; y < halt_count; ++y)
		{
			if (x != y)
			{
				// print origin
				printf("\n\nOrigin :  %s\n", halt_list[x]->get_name());

				transfer_halt = matrix[x][y].next_transfer;

				if (matrix[x][y].aggregate_time == UINT32_MAX_VALUE)
				{
					printf("\t\t\t\t******** No Route ********\n");
				}
				else
				{
					while (transfer_halt != halt_list[y])
					{
						printf("\t\t\t\t%s\n", transfer_halt->get_name());

						if ( halt_map[transfer_halt.get_id()] != 65535)
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
													  uint32 &aggregate_time, halthandle_t &next_transfer)
{
	uint16 origin_index, target_index;

	// check if origin and target halts are both present in matrix; if yes, check the validity of the next transfer
	if ( paths_available /*&& origin_halt.is_bound() && target_halt.is_bound()*/
			&& ( origin_index = finished_halt_index_map[ origin_halt.get_id() ] ) != 65535
			&& ( target_index = finished_halt_index_map[ target_halt.get_id() ] ) != 65535
			&& finished_matrix[origin_index][target_index].next_transfer.is_bound() )
	{
		aggregate_time = finished_matrix[origin_index][target_index].aggregate_time;
		next_transfer = finished_matrix[origin_index][target_index].next_transfer;
		return true;
	}

	// requested path not found
	aggregate_time = UINT32_MAX_VALUE;
	next_transfer = halthandle_t();
	return false;
}


void path_explorer_t::compartment_t::set_category(uint8 category)
{
	catg = category;
	const goods_desc_t *ware_type = goods_manager_t::get_info_catg_index(catg);
	catg_name = ware_type->get_catg_name();
}

void path_explorer_t::compartment_t::set_class(uint8 value)
{
	g_class = value;
	delete[] class_name;
	class_name = new char[96];

	if (catg == goods_manager_t::INDEX_PAS)
	{
		sprintf(class_name, "%s_%u", translator::translate("p_class"), g_class);
	}
	else if (catg == goods_manager_t::INDEX_MAIL)
	{
		sprintf(class_name, "%s_%u", translator::translate("m_class"), g_class);
	}
	else
	{
		class_name[0] = '\0';
	}
}


void path_explorer_t::compartment_t::initialise_connexion_list()
{
	for (uint32 i = 0; i < 65536; ++i)
	{
		// Note that the connexion_tables created here will be
		// swapped with connexion_tables created/stored in halts.
		connexion_list[i].connexion_table = new quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*>();
		connexion_list[i].serving_transport = 0;
	}
}


void path_explorer_t::compartment_t::reset_connexion_entry(const uint16 halt_id)
{
	if ( !connexion_list[halt_id].connexion_table->empty() )
	{
		FOR(haltestelle_t::connexions_map, const& iter, (*(connexion_list[halt_id].connexion_table)))
		{
			delete iter.value;
		}

		connexion_list[halt_id].connexion_table->clear();
	}
	connexion_list[halt_id].serving_transport = 0;
}


void path_explorer_t::compartment_t::reset_connexion_list()
{
	for (uint32 i = 0; i < 65536; ++i)
	{
		if ( connexion_list[i].connexion_table )
		{
			reset_connexion_entry(i);
		}
	}
}


void path_explorer_t::compartment_t::finalise_connexion_list()
{
	for (uint32 i = 0; i < 65536; ++i)
	{
		reset_connexion_entry(i);
		delete connexion_list[i].connexion_table;
		connexion_list[i].connexion_table = NULL;
	}
}

void path_explorer_t::compartment_t::rdwr(loadsave_t* file)
{
	file->rdwr_longlong(refresh_start_time);

	file->rdwr_short(finished_halt_count);

	bool finished_halt_index_map_live = finished_halt_index_map != NULL;
	file->rdwr_bool(finished_halt_index_map_live);

	if (finished_halt_index_map_live)
	{
		if (file->is_loading())
		{
			finished_halt_index_map = new uint16[65536];
		}
		for (uint32 i = 0; i < 65536; ++i)
		{
			file->rdwr_short(finished_halt_index_map[i]);
		}
	}

	bool finished_matrix_live = finished_matrix != NULL;
	file->rdwr_bool(finished_matrix_live);

	if (finished_matrix_live)
	{
		if (file->is_saving())
		{
			uint16 tmp_idx;
			for (uint16 i = 0; i < finished_halt_count; i++)
			{
				//  This is a 2 dimensional array
				for (uint32 j = 0; j < finished_halt_count; j++)
				{
					file->rdwr_long(finished_matrix[i][j].aggregate_time);
					tmp_idx = finished_matrix[i][j].next_transfer.get_id();
					file->rdwr_short(tmp_idx);
				}
			}
		}
		else // Loading
		{
			// Create the matrices
			if (finished_halt_count > 0)
			{
				// Build the (empty) finished matrix
				uint16 tmp_idx;
				finished_matrix = new path_element_t*[finished_halt_count];
				for (uint16 i = 0; i < finished_halt_count; ++i)
				{
					finished_matrix[i] = new path_element_t[finished_halt_count];
				}

				// Now load them. These are 2 dimensional arrays.
				for (uint16 i = 0; i < finished_halt_count; i++)
				{
					for (uint32 j = 0; j < finished_halt_count; j++)
					{
						file->rdwr_long(finished_matrix[i][j].aggregate_time);
						file->rdwr_short(tmp_idx);
						finished_matrix[i][j].next_transfer.set_id(tmp_idx);
					}
				}
			}
		}
	}

	file->rdwr_short(working_halt_count);

	// Working matrix
	bool working_matrix_live = working_matrix != NULL;
	file->rdwr_bool(working_matrix_live);

	if (working_matrix_live)
	{
		if (file->is_saving())
		{
			uint16 tmp_idx;
			for (uint16 i = 0; i < working_halt_count; i++)
			{
				for (uint32 j = 0; j < working_halt_count; j++)
				{
					file->rdwr_long(working_matrix[i][j].aggregate_time);
					tmp_idx = working_matrix[i][j].next_transfer.get_id();
					file->rdwr_short(tmp_idx);

					file->rdwr_short(transport_matrix[i][j].first_transport);
					file->rdwr_short(transport_matrix[i][j].last_transport);
				}
			}
		}

		else // Loading
		{
			// Create the matrices
			if (working_halt_count > 0)
			{
				// build working matrix
				uint16 tmp_idx;
				working_matrix = new path_element_t*[working_halt_count];
				for (uint16 i = 0; i < working_halt_count; ++i)
				{
					working_matrix[i] = new path_element_t[working_halt_count];

				}

				// build transport matrix
				transport_matrix = new transport_element_t*[working_halt_count];
				for (uint16 i = 0; i < working_halt_count; ++i)
				{
					transport_matrix[i] = new transport_element_t[working_halt_count];
				}

				// Now load them. These are 2 dimensional arrays.
				for (uint16 i = 0; i < working_halt_count; i++)
				{
					for (uint32 j = 0; j < working_halt_count; j++)
					{
						file->rdwr_long(working_matrix[i][j].aggregate_time);
						file->rdwr_short(tmp_idx);
						working_matrix[i][j].next_transfer.set_id(tmp_idx);

						file->rdwr_short(transport_matrix[i][j].first_transport);
						file->rdwr_short(transport_matrix[i][j].last_transport);
					}
				}
			}
		}
	}

	bool working_halt_index_map_live = working_halt_index_map != NULL;
	file->rdwr_bool(working_halt_index_map_live);

	if (working_halt_index_map_live)
	{
		if (file->is_loading())
		{
			working_halt_index_map = new uint16[65536];
		}
		for (uint32 i = 0; i < 65536; ++i)
		{
			file->rdwr_short(working_halt_index_map[i]);
		}
	}

	bool transport_index_map_live = transport_index_map != NULL;
	file->rdwr_bool(transport_index_map_live);

	if (transport_index_map_live)
	{
		if (file->is_loading())
		{
			transport_index_map = new uint16[131072]();		// initialise all elements to zero
		}

		for (uint32 i = 0; i < 131072; i++)
		{
			file->rdwr_short(transport_index_map[i]);
		}
	}

	file->rdwr_short(all_halts_count);

	bool all_halts_list_live = all_halts_list != NULL;
	file->rdwr_bool(all_halts_list_live);

	if(all_halts_list_live)
	{
		if(file->is_loading())
		{
			all_halts_list = new halthandle_t[all_halts_count];
		}

		for(uint32 i = 0; i < all_halts_count; i ++)
		{
			if (file->is_saving())
			{
				uint16 id = all_halts_list[i].get_id();
				file->rdwr_short(id);
			}
			else
			{
				uint16 id;
				file->rdwr_short(id);
				all_halts_list[i].set_id(id);
			}
		}
	}

	bool working_halt_list_live = working_halt_list != NULL;
	file->rdwr_bool(working_halt_list_live);

	if(working_halt_list_live)
	{
		if(file->is_loading())
		{
			working_halt_list = new halthandle_t[all_halts_count];
		}

		for(uint32 i = 0; i < all_halts_count; i ++)
		{
			if (file->is_saving())
			{
				uint16 id = working_halt_list[i].get_id();
				file->rdwr_short(id);
			}
			else
			{
				uint16 id;
				file->rdwr_short(id);
				working_halt_list[i].set_id(id);
			}
		}
	}

	bool linkages_live = linkages != NULL;
	file->rdwr_bool(linkages_live);

	if(linkages_live)
	{
		uint32 linkages_count;
		if(file->is_saving())
		{
			linkages_count = linkages->get_count();
		}

		file->rdwr_long(linkages_count);

		if(file->is_loading())
		{
			linkages = new vector_tpl<linkage_t>(linkages_count);
		}

		uint16 cnv_id;
		uint16 line_id;
		for (uint32 i = 0; i < linkages_count; i++)
		{
			if (file->is_saving())
			{
				cnv_id = linkages->get_element(i).convoy.get_id();
				line_id = linkages->get_element(i).line.get_id();
			}

			file->rdwr_short(cnv_id);
			file->rdwr_short(line_id);

			if(file->is_loading())
			{
				linkage_t tmp;
				tmp.convoy.set_id(cnv_id);
				tmp.line.set_id(line_id);
				linkages->append(tmp);
			}
		}
	}

	file->rdwr_short(transfer_count);
	if (file->is_loading())
	{
		// build transfer list
		transfer_list = new uint16[working_halt_count];
	}

	for(uint16 i = 0; i < transfer_count; i ++)
	{
		file->rdwr_short(transfer_list[i]);
	}

	file->rdwr_byte(catg);
	set_category(catg); // Necessary to set the name
	file->rdwr_byte(g_class);
	set_class(g_class); // Necessary to set the name

	file->rdwr_short(step_count);

	file->rdwr_bool(paths_available);
	file->rdwr_bool(refresh_completed);
	file->rdwr_bool(refresh_requested);

	file->rdwr_byte(current_phase);

	file->rdwr_short(phase_counter);
	file->rdwr_long(iterations);
	file->rdwr_long(total_iterations);

	file->rdwr_short(via_index);
	file->rdwr_long(origin_cluster_index);
	file->rdwr_long(target_cluster_index);
	file->rdwr_long(origin_member_index);

	bool inbound_connections_live = inbound_connections != NULL;
	file->rdwr_bool(inbound_connections_live);

	if(inbound_connections_live)
	{
		if(file->is_loading())
		{
			inbound_connections = new connection_t(64u, working_halt_count);
		}
		inbound_connections->rdwr(file);
	}

	bool outbound_connections_live = outbound_connections != NULL;
	file->rdwr_bool(outbound_connections_live);

	if(outbound_connections_live)
	{
		if(file->is_loading())
		{
			outbound_connections = new connection_t(64u, working_halt_count);
		}
		outbound_connections->rdwr(file);
	}

	file->rdwr_bool(process_next_transfer);

	file->rdwr_long(statistic_duration);
	file->rdwr_long(statistic_iteration);
}

void path_explorer_t::compartment_t::connection_t::rdwr(loadsave_t* file)
{
	uint32 connection_cluster_count;
	if(file->is_saving())
	{
		connection_cluster_count = connection_clusters.get_count();
	}

	file->rdwr_long(connection_cluster_count);

	for(uint32 i = 0; i < connection_cluster_count; i ++)
	{
		if(file->is_saving())
		{
			connection_clusters[i]->rdwr(file);
		}
		else
		{
			connection_cluster_t* tmp_cnx = new connection_cluster_t(file);
			connection_clusters.append(tmp_cnx);
		}
	}

	file->rdwr_long(usage_level);
	file->rdwr_long(halt_vector_size);

	if (file->is_saving())
	{
		uint32 cluster_map_count = cluster_map.get_count();
		file->rdwr_long(cluster_map_count);
		FOR(cluster_map_type, iter, cluster_map)
		{
			file->rdwr_short(iter.key);
			iter.value->rdwr(file);

			cluster_map_count++;
		}
	}
	else if (file->is_loading())
	{
		uint32 cluster_map_count;
		file->rdwr_long(cluster_map_count);
		uint16 key;
		for(uint32 i = 0; i < cluster_map_count; i ++)
		{
			file->rdwr_short(key);
			connection_cluster_t* value = new connection_cluster_t(file);
			cluster_map.put(key, value);
		}
	}
}

void path_explorer_t::compartment_t::connection_t::connection_cluster_t::rdwr(loadsave_t* file)
{
	assert(file->is_saving());
	file->rdwr_short(transport);
	uint32 connected_halts_count = connected_halts.get_count();
	file->rdwr_long(connected_halts_count);
	for(uint32 i = 0; i < connected_halts_count; i ++)
	{
		file->rdwr_short(connected_halts[i]);
	}
}

path_explorer_t::compartment_t::connection_t::connection_cluster_t::connection_cluster_t(loadsave_t* file)
{
	assert(file->is_loading());
	file->rdwr_short(transport);
	uint32 connected_halts_count;
	file->rdwr_long(connected_halts_count);
	connected_halts.resize(connected_halts_count);

	for(uint32 i = 0; i < connected_halts_count; i ++)
	{
		uint16 tmp;
		file->rdwr_short(tmp);
		connected_halts.append(tmp);
	}
}
