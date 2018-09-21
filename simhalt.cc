/*
 * Copyright (c) 1997 - 2001 Hansj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

/*
 * Stations for Simutrans
 * 03.2000 moved from simfab.cc
 *
 * Hj. Malthaner
 */
#include <algorithm>

#include "freight_list_sorter.h"

#include "path_explorer.h"
#include "simcity.h"
#include "simcolor.h"
#include "simconvoi.h"
#include "simdebug.h"
#include "simdepot.h"
#include "simfab.h"
#include "simhalt.h"
#include "simintr.h"
#include "simline.h"
#include "simmem.h"
#include "simmesg.h"
#include "simplan.h"
#include "utils/simrandom.h"
#include "player/simplay.h"
#include "player/finance.h"
#include "gui/simwin.h"
#include "simworld.h"
#include "simware.h"
#include "simsys.h"

#include "bauer/hausbauer.h"
#include "bauer/goods_manager.h"

#include "descriptor/goods_desc.h"

#include "boden/boden.h"
#include "boden/grund.h"
#include "boden/wasser.h"
#include "boden/wege/strasse.h"

#include "dataobj/settings.h"
#include "dataobj/schedule.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "dataobj/environment.h"

#include "obj/gebaeude.h"
#include "obj/label.h"
#include "obj/signal.h"

#include "gui/halt_info.h"
#include "gui/halt_detail.h"
#include "gui/karte.h"

#include "utils/simstring.h"

#include "vehicle/simpeople.h"

#include "descriptor/goods_desc.h"

karte_ptr_t haltestelle_t::welt;

vector_tpl<halthandle_t> haltestelle_t::alle_haltestellen;

stringhashtable_tpl<halthandle_t> haltestelle_t::all_names;

vector_tpl<lines_loaded_t> haltestelle_t::lines_loaded;

uint8 haltestelle_t::pedestrian_limit = 0;
// hash table only used during loading
inthashtable_tpl<sint32,halthandle_t> *haltestelle_t::all_koords = NULL;
// since size_x*size_y < 0x1000000, we have just to shift the high bits
#define get_halt_key(k,width) ( ((k).x*(width)+(k).y) /*+ ((k).z << 25)*/ )

//uint8 haltestelle_t::status_step = 0;
uint8 haltestelle_t::reconnect_counter = 0;

static const uint8 pedestrian_generate_max = 16;

// controls the halt iterator in step_all():
static bool restart_halt_iterator = true;

void haltestelle_t::step_all()
{
	const uint32 count = alle_haltestellen.get_count();
	if (count)
	{
		const uint32 loops = min(count, 256u);
		static vector_tpl<halthandle_t>::iterator iter;
		for (uint32 i = 0; i < loops; ++i)
		{
			if (restart_halt_iterator || iter == alle_haltestellen.end())
			{
				restart_halt_iterator = false;
				iter = alle_haltestellen.begin();
			}
			(*iter++)->step();
		}
	}
}


halthandle_t haltestelle_t::get_halt(const koord pos, const player_t *player )
{
	const planquadrat_t *plan = welt->access(pos);
	if(plan)
	{
		for(  uint8 i=0;  i < plan->get_boden_count();  i++  ) {
			halthandle_t my_halt = plan->get_boden_bei(i)->get_halt();
			if(  my_halt.is_bound()  &&  my_halt->check_access(player)  ) {
				// Stop at first halt found (always prefer ground level)
				return my_halt;
			}
		}
		// no halt? => we do the water check
		if(plan->get_kartenboden()->is_water())
		{
			// may catch bus stops close to water ...
			const uint8 cnt = plan->get_haltlist_count();
			// first check for own stop
			for(uint8 i = 0; i < cnt; i++)
			{
				if(plan->get_haltlist()[i].halt->get_owner() == player)
				{
					const uint8 distance_to_dock = plan->get_haltlist()[i].distance;
					if(distance_to_dock <= 1)
					{
						return plan->get_haltlist()[i].halt;
					}
				}
			}
			// then for other stops to which access is allowed
			// (This is second because it is preferable to dock at one's own
			// port to avoid charges)
			for(uint8 i = 0; i < cnt; i++)
			{
				if(plan->get_haltlist()[i].halt->check_access(player))
				{
					const uint8 distance_to_dock = plan->get_haltlist()[i].distance;
					if(distance_to_dock <= 1)
					{
						return plan->get_haltlist()[i].halt;
					}
				}
			}
			// so: nothing found
		}
	}
	return halthandle_t();
}

static vector_tpl<convoihandle_t>stale_convois;
static vector_tpl<linehandle_t>stale_lines;


void haltestelle_t::start_load_game()
{
	all_koords = new inthashtable_tpl<sint32,halthandle_t>;
}


void haltestelle_t::end_load_game()
{
	delete all_koords;
	all_koords = NULL;
}

/**
 * return an index to a halt; it is only used for old games
 * by default create a new halt if none found
 */
halthandle_t haltestelle_t::get_halt_koord_index(koord k)
{
	if(!welt->is_within_limits(k)) {
		return halthandle_t();
	}
	// check in hashtable
	halthandle_t h;
	const sint32 n = get_halt_key(koord3d(k,-128), welt->get_size().y);
	assert(all_koords);
	h = all_koords->get( n );

	if(  !h.is_bound()  ) {
		// No halts found => create one
		h = haltestelle_t::create(k, NULL );
		all_koords->set( n,  h );
	}
	return h;
}


/* we allow only for a single stop per grund
 * this will only return something if this stop is accessible by the player passed in the second parameter
 */
halthandle_t haltestelle_t::get_halt(const koord3d pos, const player_t *player )
{
	const grund_t *gr = welt->lookup(pos);
	if(gr)
	{
		weg_t *w = gr->get_weg_nr(0);

		if(!w)
		{
			w = gr->get_weg_nr(1);
		}

		// Stops on public roads, even those belonging to other players, should be able to be used by all players.
		if(gr->get_halt().is_bound() && (gr->get_halt()->check_access(player) ||
			(w && player_t::check_owner(w->get_owner(), player))) ||
			(w && (w->get_waytype() == road_wt || w->get_waytype() == tram_wt) && (w->get_owner() == NULL || w->get_owner()->is_public_service())))
		{
			return gr->get_halt();
		}
		// no halt? => we do the water check
		if(gr->is_water())
		{
			// may catch bus stops close to water ...
			const planquadrat_t *plan = welt->access(pos.get_2d());
			const uint8 cnt = plan->get_haltlist_count();
			// first check for own stop
			for(uint8 i = 0; i < cnt; i++)
			{

				halthandle_t halt = plan->get_haltlist()[i].halt;
				const uint8 distance_to_dock = plan->get_haltlist()[i].distance;
				if(halt->get_owner() == player && distance_to_dock <= 1 && halt->get_station_type() & dock)
				{
					return halt;
				}
			}
			// then for other stops to which access is allowed
			// (This is second because it is preferable to dock at one's own
			// port to avoid charges)
			for(uint8 i = 0; i < cnt; i++)
			{
				halthandle_t halt = plan->get_haltlist()[i].halt;
				const uint8 distance_to_dock = plan->get_haltlist()[i].distance;
				if(halt->check_access(player) && distance_to_dock <= 1 && halt->get_station_type() & dock)
				{
					return halt;
				}
			}
			// so: nothing found
		}
	}
	return halthandle_t();
}


koord haltestelle_t::get_basis_pos() const
{
	if (tiles.empty()) return koord::invalid;
	assert(tiles.front().grund->get_pos().get_2d() == init_pos);
	return tiles.front().grund->get_pos().get_2d();
}


koord3d haltestelle_t::get_basis_pos3d() const
{
	if (tiles.empty()) return koord3d::invalid;
	return tiles.front().grund->get_pos();
}


/**
 * Station factory method. Returns handles instead of pointers.
 * @author Hj. Malthaner
 */
halthandle_t haltestelle_t::create(koord pos, player_t *player)
{
	haltestelle_t * p = new haltestelle_t(pos, player);
	return p->self;
}


/*
 * removes a ground tile from a station
 * @author prissi
 */
bool haltestelle_t::remove(player_t *player, koord3d pos)
{
	grund_t *bd = welt->lookup(pos);

	// wrong ground?
	if(bd==NULL) {
		dbg->error("haltestelle_t::remove()","illegal ground at %d,%d,%d", pos.x, pos.y, pos.z);
		return false;
	}

	halthandle_t halt = bd->get_halt();
	if(!halt.is_bound()) {
		dbg->error("haltestelle_t::remove()","no halt at %d,%d,%d", pos.x, pos.y, pos.z);
		return false;
	}

DBG_MESSAGE("haltestelle_t::remove()","removing segment from %d,%d,%d", pos.x, pos.y, pos.z);
	// otherwise there will be marked tiles left ...
	halt->mark_unmark_coverage(false);

	// only try to remove connected buildings, when still in list to avoid infinite loops
	if(  halt->rem_grund(bd)  ) {
		// remove station building?
		gebaeude_t* gb = bd->find<gebaeude_t>();
		if(gb)
		{
			DBG_MESSAGE("haltestelle_t::remove()",  "removing building" );
			if(gb->get_tile()->get_desc()->get_is_control_tower())
			{
				halt->remove_control_tower();
				halt->recalc_status();
			}
			hausbauer_t::remove( player, gb, false );
			bd = NULL;	// no need to recalc image
			// removing the building could have destroyed this halt already
			if (!halt.is_bound()){
				return true;
			}
		}
	}

	if(!halt->existiert_in_welt()) {
DBG_DEBUG("haltestelle_t::remove()","remove last");
		// all deleted?
DBG_DEBUG("haltestelle_t::remove()","destroy");
		haltestelle_t::destroy( halt );
	}
	else {
DBG_DEBUG("haltestelle_t::remove()","not last");
		// acceptance and type may have been changed ... (due to post office/dock/railways station deletion)
 		halt->recalc_station_type();
	}

	// if building was removed this is false!
	if(bd) {
		bd->calc_image();
		reliefkarte_t::get_karte()->calc_map_pixel(pos.get_2d());
	}
	return true;
}



/**
 * Station factory method. Returns handles instead of pointers.
 * @author Hj. Malthaner
 */
halthandle_t haltestelle_t::create(loadsave_t *file)
{
	haltestelle_t *p = new haltestelle_t(file);
	return p->self;
}


/**
 * Station destruction method.
 * @author Hj. Malthaner
 */
void haltestelle_t::destroy(halthandle_t const halt)
{
	// just play safe: restart iterator at zero ...
	restart_halt_iterator = true;

	delete halt.get_rep();
}


/**
 * Station destruction method.
 * Da destroy() alle_haltestellen modifiziert kann kein Iterator benutzt
 * werden! V. Meyer
 * @author Hj. Malthaner
 */
void haltestelle_t::destroy_all()
{
	while (!alle_haltestellen.empty()) {
		halthandle_t halt = alle_haltestellen.back();
		destroy(halt);
	}
	delete all_koords;
	all_koords = NULL;
	//status_step = 0;
}


haltestelle_t::haltestelle_t(loadsave_t* file)
{
	last_loading_step = welt->get_steps();

	const uint8 max_categories = goods_manager_t::get_max_catg_index();
	const uint8 max_classes = max(goods_manager_t::passengers->get_number_of_classes(), goods_manager_t::mail->get_number_of_classes());

	cargo = (vector_tpl<ware_t> **)calloc( max_categories, sizeof(vector_tpl<ware_t> *) );
	non_identical_schedules = new uint8[ max_categories ];

	for ( uint8 i = 0; i < max_categories; i++ ) {
		non_identical_schedules[i] = 0;
	}

	connexions = new quickstone_hashtable_tpl<haltestelle_t, connexion*>*[max_categories];

	// See here for an explanation of the below: http://stackoverflow.com/questions/29375797/copy-2d-array-using-memcpy/29375830#29375830
	// This is not a true 2d array as the number of categories is not fixed.
	waiting_times = new inthashtable_tpl<uint32, waiting_time_set >*[max_categories];
	waiting_times[0] = new inthashtable_tpl<uint32, waiting_time_set >[max_categories * max_classes];
	for (uint8 i = 1; i < max_categories; i++)
	{
		waiting_times[i] = waiting_times[i - 1] + max_classes;
	}

#ifdef MULTI_THREAD
	transferring_cargoes = new vector_tpl<transferring_cargo_t>[world()->get_parallel_operations() + 2];
#else
	transferring_cargoes = new vector_tpl<transferring_cargo_t>[1];
#endif

	// Knightly : create the actual connexion hash tables
	for(uint8 i = 0; i < max_categories; i ++)
	{
		connexions[i] = new quickstone_hashtable_tpl<haltestelle_t, connexion*>();
	}
	do_alternative_seats_calculation = true;

	status_color = COL_YELLOW;
	last_status_color = COL_PURPLE;
	last_bar_count = 0;

	enables = NOT_ENABLED;

	// @author hsiegeln
	sortierung = freight_list_sorter_t::by_name;
	resort_freight_info = true;

	init_financial_history();
	rdwr(file);

	alle_haltestellen.append(self);
	restart_halt_iterator = true;

	// Added by : Knightly
	inauguration_time = 0;
}


haltestelle_t::haltestelle_t(koord k, player_t* player)
{
	self = halthandle_t(this);
	assert( !alle_haltestellen.is_contained(self) );
	alle_haltestellen.append(self);
	restart_halt_iterator = true;

	//markers[ self.get_id() ] = current_marker;

	last_loading_step = welt->get_steps();

	this->init_pos = k;
	owner = player;

	enables = NOT_ENABLED;

	last_catg_index = 255;	// force total rerouting

	const uint8 max_categories = goods_manager_t::get_max_catg_index();
	const uint8 max_classes = max(goods_manager_t::passengers->get_number_of_classes(), goods_manager_t::mail->get_number_of_classes());

	cargo = (vector_tpl<ware_t> **)calloc( max_categories, sizeof(vector_tpl<ware_t> *) );
	non_identical_schedules = new uint8[ max_categories ];

	for ( uint8 i = 0; i < max_categories; i++ ) {
		non_identical_schedules[i] = 0;
	}

	connexions = new quickstone_hashtable_tpl<haltestelle_t, connexion*>*[max_categories];

	// See here for an explanation of the below: http://stackoverflow.com/questions/29375797/copy-2d-array-using-memcpy/29375830#29375830
	// This is not a true 2d array as the number of categories is not fixed.
	waiting_times = new inthashtable_tpl<uint32, waiting_time_set >*[max_categories];
	waiting_times[0] = new inthashtable_tpl<uint32, waiting_time_set >[max_categories * max_classes];
	for (uint8 i = 1; i < max_categories; i++)
	{
	waiting_times[i] = waiting_times[i - 1] + max_classes;
	}

	// Knightly : create the actual connexion hash tables
	for(uint8 i = 0; i < max_categories; i ++)
	{
		connexions[i] = new quickstone_hashtable_tpl<haltestelle_t, connexion*>();
	}
	do_alternative_seats_calculation = true;

	status_color = COL_YELLOW;
	last_status_color = COL_PURPLE;
	last_bar_count = 0;

	sortierung = freight_list_sorter_t::by_name;
	init_financial_history();

	check_waiting = 0;

	check_nearby_halts();

	// Added by : Knightly
	//inauguration_time = dr_time();
	inauguration_time = welt->get_ticks(); // Possibly more network safe than the original (commented out above)

	control_towers = 0;

	transfer_time = 0;

	for(sint32 i = 0; i < 4; i ++)
	{
		train_last_departed[i] = 0;
	}

#ifdef MULTI_THREAD
	transferring_cargoes = new vector_tpl<transferring_cargo_t>[world()->get_parallel_operations() + 2];
#else
	transferring_cargoes = new vector_tpl<transferring_cargo_t>[1];
#endif
}


haltestelle_t::~haltestelle_t()
{
	assert(self.is_bound());

	// first: remove halt from all lists
	int i=0;
	while(alle_haltestellen.remove(self)) {
		i++;
	}
	if (i != 1) {
		dbg->error("haltestelle_t::~haltestelle_t()", "handle %i found %i times in haltlist!", self.get_id(), i );
	}

	if(!welt->is_destroying())
	{
		ITERATE(halts_within_walking_distance, n)
		{
			halthandle_t walking_distance_halt = get_halt_within_walking_distance(n);
			if ( walking_distance_halt.is_bound() )
			{
				walking_distance_halt->remove_halt_within_walking_distance(self);
			}
		}

		// At every other stop which may have waiting times for going here, clean this stop out
		// of that stop's waiting times list
		FOR(vector_tpl<halthandle_t>, & current_halt, alle_haltestellen)
		{
			// If it's not bound, or waiting_times isn't initialized, this could crash
			if(current_halt.is_bound() && current_halt->waiting_times)
			{
				for(uint8 category = 0; category < goods_manager_t::get_max_catg_index(); category++)
				{
					uint8 number_of_classes;
					if (category == goods_manager_t::INDEX_PAS)
					{
						number_of_classes = goods_manager_t::passengers->get_number_of_classes();
					}
					else if (category == goods_manager_t::INDEX_MAIL)
					{
						number_of_classes = goods_manager_t::mail->get_number_of_classes();
					}
					else
					{
						number_of_classes = 1;
					}

					for (uint8 g_class = 0; g_class < number_of_classes; g_class++ )
					{
						current_halt->waiting_times[category][g_class].remove(self.get_id());
					}
				}
			}
		}
	}

	// free name
	set_name(NULL);

	if(!welt->is_destroying())
	{
		// remove from ground and planquadrat (tile) haltlists
		koord ul(32767,32767);
		koord lr(0,0);
		while(  !tiles.empty()  ) {
			koord pos = tiles.remove_first().grund->get_pos().get_2d();
			planquadrat_t *pl = welt->access_nocheck(pos);
			assert(pl);
			for( uint8 i=0;  i<pl->get_boden_count();  i++  ) {
				pl->get_boden_bei(i)->set_halt( halthandle_t() );
			}
			// bounding box for adjustments
			// bounding box for adjustments
			ul.clip_max(pos);
			lr.clip_min(pos);
		}

		// remove from all haltlists
		uint16 const cov = welt->get_settings().get_station_coverage();
		vector_tpl<fabrik_t*> affected_fab_list;
		ul.x = max(0, ul.x - cov);
		ul.y = max(0, ul.y - cov);
		lr.x = min(welt->get_size().x, lr.x + 1 + cov);
		lr.y = min(welt->get_size().y, lr.y + 1 + cov);
		for(  int y=ul.y;  y<lr.y;  y++  ) {
			for(  int x=ul.x;  x<lr.x;  x++  ) {
				planquadrat_t *plan = welt->access_nocheck(x,y);
				if(plan->get_haltlist_count()>0) {
					plan->remove_from_haltlist(self);
				}
				const grund_t* gr = plan->get_kartenboden();
				// If there's a factory here, add it to the working list
				const gebaeude_t* gb = gr->find<gebaeude_t>();
				if (gb) {
					fabrik_t* fab = gb->get_fabrik();
					if (fab && !affected_fab_list.is_contained(fab) ) {
						affected_fab_list.append(fab);
					}
				}
			}
		}

		// Update nearby factories' lists of connected halts.
		// Must be done AFTER updating the planquadrats
		FOR (vector_tpl<fabrik_t*>, fab, affected_fab_list)
		{
			fab->recalc_nearby_halts();
		}
	}

	destroy_win( magic_halt_info + self.get_id() );
	destroy_win( magic_halt_detail + self.get_id() );

	// finally detach handle
	// before it is needed for clearing up the planqudrat and tiles
	self.detach();

	const uint8 max_categories = goods_manager_t::get_max_catg_index();

	for(uint8 i = 0; i < max_categories; i++) {
		if (cargo[i]) {
			FOR(vector_tpl<ware_t>, const &w, *cargo[i]) {
				fabrik_t::update_transit(w, false);
			}
			delete cargo[i];
			cargo[i] = NULL;
		}
	}
	free(cargo);
	
#ifdef MULTI_THREAD
	welt->stop_path_explorer();
#endif
	for(uint8 i = 0; i < max_categories; i++)
	{		
		if (!welt->is_destroying())
		{
			reset_connexions(i);
			path_explorer_t::refresh_category(i);
		}
		delete connexions[i];
	}		

	delete[] connexions;
	// See here for an explanation of the below: http://stackoverflow.com/questions/29375797/copy-2d-array-using-memcpy/29375830#29375830
	// This is not a true 2d array as the number of categories is not fixed.
	delete[] waiting_times[0];
	delete[] waiting_times;

	delete[] non_identical_schedules;
//	delete[] all_links;

	delete[] transferring_cargoes;
}


void haltestelle_t::rotate90( const sint16 y_size )
{
	init_pos.rotate90( y_size );

	// rotate goods destinations
	// iterate over all different categories
	for(uint8 i=0; i<goods_manager_t::get_max_catg_index(); i++) {
		if(cargo[i]) {
			vector_tpl<ware_t>& warray = *cargo[i];
			for (size_t j = warray.get_count(); j-- > 0;) {
				ware_t& ware = warray[j];
				if(ware.menge>0) {
					ware.rotate90(y_size);
				}
				else
				{
					// empty => remove
					warray.remove_at(j);
				}
			}
		}
	}

	for (uint32 i = 0; i < world()->get_parallel_operations(); i++)
	{
		vector_tpl<transferring_cargo_t>& tcarray = transferring_cargoes[i];
		for (size_t j = tcarray.get_count(); j-- > 0;)
		{
			transferring_cargo_t& tc = tcarray[j];
			if (tc.ware.menge>0)
			{
				tc.ware.rotate90(y_size);
			}
			else
			{
				// empty => remove
				tcarray.remove_at(j);
			}
		}
	}

	vector_tpl<koord3d> rotated_station_signals;

	FOR(vector_tpl<koord3d>, i, station_signals)
	{
		i.rotate90(y_size);
		rotated_station_signals.append(i);
	}

	station_signals.clear();

	FOR(vector_tpl<koord3d>, i, rotated_station_signals)
	{
		station_signals.append(i);
	}

	sint64 temp_last_departed[4];
	// Rotate station signal timings
	for(uint32 i = 0; i < 4; i ++)
	{
		temp_last_departed[i] = train_last_departed[i];
	}

	// Rotation is clockwise.

	// North becomes East
	train_last_departed[2] = temp_last_departed[0];

	// South becomes West
	train_last_departed[3] = temp_last_departed[1];

	// East becomes South
	train_last_departed[1] = temp_last_departed[2];

	// West becomes North
	train_last_departed[0] = temp_last_departed[3];

	// Simworld will update nearby factories' lists of connected halts.
}



const char* haltestelle_t::get_name() const
{
	const char *name = "Unknown";
	if (tiles.empty()) {
		name = "Unnamed";
	}
	else {
		grund_t* bd = welt->lookup(get_basis_pos3d());
		if(bd  &&  bd->get_flag(grund_t::has_text)) {
			name = bd->get_text();
		}
	}
	return name;
}



/**
 * Sets the name. Creates a copy of name.
 * @author Hj. Malthaner
 */
void haltestelle_t::set_name(const char *new_name)
{
	grund_t *gr = welt->lookup(get_basis_pos3d());
	if(gr) {
		if(gr->get_flag(grund_t::has_text)) {
			halthandle_t h = all_names.remove(gr->get_text());
			if(h!=self) {
				DBG_MESSAGE("haltestelle_t::set_name()","removing name %s already used!",gr->get_text());
			}
		}
		if(!gr->find<label_t>()) {
			gr->set_text( new_name );
			if(new_name  &&  all_names.set(gr->get_text(),self).is_bound() ) {
 				DBG_MESSAGE("haltestelle_t::set_name()","name %s already used!",new_name);
			}
		}
		// Knightly : need to update the title text of the associated halt detail and info dialogs, if present
		halt_detail_t *const details_frame = dynamic_cast<halt_detail_t *>( win_get_magic( magic_halt_detail + self.get_id() ) );
		if(  details_frame  ) {
			details_frame->set_name( get_name() );
		}
		halt_info_t *const info_frame = dynamic_cast<halt_info_t *>( win_get_magic( magic_halt_info + self.get_id() ) );
		if(  info_frame  ) {
			info_frame->set_name( get_name() );
		}
	}
}


// returns the number of % in a printf string ....
static int count_printf_param( const char *str )
{
	int count = 0;
	while( *str!=0  ) {
		if(  *str=='%'  ) {
			str++;
			if(  *str!='%'  ) {
				count++;
			}
		}
		str ++;
	}
	return count;
}


// creates stops with unique! names
char* haltestelle_t::create_name(koord const k, char const* const typ)
{
	int const lang = welt->get_settings().get_name_language_id();
	stadt_t *stadt = welt->find_nearest_city(k);
	const char *stop = translator::translate(typ,lang);
	cbuffer_t buf;

	// this fails only, if there are no towns at all!
	if(stadt==NULL) {
		for(  uint32 i=1;  i<65536;  i++  ) {
			// get a default name
			buf.printf( translator::translate("land stop %i %s",lang), i, stop );
			if(  !all_names.get(buf).is_bound()  ) {
				return strdup(buf);
			}
			buf.clear();
		}
		return strdup("Unnamed");
	}

	// now we have a city
	const char *city_name = stadt->get_name();

	sint16 li_gr = stadt->get_linksoben().x - 2;
	sint16 re_gr = stadt->get_rechtsunten().x + 2;
	sint16 ob_gr = stadt->get_linksoben().y - 2;
	sint16 un_gr = stadt->get_rechtsunten().y + 2;

	// The location is anywhere inside the town boundary
	const bool inside = (li_gr < k.x  &&  re_gr > k.x  &&  ob_gr < k.y  &&  un_gr > k.y);

	// The location is a short distance outside the town boundary
	const bool suburb = !inside  &&  (li_gr - 6 < k.x  &&  re_gr + 6 > k.x  &&  ob_gr - 6 < k.y  &&  un_gr + 6 > k.y);

	const koord townhall_pos = stadt->get_townhall_road();
	//const uint16 tiles_to_edge = inside ? max(max(k.x - li_gr, k.y - ob_gr), max(un_gr - k.y, re_gr - k.x)) : 1;
	const uint16 townhall_tiles_to_edge = max(max(townhall_pos.x - li_gr, townhall_pos.y - ob_gr), max(un_gr - townhall_pos.y, re_gr - townhall_pos.x));

	// This location is in the geographical centre of the town
	const bool inner = shortest_distance(k, townhall_pos) < townhall_tiles_to_edge / 2;

	if (!welt->get_settings().get_numbered_stations()) {
		static const koord next_building[24] = {
			koord( 0, -1), // north
			koord( 1,  0), // east
			koord( 0,  1), // south
			koord(-1,  0), // west
			koord( 1, -1), // northeast
			koord( 1,  1), // southeast
			koord(-1,  1), // southwest
			koord(-1, -1), // northwest
			koord( 0, -2), // double nswo
			koord( 2,  0),
			koord( 0,  2),
			koord(-2,  0),
			koord( 1, -2), // all the remaining 3s
			koord( 2, -1),
			koord( 2,  1),
			koord( 1,  2),
			koord(-1,  2),
			koord(-2,  1),
			koord(-2, -1),
			koord(-1, -2),
			koord( 2, -2), // and now all buildings with distance 4
			koord( 2,  2),
			koord(-2,  2),
			koord(-2, -2)
		};

		// standard names:
		// order: factory, attraction, direction, normal name
		// prissi: first we try a factory name
		// is there a translation for factory defined?
		const char *fab_base_text = "%s factory %s %s";
		const char *fab_base = translator::translate(fab_base_text,lang);
		if(  fab_base_text != fab_base  ) {
			slist_tpl<fabrik_t *>fabs;
			if (self.is_bound()) {
				// first factories (so with same distance, they have priority)
				int this_distance = 999;
				FOR(slist_tpl<fabrik_t*>, const f, get_fab_list()) {
					int distance = koord_distance(f->get_pos().get_2d(), k);
					if(  distance < this_distance  ) {
						fabs.insert(f);
						this_distance = distance;
					}
					else {
						fabs.append(f);
					}
				}
			}
			else {
				// since the distance are presorted, we can just append for a good choice ...
				for(  int test=0;  test<24;  test++  ) {
					fabrik_t *fab = fabrik_t::get_fab(k+next_building[test]);
					if(fab  &&  fabs.is_contained(fab)) {
						fabs.append(fab);
					}
				}
			}

			// are there fabs?
			FOR(slist_tpl<fabrik_t*>, const f, fabs) {
				// with factories
				buf.printf(fab_base, city_name, f->get_name(), stop);
				if(  !all_names.get(buf).is_bound()  ) {
					return strdup(buf);
				}
				buf.clear();
			}
		}

		// no fabs or all names used up already
		// is there a translation for buildings defined?
		const char *building_base_text = "%s building %s %s";
		const char *building_base = translator::translate(building_base_text,lang);
		if(  building_base_text != building_base  ) {
			// check for other special building (townhall, monument, tourist attraction)
			for (int i=0; i<24; i++) {
				grund_t *gr = welt->lookup_kartenboden( next_building[i] + k);
				if(gr==NULL  ||  gr->get_typ()!=grund_t::fundament) {
					// no building here
					continue;
				}
				// since closes coordinates are tested first, we do not need to not sort this
				const char *building_name = NULL;
				const gebaeude_t* gb = gr->find<gebaeude_t>();
				if(gb==NULL) {
					// field may have foundations but no building
					continue;
				}
				// now we have a building here
				if (gb->is_monument()) {
					building_name = translator::translate(gb->get_name(),lang);
				}
				else if (gb->is_townhall() ||
					gb->get_tile()->get_desc()->get_type() == building_desc_t::attraction_land || // land attraction
					gb->get_tile()->get_desc()->get_type() == building_desc_t::attraction_city) { // town attraction
					building_name = make_single_line_string(translator::translate(gb->get_tile()->get_desc()->get_name(),lang), 2);
				}
				else {
					// normal town house => not suitable for naming
					continue;
				}
				// now we have a name: try it
				buf.printf( building_base, city_name, building_name, stop );
				if(  !all_names.get(buf).is_bound()  ) {
					return strdup(buf);
				}
				buf.clear();
			}
		}

		// Use the standard naming scheme before reverting to street names:
		// this way, the major stops in a town will be likely to be distinguished
		// automatically from the minor stops by name automatically.

		char numbername[10];
		if(inside)
		{
			if (inner)
			{
				// Names for town centre locations
				strcpy(numbername, "0center");
			}
			else
			{
				// Names for locations inside the towns, but in their periphery
				strcpy(numbername, "0outer");
			}
		}
		else if(suburb)
		{
			// Names for locations just outside towns
			strcpy( numbername, "0suburb" );
		}
		else
		{
			// Names for locations a long way outside towns
			strcpy( numbername, "0extern" );
		}

		const char *dirname = NULL;
		static const char *diagonal_name[4] = { "northwest", "nordost", "southeast", "southwest" };
		static const char *direction_name[4] = { "north", "east", "south", "west" };

		uint8 const rot = welt->get_settings().get_rotation();
		if (k.y < ob_gr  ||  (inside  &&  k.y*3 < (un_gr+ob_gr+ob_gr))  ) {
			if (k.x < li_gr) {
				dirname = diagonal_name[(4 - rot) % 4];
			}
			else if (k.x > re_gr) {
				dirname = diagonal_name[(5 - rot) % 4];
			}
			else {
				dirname = direction_name[(4 - rot) % 4];
			}
		} else if (k.y > un_gr  ||  (inside  &&  k.y*3 > (un_gr+un_gr+ob_gr))  ) {
			if (k.x < li_gr) {
				dirname = diagonal_name[(3 - rot) % 4];
			}
			else if (k.x > re_gr) {
				dirname = diagonal_name[(6 - rot) % 4];
			}
			else {
				dirname = direction_name[(6 - rot) % 4];
			}
		} else {
			if (k.x <= stadt->get_pos().x) {
				dirname = direction_name[(3 - rot) % 4];
			}
			else {
				dirname = direction_name[(5 - rot) % 4];
			}
		}
		dirname = translator::translate(dirname,lang);

		// Try everything to get a unique name
		while(true) {
			// well now try them all from "0..." over "9..." to "A..." to "Z..."
			for(  int i=0;  i<10+26;  i++  ) {
				const int random_base = simrand(10 + 26, "haltestelle_t::create_name()"); 
				numbername[0] = random_base < 10 ? '0' + random_base : 'A' + random_base - 10;
				const char *base_name = translator::translate(numbername,lang);
				if(base_name==numbername) {
					// not translated ... try next
					continue;
				}
				// allow for names without direction
				uint8 count_s = count_printf_param( base_name );
				if(count_s==3) {
					if (cbuffer_t::check_format_strings("%s %s %s", base_name) ) {
						// ok, try this name, if free ...
						buf.printf( base_name, city_name, dirname, stop );
					}
				}
				else {
					if (cbuffer_t::check_format_strings("%s %s", base_name) ) {
						// ok, try this name, if free ...
						buf.printf( base_name, city_name, stop );
					}
				}
				if(  buf.len()>0  &&  !all_names.get(buf).is_bound()  ) {
					return strdup(buf);
				}
				buf.clear();
			}
			// here we did not find a suitable name ...
			// ok, no suitable city names, try the outer ones ...
			if(  strcmp(numbername+1,"center")==0  )
			{
				strcpy( numbername, "0outer" );
			}
			// Try suburb names if the outer names cannot be found
			if (strcmp(numbername + 1, "outer") == 0)
			{
				strcpy(numbername, "0suburb");
			}
			// ok, no suitable suburb names, try the external ones (if not inside city) ...
			else if(  strcmp(numbername+1,"suburb")==0  &&  !inside  )
			{
				strcpy( numbername, "0extern" );
			}
			else
			{
				// no suitable unique name found at all ...
				break;
			}
		}
	}

	// If we cannot use a standard name, use a name from the list of street names
	if (inside || suburb) {
		const vector_tpl<char*>& street_names(translator::get_street_name_list());
		// make sure we do only ONE random call regardless of how many names are available (to avoid desyncs in network games)
		if (const uint32 count = street_names.get_count()) {
			uint32 idx = simrand(count, "char* haltestelle_t::create_name(koord const k, char const* const typ)");
			static const uint32 some_primes[] = { 19, 31, 109, 199, 409, 571, 631, 829, 1489, 1999, 2341, 2971, 3529, 4621, 4789, 7039, 7669, 8779, 9721 };
			// find prime that does not divide count
			uint32 offset = 1;
			for (uint8 i = 0; i<lengthof(some_primes); i++) {
				if (count % some_primes[i] != 0) {
					offset = some_primes[i];
					break;
				}
			}
			// as count % offset != 0 we are guaranteed to test all street names
			for (uint32 i = 0; i<count; i++) {
				buf.clear();
				if (cbuffer_t::check_format_strings("%s %s", street_names[idx])) {
					buf.printf(street_names[idx], city_name, stop);
					if (!all_names.get(buf).is_bound()) {
						return strdup(buf);
					}
				}
				idx = (idx + offset) % count;
			}
			buf.clear();
		}
		else {
			/* the one random call to avoid desyncs */
			simrand(5, "char* haltestelle_t::create_name(koord const k, char const* const typ) dummy");
		}
	}

	/* so far we did not found a matching station name
	 * as a last resort, we will try numbered names
	 * (or the user requested this anyway)
	 */

	// strings for intown / outside of town
	const char *base_name = translator::translate( inside ? "%s city %d %s" : "%s land %d %s", lang);

	// finally: is there a stop with this name already?
	for(  uint32 i=1;  i<65536;  i++  ) {
		buf.printf( base_name, city_name, i, stop );
		if(  !all_names.get(buf).is_bound()  ) {
			return strdup(buf);
		}
		buf.clear();
	}

	// emergency measure: But before we should run out of handles anyway ...
	assert(0);
	return strdup("Unnamed");
}

class lines_loaded_compare_t
{
public:
	linehandle_t line;
	bool reversed;
	uint8 current_stop;
	uint8 fracht_index;

	bool operator== (const lines_loaded_compare_t &x) const  { return (line == x.line  &&  reversed == x.reversed  &&  current_stop == x.current_stop   &&  fracht_index == x.fracht_index); }
};

vector_tpl<lines_loaded_compare_t> lines_loaded; // used to skip loading multiple convois on the same line during the same step


// Add convoy to loading
void haltestelle_t::request_loading(convoihandle_t cnv)
{
	if(!loading_here.is_contained(cnv))
	{
		estimated_convoy_arrival_times.remove(cnv.get_id());
		loading_here.append(cnv);
	}
	if(last_loading_step != welt->get_steps())
	{
		last_loading_step = welt->get_steps();
		lines_loaded.clear();

		// now iterate over all convois
		for(slist_tpl<convoihandle_t>::iterator i = loading_here.begin(), end = loading_here.end();  i != end;)
		{
			convoihandle_t const c = *i;
			if (c.is_bound()
				&& (c->get_state() == convoi_t::LOADING || c->get_state() == convoi_t::REVERSING || c->get_state() == convoi_t::WAITING_FOR_CLEARANCE)
				&& ((get_halt(c->get_pos(), owner) == self)
					|| (c->get_vehicle(0)->get_waytype() == water_wt
					&& c->get_state() == convoi_t::LOADING
					&& get_halt(c->get_schedule()->get_current_entry().pos, owner) == self)))
			{
				++i;

				// now we load into convoi
				c->hat_gehalten(self);
			}
			else
			{
				i = loading_here.erase(i);
			}
		}
	}
}

void haltestelle_t::check_transferring_cargoes()
{
	const sint64 current_time = welt->get_ticks();
	ware_t ware;

#ifdef MULTI_THREAD
	sint32 po = world()->get_parallel_operations();
#else
	sint32 po = 1;
#endif

	for (sint32 i = 0; i < po; i++)
	{
		FOR(vector_tpl<transferring_cargo_t>, tc, transferring_cargoes[i])
		{
			//const uint32 ready_seconds = world()->ticks_to_seconds((tc.ready_time - current_time));
			//const uint32 ready_minutes = ready_seconds / 60;
			//const uint32 ready_hours = ready_minutes / 60;
			bool removed; // This check is necessary because, for some odd reason, the iterator sometimes repeats a tc object.

			if (tc.ready_time <= current_time)
			{
				ware = tc.ware;
				removed = transferring_cargoes[i].remove(tc);
				if (removed && ware.get_ziel() == self)
				{
					// This is the final destination: register the cargoes
					// at their ultimate end point.

					world()->deposit_ware_at_destination(ware);
					resort_freight_info = true;
				}
				else if (removed)
				{
					// This is just a transfer - add this to the stop's
					// internal storage for onward travel.
					add_ware_to_halt(ware);
					resort_freight_info = true;
				}
			}
		}
	}
}




void haltestelle_t::step()
{
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
	bool talk = !strcmp(get_name(), "Newton Abbot Railway Station");
#endif
#endif
	// Knightly : update status
	//   There is no idle state in Extended
	//   as rerouting requests may be sent via
	//   refresh_routing() instead of
	//   karte_t::set_schedule_counter()

	COLOR_VAL old_status_color = status_color;

	FOR(vector_tpl<uint8>, catg, categories_to_refresh_next_step)
	{
		reroute_goods(catg);
	}
	categories_to_refresh_next_step.clear();

	check_transferring_cargoes();

	recalc_status();

	if(status_color == COL_RED || old_status_color != status_color)
	{
		// The transfer time needs recalculating if the stop is overcrowded.
		calc_transfer_time();
	}

	// Every 256 steps - check whether passengers/goods have been waiting too long.
	// Will overflow at 255.
	if(++ check_waiting == 0)
	{
		vector_tpl<ware_t> *warray;
		for(uint16 j = 0; j < goods_manager_t::get_max_catg_index(); j ++)
		{
			warray = cargo[j];
			if(warray == NULL)
			{
				continue;
			}
			for(uint32 i = 0; i < warray->get_count(); i++)
			{
				ware_t &tmp = (*warray)[i];

				// skip empty entries
				if(tmp.menge == 0)
				{
					continue;
				}

				// Check whether these goods/passengers are waiting to go to a factory that has been deleted.
				const grund_t* gr = welt->lookup_kartenboden(tmp.get_zielpos());
				const gebaeude_t* const gb = gr ? gr->get_building() : NULL;
				fabrik_t* const fab = gb ? gb->get_fabrik() : NULL;
				if(!gb || tmp.is_freight() && !fab)
				{
					// The goods/passengers leave.  We must record the lower "in transit" count on factories.
					fabrik_t::update_transit(tmp, false);
					tmp.menge = 0;

					// No need to record waiting times if the goods are discarded because their destination
					// does not exist.
					continue;
				}

				uint32 waiting_tenths = convoi_t::get_waiting_minutes(welt->get_ticks() - tmp.arrival_time);

				// Checks to see whether the freight has been waiting too long.
				// If so, discard it.

				if(tmp.get_desc()->get_speed_bonus() > 0u) // TODO: Consider what to do about this now that speed boni are deprecated. Should the base data for speed boni be retained just for this?
				{
					// Only consider for discarding if the goods (ever) care about their timings.
					// Use 32-bit math; it's very easy to overflow 16 bits.
					const uint32 max_wait = welt->get_settings().get_passenger_max_wait();
					const uint32 max_wait_minutes = max_wait / tmp.get_desc()->get_speed_bonus();
					uint32 max_wait_tenths = max_wait_minutes * 10u;

					// Passengers' maximum waiting times were formerly limited to thrice their estimated
					// journey time, but this is no longer so from version 11.14 onwards.

#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
					if (talk && i == 2198)
						dbg->message("haltestelle_t::step", "%u) check %u of %u minutes: %u %s to \"%s\"",
						i, waiting_tenths, max_wait_tenths, tmp.menge, tmp.get_desc()->get_name(), tmp.get_ziel()->get_name());
#endif
#endif
					if(waiting_tenths > max_wait_tenths)
					{
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
						if (talk)
							dbg->message("haltestelle_t::step", "%u) discard after %u of %u minutes: %u %s to \"%s\"",
							i, waiting_tenths, max_wait_tenths, tmp.menge, tmp.get_desc()->get_name(), tmp.get_ziel()->get_name());
#endif
#endif
						bool passengers_walked = false;
						// Waiting too long: discard
						if(tmp.is_passenger())
						{
							// Check to see whether they can walk to their ultimate destination or next transfer first.
							const uint16 max_walking_distance = welt->get_settings().get_station_coverage() / 2;
							if(shortest_distance(get_next_pos(tmp.get_zielpos()), tmp.get_zielpos()) <= max_walking_distance)
							{
								// Passengers can walk to their ultimate destination.
								add_to_waiting_list(tmp, calc_ready_time(tmp, false));
								passengers_walked = true;
							}

							if(tmp.get_zwischenziel().is_bound() && shortest_distance(get_next_pos(tmp.get_zwischenziel()->get_basis_pos()), get_next_pos(tmp.get_zwischenziel()->get_basis_pos())) <= max_walking_distance)
							{
								// Passengers can walk to their next transfer.
								pedestrian_t::generate_pedestrians_at(get_basis_pos3d(), tmp.menge);
								tmp.set_last_transfer(self);
								tmp.get_zwischenziel()->liefere_an(tmp, 1);
								passengers_walked = true;
							}

							// Passengers - use unhappy graph. Even passengers able to walk to their destination or
							// next transfer are not happy about it if they expected to be able to take a ride there.
							add_pax_unhappy(tmp.menge);
						}

						// If they are discarded, a refund is due.

						if(!passengers_walked && tmp.get_origin().is_bound() && get_owner()->get_finance()->get_account_balance() > 0)
						{
							// Cannot refund unless we know the origin.
							// Also, ought not refund unless the player is solvent.
							// Players ought not be put out of business by refunds, as this makes gameplay too unpredictable,
							// especially in online games, where joining one player's network to another might lead to a large
							// influx of passengers which one of the networks cannot cope with.
							const uint16 distance = shortest_distance(get_basis_pos(), tmp.get_origin()->get_basis_pos());
							if(distance > 0) // No point in calculating refund if passengers/goods are discarded from their origin stop.
							{
								const uint32 distance_meters = (uint32) distance * welt->get_settings().get_meters_per_tile();
								// Refund is approximation: 2x distance at standard rate with no adjustments.
								const sint64 refund_amount = (tmp.menge * tmp.get_desc()->get_refund(distance_meters) + 2048ll) / 4096ll;

								owner->book_revenue(-refund_amount, get_basis_pos(), ignore_wt, ATV_REVENUE_PASSENGER);
								// Find the line the pasenger was *trying to go on* -- make it pay the refund
								linehandle_t account_line = get_preferred_line(tmp.get_zwischenziel(), tmp.get_catg());
								if(account_line.is_bound())
								{
									account_line->book(-refund_amount, LINE_PROFIT);
									account_line->book(-refund_amount, LINE_REFUNDS);
								}
								else
								{
									convoihandle_t account_convoy = get_preferred_convoy(tmp.get_zwischenziel(), tmp.get_catg());
									if(account_convoy.is_bound())
									{
										account_convoy->book(-refund_amount, convoi_t::CONVOI_PROFIT);
										account_convoy->book(-refund_amount, convoi_t::CONVOI_REFUNDS);
									}
								}
							}
						}

						// If goods/passengers leave, then they must register a waiting time, or else
						// overcrowded stops would have excessively low waiting times. Because they leave
						// before they have got transport, the waiting time registered must be increased
						// by 4x to reflect an estimate of how long that they would likely have had to
						// have waited to get transport.
						waiting_tenths *= 4;

						add_waiting_time(waiting_tenths, tmp.get_zwischenziel(), tmp.get_desc()->get_catg_index(), tmp.get_class());

						// The goods/passengers leave.  We must record the lower "in transit" count on factories.
						fabrik_t::update_transit(tmp, false);
						tmp.menge = 0;

						// Normally we record long waits below, but we just did, so don't do it twice.
						continue;
					}
				}

				// Check to see whether these passengers/this freight has been waiting more than 2x as long
				// as the existing registered waiting times. If so, register the waiting time to prevent an
				// artificially low time from being recorded if there is a long service interval.
				if(waiting_tenths > 2 * get_average_waiting_time(tmp.get_zwischenziel(), tmp.get_desc()->get_catg_index(), tmp.get_class()))
				{
					add_waiting_time(waiting_tenths, tmp.get_zwischenziel(), tmp.get_desc()->get_catg_index(), tmp.get_class());
				}
			}
		}
	}
}

/**
 * Called every month
 * @author Hj. Malthaner
 */
void haltestelle_t::new_month()
{
	if(  welt->get_active_player()==owner  &&  status_color==COL_RED  ) {
		cbuffer_t buf;
		buf.printf( translator::translate("%s\nis crowded."), get_name() );
		welt->get_message()->add_message(buf, get_basis_pos(),message_t::full, PLAYER_FLAG|owner->get_player_nr(), IMG_EMPTY );
		enables &= (PAX|POST|WARE);
	}

	// If the waiting times have not been updated for too long, gradually re-set them; also increment the timing records.
	for (uint8 category = 0; category < goods_manager_t::get_max_catg_index(); category++)
	{
		uint8 number_of_classes;
		if (category == goods_manager_t::INDEX_PAS)
		{
			number_of_classes = goods_manager_t::passengers->get_number_of_classes();
		}
		else if (category == goods_manager_t::INDEX_MAIL)
		{
			number_of_classes = goods_manager_t::mail->get_number_of_classes();
		}
		else
		{
			number_of_classes = 1;
		}

		for (uint8 g_class = 0; g_class < number_of_classes; g_class++)
		{
			FOR(waiting_time_map, &iter, waiting_times[category][g_class])
			{
				// If the waiting time data are stale (more than two months old), gradually flush them.
				// After a month, values of the estimated waiting time are appended to the list of waiting times.
				// This helps gradually to reduce times which were high as a result of a one-off problem,
				// whilst still allowing rarely-travelled connections to have sensible waiting times.
				if (iter.value.month >= 1)
				{
					halthandle_t check_halt;
					check_halt.set_id(iter.key);

					const uint32 service_frequency = get_service_frequency(check_halt, category); // Note that service frequency is currently class agnostic
					const uint32 estimated_waiting_time = service_frequency / 2;
					const uint32 average_waiting_time = get_average_waiting_time(check_halt, category, g_class);

					if (average_waiting_time > service_frequency)
					{
						iter.value.times.clear();
						iter.value.month = 0;
					}
					else if (iter.value.month > 2)
					{
						const uint32 max_iteration = average_waiting_time > average_waiting_time ? min(8, iter.value.month) : 1;
						for (uint32 i = 0; i < max_iteration; i++)
						{
							iter.value.times.add_to_tail(estimated_waiting_time);
						}
					}
				}
				// Update the waiting time timing records.
				// This is how many months that it has been since
				// any waiting time data were actually stored here.
				iter.value.month++;
			}
		}
	}

	check_nearby_halts();

	// hsiegeln: roll financial history
	for (int j = 0; j<MAX_HALT_COST; j++) {
		for (int k = MAX_MONTHS-1; k>0; k--) {
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}
	// number of waitung should be constant ...
	financial_history[0][HALT_WAITING] = financial_history[1][HALT_WAITING];
}


// Added by		: Knightly
// Adapted from : reroute_goods()
// Purpose		: re-route goods of a single ware category
uint32 haltestelle_t::reroute_goods(const uint8 catg)
{
	if(cargo[catg])
	{
		vector_tpl<ware_t> * warray = cargo[catg];
		const uint32 packet_count = warray->get_count();
		vector_tpl<ware_t> * new_warray = new vector_tpl<ware_t>(packet_count);

#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
		bool talk = catg == 0 && !strcmp(get_name(), "Newton Abbot Railway Station");

		if (talk)
			dbg->message("haltestelle_t::reroute_goods", "halt \"%s\", old packet count %u ", get_name(), packet_count);
#endif
#endif

		// Hajo:
		// Step 1: re-route goods now and then to adapt to changes in
		// world layout, remove all goods which destination was removed from the map
		// prissi;
		// also the empty entries of the array are cleared
		for(int j = packet_count - 1; j  >= 0; j--)
		{
			ware_t & ware = (*warray)[j];

			if(ware.menge == 0)
			{
				continue;
			}

			// If we are within delivery distance of our target factory, go there.
			if(fabrik_t* fab = fabrik_t::get_fab(ware.get_zielpos()))
			{
				// If there's no factory there, wait.
				if ( fab_list.is_contained(fab) ) {
					// If this factory is on our list of connected factories... we're there!

					add_to_waiting_list(ware, calc_ready_time(ware, true));
					continue;
				}
			}

			// check if this good can still reach its destination

			if(find_route(ware) == UINT32_MAX_VALUE)
			{
				// remove invalid destinations
				continue;
			}

			// If the passengers have re-routed so that they now
			// walk to the next transfer, go there immediately.
			if(ware.is_passenger()
			   && is_within_walking_distance_of(ware.get_zwischenziel())
			   && !get_preferred_convoy(ware.get_zwischenziel(), 0).is_bound()
			   && !get_preferred_line(ware.get_zwischenziel(), 0).is_bound())
			{
				pedestrian_t::generate_pedestrians_at(get_basis_pos3d(), ware.menge);
				ware.get_zwischenziel()->liefere_an(ware, 1); // start counting walking steps at 1 again
				continue;
			}

			// add to new array
			new_warray->append( ware );
		}

#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
		if (talk)
			dbg->message("haltestelle_t::reroute_goods", "halt \"%s\", new packet count %u ", get_name(), new_warray->get_count());
#endif
#endif

		// delete, if nothing connects here
		if (new_warray->empty())
		{
			if(connexions[catg]->empty())
			{
				// no connections from here => delete
				delete new_warray;
				new_warray = NULL;
				ware_t ware;

				for (uint32 i = 0; i < packet_count; i++)
				{
					ware = warray->get_element(i);
					if (ware.is_freight())
					{
						const grund_t* gr = welt->lookup_kartenboden(ware.get_zielpos());
						if (gr)
						{
							const gebaeude_t* building = gr->get_building();
							const fabrik_t* fab = building ? building->get_fabrik() : NULL;
							if (fab)
							{
								fab->update_transit(ware, false);
							}
						}
					}
				}
			}
		}

		// replace the array
		delete cargo[catg];
		cargo[catg] = new_warray;

		// likely the display must be updated after this
		resort_freight_info = true;

		return packet_count;
	}
	else
	{
		return 0;
	}
}

void haltestelle_t::add_factory(fabrik_t* fab)
{
	fab_list.append_unique(fab);
}

/*
 * Checks for local industries. The industries now
 * take care of adding halts themselves, but this old
 * method from Standard is still needed in some cases,
 * e.g. when a halt is expanded.
 */
void haltestelle_t::verbinde_fabriken()
{
	// Do not do this any longer:
	// this is a residue from when this used to do all of the recalculation.
	//fab_list.clear();

	if (tiles.begin() != tiles.end())
	{
		// then reconnect
		int const cov = welt->get_settings().get_station_coverage_factories();
		koord const coverage(cov, cov);

		// As fabrik_t::get_fab() is very expensive avoid any unnecessary calls.
		// Minimum: once per covered koord:

		// build a 2d map of the halt including covered area:
		const koord& p = tiles.begin()->grund->get_pos().get_2d();
		koord p0 = p - coverage, p1 = p + coverage;
		FOR(slist_tpl<tile_t>, const& i, tiles) {
			const koord& k = i.grund->get_pos().get_2d();
			koord k0 = k - coverage, k1 = k + coverage;
			if (p0.x > k0.x) p0.x = k0.x;
			if (p0.y > k0.y) p0.y = k0.y;
			if (p1.x < k1.x) p1.x = k1.x;
			if (p1.y < k1.y) p1.y = k1.y;
		}
		koord map_size = p1 - p0 + koord(1, 1);

		int size = map_size.x * map_size.y;
		uint8* halt_map = new uint8[size];
		uint8* ptr = halt_map;
		for (int i = 0; i < size; ++i)
			*ptr++ = 0;

		// set 1 to koords, that are covered by the halt:
		FOR(slist_tpl<tile_t>, const& i, tiles) {
			const koord& k = i.grund->get_pos().get_2d();
			koord k0 = k - coverage, k1 = k + coverage;
			for (int y = k0.y; y <= k1.y; ++y) {
				uint8* halt_row = &halt_map[map_size.x * (y - p0.y)];
				for (int x = k0.x; x <= k1.x; ++x) {
					halt_row[x - p0.x] = 1;
				}
			}
		}

		// process each covered koord:
		koord k;
		for (k.y = p0.y; k.y <= p1.y; ++k.y) {
			uint8* halt_row = &halt_map[map_size.x * (k.y - p0.y)];
			for (k.x = p0.x; k.x <= p1.x; ++k.x) {
				if (halt_row[k.x - p0.x]) {
					fabrik_t *fab = fabrik_t::get_fab(k);
					if(fab && !fab_list.is_contained(fab))
					{
						// This is slower than the old Standard logic, as
						// the checking for nearby halts has to be done twice,
						// but this is much more rarely used.
						fab->recalc_nearby_halts();

						// The old Standard logic is below
						/*
						// water factories can only connect to docks
						if(  fab->get_desc()->get_placement() != factory_desc_t::Water  ||  (station_type & dock) > 0  ) {
							// do no link to oil rigs via stations ...
							fab_list.insert(fab);
						}*/
					}
				}
			}
		}

		delete [] halt_map;

		// this is 6 times faster than the previous implementation below:
	}
/* was:
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		koord const p = i.grund->get_pos().get_2d();

		FOR(vector_tpl<fabrik_t*>, const fab, fabrik_t::sind_da_welche(p - coverage, p + coverage)) {
			if(!fab_list.is_contained(fab)) {
				// water factories can only connect to docks
				if(  fab->get_desc()->get_placement() != factory_desc_t::Water  ||  (station_type & dock) > 0  ) {
					// do no link to oil rigs via stations ...
					fab_list.insert(fab);
				}
			}
		}
	}
*/
}



/*
 * removes factory to a halt
 */
void haltestelle_t::remove_fabriken(fabrik_t *fab)
{
	fab_list.remove(fab);
}

// TODO: Check whether this can be removed entirely.
sint8 haltestelle_t::is_connected(halthandle_t halt, uint8 catg_index) const
{
	if (!halt.is_bound()) {
		return 0; // not connected
	}
	connexions_map *linka = connexions[catg_index];
	connexions_map *linkb = halt->connexions[catg_index];
	if (linka->empty() || linkb->empty()) {
		return 0; // empty connections -> not connected
	}
	return linka->get(halt)  ||  linkb->get(self) ? 1 : 0;
}

uint32 haltestelle_t::get_average_waiting_time(halthandle_t halt, uint8 category, uint8 g_class)
{
	inthashtable_tpl<uint32, haltestelle_t::waiting_time_set> * const wt = &waiting_times[category][g_class];
	if(wt->is_contained((halt.get_id())))
	{
		fixed_list_tpl<uint32, 32> times = waiting_times[category][g_class].get(halt.get_id()).times;
		const uint32 count = times.get_count();
		if(count > 0 && halt.is_bound())
		{
#ifdef USE_MEDIAN_WAITING_TIME
			// This is *much* slower than the mean average and
			// makes only a very modest difference to long waiting times.
			binary_heap_tpl<uint32*> sorted_waiting_times;
			uint32 tt[32];

			ITERATE(times, i)
			{
				tt[i] = times.get_element(i);
				sorted_waiting_times.insert(&tt[i]);
			}

			const uint32 count = sorted_waiting_times.get_count();
			vector_tpl<uint32> sorted_vector;

			while (!sorted_waiting_times.empty())
			{
				uint32 time = *sorted_waiting_times.pop();
				sorted_vector.append(time);
			}

			const uint32 output = sorted_vector[(count - 1) / 2];
			return output;
#else

			uint32 total_times = 0;
			ITERATE(times,i)
			{
				total_times += times.get_element(i);
			}
			total_times /= count;
			return total_times;
#endif
		}
	}
	// The service frequency is divided by two to get the waiting times
	// because the time that passengers, goods, etc. wait is, on average,
	// half the interval between services, because they do not all arrive
	// just after the previous service has departed.
	uint32 service_frequency = get_service_frequency(halt, category);
	const uint32 estimated_waiting_time = service_frequency / 2;
	fixed_list_tpl<uint32, 32> tmp;
	waiting_time_set set;
	set.times = tmp;
	set.month = 2; // Set this so as to be stale and flushed quickly to keep this up to date.
	set.times.add_to_tail(estimated_waiting_time);

#ifdef ALWAYS_CACHE_SERVICE_INTERVAL

	// This is network safe for multi-threading only because
	// the data in service_frequencies are only read by
	// methods called from parts of the code that never run
	// concurrently with the path explorer (being the
	// single-threaded part of the convoy stepping and the
	// (also single threaded) stepping of the halts.
	service_frequency_specifier spec;
	spec.x = category;
	spec.y = halt.get_id();
	service_frequencies.set(spec, service_frequency);
#endif

	return estimated_waiting_time;
}

uint32 haltestelle_t::get_service_frequency(halthandle_t destination, uint8 category) const
{
	// Check whether the value is in the hashtable. If not, calculate it.

	// NOTE: This does not separate by class. This is probably acceptable, as
	// this gives only an estimate, and separating by class would require a new
	// struct for service_frequency_specifier, meaning, in turn, that something other
	// than the koordhashtable would have to be used (a new hashtable class,
	// essentially, the hashing algorithm for which would be hard to write).
	// Since real waiting times will be separated by class, this should not be
	// too much of a difficulty in practice, but might need reviewing if
	// problems be revealed in use.

	service_frequency_specifier spec;
	spec.x = category;
	spec.y = destination.get_id();

	if (service_frequencies.is_contained(spec))
	{
		return service_frequencies.get(spec);
	}

	dbg->message("uint32 haltestelle_t::get_service_frequency(halthandle_t destination, uint8 category) const", "Service frequency not in the database: calculating");
	return calc_service_frequency(destination, category);
}

uint32 haltestelle_t::calc_service_frequency(halthandle_t destination, uint8 category) const
{
	uint32 service_frequency = 0;

	for (uint32 i = 0; i < registered_lines.get_count(); i++)
	{
		if (!registered_lines[i]->get_goods_catg_index().is_contained(category))
		{
			continue;
		}
		uint8 schedule_count = registered_lines[i]->get_schedule()->get_count();
		uint32 timing = 0;
		bool line_serves_destination = false;
		uint32 number_of_calls_at_this_stop = 0;
		koord current_halt;
		koord next_halt;
		for (uint8 n = 0; n < schedule_count; n++)
		{
			current_halt = registered_lines[i]->get_schedule()->entries[n].pos.get_2d();
			if (n < schedule_count - 2)
			{
				next_halt = registered_lines[i]->get_schedule()->entries[n + 1].pos.get_2d();
			}
			else
			{
				next_halt = registered_lines[i]->get_schedule()->entries[0].pos.get_2d();
			}
			if (n < schedule_count - 1)
			{
				const uint32 average_time = registered_lines[i]->get_average_journey_times().get(id_pair(haltestelle_t::get_halt(current_halt, owner).get_id(), haltestelle_t::get_halt(next_halt, owner).get_id())).get_average();
				if (average_time != 0 && average_time != UINT32_MAX_VALUE)
				{
					timing += average_time;
				}
				else
				{
					// Fallback to convoy's general average speed if a point-to-point average is not available.
					const uint32 distance = shortest_distance(current_halt, next_halt);
					const uint32 recorded_average_speed = registered_lines[i]->get_finance_history(1, LINE_AVERAGE_SPEED);
					const uint32 average_speed = recorded_average_speed > 0 ? recorded_average_speed : speed_to_kmh(registered_lines[i]->get_convoy(0)->get_min_top_speed()) >> 1;
					const uint32 journey_time = welt->travel_time_tenths_from_distance(distance, average_speed);

					timing += journey_time;
				}
			}

			halthandle_t current_halthandle = haltestelle_t::get_halt(current_halt, owner);

			if (current_halthandle == destination)
			{
				// This line serves this destination.
				line_serves_destination = true;
			}

			if (current_halthandle == self)
			{
				number_of_calls_at_this_stop++;
			}
		}

		if (!line_serves_destination)
		{
			continue;
		}

		// Divide the round trip time by the number of convoys in the line and by the number of times that it calls at this stop in its schedule.
		timing /= (registered_lines[i]->count_convoys() * (number_of_calls_at_this_stop == 0 ? 1 : number_of_calls_at_this_stop));

		if (registered_lines[i]->get_schedule()->get_spacing() > 0)
		{
			// Check whether the spacing setting affects things.
			const sint64 spacing_ticks = welt->ticks_per_world_month / (sint64)registered_lines[i]->get_schedule()->get_spacing();
			uint32 spacing_time = welt->ticks_to_tenths_of_minutes(spacing_ticks);
			timing = max(spacing_time, timing);
		}

		timing = max(1, timing);

		if (service_frequency == 0)
		{
			// This is the only time that this has been set so far, so compute for single line timing.
			service_frequency = max(1, timing);
		}
		else
		{
			// There are multiple lines serving this stop, so compute for multiple line timing
			if (service_frequency == timing)
			{
				// Two equally timed lines: halve the service frequency
				service_frequency /= 2;
			}
			else if (service_frequency > timing)
			{
				// The new timing is more frequent than the service interval calculated so far
				uint32 proportion_10 = (service_frequency * 10) / timing; // This is the number of new convoys per old convoy in any given time, * 10
				proportion_10 += 10; // Adding 10 to add back the original convoy: this is now the total number of convoys per service_frequency, * 10
				service_frequency = (service_frequency * 10) / proportion_10;
			}
			else
			{
				// The new timing is less frequent than the service interval calculated so far
				uint32 proportion_10 = (timing * 10) / service_frequency; // This is the number of new convoys per old convoy in any given time, * 10
				proportion_10 += 10; // Adding 10 to add back the original convoy: this is now the total number of convoys per service_frequency, * 10
				service_frequency = (timing * 10) / proportion_10;
			}
		}
	}

	return service_frequency;
}

linehandle_t haltestelle_t::get_preferred_line(halthandle_t transfer, uint8 category) const
{
	if(connexions[category]->empty() || connexions[category]->get(transfer) == NULL)
	{
		linehandle_t dummy;
		return dummy;
	}
	linehandle_t best_line = connexions[category]->get(transfer)->best_line;
	return best_line;
}

convoihandle_t haltestelle_t::get_preferred_convoy(halthandle_t transfer, uint8 category) const
{
	if(connexions[category]->empty() || connexions[category]->get(transfer) == NULL)
	{
		convoihandle_t dummy;
		return dummy;
	}
	convoihandle_t best_convoy = connexions[category]->get(transfer)->best_convoy;
	return best_convoy;
}

void haltestelle_t::reset_connexions(uint8 category)
{
	if(connexions[category]->empty())
	{
		// Nothing to do here
		return;
	}

	FOR(connexions_map, & iter, *connexions[category] )
	{
        delete iter.value;
	}
}

// Added by		: Knightly
// Adapted from : Jamespetts' code
// Purpose		: To notify relevant halts to rebuild connexions
// @jamespetts: modified the code to combine with previous method and provide options about partially delayed refreshes for performance.
void haltestelle_t::refresh_routing(const schedule_t *const sched, const minivec_tpl<uint8> &categories, const minivec_tpl<uint8> *passenger_classes, const minivec_tpl<uint8> *mail_classes, const player_t *const player)
{
	halthandle_t tmp_halt;

	if(sched && player)
	{
		const uint8 catg_count = categories.get_count();
#ifdef MULTI_THREAD
		if (!welt->is_destroying())
		{
			world()->stop_path_explorer();
		}
#endif

		for (uint8 i = 0; i < catg_count; i++)
		{
			path_explorer_t::refresh_category(categories[i]);
		}

		if ((passenger_classes != NULL) && categories.is_contained(goods_manager_t::INDEX_PAS))
		{

			// These minivecs should only have anything in them if their respective categories have not been refreshed entirely.
			FOR(minivec_tpl<uint8>, const & g_class, *passenger_classes)
			{
				path_explorer_t::refresh_class_category(goods_manager_t::INDEX_PAS, g_class);
			}
		}

		if ((mail_classes != NULL) && categories.is_contained(goods_manager_t::INDEX_MAIL))
		{

			// These minivecs should only have anything in them if their respective categories have not been refreshed entirely.
			FOR(minivec_tpl<uint8>, const & g_class, *mail_classes)
			{
				path_explorer_t::refresh_class_category(goods_manager_t::INDEX_MAIL, g_class);
			}
		}
	}
	else
	{
		dbg->error("convoi_t::refresh_routing()", "Schedule or player is NULL");
	}
}

void haltestelle_t::get_destination_halts_of_ware(ware_t &ware, vector_tpl<halthandle_t>& destination_halts_list) const
{
	const goods_desc_t * warentyp = ware.get_desc();

	if(ware.get_zielpos() == koord::invalid && ware.get_ziel().is_bound())
	{
		ware.set_zielpos(ware.get_ziel()->get_basis_pos());
	}

	const planquadrat_t *const plan = welt->access(ware.get_zielpos());
	fabrik_t* const fab = fabrik_t::get_fab(ware.get_zielpos());

	destination_halts_list.resize(plan->get_haltlist_count());

	const grund_t* gr = welt->lookup_kartenboden(ware.get_zielpos());
	const gebaeude_t* gb = gr->find<gebaeude_t>();
	const building_desc_t *desc = gb ? gb->get_tile()->get_desc() : NULL;
	const koord size = desc ? desc->get_size() : koord(1,1);

	if(fab || size.x > 1 || size.y > 1)
	{
		// Check all tiles of a multi-tiled building of any type.
		vector_tpl<koord> tile_list;
		if(fab)
		{
			fab->get_tile_list(tile_list);
		}
		else
		{
			koord test;
			// Must add tiles manually for buildings other than industries.
			// This formula is based on the fabrik_t::get_tile_list function.
			for(test.x = 0; test.x < size.x; test.x++)
			{
				for(test.y = 0; test.y < size.y; test.y++)
				{
					tile_list.append(ware.get_zielpos() + test);
				}
			}
		}
		FOR(vector_tpl<koord>, const k, tile_list)
		{
			const planquadrat_t* plan = welt->access(k);
			if(plan)
			{
				const uint8 haltlist_count = plan->get_haltlist_count();
				if(haltlist_count)
				{
					const nearby_halt_t *haltlist = plan->get_haltlist();
					for(int i = 0; i < haltlist_count; i++)
					{
						if(haltlist[i].halt->is_enabled(warentyp))
						{
							// OK, the halt accepts the ware type.
							// If this is passengers or mail, accept it.
							//
							// However, the smaller freight coverage rules mean
							// that we may have halts too far away.  The halt will
							// know whether it is linked to the factory.
							if (!ware.is_freight()
									|| (fab && haltlist[i].halt->get_fab_list().is_contained(fab))
									)
							{
								destination_halts_list.append(haltlist[i].halt);
							}
						}
					}
				}
			}
		}
	}
	else
	{
		// This simpler routine is available ONLY when the destination is
		// a single-tile building.
		const nearby_halt_t *haltlist = plan->get_haltlist();
		for(uint16 h = 0; h < plan->get_haltlist_count(); h++)
		{
			halthandle_t halt = haltlist[h].halt;
			if(halt->is_enabled(warentyp))
			{
				destination_halts_list.append(halt);
			}
		}
	}
}

uint32 haltestelle_t::find_route(const vector_tpl<halthandle_t>& destination_halts_list, ware_t &ware, const uint32 previous_journey_time, const koord destination_pos) const
{
	// ** Beware ** This is the one of the most computationally intensive (taking into account how often that it is called) functions in the game
	// Find the best route (sequence of halts) for a given packet
	// from here to its final destination -- *and* reroute the packet.
	//
	// If a previous journey time is specified, we must beat that time
	// in order to reroute the packet.
	//
	// If there are no potential destination halts, make this the final destination halt.
	//
	// Authors: Knightly, James Petts, Nathanael Nerode (neroden)

	uint32 best_journey_time = previous_journey_time;
	halthandle_t best_destination_halt;
	halthandle_t best_transfer;

 	koord destination_stop_pos = destination_pos;

	const uint8 ware_catg = ware.get_desc()->get_catg_index();

	bool found_a_halt = false;
	for(vector_tpl<halthandle_t>::const_iterator destination_halt = destination_halts_list.begin(); destination_halt != destination_halts_list.end(); destination_halt++)
	{
		if (self == *destination_halt)
		{
			continue;
		}
		uint32 test_time;
		halthandle_t test_transfer;
		path_explorer_t::get_catg_path_between(ware_catg, self, *destination_halt, test_time, test_transfer, ware.g_class);

		if(!destination_halt->is_bound())
		{
			// This halt has been deleted recently.  Don't go there.
			continue;
		}

		found_a_halt = true;

		koord real_destination_pos = koord::invalid;
		if(destination_pos != koord::invalid)
		{
			// Called with a specific destination position, not set by ware
			// Done for passenger alternate-destination searches, I think?
			real_destination_pos = destination_pos;
		}
		else
		{
			// Packet has a specific destination postition
			// Done for real packets
			real_destination_pos = ware.get_zielpos();
		}

		/**
		* The below is far too computationally expensive. Find the standard halt location instead.
		*
		// Find the halt square closest to the real destination (closest exit)
		destination_stop_pos = (*destination_halt)->get_next_pos(real_destination_pos);
		*/

		destination_stop_pos = (*destination_halt)->get_init_pos();

		// And find the shortest walking distance to there.
		const uint32 walk_distance = shortest_distance(destination_stop_pos, real_destination_pos);

		if (test_time < UINT32_MAX_VALUE)
		{
			// The above check is necessary or there will be an overflow causing spurious routes to be found.
			if (!ware.is_freight())
			{
				// Passengers or mail.
				// Calculate walking time from destination stop to final destination; add it.
				test_time += welt->walking_time_tenths_from_distance(walk_distance);
			}
			else
			{
				// Freight
				// Calculate a transshipment time based on a notional 1km/h dispersal speed; add it.
				test_time += welt->walk_haulage_time_tenths_from_distance(walk_distance);
			}
		}

		if(test_time < best_journey_time)
		{
			// This is quicker than the last halt we tried.
			best_destination_halt = *destination_halt;
			best_journey_time = test_time;
			best_transfer = test_transfer;
		}
	}

	if (self == best_destination_halt)
	{
		// There is no point in starting and finishing at the same stop.
		found_a_halt = false;
	}

	if(!found_a_halt)
	{
		//no target station found
		ware.set_ziel(halthandle_t());
		ware.set_zwischenziel(halthandle_t());
		return UINT32_MAX_VALUE;
	}

	if(best_journey_time < previous_journey_time)
	{
		ware.set_ziel(best_destination_halt);
		ware.set_zwischenziel(best_transfer);
	}
	return best_journey_time;
}

uint32 haltestelle_t::find_route(ware_t &ware, const uint32 previous_journey_time) const
{
	vector_tpl<halthandle_t> destination_halts_list;
	get_destination_halts_of_ware(ware, destination_halts_list);
	const uint32 journey_time = find_route(destination_halts_list, ware, previous_journey_time);
	return journey_time;
}

/**
 * Found route and station uncrowded
 * @author Hj. Malthaner
 * As of Simutrans-Extended 7.2,
 * this method is called instead when
 * passengers *arrive* at their destination.
 */
void haltestelle_t::add_pax_happy(int n)
{
	book(n, HALT_HAPPY);
	// The below is probably not thread-safe for multiplayer
	if (!env_t::networkmode)
	{
		recalc_status();
	}
}

/**
 * Station crowded
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_unhappy(int n)
{
	book(n, HALT_UNHAPPY);
	// The below is probably not thread-safe for multiplayer
	if (!env_t::networkmode)
	{
		recalc_status();
	}
}

// Found a route, but too slow.
// @author: jamespetts

void haltestelle_t::add_pax_too_slow(int n)
{
	book(n, HALT_TOO_SLOW);
}

/**
 * Found no route
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_no_route(int n)
{
	book(n, HALT_NOROUTE);
}

/* retrieves a ware packet for any destination in the list
 * needed, if the factory in question wants to remove something
 */
bool haltestelle_t::recall_ware( ware_t& w, uint32 menge )
{
	w.menge = 0;
	vector_tpl<ware_t> *warray = cargo[w.get_desc()->get_catg_index()];
	if(warray!=NULL) {
		FOR(vector_tpl<ware_t>, & tmp, *warray) {
			// skip empty entries
			if(tmp.menge==0  ||  w.get_index()!=tmp.get_index()  ||  w.get_zielpos()!=tmp.get_zielpos()) {
				continue;
			}

			// not too much?
			if(tmp.menge > menge) {
				// not all can be loaded
				tmp.menge -= menge;
				w.menge = menge;
			}
			else {
				// leave an empty entry => joining will more often work
				w.menge = tmp.menge;
				tmp.menge = 0;
			}
			book(w.menge, HALT_ARRIVED);
			fabrik_t::update_transit( w, false );
			resort_freight_info = true;
			return true;
		}
	}
	// nothing to take out
	return false;
}


bool haltestelle_t::fetch_goods(slist_tpl<ware_t> &load, const goods_desc_t *good_category, uint32 requested_amount, const schedule_t *schedule, const player_t *player, convoi_t* cnv, bool overcrowded, const uint8 g_class, const bool use_lower_classes, bool& other_classes_available)
{
	bool skipped = false;
	const uint8 catg_index = good_category->get_catg_index();
	vector_tpl<ware_t> *warray = cargo[catg_index];
	if(warray && warray->get_count() > 0)
	{
		binary_heap_tpl<ware_t*> goods_to_check;
		for(uint32 i = 0;  i < warray->get_count();  )
		{
			// Load first the goods/passengers/mail that have been waiting the longest.
			// Do this by adding them all to a binary heap sorted by arrival time.
			ware_t* const ware = &(*warray)[i];
			if(ware->menge > 0)
			{
				i++;
				if (ware->get_class() >= g_class)
				{
					// We know at this stage that we cannot load passengers of a *lower* class into higher class accommodation,
					// but we cannot yet know whether or not to load passengers of a higher class into lower class accommodation.
					// Note that this method is called for each class of accommodation in each vehicle in each convoy.
					goods_to_check.insert(ware);
				}
				else
				{
					other_classes_available = true;
				}
			}
			else
			{
				// There is no need any longer to have empty ware packets hanging around.
				warray->remove_at(i, false);
			}
		}

		halthandle_t cached_halts[256];


		while(!goods_to_check.empty())
		{
			ware_t* const next_to_load = goods_to_check.pop();
			uint8 index = schedule->get_current_stop();
			bool reverse = cnv->get_reverse_schedule();
			if(cnv->get_state() != convoi_t::REVERSING)
			{
				schedule->increment_index(&index, &reverse);
			}

			int count = 0;
			while(index != schedule->get_current_stop() || (cnv->get_state() == convoi_t::REVERSING && count == 0))
			{
				halthandle_t& schedule_halt = cached_halts[index];
				if(schedule_halt.is_null())
				{
					schedule_halt = haltestelle_t::get_halt(schedule->entries[index].pos, player);
				}

				if(schedule_halt == self)
				{
					// The convoy returns here later, so do not load goods/passengers just to go on a detour.
					if(count == 0)
					{
						// However, this makes no sense if we start with where we are now.
						schedule->increment_index(&index, &reverse);
						continue;
					}
					else
					{
						break;
					}
				}

				count ++;

				const halthandle_t next_transfer = next_to_load->get_zwischenziel();
				const halthandle_t destination = next_to_load->get_ziel();

				const bool bound_for_next_transfer = next_transfer == schedule_halt;
				const bool bound_for_destination = destination == schedule_halt;

				if(schedule_halt.is_bound() && (bound_for_next_transfer || bound_for_destination) && schedule_halt->is_enabled(catg_index))
				{

					// Check to see whether this is the convoy departing from this stop that will arrive at the next transfer or ultimate destination the soonest.
					convoihandle_t fast_convoy;
					sint64 best_arrival_time;
					if (bound_for_next_transfer)
					{
						best_arrival_time = calc_earliest_arrival_time_at(next_transfer, fast_convoy, catg_index, next_to_load->g_class);
					}
					else
					{
						// This should be called only relatively rarely, when a convoy bound for this packet's ultimate destination arrives,
						// but this convoy is routed via an intermediate halt. Check whether it is sensible to stick to the planned route
						// here.
						uint32 test_time = 0;
						halthandle_t test_transfer;
						path_explorer_t::get_catg_path_between(catg_index, self, destination, test_time, test_transfer, next_to_load->g_class);
						const sint64 test_time_in_ticks = welt->get_seconds_to_ticks(test_time * 6);
						best_arrival_time = test_time_in_ticks + welt->get_ticks();
					}

					const halthandle_t check_halt = bound_for_next_transfer ? next_transfer : destination;

					const arrival_times_map& check_arrivals = check_halt->get_estimated_convoy_arrival_times();
					const sint64 this_arrival_time = check_arrivals.get(cnv->self.get_id());

					bool wait_for_faster_convoy = true;

					if(best_arrival_time < this_arrival_time)
					{
						// Do not board this convoy if another will reach the next transfer more quickly;
						// but add a margin of error and, if the difference is small, board this convoy
						// anyway on the bird in hand principle.

						const sint64 difference_in_arrival_times = this_arrival_time - best_arrival_time;
						sint64 fast_here_departure_time = get_estimated_convoy_departure_times().get(fast_convoy.get_id());
						const bool fast_is_here = loading_here.is_contained(fast_convoy);
						sint64 fast_here_arrival_time = get_estimated_convoy_arrival_times().get(fast_convoy.get_id());
						if(fast_here_arrival_time <= welt->get_ticks() && !fast_is_here)
						{
							// The faster convoy is late.
							// Estimate its arrival time based on the degree of delay so far (somewhat pessimistically).
							fast_here_departure_time += (((welt->get_ticks() - fast_here_arrival_time) + 1) * waiting_multiplication_factor);
						}

						sint64 waiting_time_for_faster_convoy = fast_here_departure_time - welt->get_ticks();
						if(!fast_is_here)
						{
							if (waiting_time_for_faster_convoy == 0)
							{
								// We know that this must be wrong, since it is not here.
								// Also, this will cause a division by zero crash if this proceeds.

								// We infer that this must be late, so estimate its arrival time.
								waiting_time_for_faster_convoy = difference_in_arrival_times * 3;
							}

							if(waiting_time_for_faster_convoy >= difference_in_arrival_times)
							{
								// Sanity check - cannot be better to wait.
								wait_for_faster_convoy = false;
							}
							else if((difference_in_arrival_times * 100ll) / waiting_time_for_faster_convoy < waiting_tolerance_ratio)
							{
								// Do not wait for the supposedly faster convoy if the extra waiting time is out of proportion to
								// the time likely to be saved.
								wait_for_faster_convoy = false;
							}
						}

						// Assume that a convoy on the same line, in the same part of its timetable will not overtake this convoy.

						if(fast_convoy.is_bound() && fast_convoy->get_line() == cnv->get_line())
						{
							// Check for the same part of the timetable, as the convoy may either be going in a circle in a reverse direction
							// or otherwise call at this stop at different parts of its timetable.
							uint8 check_index = schedule->get_current_stop();
							bool check_reverse = cnv->get_reverse_schedule();
							schedule_t* fast_schedule = fast_convoy->get_schedule();
							uint8 fast_index = fast_schedule->get_current_stop();
							bool fast_reverse = fast_convoy->get_reverse_schedule();
							const player_t* player = cnv->get_owner();
							halthandle_t fast_convoy_halt;

							for(int i = 0; i < fast_schedule->get_count() * 2; i ++)
							{
								fast_convoy_halt = haltestelle_t::get_halt(fast_schedule->entries[fast_index].pos, player);
								if(fast_convoy_halt == self)
								{
									if(fast_index == check_index && fast_reverse == check_reverse)
									{
										// The next convoy of the same line will arrive at the same position in its schedule, so
										// do not wait for it as it is not likely actually to be faster, no matter what the estimated
										// times may say.
										wait_for_faster_convoy = false;
										break;
									}
									else
									{
										// The next convoy is at a different point in its schedule, so respect the estimated times.
										break;
									}
								}
								fast_schedule->increment_index(&fast_index, &fast_reverse);
							}
						}

						// Also, if this stop has a wait for load order without a maximum time and the faster convoy
						// also has that, do not wait for a "faster" convoy, as it may never come.

						const schedule_entry_t schedule_entry = cnv->get_schedule()->get_current_entry();
						if (!fast_convoy.is_bound())
						{
							wait_for_faster_convoy = false;
						}
						else if(schedule_entry.minimum_loading > 0 && !schedule_entry.wait_for_time && schedule_entry.waiting_time_shift == 0)
						{
							// This convoy has an untimed wait for load order.
							if(fast_convoy->get_line() == cnv->get_line())
							{
								wait_for_faster_convoy = false;
							}
							else
							{
								// Check to see whether this has the same untimed wait for load order even if it is not on the same line.
								schedule_entry_t fast_convoy_schedule_entry = fast_convoy->get_schedule()->get_current_entry();
								if(haltestelle_t::get_halt(fast_convoy_schedule_entry.pos, cnv->get_owner()) == self)
								{
									if(fast_convoy_schedule_entry.minimum_loading > 0 && !fast_convoy_schedule_entry.wait_for_time && fast_convoy_schedule_entry.waiting_time_shift == 0)
									{
										wait_for_faster_convoy = false;
									}
								}
								else
								{
									for(int i = 0; i < fast_convoy->get_schedule()->get_count(); i++)
									{
										fast_convoy_schedule_entry = fast_convoy->get_schedule()->entries[i];
										if(haltestelle_t::get_halt(fast_convoy_schedule_entry.pos, cnv->get_owner()) == self)
										{
											if(fast_convoy_schedule_entry.minimum_loading > 0 && !fast_convoy_schedule_entry.wait_for_time && fast_convoy_schedule_entry.waiting_time_shift == 0)
											{
												wait_for_faster_convoy = false;
												break;
											}
										}
									}
								}
							}
						}

						if(wait_for_faster_convoy)
						{
							schedule->increment_index(&index, &reverse);
							skipped = true;
							continue;
						}
					}
					else
					{
						wait_for_faster_convoy = false;
					}

					if(!bound_for_next_transfer && !wait_for_faster_convoy)
					{
						// The direct route is faster than the planned route:
						// update the next transfer to reflect this.
						next_to_load->set_zwischenziel(destination);
					}

					if (next_to_load->is_passenger() && next_to_load->g_class > 0 && cnv->get_classes_carried(goods_manager_t::INDEX_PAS)->get_count() > 1)
					{
						// For passengers, check whether to downgrade to or from this class.
						const sint64 journey_time_ticks = this_arrival_time - welt->get_ticks();
						const sint64 journey_time_seconds = welt->ticks_to_seconds(journey_time_ticks);

						const sint64 ideal_comfort_time_multiplier = (sint64)next_to_load->comfort_preference_percentage;
						const sint64 ideal_comfort_time = (journey_time_seconds * ideal_comfort_time_multiplier) / 100ll;

						uint8 best_class = g_class;

						if (!use_lower_classes)
						{
							// If there is overcrowding, load willy nilly: any class that the passengers can board will do.

							// The classes are called in non-deterministic order (because vehicles of any class may be in any order in the convoy).
							// We must therefore have an algorithm that deterministically decides to which, if any, class that any given passengers will downgrade.
							for (uint8 current_class = next_to_load->g_class; current_class > 0; current_class--)
							{
								// Find the ideal class, assuming space available in all classes
								if (current_class == g_class || cnv->get_classes_carried(goods_manager_t::INDEX_PAS)->is_contained(current_class))
								{
									const uint8 comfort_this_class = cnv->get_comfort(current_class);
									const uint32 max_tolerable_journey_this_comfort = welt->get_settings().max_tolerable_journey(comfort_this_class);

									if (max_tolerable_journey_this_comfort >= ideal_comfort_time)
									{
										best_class = current_class;
									}
								}
							}
						}

						if (g_class != best_class)
						{
							// Wait for a better class of accommodation
							schedule->increment_index(&index, &reverse);
							other_classes_available = true;
							continue;
						}
					}

					// Refuse to be overcrowded if alternative exists
					connexion * const next_connexion = connexions[catg_index]->get(check_halt);
					if(next_connexion  &&  overcrowded  &&  next_connexion->alternative_seats)
					{
						schedule->increment_index(&index, &reverse);
						skipped = true;
						continue;
					}

					// not too much?
					ware_t neu(*next_to_load);
					if(next_to_load->menge > requested_amount)
					{
						// not all can be loaded
						neu.menge = requested_amount;
						next_to_load->menge -= requested_amount;
						requested_amount = 0;
					}
					else
					{
						requested_amount -= next_to_load->menge;
						next_to_load->menge = 0; // leave an empty entry => will be deleted next time for performance
					}
					load.insert(neu);

					book(neu.menge, HALT_DEPARTED);
					resort_freight_info = true;

					if(requested_amount == 0)
					{
						goods_to_check.clear();
						break;
					}
				}
				// nothing there to load

				// if the schedule is mirrored and has reached its end, break
				// as the convoy will be returning this way later.
				if(schedule->is_mirrored() && (index == 0 || index == (schedule->get_count() - 1)))
				{
					break;
				}

				schedule->increment_index(&index, &reverse);
			}
		}
	}
	return skipped;
}


/**
 * It will calculate number of free seats in all other (not cnv) convoys at stop
 * @author Inkelyad, adapted from fetch_goods
 */
void haltestelle_t::update_alternative_seats(convoihandle_t cnv)
{
	if ( loading_here.get_count() > 1) {
		do_alternative_seats_calculation = true;
	}

	if (!do_alternative_seats_calculation )
	{
		return;
	}

	int catg_index = goods_manager_t::passengers->get_catg_index();
	FOR(connexions_map, const& iter, *(connexions[catg_index]))
	{
		iter.value->alternative_seats = 0;
	}

	if (loading_here.get_count() < 2 ) { // Alternatives don't exist, only one convoy here
		do_alternative_seats_calculation = false; // so we will not do clean-up again
		return;
	}

	for (slist_tpl<convoihandle_t>::iterator cnv_i = loading_here.begin(), end = loading_here.end();  cnv_i != end;  ++cnv_i)
	{
		if (!(*cnv_i).is_bound() || (*cnv_i) == cnv || ! (*cnv_i)->get_free_seats() )
		{
			continue;
		}
		const schedule_t *schedule = (*cnv_i)->get_schedule();
		const player_t *player = (*cnv_i)->get_owner();
		const uint8 count = schedule->get_count();

		// uses schedule->increment_index to iterate over stops
		uint8 index = schedule->get_current_stop();
		bool reverse = cnv->get_reverse_schedule();
		schedule->increment_index(&index, &reverse);

		while (index != schedule->get_current_stop()) {
			const halthandle_t plan_halt = haltestelle_t::get_halt(schedule->entries[index].pos, player);
			if(plan_halt == self)
			{
				// we will come later here again ...
				break;
			}
			if(plan_halt.is_bound() && plan_halt->get_pax_enabled())
			{
				connexion * const next_connexion = connexions[catg_index]->get(plan_halt);
				if (next_connexion) {
					next_connexion->alternative_seats += (*cnv_i)->get_free_seats();
				}
			}

			// if the schedule is mirrored and has reached its end, break
			// as the convoi will be returning this way later.
			if( schedule->is_mirrored() && (index==0 || index==(count-1)) ) {
				break;
			}
			schedule->increment_index(&index, &reverse);
		}
	}
}

uint32 haltestelle_t::get_ware_summe(const goods_desc_t *wtyp) const
{
	int sum = 0;
	const vector_tpl<ware_t> * warray = cargo[wtyp->get_catg_index()];
	if(warray!=NULL) {
		FOR(vector_tpl<ware_t>, const& i, *warray) {
			if (wtyp->get_index() == i.get_index()) {
				sum += i.menge;
			}
		}
	}
	return sum;
}



uint32 haltestelle_t::get_ware_fuer_zielpos(const goods_desc_t *wtyp, const koord zielpos) const
{
	const vector_tpl<ware_t> * warray = cargo[wtyp->get_catg_index()];
	if(warray!=NULL) {
		FOR(vector_tpl<ware_t>, const& ware, *warray) {
			if(wtyp->get_index()==ware.get_index()  &&  ware.get_zielpos()==zielpos) {
				return ware.menge;
			}
		}
	}
	return 0;
}

#ifdef CHECK_WARE_MERGE
bool haltestelle_t::vereinige_waren(const ware_t &ware) //"unite were" (Google)
{
	// pruefen ob die ware mit bereits wartender ware vereinigt werden kann
	// "examine whether the ware with software already waiting to be united" (Google)
	vector_tpl<ware_t> * warray = cargo[ware.get_desc()->get_catg_index()];
	if(warray != NULL)
	{
		FOR(vector_tpl<ware_t>, & tmp, *warray)
		{

			/*
			* OLD SYSTEM - did not take account of origins and timings when merging.
			*
			* // es wird auf basis von Haltestellen vereinigt
			* // prissi: das ist aber ein Fehler für all anderen Güter, daher Zielkoordinaten für alles, was kein passagier ist ...
			*
			* //it is based on uniting stops.
			* //prissi: but that is a mistake for all other goods, therefore, target coordinates for everything that is not a passenger ...
			* // (Google)
			*
			* if(ware.same_destination(tmp)) {
			*/

			// NEW SYSTEM
			// Adds more checks.
			// @author: jamespetts
			if(ware.can_merge_with(tmp))
			{
				if(  ware.get_zwischenziel().is_bound()  &&  ware.get_zwischenziel()!=self  )
				{
					// update route if there is newer route
					tmp.set_zwischenziel( ware.get_zwischenziel() );
				}

				// Merge waiting times.
				if(ware.menge > 0)
				{
					//The waiting time for ware will always be zero.
					tmp.arrival_time = welt->get_ticks() - ((welt->get_ticks() - tmp.arrival_time) * tmp.menge) / (tmp.menge + ware.menge);
				}

				tmp.menge += ware.menge;
				resort_freight_info = true;
				return true;
			}
		}
	}
	return false;
}
#endif


// put the ware into the internal storage
// take care of all allocation necessary
void haltestelle_t::add_ware_to_halt(ware_t ware, bool from_saved)
{
	// @author: jamespetts
	if(!from_saved)
	{
		ware.arrival_time = welt->get_ticks();
	}

	ware.set_last_transfer(self);

	// now we have to add the ware to the stop
	vector_tpl<ware_t> * warray = cargo[ware.get_desc()->get_catg_index()];
	if(warray==NULL)
	{
		// this type was not stored here before ...
		warray = new vector_tpl<ware_t>(4);
		cargo[ware.get_desc()->get_catg_index()] = warray;
	}
	resort_freight_info = true;
	if(!from_saved)
	{
		// the ware will be put into the first entry with menge==0
		FOR(vector_tpl<ware_t>, & i, *warray) {
			if (i.menge == 0) {
				i = ware;
				return;
			}
		}
		// here, if no free entries found
	}
	warray->append(ware);
}

void haltestelle_t::add_to_waiting_list(ware_t ware, sint64 ready_time)
{
	transferring_cargo_t tc;
	tc.ware = ware;
	tc.ready_time = ready_time;
#ifdef MULTI_THREAD
	transferring_cargoes[karte_t::passenger_generation_thread_number].append(tc);
#else
	transferring_cargoes[0].append(tc);
#endif
	resort_freight_info = true;
}

sint64 haltestelle_t::calc_ready_time(ware_t ware, bool arriving_from_vehicle, koord origin_pos) const
{
	sint64 ready_time = welt->get_ticks();

	if (ware.is_freight())
	{
		ready_time += world()->get_seconds_to_ticks(transshipment_time * 6);
	}
	else // Passengers
	{
		ready_time += world()->get_seconds_to_ticks(transfer_time * 6);
	}

	if (/*!arriving_from_vehicle &&*/ ware.get_ziel() == self)
	{
		uint16 distance;
		if (origin_pos == koord::invalid)
		{
			distance = shortest_distance(get_basis_pos(), ware.get_zielpos());
		}
		else
		{
			distance = shortest_distance(get_basis_pos(), origin_pos);
		}

		if (ware.is_freight())
		{
			const uint32 tenths_of_minutes = world()->walk_haulage_time_tenths_from_distance(distance);
			const sint64 carting_time = world()->get_seconds_to_ticks(tenths_of_minutes * 6);
			ready_time += carting_time;
		}
		else
		{
			const uint32 seconds = world()->walking_time_secs_from_distance(distance);
			const sint64 walking_time = world()->get_seconds_to_ticks(seconds);
			ready_time += walking_time;
		}
	}
	return ready_time;
}


/* same as liefere an, but there will be no route calculated,
 * since it hase be calculated just before
 * (execption: route contains us as intermediate stop)
 * @author prissi
 */
void haltestelle_t::starte_mit_route(ware_t ware, koord origin_pos)
{
#ifdef FORBID_PUBLIC_TRANSPORT
	return;
#endif
#ifdef DEBUG_SIMRAND_CALLS
	bool talk = !strcmp(get_name(), "Newton Abbot Railway Station");

	if (talk)
		dbg->message("haltestelle_t::starte_mit_route", "halt \"%s\", ware \"%s\": menge %u", get_name(), ware.get_desc()->get_name(), ware.menge);
#endif

	if(ware.get_ziel()==self) {
		if(fabrik_t::get_fab(ware.get_zielpos()))
		{
			add_to_waiting_list(ware, calc_ready_time(ware, false));
		}
		// already there: finished (may be happen with overlapping areas and returning passengers)
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
		if (talk)
			dbg->message("\t", "already finished");
#endif
#endif
		return;
	}

	// no valid next stops? Or we are the next stop?
	if(ware.get_zwischenziel() == self)
	{
		// Route cannot contain self as first transfer.
		if(find_route(ware) == UINT32_MAX_VALUE)
		{
			// no route found?
			return;
		}
	}

	if(ware.is_passenger()
	   && is_within_walking_distance_of(ware.get_zwischenziel())
	   && !get_preferred_convoy(ware.get_zwischenziel(), 0).is_bound()
	   && !get_preferred_line(ware.get_zwischenziel(), 0).is_bound())
	{
		// We allow walking from the first station because of the way passenger return journeys work;
		// they automatically start from the destination halt for the outgoing journey
		// This is a bug which should be fixed.  The passenger has already walked here,
		// and presumably does not wish to walk further... --neroden
		// If this is within walking distance of the next transfer, and there is not a faster way there, walk there.
		pedestrian_t::generate_pedestrians_at(get_basis_pos3d(), ware.menge);
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
		if (talk)
			dbg->message("\t", "walking to %s", ware.get_zwischenziel()->get_name());
#endif
#endif
		ware.set_last_transfer(self);
		ware.get_zwischenziel()->liefere_an(ware, 1);
		return;
	}

	add_to_waiting_list(ware, calc_ready_time(ware, false, origin_pos));

#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
	if (talk)
	{
		const vector_tpl<ware_t> * warray = cargo[ware.get_desc()->get_catg_index()];
		dbg->message("\t", "warray count %d", (*warray).get_count());
	}
#endif
#endif
	return;
}



/* Receives ware and tries to route it further on
 * if no route is found, it will be removed
 *
 * walked_between_stations defaults to 0; it should be set to 1 when walking here from another station
 * and incremented if this happens repeatedly
 *
 * @author prissi
 */
void haltestelle_t::liefere_an(ware_t ware, uint8 walked_between_stations)
{
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
	bool talk = !strcmp(get_name(), "Newton Abbot Railway Station");
#endif
#endif

	if (walked_between_stations > 4)
	{
		// With repeated walking between stations -- and as long as the walking takes no actual time
		// (which is a bug which should be fixed [and now has been fixed]) -- there is some danger of infinite loops.
		// Check for an excessively long number of walking steps.  If we have one, complain and fail.
		//
		// This was the 5th consecutive attempt to walk between stations.  Fail.
		// UPDATE December 2016: Walking between stations now does take actual time. Is this still needed?
#ifdef MULTI_THREAD
		int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
		dbg->warning("haltestelle_t::liefere_an()","%d %s delivered to %s has walked between too many consecutive stops: terminating early to avoid infinite loops", ware.menge, translator::translate(ware.get_name()), get_name() );
#ifdef MULTI_THREAD
		int error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
		assert(error == 0);
#endif
		return;
	}

	// no valid next stops?
	if(!ware.get_ziel().is_bound() || !ware.get_zwischenziel().is_bound())
	{
		if(ware.is_passenger() && shortest_distance(ware.get_zielpos(), get_next_pos(ware.get_zielpos())) < welt->get_settings().get_station_coverage() / 2)
		{
			// Passengers can walk to their destination if it is close enough.
			add_to_waiting_list(ware, calc_ready_time(ware, false));
		}

		// write a log entry and discard the goods
#ifdef MULTI_THREAD
		int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
		dbg->warning("haltestelle_t::liefere_an()","%d %s delivered to %s have no longer a route to their destination!", ware.menge, translator::translate(ware.get_name()), get_name() );
#ifdef MULTI_THREAD
		mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
		return;
	}

	const planquadrat_t* plan = welt->access(ware.get_zielpos());
	const grund_t* gr = plan ? plan->get_kartenboden() : NULL;
	gebaeude_t* const gb = gr ? gr->get_building() : NULL;
	fabrik_t* const fab = gb ? gb->get_fabrik() : NULL;
	if(!gb || ware.is_freight() && !fab)
	{
		// Destination factory has been deleted: write a log entry and discard the goods.
#ifdef MULTI_THREAD
		int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
		dbg->warning("haltestelle_t::liefere_an()","%d %s delivered to %s were intended for a factory that has been deleted.", ware.menge, translator::translate(ware.get_name()), get_name() );
#ifdef MULTI_THREAD
		mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
		return;
	}
	// Now we know, that "ware" has a vaild destination building "gb", which implies having a "plan".
	bool twice = false;
	arrived:
	// have we arrived?
	if(ware.get_ziel() == self)
	{
		// Arrived at destination stop. Check whether we can still reach the destination building.
		if(plan->is_connected(self))
		{
			// The destination tile is within the station coverage area
			add_to_waiting_list(ware, calc_ready_time(ware, true));
			return;
		}
		else
		{
			// If the destination tile is not within the station coverage area, check whether this
			// is a multi-tile building at least one tile of which *is* within the station coverage area.
			if(fab && fab_list.is_contained(fab))
			{
				// This is a connected factory: destination reached.
				add_to_waiting_list(ware, calc_ready_time(ware, true));
				return;
			}

			if(!ware.is_freight())
			{
				//  Not a factory: must check manually.
				FOR(slist_tpl<tile_t>, const& t, tiles)
				{
					gebaeude_t* check_building = t.grund->get_building();
					if(check_building->is_same_building(gb))
					{
						// This is a multi-tile building other than a factory,
						// part of which is in the coverage area.
						add_to_waiting_list(ware, calc_ready_time(ware, true));
						return;
					}
				}

				// Checking this halt's tiles did not work. Try a reverse check.
				minivec_tpl<const planquadrat_t*> const &tile_list = gb->get_tiles();
				FOR(minivec_tpl<const planquadrat_t*>, const& current_tile, tile_list)
				{
					if (current_tile->is_connected(self))
					{
						add_to_waiting_list(ware, calc_ready_time(ware, true));
						return;
					}
				}
			}
		}
	}
	uint16 straight_line_distance_destination;
	bool destination_is_within_coverage;

	const uint32 route_time = find_route(ware);
	if (ware.get_ziel() == self)
	{
#ifdef MULTI_THREAD
		int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
		DBG_MESSAGE("haltestelle_t::liefere_an()", "%s has discovered that it is quicker to walk to its destination from %s than take its previously planned route.", translator::translate(ware.get_name()), get_name());
#ifdef MULTI_THREAD
		mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
		if (!twice)
		{
			twice = true;
			goto arrived;
		}
		else
		{
			// Otherwise, bad things happen.
			add_to_waiting_list(ware, calc_ready_time(ware, true));
			return;
		}
	}

	if(route_time == UINT32_MAX_VALUE)
	{
		if(ware.is_passenger() && is_within_walking_distance_of(ware.get_zwischenziel()) && ware.get_zwischenziel() != self && ware.get_zwischenziel().is_bound())
		{
			// There is no longer a route, but we can walk to the next stop
			goto walking;
		}
		// target no longer reachable => delete

		//INT_CHECK("simhalt 1364");

#ifdef MULTI_THREAD
		int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
		// target halt no longer there => delete and remove from fab in transit
		fabrik_t::update_transit( ware, false );
#ifdef MULTI_THREAD
		mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
		return;
	}

	if(ware.is_passenger())
	{
		// Check whether, on arriving, passengers can walk to their next stop or ultimate destination more quickly than waiting for the next convoy.
		straight_line_distance_destination = shortest_distance(get_init_pos(), ware.get_zielpos());
		destination_is_within_coverage = straight_line_distance_destination <= (welt->get_settings().get_station_coverage() / 2);
		if(is_within_walking_distance_of(ware.get_ziel()) || is_within_walking_distance_of(ware.get_zwischenziel()) || destination_is_within_coverage)
		{
			convoihandle_t dummy;
			const sint64 best_arrival_time_destination_stop = calc_earliest_arrival_time_at(ware.get_ziel(), dummy, ware.get_desc()->get_catg_index(), ware.g_class);
			sint64 best_arrival_time_transfer = ware.get_zwischenziel() != ware.get_ziel() ? calc_earliest_arrival_time_at(ware.get_zwischenziel(), dummy, ware.get_desc()->get_catg_index(), ware.g_class) : SINT64_MAX_VALUE;

			const sint64 arrival_after_walking_to_destination = welt->get_seconds_to_ticks(welt->walking_time_tenths_from_distance((uint32)straight_line_distance_destination) * 6) + welt->get_ticks();

			const uint16 straight_line_distance_to_next_transfer = shortest_distance(get_init_pos(), ware.get_zwischenziel()->get_next_pos(get_next_pos(ware.get_zwischenziel()->get_basis_pos())));
			const sint64 arrival_after_walking_to_next_transfer = welt->get_seconds_to_ticks(welt->walking_time_tenths_from_distance((uint32)straight_line_distance_to_next_transfer) * 6) + welt->get_ticks();

			sint64 extra_time_to_ultimate_destination = 0;
			if(best_arrival_time_transfer < SINT64_MAX_VALUE)
			{
				// This cannot be evaluated properly if we cannot get the arrival time.
				if(best_arrival_time_destination_stop < best_arrival_time_transfer)
				{
					ware.set_zwischenziel(ware.get_ziel());
					best_arrival_time_transfer = best_arrival_time_destination_stop;
					const uint16 distance_destination_stop_to_destination = shortest_distance(ware.get_zielpos(), ware.get_ziel()->get_next_pos(ware.get_zielpos()));
					extra_time_to_ultimate_destination = welt->get_seconds_to_ticks(welt->walking_time_tenths_from_distance((uint32)distance_destination_stop_to_destination) * 6);
				}

				if(destination_is_within_coverage && arrival_after_walking_to_destination < best_arrival_time_transfer + extra_time_to_ultimate_destination)
				{
					add_to_waiting_list(ware, calc_ready_time(ware, false));
					return;
				}

				if(arrival_after_walking_to_next_transfer < best_arrival_time_transfer)
				{
					goto walking;
				}
			}
		}

		if (!get_preferred_convoy(ware.get_zwischenziel(), 0).is_bound()
		    && !get_preferred_line(ware.get_zwischenziel(), 0).is_bound())
		{
			// If this is within walking distance of the next transfer, and there is not a faster way there, walk there.
		walking:
			pedestrian_t::generate_pedestrians_at(get_basis_pos3d(), ware.menge);
			ware.set_last_transfer(self);
	#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
			if (talk)
				dbg->message("haltestelle_t::liefere_an", "%d walk to station \"%s\" cargo[0].count %d", ware.menge, ware.get_zwischenziel()->get_name(), get_warray(0)->get_count());
#endif
#endif
			ware.get_zwischenziel()->liefere_an(ware, walked_between_stations + 1);
			return;
		}
	}

	add_to_waiting_list(ware, calc_ready_time(ware, false));
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
	if (talk)
		dbg->message("haltestelle_t::liefere_an", "%d waiting for transfer to station \"%s\" cargo[0].count %d", ware.menge, ware.get_zwischenziel()->get_name(), get_warray(0)->get_count());
#endif
#endif
	return;
}

void haltestelle_t::info(cbuffer_t & buf, bool dummy) const
{
	if( has_character( 0x263A ) ) {
		utf8 happy[4], unhappy[4];
		happy[ utf16_to_utf8( 0x263A, happy ) ] = 0;
		unhappy[ utf16_to_utf8( 0x2639, unhappy ) ] = 0;
		buf.printf(translator::translate("Passengers %d %s, %d %s, %d no route, %d too slow"), get_pax_happy(), happy, get_pax_unhappy(), unhappy, get_pax_no_route(), get_pax_too_slow());
	}
	else {
		buf.printf(translator::translate("Passengers %d %c, %d %c, %d no route, %d too slow"), get_pax_happy(), 30, get_pax_unhappy(), 31, get_pax_no_route(), get_pax_too_slow());
	}
	buf.append("\n\n");
}


/**
 * @param buf the buffer to fill
 * @return Goods description text (buf)
 * @author Hj. Malthaner
 */
void haltestelle_t::get_freight_info(cbuffer_t & buf)
{
	if (resort_freight_info)
	{
		// resort only inf absolutely needed ...
		resort_freight_info = false;
		buf.clear();

		for (unsigned i = 0; i < goods_manager_t::get_max_catg_index(); i++)
		{
			const vector_tpl<ware_t> * warray = cargo[i];
			if (warray)
			{
				freight_list_sorter_t::sort_freight(*warray, buf, (freight_list_sorter_t::sort_mode_t)sortierung, NULL, "waiting", NULL, NULL, NULL);
			}
		}

		buf.append("\n");
		if (get_transferring_cargoes_count() > 0)
		{
			buf.printf("%s:\n", translator::translate("transfers"));
		}
		vector_tpl<ware_t> ware_transfers;
		ware_t ware;
		const sint64 current_time = welt->get_ticks();
#ifdef MULTI_THREAD
		sint32 po = world()->get_parallel_operations();
#else
		sint32 po = 1;
#endif
		for (sint32 i = 0; i < po; i++)
		{
			FOR(vector_tpl<transferring_cargo_t>, tc, transferring_cargoes[i])
			{
				ware = tc.ware;
				ware_transfers.append(ware);
			}
		}
		// show new info
		freight_list_sorter_t::sort_freight(ware_transfers, buf, (freight_list_sorter_t::sort_mode_t)sortierung, NULL, "transferring", NULL, NULL, NULL);
	}
}

void haltestelle_t::get_short_freight_info(cbuffer_t & buf) const
{
	bool got_one = false;

	for(unsigned int i=0; i<goods_manager_t::get_count(); i++) {
		const goods_desc_t *wtyp = goods_manager_t::get_info(i);
		if(gibt_ab(wtyp)) {

			// ignore goods with sum=zero
			const int summe=get_ware_summe(wtyp);
			if(summe>0) {

				if(got_one) {
					buf.append(", ");
				}

				buf.printf("%d%s %s", summe, translator::translate(wtyp->get_mass()), translator::translate(wtyp->get_name()));

				got_one = true;
			}
		}
	}

	if(got_one) {
		buf.append(" ");
		buf.append(translator::translate("waiting"));
		buf.append("\n");
	}
	else {
		buf.append(translator::translate("no goods waiting"));
		buf.append("\n");
	}
}



void haltestelle_t::show_info()
{
	create_win( new halt_info_t(self), w_info, magic_halt_info + self.get_id() );
}


sint64 haltestelle_t::calc_maintenance() const
{
	sint64 maintenance = 0;
	FOR(slist_tpl<tile_t>, const& i, tiles)
	{
		if (gebaeude_t* const gb = i.grund->find<gebaeude_t>())
		{
			const building_desc_t* desc = gb->get_tile()->get_desc();
			if(desc->get_base_maintenance() == PRICE_MAGIC)
			{
				// Default value - no specific maintenance set. Use the old method
				maintenance += welt->get_settings().maint_building * desc->get_level();
			}
			else
			{
				// New method - get the specified factor.
				maintenance += desc->get_maintenance();
			}
		}
	}
	return maintenance;
}


// changes this to a public transfer exchange stop
bool haltestelle_t::make_public_and_join(player_t *player)
{
	player_t *public_owner = welt->get_public_player();
	const bool compensate = player == public_owner;
	slist_tpl<halthandle_t> joining;

	// only something to do if not yet owner ...
	if(owner != public_owner)
	{
		// First run through to see if we can afford this.
		sint64 total_charge = 0;
		FOR(slist_tpl<tile_t>, const& i, tiles)
		{
			grund_t* const gr = i.grund;
			gebaeude_t* gb = gr->find<gebaeude_t>();
			if(gb)
			{
				const building_desc_t* desc = gb->get_tile()->get_desc();
				sint32 costs;
				if(desc->get_base_maintenance() == PRICE_MAGIC)
				{
					// Default value - no specific maintenance set. Use the old method
					costs = welt->get_settings().maint_building * desc->get_level();
				}
				else
				{
					// New method - get the specified factor.
					costs = desc->get_maintenance();
				}

				if(!compensate)
				{
					// Sixty months of maintenance is the payment to the public player for this
					total_charge += welt->calc_adjusted_monthly_figure(costs * 60);
				}
				else
				{
					// If the public player itself is doing this, it pays a normal price.
					total_charge += welt->calc_adjusted_monthly_figure(costs);
				}
				if(!player->can_afford(total_charge))
				{
					// We can't afford this.  Bail out.
					return false;
				}
			}
		}
		// Now run through to actually transfer ownership
		// and recalculate maintenance; must do maintenance here, not above,
		// in order to properly assign it by waytype
		FOR(slist_tpl<tile_t>, const& i, tiles)
		{
			grund_t* const gr = i.grund;
			gebaeude_t* gb = gr->find<gebaeude_t>();
			if(gb)
			{
				player_t *gb_player = gb->get_owner();
				const building_desc_t* desc = gb->get_tile()->get_desc();
				sint32 costs;
				if(desc->get_base_maintenance() == PRICE_MAGIC)
				{
					// Default value - no specific maintenance set. Use the old method
					costs = welt->get_settings().maint_building * desc->get_level();
				}
				else
				{
					// New method - get the specified factor.
					costs = desc->get_maintenance();
				}
				player_t::add_maintenance( gb_player, -costs, gb->get_waytype() );
				gb->set_owner(public_owner);
				gb->set_flag(obj_t::dirty);
				player_t::add_maintenance(public_owner, costs, gb->get_waytype() );
				if(!compensate)
				{
					// Player is voluntarily turning this over to the public player:
					// pay a fee for the public player for future maintenance.
					sint64 charge = welt->calc_adjusted_monthly_figure(costs * welt->get_settings().cst_make_public_months);
					player_t::book_construction_costs(player,         -charge, get_basis_pos(), gb->get_waytype());
					player_t::book_construction_costs(public_owner, charge, koord::invalid, gb->get_waytype());
				}
				else
				{
					// The public player itself is acquiring this stop compulsorily, so pay compensation.
					sint64 charge = welt->calc_adjusted_monthly_figure(costs);
					player_t::book_construction_costs(player, -charge, get_basis_pos(), gb->get_waytype());
					player_t::book_construction_costs(owner, charge, koord::invalid, gb->get_waytype());
				}
			}
			// ok, valid start, now we can join them
			// First search the same square
			const planquadrat_t *pl = welt->access(gr->get_pos().get_2d());
			for(  uint8 i=0;  i < pl->get_boden_count();  i++  ) {
				halthandle_t my_halt = pl->get_boden_bei(i)->get_halt();
				if(  my_halt.is_bound()  &&  my_halt->get_owner()==public_owner  &&  !joining.is_contained(my_halt)  ) {
					joining.append(my_halt);
				}
			}
			// Now neighboring squares
			for( uint8 i=0;  i<8;  i++  ) {
				const planquadrat_t *pl2 = welt->access(gr->get_pos().get_2d()+koord::neighbours[i]);
				if(  pl2  ) {
					for(  uint8 i=0;  i < pl2->get_boden_count();  i++  ) {
						halthandle_t my_halt = pl2->get_boden_bei(i)->get_halt();
						if(  my_halt.is_bound()  &&  my_halt->get_owner()==public_owner  &&  !joining.is_contained(my_halt)  ) {
							joining.append(my_halt);
						}
					}
				}
			}
		}
		// transfer ownership
		owner = public_owner;
	}

	// set name to name of first public stop
	if(  !joining.empty()  ) {
		set_name( joining.front()->get_name());
	}

	while(!joining.empty()) {
		// join this halt with me
		halthandle_t halt = joining.remove_first();

		// now with the second stop
		while(  halt.is_bound()  &&  halt!=self  ) {
			// add statistics
			for(  int month=0;  month<MAX_MONTHS;  month++  ) {
				for(  int type=0;  type<MAX_HALT_COST;  type++  ) {
					financial_history[month][type] += halt->financial_history[month][type];
					halt->financial_history[month][type] = 0;	// to avoid counting twice
				}
			}

			// we always take the first remaining tile and transfer it => more safe
			koord3d t = halt->get_basis_pos3d();
			grund_t *gr = welt->lookup(t);
			gebaeude_t* gb = gr->find<gebaeude_t>();
			if(gb) {
				// there are also water tiles, which may not have a building
				player_t *gb_player=gb->get_owner();
				if(public_owner!=gb_player) {
					sint32 costs;

					if(gb->get_tile()->get_desc()->get_base_maintenance() == PRICE_MAGIC)
					{
						// Default value - no specific maintenance set. Use the old method
						costs = welt->get_settings().maint_building * gb->get_tile()->get_desc()->get_level();
					}
					else
					{
						// New method - get the specified factor.
						costs = gb->get_tile()->get_desc()->get_maintenance();
					}

					player_t::add_maintenance( gb_player, -costs, gb->get_waytype() );

					player_t::book_construction_costs(gb_player,         costs*60, gr->get_pos().get_2d(), gb->get_waytype());
					player_t::book_construction_costs(public_owner, -costs*60, koord::invalid, gb->get_waytype());

					gb->set_owner(public_owner);
					gb->set_flag(obj_t::dirty);
					player_t::add_maintenance(public_owner, costs, gb->get_waytype() );
				}
			}
			// transfer tiles to us
			halt->rem_grund(gr);
			add_grund(gr);
			// and check for existence
			if(!halt->existiert_in_welt()) {
				// transfer goods
				halt->transfer_goods(self);

				destroy(halt);
			}
		}
	}

	// tell the world of it ...
	if(  player->get_player_nr()!=1  &&  env_t::networkmode  ) {
		cbuffer_t buf;
		buf.printf( translator::translate("%s at (%i,%i) now public stop."), get_name(), get_basis_pos().x, get_basis_pos().y );
		welt->get_message()->add_message( buf, get_basis_pos(), message_t::ai, PLAYER_FLAG|player->get_player_nr(), IMG_EMPTY );
	}

	recalc_station_type();
	return true;
}


void haltestelle_t::transfer_goods(halthandle_t halt)
{
	if (!self.is_bound() || !halt.is_bound()) {
		return;
	}
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
	bool talk = !strcmp(get_name(), "Newton Abbot Railway Station") || !strcmp(halt->get_name(), "Newton Abbot Railway Station");
#endif
#endif
	// transfer goods to halt
	for(uint8 i=0; i<goods_manager_t::get_max_catg_index(); i++) {
		const vector_tpl<ware_t> * warray = cargo[i];
		if (warray) {
			FOR(vector_tpl<ware_t>, const& j, *warray) {
				halt->add_ware_to_halt(j);
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
				if (talk)
					dbg->message("haltestelle_t::transfer_goods", "%d transfer from station \"%s\"(warr cnt %d) to \"%s\"(warr cnt %d)",
					    j.menge, get_name(), get_warray(i)->get_count(), halt->get_name(), halt->get_warray(i)->get_count());
#endif
#endif
			}
			delete cargo[i];
			cargo[i] = NULL;
		}
	}
}


// private helper function for recalc_station_type()
void haltestelle_t::add_to_station_type( grund_t *gr )
{
	// init in any case ...
	if(  tiles.empty()  ) {
		capacity[0] = 0;
		capacity[1] = 0;
		capacity[2] = 0;
		enables &= CROWDED;	// clear flags
		station_type = invalid;
	}

	const gebaeude_t* gb = gr->find<gebaeude_t>();
	const building_desc_t *desc=gb?gb->get_tile()->get_desc():NULL;

	if(gr->is_water() && gb) {
		// may happen around oil rigs and so on
		station_type |= dock;
		// for water factories
		if(desc) {
			// enabled the matching types
			enables |= desc->get_enabled();
			if (welt->get_settings().is_separate_halt_capacities()) {
				if(desc->get_enabled()&1) {
					capacity[0] += desc->get_capacity();
				}
				if(desc->get_enabled()&2) {
					capacity[1] += desc->get_capacity();
				}
				if(desc->get_enabled()&4) {
					capacity[2] += desc->get_capacity();
				}
			}
			else {
				// no separate capacities: sum up all
				capacity[0] += desc->get_capacity();
				capacity[2] = capacity[1] = capacity[0];
			}
		}
		return;
	}

	if(desc==NULL) {
		// no desc, but solid ground?!?
		dbg->error("haltestelle_t::get_station_type()","ground belongs to halt but no desc?");
		return;
	}
	//if(desc) DBG_DEBUG("haltestelle_t::get_station_type()","desc(%p)=%s",desc,desc->get_name());

	// there is only one loading bay ...
	switch (desc->get_type()) {
		case building_desc_t::dock:
		case building_desc_t::flat_dock: station_type |= dock;         break;

		// two ways on ground can only happen for tram tracks on streets, there buses and trams can stop
		case building_desc_t::generic_stop:
			switch (desc->get_extra()) {
				case road_wt:
					station_type |= (desc->get_enabled()&3)!=0 ? busstop : loadingbay;
					if (gr->has_two_ways()) { // tram track on street
						station_type |= tramstop;
					}
					break;
				case water_wt:       station_type |= dock;            break;
				case air_wt:         station_type |= airstop;         break;
				case monorail_wt:    station_type |= monorailstop;    break;
				case track_wt:       station_type |= railstation;     break;
				case tram_wt:
					station_type |= tramstop;
					if (gr->has_two_ways()) { // tram track on street
						station_type |= (desc->get_enabled()&3)!=0 ? busstop : loadingbay;
					}
					break;
				case maglev_wt:      station_type |= maglevstop;      break;
				case narrowgauge_wt: station_type |= narrowgaugestop; break;
				default: ;
			}
			break;
		default: ;
	}

	// enabled the matching types
	enables |= desc->get_enabled();
	if (welt->get_settings().is_separate_halt_capacities()) {
		if(desc->get_enabled()&1) {
			capacity[0] += desc->get_capacity();
		}
		if(desc->get_enabled()&2) {
			capacity[1] += desc->get_capacity();
		}
		if(desc->get_enabled()&4) {
			capacity[2] += desc->get_capacity();
		}
	}
	else {
		// no separate capacities: sum up all
		capacity[0] += desc->get_capacity();
		capacity[2] = capacity[1] = capacity[0];
	}
}

/*
 * recalculated the station type(s)
 * since it iterates over all ground, this is better not done too often, because line management and station list
 * queries this information regularly; Thus, we do this, when adding new ground
 * This recalculates also the capacity from the building levels ...
 * @author Weber/prissi
 */
void haltestelle_t::recalc_station_type()
{
	capacity[0] = 0;
	capacity[1] = 0;
	capacity[2] = 0;
	enables &= CROWDED;	// clear flags
	station_type = invalid;

	// iterate over all tiles
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		grund_t* const gr = i.grund;
		add_to_station_type( gr );
	}
	// and set halt info again
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		i.grund->set_halt( self );
	}
	recalc_status();
}

// necessary to load pre0.99.13 savegames
void warenziel_rdwr(loadsave_t *file)
{
	koord ziel;
	ziel.rdwr(file);
	char tn[256];
	file->rdwr_str(tn, lengthof(tn));
}

void haltestelle_t::rdwr(loadsave_t *file)
{
	xml_tag_t h( file, "haltestelle_t" );

	sint32 owner_n;
	koord3d k;
#ifdef DEBUG_SIMRAND_CALLS
	loading = file->is_loading();
#endif
	// will restore halthandle_t after loading
	if(file->get_version() > 110005)
	{
		if(file->is_saving())
		{
			if(!self.is_bound())
			{
				// Something has gone a bit wrong here, as the handle to self is not bound.
				// Disabled, as this is apparently undefined.
				/*if(!this)
				{
					// Probably superfluous, but best to be sure that this is really not a dud pointer.
					dbg->error("void haltestelle_t::rdwr(loadsave_t *file)", "Handle to self not bound when saving a halt");
					return;
				}*/
				if(self.get_rep() != this)
				{
					uint16 id = self.get_id();
					self = halthandle_t(this, id);
				}
			}
			uint16 halt_id = self.is_bound() ? self.get_id() : 0;
			file->rdwr_short(halt_id);
		}
		else
		{
			uint16 halt_id;
			file->rdwr_short(halt_id);
			self.set_id(halt_id);
			if((file->get_extended_version() >= 10 || file->get_extended_version() == 0) && halt_id != 0)
			{
				self = halthandle_t(this, halt_id);
			}
			else
			{
				self = halthandle_t(this);
			}
		}
	}
	else
	{
		if (file->is_loading())
		{
			self = halthandle_t(this);
		}
	}

	if(file->is_saving()) {
		owner_n = welt->sp2num( owner );
	}

	if(file->get_version()<99008) {
		init_pos.rdwr( file );
	}
	file->rdwr_long(owner_n);

	if(file->get_version()<=88005) {
		bool dummy;
		file->rdwr_bool(dummy); // pax
		file->rdwr_bool(dummy); // mail
		file->rdwr_bool(dummy);	// ware
	}

	if(file->is_loading()) {
		owner = welt->get_player(owner_n);
		k.rdwr( file );
		while(k!=koord3d::invalid) {
			grund_t *gr = welt->lookup(k);
			if(!gr) {
				dbg->error("haltestelle_t::rdwr()", "invalid position %s", k.get_str() );
				gr = welt->lookup_kartenboden(k.get_2d());
				if(gr)
				{
					dbg->error("haltestelle_t::rdwr()", "setting to %s", gr->get_pos().get_str() );
				}
			}
			// during loading and saving halts will be referred by their base position
			// so we may already be defined ...
			if(gr && gr->get_halt().is_bound()) {
				dbg->warning( "haltestelle_t::rdwr()", "bound to ground twice at (%i,%i)!", k.x, k.y );
			}
			// prissi: now check, if there is a building -> we allow no longer ground without building!
			const gebaeude_t* gb = gr ? gr->find<gebaeude_t>() : NULL;
			const building_desc_t *desc=gb ? gb->get_tile()->get_desc():NULL;
			if(desc)
			{
				add_grund( gr, false /*do not relink factories now*/, !(file->get_extended_version() >= 13 || file->get_extended_revision() >= 21) /*do not recalculate nearby halts now unless loading an older version*/  );
				// Factories will be re-linked on loading
			}
			else {
				dbg->warning("haltestelle_t::rdwr()", "will no longer add ground without building at %s!", k.get_str() );
			}
			k.rdwr( file );
		}
	}
	else {
		FOR(slist_tpl<tile_t>, const& i, tiles) {
			k = i.grund->get_pos();
			k.rdwr( file );
		}
		k = koord3d::invalid;
		k.rdwr( file );
	}

	// BG, 07-MAR-2010: the number of goods categories has to be read from the savegame,
	// as goods and categories can be added by the user and thus do not depend on file
	// version and therefore not predicatable by simutrans.

	uint8 max_catg_count_game = goods_manager_t::get_max_catg_index();
	uint8 max_catg_count_file = max_catg_count_game;

	if(file->get_extended_version() >= 11)
	{
		// Version 11 and above - the maximum category count is saved with the game.
		file->rdwr_byte(max_catg_count_file);
	}

	init_pos = tiles.empty() ? koord::invalid : tiles.front().grund->get_pos().get_2d();
	if(file->is_saving())
	{
		const char *s;
		for(unsigned i=0; i<max_catg_count_file; i++)
		{
			vector_tpl<ware_t> *warray = cargo[i];
			uint32 ware_count = 1;

			if(warray)
			{
				s = "y";	// needs to be non-empty
				file->rdwr_str(s);
				bool has_uint16_count;
				if(file->get_version() <= 112002 || file->get_extended_version() <= 10)
				{
					const uint32 count = warray->get_count();
					uint16 short_count = min(count, 65535u);
					file->rdwr_short(short_count);
					has_uint16_count = true;
				}
				else
				{
					// Extended version 11 and above - very large/busy halts might
					// have a count > 65535, so use the proper uint32 value.

					// In previous versions, the use of "short" lead to corrupted saved
					// games when the value was larger than 32,767.

					uint32 count = warray->get_count();
					file->rdwr_long(count);
					has_uint16_count = false;
				}
				FOR(vector_tpl<ware_t>, & ware, *warray)
				{
					if(has_uint16_count && ware_count++ > 65535)
					{
						// Discard ware packets > 65535 if the version is < 11, as trying
						// to save greater than this number will corrupt the save.
						ware.menge = 0;
					}
					else
					{
						ware.rdwr(file);
					}
				}
			}
		}
		s = "";
		file->rdwr_str(s);

#ifdef DEBUG_SIMRAND_CALLS
		if (cargo[0])
		{
			if (!strcmp(get_name(), "Newton Abbot Railway Station"))
			{
				dbg->message("haltestelle_t::rdwr", "at stop \"%s\" cargo[0]->get_count() is %u ", get_name(), cargo[0]->get_count());
				//for (int i = 0; i < cargo[0]->get_count(); ++i)
				//{
				//	char buf[16];
				//	const ware_t &ware = (*cargo[0])[i];
				//	sprintf(buf, "% 8u)", i);
				//	dbg->message(buf, "%u to %s", ware.menge, ware.get_ziel()->get_name());
				//}
				//int x = 0;
			}
		}
#endif
	}
	else
	{
		// restoring all goods in the station
		char s[256];
		file->rdwr_str(s, lengthof(s));
		while(*s) {
			uint32 count;
			if(  file->get_version() <= 112002  || (file->get_extended_version() > 0 && file->get_extended_version() <= 10) ) {
				// Older versions stored only 16-bit count values.
				uint16 scount;
				file->rdwr_short(scount);
				count = scount;
			}
			else {
				file->rdwr_long(count);
			}
			if(count>0) {
				for(  uint32 i = 0;  i < count;  i++  ) {
					// add to internal storage (use this function, since the old categories were different)
					ware_t ware(file);
					if( ware.get_desc() && ware.menge>0 && welt->is_within_limits(ware.get_zielpos()) ) {
						add_ware_to_halt(ware, true);
						/*
						 * It's very easy for in-transit information to get corrupted,
						 * if an intermediate program version fails to compute it right.
						 * So *always* compute it fresh.
						 */
						fabrik_t::update_transit( ware, true );
					}
					else if(  ware.menge>0  )
					{
						if(  ware.get_desc()  ) {
							dbg->error( "haltestelle_t::rdwr()", "%i of %s to %s ignored!", ware.menge, ware.get_name(), ware.get_zielpos().get_str() );
						}
						else {
							dbg->error( "haltestelle_t::rdwr()", "%i of unknown to %s ignored!", ware.menge, ware.get_zielpos().get_str() );
						}
					}
				}
			}
			file->rdwr_str(s, lengthof(s));
		}

		// old games save the list with stations
		// however, we have to rebuilt them anyway for the new format
		if(file->get_version()<99013) {
			uint16 count;
			file->rdwr_short(count);

			for(int i=0; i<count; i++)
			{
				if(file->is_loading())
				{
					warenziel_rdwr(file);

					char dummy[256];
					file->rdwr_str(dummy,256);
				}
			}

		}


#ifdef DEBUG_SIMRAND_CALLS
		if (cargo[0])
		{
			if (!strcmp(get_name(), "Newton Abbot Railway Station"))
			{
				dbg->message("haltestelle_t::rdwr", "at stop \"%s\" cargo[0]->get_count() is %u ", get_name(), cargo[0]->get_count());
				//for (int i = 0; i < cargo[0]->get_count(); ++i)
				//{
				//	char buf[16];
				//	const ware_t &ware = (*cargo[0])[i];
				//	sprintf(buf, "% 8u)", i);
				//	dbg->message(buf, "%u", ware.menge);
				//}
				//int x = 0;
			}
		}
#endif
	}

	if(file->get_extended_version() >= 5)
	{
		for (int j = 0; j < 8 /*MAX_HALT_COST*/; j++)
		{
			for (int k = MAX_MONTHS	- 1; k >= 0; k--)
			{
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
	}
	else
	{
		// Earlier versions did not have pax_too_slow
		for (int j = 0; j < 8 /*MAX_HALT_COST*/; j++)
		{
			for (int k = MAX_MONTHS - 1; k >= 0; k--)
			{
				if(j == 7)
				{
					// Walked passengers in Standard
					// (Extended stores these in cities, not stops)
					if(file->get_version() >= 111001)
					{
						sint64 dummy = 0;
						file->rdwr_longlong(dummy);
					}
					else
					{
						continue;
					}
				}
				else
				{
					file->rdwr_longlong(financial_history[k][j]);
				}
			}
		}
		for (int k = MAX_MONTHS - 1; k >= 0; k--)
		{
			if(file->is_loading())
			{
				financial_history[k][HALT_TOO_SLOW] = 0;
			}
		}
	}

	if(file->get_extended_version() >= 2)
	{
		for(int i = 0; i < max_catg_count_file; i ++)
		{
			if(file->is_saving())
			{
				uint8 passenger_classes;
				uint8 mail_classes;

				if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 22)
				{
					passenger_classes = goods_manager_t::passengers->get_number_of_classes();
					mail_classes = goods_manager_t::mail->get_number_of_classes();

					file->rdwr_byte(passenger_classes);
					file->rdwr_byte(mail_classes);
				}
				else
				{
					passenger_classes = 1;
					mail_classes = 1;
				}

				uint8 class_count_this_catg;
				if (i == goods_manager_t::INDEX_PAS)
				{
					class_count_this_catg = passenger_classes;
				}
				else if (i == goods_manager_t::INDEX_MAIL)
				{
					class_count_this_catg = mail_classes;
				}
				else
				{
					class_count_this_catg = 1;
				}

				for (uint8 j = 0; j < class_count_this_catg; j++)
				{
					uint16 halts_count;
					halts_count = waiting_times[i][j].get_count();
					file->rdwr_short(halts_count);
					halthandle_t halt;

					FOR(waiting_time_map, &iter, waiting_times[i][j])
					{
						uint16 id = iter.key;

						if (file->get_extended_version() >= 10)
						{
							file->rdwr_short(id);
						}
						else
						{
							halt.set_id(id);
							koord save_koord = koord::invalid;
							if (halt.is_bound())
							{
								save_koord = halt->get_basis_pos();
							}
							save_koord.rdwr(file);
						}

						uint8 waiting_time_count = iter.value.times.get_count();
						file->rdwr_byte(waiting_time_count);
						ITERATE(iter.value.times, n)
						{
							// Store each waiting time
							if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
							{
								uint32 current_time = iter.value.times.get_element(n);
								file->rdwr_long(current_time);
							}
							else
							{
								uint32 ct = iter.value.times.get_element(n);
								if (ct == UINT32_MAX_VALUE)
								{
									ct = 65535;
								}
								else if (ct > 65534)
								{
									ct = 65534;
								}
								uint16 current_time = (uint16)ct;
								file->rdwr_short(current_time);
							}
						}

						if (file->get_extended_version() >= 9)
						{
							waiting_time_set wt = iter.value;
							file->rdwr_byte(wt.month);
						}
					}
					halt.set_id(0);
				}
			}

			else // Loading
			{
				uint8 passenger_classes;
				uint8 mail_classes;

				if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 22)
				{
					file->rdwr_byte(passenger_classes);
					file->rdwr_byte(mail_classes);
				}
				else
				{
					passenger_classes = 1;
					mail_classes = 1;
				}

				uint8 class_count_this_catg;
				uint8 actual_class_count_this_catg;

				if (i == goods_manager_t::INDEX_PAS)
				{
					class_count_this_catg = passenger_classes;
					actual_class_count_this_catg = goods_manager_t::passengers->get_number_of_classes();
				}
				else if (i == goods_manager_t::INDEX_MAIL)
				{
					class_count_this_catg = mail_classes;
					actual_class_count_this_catg = goods_manager_t::mail->get_number_of_classes();
				}
				else
				{
					class_count_this_catg = 1;
					actual_class_count_this_catg = 1;
				}

				for (uint8 j = 0; j < class_count_this_catg; j++)
				{

					waiting_times[i][j].clear();
					uint16 halts_count;
					file->rdwr_short(halts_count);
					uint16 id = 0;
					for (uint16 k = 0; k < halts_count; k++)
					{
						if (file->get_extended_version() >= 10)
						{
							file->rdwr_short(id);
						}
						else
						{
							// Extended versions 2-9, loading
							koord halt_position;
							halt_position.rdwr(file);
							const planquadrat_t* plan = welt->access(halt_position);
							if (plan)
							{
								for (uint8 i = 0; i < plan->get_boden_count(); i++) {
									halthandle_t my_halt = plan->get_boden_bei(i)->get_halt();
									if (my_halt.is_bound()) {
										// Stop at first halt found (always prefer ground level)
										id = my_halt.get_id();
										break;
									}
								}
							}
						}

						fixed_list_tpl<uint32, 32> list;
						uint8 month;
						waiting_time_set set;
						uint8 waiting_time_count;
						file->rdwr_byte(waiting_time_count);
						for (uint8 j = 0; j < waiting_time_count; j++)
						{
							uint32 current_time;
							if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
							{
								file->rdwr_long(current_time);
							}
							else
							{
								uint16 old_current_time;
								file->rdwr_short(old_current_time);
								if (old_current_time == 65535)
								{
									current_time = UINT32_MAX_VALUE;
								}
								else
								{
									current_time = (uint32)old_current_time;
								}
							}
							list.add_to_tail(current_time);
						}
						if (file->get_extended_version() >= 9)
						{
							file->rdwr_byte(month);
						}
						else
						{
							month = 0;
						}
						set.month = month;
						set.times = list;

						// Discard the data if we have fewer classes than the savegame
						if (j < actual_class_count_this_catg){
							waiting_times[i][j].put(id, set);
						}
					}
				}
			}
		}

		if(file->get_extended_version() > 0 && file->get_extended_version() < 3)
		{
			uint16 old_paths_timestamp = 0;
			uint16 old_connexions_timestamp = 0;
			bool old_reschedule = false;
			file->rdwr_short(old_paths_timestamp);
			file->rdwr_short(old_connexions_timestamp);
			file->rdwr_bool(old_reschedule);
		}
		else if(file->get_extended_version() > 0 && file->get_extended_version() < 10)
		{
			for(short i = 0; i < max_catg_count_file; i ++)
			{
				sint16 dummy;
				bool dummy_2;
				file->rdwr_short(dummy);
				file->rdwr_short(dummy);
				file->rdwr_bool(dummy_2);
			}
		}
	}

	if(file->get_extended_version() >=9 && file->get_version() >= 110000)
	{
		file->rdwr_bool(do_alternative_seats_calculation);
	}

	if(file->get_extended_version() >= 10)
	{
		if(file->is_saving())
		{
			uint32 count = halts_within_walking_distance.get_count();
			file->rdwr_long(count);
			ITERATE(halts_within_walking_distance, n)
			{
				halts_within_walking_distance[n].rdwr(file);
			}
		}
		else
		{
			// Loading
			uint32 count = 0;
			file->rdwr_long(count);
			halthandle_t halt;
			for(uint32 n = 0; n < count; n ++)
			{
				halt.rdwr(file);
				add_halt_within_walking_distance(halt);
			}
		}
		if(file->get_version() >= 111002)
		{
			file->rdwr_byte(control_towers);
		}
	}
	else
	{
		if(file->is_loading())
		{
			check_nearby_halts();
		}
	}

	if(file->get_extended_version() >= 11)
	{
		if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
		{
			file->rdwr_long(transfer_time);
		}
		else
		{
			uint16 old_tt = min(transfer_time, 65535u);
			file->rdwr_short(old_tt);
			if (old_tt == 65535)
			{
				transfer_time = UINT32_MAX_VALUE;
			}
			else
			{
				transfer_time = (uint32)old_tt;
			}
		}
	}

	if(file->get_extended_version() >= 12 || (file->get_version() >= 112007 && file->get_extended_version() >= 11))
	{
		file->rdwr_byte(check_waiting);
	}
	else
	{
		check_waiting = 0;
	}

	if(file->get_extended_version() >= 12)
	{
		// Load/save the estimated arrival and departure times.
		uint16 convoy_id;
		sint64 time;

		if(file->is_saving())
		{
			// These should probably be the same, but there is no harm in a little redundancy just in case.
			uint32 arrival_count = estimated_convoy_arrival_times.get_count();
			uint32 departure_count = estimated_convoy_departure_times.get_count();

			file->rdwr_long(arrival_count);
			file->rdwr_long(departure_count);

			FOR(arrival_times_map, const& iter, estimated_convoy_arrival_times)
			{
				convoy_id = iter.key;
				time = iter.value;
				file->rdwr_short(convoy_id);
				file->rdwr_longlong(time);
			}

			FOR(arrival_times_map, const& iter, estimated_convoy_departure_times)
			{
				convoy_id = iter.key;
				time = iter.value;
				file->rdwr_short(convoy_id);
				file->rdwr_longlong(time);
			}
		}

		if(file->is_loading())
		{
			estimated_convoy_arrival_times.clear();
			estimated_convoy_departure_times.clear();

			uint32 arrival_count;
			uint32 departure_count;

			file->rdwr_long(arrival_count);
			file->rdwr_long(departure_count);

			for(int i = 0; i < arrival_count; i++)
			{
				file->rdwr_short(convoy_id);
				file->rdwr_longlong(time);
				estimated_convoy_arrival_times.put(convoy_id, time);
			}

			for(int i = 0; i < departure_count; i++)
			{
				file->rdwr_short(convoy_id);
				file->rdwr_longlong(time);
				estimated_convoy_departure_times.put(convoy_id, time);
			}
		}
	}

	if((file->get_extended_version() >= 12 && file->get_extended_revision() >= 11) || file->get_extended_version() >= 13)
	{
		uint32 station_signals_count = station_signals.get_count();
		file->rdwr_long(station_signals_count);
		if(file->is_loading())
		{
			station_signals.clear();
		}
		for(uint32 n = 0; n < station_signals_count; n++)
		{
			if(file->is_saving())
			{
				station_signals[n].rdwr(file);
			}
			else
			{
				// Loading
				koord3d k;
				k.rdwr(file);
				if(get_halt(k, get_owner()).is_bound())
				{
					station_signals.append(k);
				}
			}
		}

		for(sint32 i = 0; i < 4; i ++)
		{
			file->rdwr_longlong(train_last_departed[i]);
		}
	}
	else
	{
		for(sint32 i = 0; i < 4; i ++)
		{
			train_last_departed[i] = 0;
		}
		station_signals.clear();
	}

	if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 15)
	{
		uint32 count;
		sint64 ready;
		ware_t ware;

		if (file->is_saving())
		{
#ifdef MULTI_THREAD
			count = 0;
			for (sint32 i = 0; i < world()->get_parallel_operations(); i++)
			{
				count += transferring_cargoes[i].get_count();
			}
#else
			count = transferring_cargoes[0].get_count();
#endif
		}

		file->rdwr_long(count);
#ifdef MULTI_THREAD
		sint32 po;
		if (file->is_saving())
		{
			po = world()->get_parallel_operations();
		}
		else
		{
			po = 1;
		}
#else
		const sint32 po = 1;
#endif
		for (sint32 i = 0; i < po; i++)
		{
#ifdef MULTI_THREAD
			if (file->is_saving())
			{
				count = transferring_cargoes[i].get_count();
			}
#endif
			for (uint32 j = 0; j < count; j++)
			{
				if (file->is_saving())
				{
					ready = transferring_cargoes[i][j].ready_time;
					ware = transferring_cargoes[i][j].ware;
				}

				file->rdwr_longlong(ready);
				ware.rdwr(file);

				if (file->is_loading())
				{
					transferring_cargo_t tc;
					tc.ready_time = ready;
					tc.ware = ware;
					transferring_cargoes[0].append(tc);
					fabrik_t* fab = fabrik_t::get_fab(tc.ware.get_zielpos());
					if (fab)
					{
						fab->update_transit(tc.ware, true);
					}

				}
			}
		}
	}

	// We do not need to save/load the service interval,
	// because this is re-set when the convoys are loaded
	// in any event. However, just to be on the safe side,
	// clear it here.

	service_frequencies.clear();

	// So compute it fresh every time
	calc_transfer_time();

	pedestrian_limit = 0;
#ifdef DEBUG_SIMRAND_CALLS
	loading = false;
#endif
}


void haltestelle_t::finish_rd(bool need_recheck_for_walking_distance)
{
	stale_convois.clear();
	stale_lines.clear();
	// fix good destination coordinates
	for(uint8 i = 0; i < goods_manager_t::get_max_catg_index(); i++)
	{
		if(cargo[i])
		{
			vector_tpl<ware_t> * warray = cargo[i];
			FOR(vector_tpl<ware_t>, & j, *warray)
			{
				j.finish_rd(welt);
			}
			// merge identical entries (should only happen with old games)
			const uint32 count = warray->get_count();
			for(uint32 j = 0; j < count; ++j)
			{
				ware_t& warj = (*warray)[j];
				if(warj.menge == 0)
				{
					continue;
				}
				for(uint32 k = j + 1; k < count; ++k)
				{
					ware_t& wark = (*warray)[k];
					if(wark.menge > 0 && warj.can_merge_with(wark))
					{
						warj.menge += wark.menge;
						wark.menge = 0;
					}
				}
			}
		}
	}

	// what kind of station here?
	recalc_station_type();

	// handle name for old stations which don't exist in kartenboden
	grund_t* bd = welt->lookup(get_basis_pos3d());
	if(bd!=NULL  &&  !bd->get_flag(grund_t::has_text) ) {
		// restore label and bridges
		grund_t* bd_old = welt->lookup_kartenboden(get_basis_pos());
		if(bd_old) {
			// transfer name (if there)
			const char *name = bd->get_text();
			if(name) {
				set_name( name );
				bd_old->set_text( NULL );
			}
			else {
				set_name( "Unknown" );
			}
		}
	}
	else {
		const char *current_name = bd ? bd->get_text() : translator::translate("Invalid stop");
		if(  all_names.get(current_name).is_bound()  &&  fabrik_t::get_fab(get_basis_pos())==NULL  ) {
			// try to get a new name ...
			const char *new_name;
			if(  station_type & airstop  ) {
				new_name = create_name( get_basis_pos(), "Airport" );
			}
			else if(  station_type & dock  ) {
				new_name = create_name( get_basis_pos(), "Dock" );
			}
			else if(  station_type & (railstation|monorailstop|maglevstop|narrowgaugestop)  ) {
				new_name = create_name( get_basis_pos(), "BF" );
			}
			else {
				new_name = create_name( get_basis_pos(), "H" );
			}
			dbg->warning("haltestelle_t::set_name()","name already used: \'%s\' -> \'%s\'", current_name, new_name );
			if(bd)
			{
				bd->set_text( new_name );
			}
			current_name = new_name;
		}
		all_names.set( current_name, self );
	}

	convoihandle_t convoy;
	slist_tpl<uint16> dead_convoys;
	FOR(arrival_times_map, const& iter, estimated_convoy_departure_times)
	{
		convoy.set_id(iter.key);
		if(!convoy.is_bound())
		{
			dead_convoys.append(iter.key);
			continue;
		}

		bool dead = true;
		if (convoy->get_schedule()) {
			for(int i=0; i<convoy->get_schedule()->get_count(); i++) {
				koord3d pos = convoy->get_schedule()->entries[i].pos;
				grund_t *gr = welt->lookup(pos);

				if (gr && gr->get_halt() == self) {
					dead = false;
					break;
				}
			}
		}

		if (dead) {
			dead_convoys.append(iter.key);
		}
	}

	FOR(arrival_times_map, const& iter, estimated_convoy_arrival_times)
	{
		convoy.set_id(iter.key);
		if(!convoy.is_bound())
		{
			dead_convoys.append(iter.key);
			continue;
		}

		bool dead = true;
		if (convoy->get_schedule()) {
			for(int i=0; i<convoy->get_schedule()->get_count(); i++) {
				koord3d pos = convoy->get_schedule()->entries[i].pos;
				grund_t *gr = welt->lookup(pos);

				if (gr && gr->get_halt() == self) {
					dead = false;
					break;
				}
			}
		}

		if (dead) {
			dead_convoys.append(iter.key);

		}
	}

	FOR(slist_tpl<uint16>, const &iter, dead_convoys)
	{
		clear_estimated_timings(iter);
	}

	if(need_recheck_for_walking_distance)
	{
		check_nearby_halts();
	}

	recalc_status();
}



void haltestelle_t::book(sint64 amount, int cost_type)
{
	assert(cost_type <= MAX_HALT_COST);
	financial_history[0][cost_type] += amount;
}



void haltestelle_t::init_financial_history()
{
	for (int j = 0; j<MAX_HALT_COST; j++)
	{
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][j] = 0;
		}
	}
}



/**
 * Calculates a status color for status bars
 * @author Hj. Malthaner
 */
void haltestelle_t::recalc_status()
{
	status_color = financial_history[0][HALT_CONVOIS_ARRIVED] > 0 ? COL_GREEN : COL_YELLOW;

	// since the status is ordered ...
	uint8 status_bits = 0;

	MEMZERO(overcrowded);

	uint64 total_sum = 0;
	if(get_pax_enabled()) {
		const uint32 max_ware = get_capacity(goods_manager_t::INDEX_PAS);
		total_sum += get_ware_summe(goods_manager_t::passengers);
		if(total_sum>max_ware) {
			overcrowded[0] |= 1;
		}
		if(get_pax_unhappy() > 40 ) {
			status_bits = (total_sum>max_ware+200 || (total_sum>max_ware  &&  get_pax_unhappy()>200)) ? 2 : 1;
		}
		else if(total_sum>max_ware) {
			status_bits = total_sum>max_ware+200 ? 2 : 1;
		}
	}

	if(get_mail_enabled()) {
		const uint32 max_ware = get_capacity(goods_manager_t::INDEX_MAIL);
		const uint32 mail = get_ware_summe(goods_manager_t::mail);
		total_sum += mail;
		if(mail>max_ware) {
			status_bits |= mail>max_ware+200 ? 2 : 1;
			overcrowded[0] |= 2;
		}
	}

	// now for all goods
	if(status_color!=COL_RED  &&  get_ware_enabled()) {
		const uint8  count = goods_manager_t::get_count();
		const uint32 max_ware = get_capacity(2);

		// For goods, include transferring goods when determining whether a stop is overcrowded.
		// This is necessary as goods tend to come all at once and take a long time to transfer.
		uint32 transferring_total = 0;
		for (uint32 i = 0; i <= welt->get_parallel_operations(); i++)
		{
			for (uint32 n = 0; n < transferring_cargoes[i].get_count(); n++)
			{
				if (transferring_cargoes[i].get_element(n).ware.is_freight())
				{
					transferring_total += transferring_cargoes[i].get_element(n).ware.menge;
				}
			}
		}

		for(  uint32 i = 3;  i < count;  i++  ) {
			goods_desc_t const* const wtyp = goods_manager_t::get_info(i);
			const uint32 ware_sum = get_ware_summe(wtyp);

			total_sum += ware_sum;
			if((ware_sum + transferring_total) > max_ware)
			{
				status_bits |= (ware_sum + transferring_total) > max_ware + 32 || enables & CROWDED ? 2 : 1;
				overcrowded[wtyp->get_index()/8] |= 1<<(wtyp->get_index()%8);
			}
		}
	}

	// take the worst color for status
	if(  status_bits  ) {
		status_color = status_bits&2 ? COL_RED : COL_ORANGE;
	}
	else
	{
		status_color = (financial_history[0][HALT_WAITING]+financial_history[0][HALT_DEPARTED] == 0) ? COL_YELLOW : COL_GREEN;
	}

	if((station_type & airstop) && has_no_control_tower())
	{
		status_color = COL_PURPLE;
	}

	financial_history[0][HALT_WAITING] = total_sum;
}



/**
 * Draws some nice colored bars giving some status information
 * @author Hj. Malthaner
 */
void haltestelle_t::display_status(KOORD_VAL xpos, KOORD_VAL ypos)
{
	// ignore freight that cannot reach to this station
	sint16 count = 0;
	for(  uint16 i = 0;  i < goods_manager_t::get_count();  i++  ) {
		if(  i == 2  ) {
			continue; // ignore freight none
		}
		if(  gibt_ab( goods_manager_t::get_info(i) )  ) {
			count++;
		}
	}
	if(  count != last_bar_count  ) {
		// bars will shift x positions, mark entire station bar region dirty
		KOORD_VAL max_bar_height = 0;
		for(  sint16 i = 0;  i < last_bar_count;  i++  ) {
			if(  last_bar_height[i] > max_bar_height  ) {
				max_bar_height = last_bar_height[i];
			}
		}
		const KOORD_VAL x = xpos - (last_bar_count * 4 - get_tile_raster_width()) / 2;
		mark_rect_dirty_wc( x - 1 - 4, ypos - 11 - max_bar_height - 6, x + last_bar_count * 4 + 12 - 2, ypos - 11 );

		// reset bar heights for new count
		last_bar_height.clear();
		last_bar_height.resize( count );
		for(  sint16 i = 0;  i < count;  i++  ) {
			last_bar_height.append(0);
		}
		last_bar_count = count;
	}

	ypos -= 11;
	xpos -= (count * 4 - get_tile_raster_width()) / 2;
	const KOORD_VAL x = xpos;

	sint16 bar_height_index = 0;
	uint32 max_capacity;
	for(  uint16 i = 0;  i < goods_manager_t::get_count();  i++  ) {
		if(  i == 2  ) {
			continue; // ignore freight none
		}
		const goods_desc_t *wtyp = goods_manager_t::get_info(i);
		if(  gibt_ab( wtyp )  ) {
			if(  i < 2  ) {
				max_capacity = get_capacity(i);
			}
			else {
				max_capacity = get_capacity(2);
			}
			const uint32 sum = get_ware_summe( wtyp );
			uint32 v = min( sum, max_capacity );
			if(  max_capacity > 512  ) {
				v = 2 + (v * 128) / max_capacity;
			}
			else {
				v = (v / 4) + 2;
			}

			display_fillbox_wh_clip( xpos, ypos - v - 1, 1, v, COL_GREY4, false);
			display_fillbox_wh_clip( xpos + 1, ypos - v - 1, 2, v, wtyp->get_color(), false);
			display_fillbox_wh_clip( xpos + 3, ypos - v - 1, 1, v, COL_GREY1, false);

			// Hajo: show up arrow for capped values
			if(  sum > max_capacity  ) {
				display_fillbox_wh_clip( xpos + 1, ypos - v - 6, 2, 4, COL_WHITE, false);
				display_fillbox_wh_clip( xpos, ypos - v - 5, 4, 1, COL_WHITE, false);
			}

			if(  last_bar_height[bar_height_index] != (KOORD_VAL)v  ) {
				if(  (KOORD_VAL)v > last_bar_height[bar_height_index]  ) {
					// bar will be longer, mark new height dirty
					mark_rect_dirty_wc( xpos, ypos - v - 6, xpos + 4 - 1, ypos + 4 - 1);
				}
				else {
					// bar will be shorter, mark old height dirty
					mark_rect_dirty_wc( xpos, ypos - last_bar_height[bar_height_index] - 6, xpos + 4 - 1, ypos + 4 - 1);
				}
				last_bar_height[bar_height_index] = v;
			}

			bar_height_index++;
			xpos += 4;
		}
	}

	// status color box below
	bool dirty = false;
	if(  get_status_farbe() != last_status_color  ) {
		last_status_color = get_status_farbe();
		dirty = true;
	}
	display_fillbox_wh_clip( x - 1 - 4, ypos, count * 4 + 12 - 2, 4, get_status_farbe(), dirty );
}



bool haltestelle_t::add_grund(grund_t *gr, bool relink_factories, bool recalc_nearby_halts)
{
	assert(gr!=NULL);

	// new halt?
	if(  tiles.is_contained(gr)  ) {
		return false;
	}

	koord pos = gr->get_pos().get_2d();
	add_to_station_type( gr );
	gr->set_halt( self );
	tiles.append( gr );

	// add to hashtable
	if (all_koords) {
		sint32 n = get_halt_key( gr->get_pos(), welt->get_size().y );
		all_koords->set( n, self );
	}

	// appends this to the ground
	// after that, the surrounding ground will know of this station
	vector_tpl<fabrik_t*> affected_fab_list;
	int const cov = welt->get_settings().get_station_coverage();
	for (int y = -cov; y <= cov; y++) {
		for (int x = -cov; x <= cov; x++) {
			koord p=pos+koord(x,y);
			planquadrat_t *plan = welt->access(p);
			if(plan) {
				if (recalc_nearby_halts)
				{
					plan->add_to_haltlist(self);
				}
				grund_t* gr = plan->get_kartenboden();
				gr->set_flag(grund_t::dirty);
				// If there's a factory here, add it to the working list
				const gebaeude_t* gb = gr->find<gebaeude_t>();
				if (gb) {
					fabrik_t* fab = gb->get_fabrik();
					if (fab && !affected_fab_list.is_contained(fab) ) {
						affected_fab_list.append(fab);
					}
				}
			}
		}
	}

	// Update our list of factories.
	if (relink_factories)
	{
		// This now tells the factories to add themselves to this halt
		// if they are within coverage range.
		verbinde_fabriken();
	}

	signal_t* signal = gr->find<signal_t>();

	if(signal && signal->get_desc()->is_longblock_signal() && (signal->get_desc()->get_working_method() == time_interval || signal->get_desc()->get_working_method() == time_interval_with_telegraph || signal->get_desc()->get_working_method() == absolute_block))
	{
		// Register station signals at the halt.
		station_signals.append(gr->get_pos());
	}

	// check if we have to register line(s) and/or lineless convoy(s) which serve this halt
	vector_tpl<linehandle_t> check_line(0);

	// public halt: must iterate over all players lines / convoys
	bool public_halt = get_owner() == welt->get_public_player();

	uint8 const pl_min = public_halt ? 0                : get_owner()->get_player_nr();
	uint8 const pl_max = public_halt ? MAX_PLAYER_COUNT : get_owner()->get_player_nr() + 1;
	// iterate over all lines (public halt: all lines, other: only player's lines)
	for(uint8 i = pl_min; i < pl_max; i++)
	{
		if(player_t *player = welt->get_player(i))
		{
			player->simlinemgmt.get_lines(simline_t::line, &check_line);
			FOR(vector_tpl<linehandle_t>, const j, check_line)
			{
				// only add unknown lines
				if(  !registered_lines.is_contained(j)  &&  j->count_convoys() > 0  ) {
					FOR(  minivec_tpl<schedule_entry_t>, const& k, j->get_schedule()->entries  ) {
						if(  get_halt(k.pos, player) == self  ) {
							registered_lines.append(j);
							break;
						}
					}
				}
			}
		}
	}
	// Knightly : iterate over all convoys
	FOR(vector_tpl<convoihandle_t>, const cnv, welt->convoys()) {
		// only check lineless convoys which have matching ownership and which are not yet registered
		if(  !cnv->get_line().is_bound()  &&  (public_halt  ||  cnv->get_owner()==get_owner())  &&  !registered_convoys.is_contained(cnv)  ) {
			if(  const schedule_t *const schedule = cnv->get_schedule()  ) {
				FOR(minivec_tpl<schedule_entry_t>, const& k, schedule->entries) {
					if (get_halt(k.pos, cnv->get_owner()) == self) {
						registered_convoys.append(cnv);
						break;
					}
				}
			}
		}
	}

	// This entire loop is just for the assertion below.
	// Consider deleting the assertion --neroden
	bool grund_is_where_it_should_be = false;
	const planquadrat_t* plan = welt->access(pos);
	for(  uint8 i=0;  i < plan->get_boden_count();  i++  ) {
		const grund_t* found_gr = plan->get_boden_bei(i);
		if (found_gr == gr) {
			grund_is_where_it_should_be = true;
			break;
		}
	}
	if (  !grund_is_where_it_should_be || gr->get_halt() != self || !gr->is_halt()  ) {
		dbg->error( "haltestelle_t::add_grund()", "no ground added to (%s)", gr->get_pos().get_str() );
	}
	assert(grund_is_where_it_should_be);
	assert(gr->get_halt() == self);
	assert(gr->is_halt());

	init_pos = tiles.front().grund->get_pos().get_2d();
	if (recalc_nearby_halts)
	{
		check_nearby_halts();
	}
	calc_transfer_time();

#ifdef MULTI_THREAD
	world()->stop_path_explorer();
#endif
	path_explorer_t::refresh_all_categories(false);

	return true;
}



bool haltestelle_t::rem_grund(grund_t *gr)
{
	// namen merken
	if(!gr) {
		return false;
	}

	station_signals.remove(gr->get_pos());

	slist_tpl<tile_t>::iterator i = std::find(tiles.begin(), tiles.end(), gr);
	if (i == tiles.end()) {
		// was not part of station => do nothing
		dbg->error("haltestelle_t::rem_grund()","removed illegal ground from halt");
		return false;
	}

	// first tile => remove name from this tile ...
	char buf[256];
	const char* station_name_to_transfer = NULL;
	if (i == tiles.begin() && i->grund->get_name()) {
		tstrncpy(buf, get_name(), lengthof(buf));
		station_name_to_transfer = buf;
		set_name(NULL);
	}

	// now remove tile from list
	tiles.erase(i);
#ifdef MULTI_THREAD
	world()->stop_path_explorer();
#endif
	path_explorer_t::refresh_all_categories(false);
	init_pos = tiles.empty() ? koord::invalid : tiles.front().grund->get_pos().get_2d();

	// re-add name
	if (station_name_to_transfer != NULL  &&  !tiles.empty()) {
		label_t *lb = tiles.front().grund->find<label_t>();
		delete lb;
		set_name( station_name_to_transfer );
	}

	bool remove_halt = true;
	planquadrat_t *pl = welt->access( gr->get_pos().get_2d() );
	if(pl) {
		// no longer present on this level
		gr->set_halt(halthandle_t());
		// still connected elsewhere?
		for(unsigned i=0;  i<pl->get_boden_count();  i++  ) {
			if(pl->get_boden_bei(i)->get_halt()==self) {
				// still connected with other ground => do not remove from plan ...
				remove_halt = false;
				break;
			}
		}
	}

	if (remove_halt) {
		// otherwise remove from plan ...
		if (pl) {
			pl->get_kartenboden()->set_flag(grund_t::dirty);
		}

		uint16 const cov = welt->get_settings().get_station_coverage();
		vector_tpl<fabrik_t*> affected_fab_list;
		for (int y = -cov; y <= cov; y++) {
			for (int x = -cov; x <= cov; x++) {
				const koord nearby_pos = gr->get_pos().get_2d()+koord(x,y);
				planquadrat_t *nearby_plan = welt->access( nearby_pos );
				if(nearby_plan) {
					// This will remove the tile from the haltlist only if appropriate
					// (::remove_from_haltlist double-checks this)
					nearby_plan->remove_from_haltlist(self);
					nearby_plan->get_kartenboden()->set_flag(grund_t::dirty);
					const grund_t* nearby_ground = nearby_plan->get_kartenboden();
					// If there's a factory here, add it to the working list
					const gebaeude_t* nearby_building = nearby_ground->find<gebaeude_t>();
					if (nearby_building) {
						fabrik_t* nearby_fab = nearby_building->get_fabrik();
						if (nearby_fab && !affected_fab_list.is_contained(nearby_fab) ) {
							affected_fab_list.append(nearby_fab);
						}
					}
				}
			}
		}


		// Update nearby factories' lists of connected halts.
		// Must be done AFTER updating the planquadrats,
		FOR (vector_tpl<fabrik_t*>, fab, affected_fab_list)
		{
			fab->recalc_nearby_halts();
		}
	}

	// needs to be done, if this was a dock
	recalc_station_type();

	// remove lines eventually
	for(  size_t j = registered_lines.get_count();  j-- != 0;  ) {
		bool ok = false;
		FOR(  minivec_tpl<schedule_entry_t>, const& k, registered_lines[j]->get_schedule()->entries  ) {
			if(  get_halt(k.pos, registered_lines[j]->get_owner()) == self  ) {
				ok = true;
				break;
			}
		}
		// need removal?
		if(!ok) {
			stale_lines.append_unique( registered_lines[j] );
			registered_lines.remove_at(j);
		}
	}

	// Knightly : remove registered lineless convoys as well
	for(  size_t j = registered_convoys.get_count();  j-- != 0;  ) {
		bool ok = false;
		FOR(  minivec_tpl<schedule_entry_t>, const& k, registered_convoys[j]->get_schedule()->entries  ) {
			if(  get_halt(k.pos, registered_convoys[j]->get_owner()) == self  ) {
				ok = true;
				break;
			}
		}
		// need removal?
		if(  !ok  ) {
			stale_convois.append_unique( registered_convoys[j] );
			registered_convoys.remove_at(j);
		}
	}

	check_nearby_halts();
	calc_transfer_time();
#ifdef MULTI_THREAD
	world()->stop_path_explorer();
#endif
	path_explorer_t::refresh_all_categories(false);

	return true;
}



bool haltestelle_t::existiert_in_welt() const
{
	return !tiles.empty();
}


/* return the closest square that belongs to this halt
 * @author prissi
 */
koord haltestelle_t::get_next_pos( koord start, bool square ) const
{
	koord find = koord::invalid;

	if (!tiles.empty()) {
		// find the closest one
		sint32	dist = 0x7FFF;
		FOR(slist_tpl<tile_t>, const& i, tiles) {
			koord const p = i.grund->get_pos().get_2d();
			sint32 d;
			if(square)
			{
				d = koord_distance(start, p);
			}
			else
			{
				d = shortest_distance(start, p);
			}
			if(d<dist) {
				// ok, this one is closer
				dist = d;
				find = p;
			}
		}
	}
	return find;
}



/* marks a coverage area
 * @author prissi
 */
void haltestelle_t::mark_unmark_coverage(const bool mark, const bool factories) const
{
	// iterate over all tiles
	uint16 const cov = factories ? welt->get_settings().get_station_coverage_factories() : welt->get_settings().get_station_coverage();
	koord  const size(cov * 2 + 1, cov * 2 + 1);
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		welt->mark_area(i.grund->get_pos() - size / 2, size, mark);
	}
}



/* Find a tile where this type of vehicle could stop
 * @author prissi
 */
const grund_t *haltestelle_t::find_matching_position(const waytype_t w) const
{
	// iterate over all tiles
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		if (i.grund->hat_weg(w)) {
			return i.grund;
		}
	}
	return NULL;
}



/* Checks whether there is an unoccupied loading bay for this kind of thing
 * @author prissi
 */
bool haltestelle_t::find_free_position(const waytype_t w,convoihandle_t cnv,const obj_t::typ d) const
{
	// iterate over all tiles
	// for road, we have to consider passing lane.
	if(  w==road_wt  ) {
		FOR(slist_tpl<tile_t>, const& i, tiles) {
			if(  !i.reservation[0].is_bound()  ||  !i.reservation[1].is_bound()  ) {
				// possibly there is empty slots.
				grund_t* const gr = i.grund;
				assert(gr);
				if(  get_empty_lane(gr, cnv)!=0  ) {
					return true;
				}
			}
		}
		// no empty tile.
		return false;
	}
	// for other waytypes...
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		if (i.reservation[0] == cnv || !i.reservation[0].is_bound()) {
			// not reserved
			grund_t* const gr = i.grund;
			assert(gr);
			// found a stop for this waytype but without object d ...
			if(gr->hat_weg(w)  &&  gr->suche_obj(d)==NULL) {
				// not occupied
				return true;
			}
		}
	}
	return false;
}


/* reserves a position (caution: rail blocks work differently!)
 * @author prissi
 */
bool haltestelle_t::reserve_position(grund_t *gr,convoihandle_t cnv)
{
	slist_tpl<tile_t>::iterator i = std::find(tiles.begin(), tiles.end(), gr);
	if (i != tiles.end()) {
		if (i->reservation[0] == cnv  ||  i->reservation[1] == cnv) {
//DBG_MESSAGE("haltestelle_t::reserve_position()","gr=%d,%d already reserved by cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
			return true;
		}
		// not reserved
		vehicle_t const& v = *cnv->front();
		// road vehicles need special process to consider passing lane.
		if(v.get_waytype()==road_wt) {
			uint8 empty_lane = get_empty_lane(gr, cnv);
			if((empty_lane&1)!=0) {
				i->reservation[0] = cnv;
				return true;
			} else if((empty_lane&2)!=0) {
				i->reservation[1] = cnv;
				return true;
			} else {
				return false;
			}
		}
		// for other vehicle types...
		if (!i->reservation[0].is_bound()) {
			grund_t* gr = i->grund;
			if(gr) {
				// found a stop for this waytype but without object d ...
				if (gr->hat_weg(v.get_waytype()) && !gr->suche_obj(v.get_typ())) {
					// not occupied
//DBG_MESSAGE("haltestelle_t::reserve_position()","success for gr=%i,%i cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
					i->reservation[0] = cnv;
					return true;
				}
			}
		}
	}
//DBG_MESSAGE("haltestelle_t::reserve_position()","failed for gr=%i,%i, cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
	return false;
}


/* frees a reserved position (caution: rail blocks work differently!)
 * @author prissi
 */
bool haltestelle_t::unreserve_position(grund_t *gr, convoihandle_t cnv)
{
	slist_tpl<tile_t>::iterator i = std::find(tiles.begin(), tiles.end(), gr);
	if (i != tiles.end()) {
		for(uint8 k=0; k<2; k++) {
			if (i->reservation[k] == cnv) {
				i->reservation[k] = convoihandle_t();
				return true;
			}
		}
	}
DBG_MESSAGE("haltestelle_t::unreserve_position()","failed for gr=%p",gr);
	return false;
}


/* can a convoi reserve this position?
 * @author prissi
 */
bool haltestelle_t::is_reservable(const grund_t *gr, convoihandle_t cnv) const
{
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		if (gr == i.grund) {
			if (i.reservation[0] == cnv) {
DBG_MESSAGE("haltestelle_t::is_reservable()","gr=%d,%d already reserved by cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
				return true;
			}
			// not reserved
			if (!i.reservation[0].is_bound()) {
				// found a stop for this waytype but without object d ...
				vehicle_t const& v = *cnv->front();
				if (gr->hat_weg(v.get_waytype()) && !gr->suche_obj(v.get_typ())) {
					// not occupied
					return true;
				}
			}
			//return false;
		}
	}
DBG_MESSAGE("haltestelle_t::reserve_position()","failed for gr=%i,%i, cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
	return false;
}

/* haltestelle_t::is_reservable for road vehicles
 * The returned value is a bit pattern.
 * 0 represents that the both lane is filled.
 * 1 represents that the traffic lane is empty.
 * 2 represents that the passing lane is empty.
 * 3 represents that the both lane is empty.
 * @author THLeaderH
 */
uint8 haltestelle_t::get_empty_lane(const grund_t *gr, convoihandle_t cnv) const {
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		if (  gr == i.grund  ) {
			if (  i.reservation[0] == cnv  ) {
				// already reserved the traffic lane.
				return 1;
			}
			if (  i.reservation[1] == cnv  ) {
				// already reserved the passing lane.
				return 2;
			}
			// not reserved
			const strasse_t* str = dynamic_cast<strasse_t*> (gr->get_weg(road_wt));
			if(  !str  ) { return 0; }
			const overtaking_mode_t overtaking_mode = str->get_overtaking_mode();
			uint8 empty = 0;
			for (uint8 k=1; k<=2; k++) {
				if(  overtaking_mode!=halt_mode  &&  k==2  ) { break; }
				if(  !i.reservation[k-1].is_bound()  ) {
					// check whether this place is not occupied...
					// since up to 2 vehicles can exist on one tile, we have to check all objects on the tile.
					empty |= k; // raise the empty bit
					for(uint8 h=0; h<gr->get_top(); h++) {
						if(  road_vehicle_t* rv = obj_cast<road_vehicle_t> (gr->obj_bei(h))  ) {
							bool is_overtaking = rv->get_convoi()->is_overtaking();
							// If a vehicle exists on the same lane, drop the empty bit.
							if(  k==1 ? !is_overtaking : is_overtaking  ) { empty &= ~k; }
						}
						else if(  private_car_t* pc = obj_cast<private_car_t> (gr->obj_bei(h))  ) {
							bool is_overtaking = pc->is_overtaking();
							if(  k==1 ? !is_overtaking : is_overtaking  ) { empty &= ~k; }
						}
					}
				}
			}
			return empty;
		}
	}
	return 0;
}


void* haltestelle_t::connexion::operator new(size_t /*size*/)
{
	return freelist_t::gimme_node(sizeof(haltestelle_t::connexion));
}

void haltestelle_t::connexion::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(haltestelle_t::connexion),p);
}


/* deletes factory references so map rotation won't segfault
*/
void haltestelle_t::release_factory_links()
{
	fab_list.clear();
}

/**
 * Get queue position for spacing calculation.
 * @author Inkelyad
 */
int haltestelle_t::get_queue_pos(convoihandle_t cnv) const
{
	linehandle_t line = cnv->get_line();
	int count = 0;
	const bool is_road_type = cnv->get_vehicle(0)->get_waytype() == road_wt;
	for(slist_tpl<convoihandle_t>::const_iterator i = loading_here.begin(), end = loading_here.end();  i != end && (*i) != cnv; ++i)
	{
		if(!(*i).is_bound() || get_halt((*i)->get_pos(), owner) != self)
		{
			continue;
		}
		const int state = (*i)->get_state();
		// QUERY: If this stop is the stop at which the reverse_route setting is set/unset, might
		// ((*i)->get_reverse_schedule() == cnv->get_reverse_schedule()) 
		// give a false negative, therefore incorrectly assigning two vehicles to the same queue position?
		// 
		// ANSWER: No, as the reverse route is only engaged/disengaged on leaving the stop.

		if((*i)->get_line() == line &&
			(((*i)->get_schedule()->get_current_stop() == cnv->get_schedule()->get_current_stop()
			&& ((*i)->get_reverse_schedule() == cnv->get_reverse_schedule())
			&& (!is_road_type || state == convoi_t::LOADING))
			|| (state == convoi_t::REVERSING
			&& !is_road_type
			&& cnv->calc_remaining_loading_time() > 100
			&& ((*i)->get_reverse_schedule() ?
				(*i)->get_schedule()->get_current_stop() + 1 == cnv->get_schedule()->get_current_stop() :
				(*i)->get_schedule()->get_current_stop() - 1 == cnv->get_schedule()->get_current_stop()))))
		{
			count++;
		}
	}
	return count + 1;
}

void haltestelle_t::add_halt_within_walking_distance(halthandle_t halt)
{
	if(halt != self)
	{
		// Halt should not be listed as connected to itself.
		halts_within_walking_distance.append_unique(halt);
	}
}

void haltestelle_t::remove_halt_within_walking_distance(halthandle_t halt)
{
	halts_within_walking_distance.remove(halt);
}

void haltestelle_t::check_nearby_halts()
{
	halts_within_walking_distance.clear();
	FOR(slist_tpl<tile_t>, const& iter, tiles)
	{
		planquadrat_t *plan = welt->access(iter.grund->get_pos().get_2d());
		if(plan)
		{
			const nearby_halt_t *const halt_list = plan->get_haltlist();

			for (int h = plan->get_haltlist_count() - 1; h >= 0; h--)
			{
				halthandle_t halt = halt_list[h].halt;
				if (halt->is_enabled(goods_manager_t::passengers))
				{
					add_halt_within_walking_distance(halt);
					halt->add_halt_within_walking_distance(self);
				}
			}
		}
	}
	// Must refresh here, but only passengers can walk, so only refresh passengers.
#ifdef MULTI_THREAD
	world()->stop_path_explorer();
#endif
	path_explorer_t::refresh_category(0);
}

bool haltestelle_t::is_within_walking_distance_of(halthandle_t halt) const
{
	return welt->get_settings().get_allow_routing_on_foot() && halts_within_walking_distance.is_contained(halt);
}

uint32 haltestelle_t::get_number_of_halts_within_walking_distance() const
{
	if(welt->get_settings().get_allow_routing_on_foot())
	{
		return halts_within_walking_distance.get_count();
	}
	else
	{
		return 0;
	}
}

bool haltestelle_t::check_access(const player_t* player) const
{
	return !player || player == owner || owner == NULL || owner->allows_access_to(player->get_player_nr());
}

bool haltestelle_t::has_no_control_tower() const
{
	return welt->get_settings().get_allow_airports_without_control_towers() ? false : control_towers == 0;
}

void haltestelle_t::calc_transfer_time()
{
	koord ul(32767,32767);
	koord lr(0,0);
	koord pos;
	FOR(slist_tpl<tile_t>, const& tile, tiles)
	{
		pos = tile.grund->get_pos().get_2d();

		// First time through, this sets ul / lr to pos.
		// Subsequent times, it pushes ul to lower numbers
		// and lr to higher numbers.
		if(pos.x < ul.x) ul.x = pos.x;
		if(pos.y < ul.y) ul.y = pos.y;
		if(pos.x > lr.x) lr.x = pos.x;
		if(pos.y > lr.y) lr.y = pos.y;
	}

	// Guaranteed to be non-negative
	// Don't add 1 -- a one-tile station should have zero transfer time
	const sint16 x_size = (lr.x - ul.x);
	const sint16 y_size = (lr.y - ul.y);

	// Revised calculation by neroden to have greater granularity.

	// this is the length (in tiles) to walk from the center tile to each of the four sides,
	// added together.  (One tile is "free"; this approximates the previous algorithm for very
	// small stations.)
	const uint32 length_around = x_size + y_size;

	// Approximate the transfer time.  (This is all inlined.)
	const uint32 walking_around = welt->walking_time_tenths_from_distance(length_around);
	// Guess that someone has to walk roughly from the middle to one end, so divide by *4*.
	// Finally, round down to a uint16 (just in case).
	transfer_time = min(walking_around / 4u, UINT32_MAX_VALUE);

	// Repeat the process for the transshipment time.  (This is all inlined.)
	const uint32 hauling_around = welt->walk_haulage_time_tenths_from_distance(length_around);
	transshipment_time = min(hauling_around / 4u, UINT32_MAX_VALUE);

	// Adjust for overcrowding - transfer time increases with a more crowded stop.
	// TODO: Better separate waiting times for different types of goods.

	const uint8 max_categories = goods_manager_t::get_max_catg_index();
	sint64 waiting_passengers = 0;
	sint64 waiting_goods = 0;
	// TODO: Consider adding waiting mail here, too
	// This would require a separete mail transfer time.

	for(uint8 i = 0; i < max_categories; i++)
	{
		if(cargo[i])
		{
			if (i == goods_manager_t::INDEX_PAS)
			{
				vector_tpl<ware_t> * warray = cargo[i];
				FOR(vector_tpl<ware_t>, &w, *warray)
				{
					waiting_passengers += w.menge;
				}
			}
			else if (i > goods_manager_t::INDEX_NONE)
			{
				vector_tpl<ware_t> * warray = cargo[i];
				FOR(vector_tpl<ware_t>, &w, *warray)
				{
					waiting_goods += w.menge;
				}
			}
		}
	}

	if(capacity[0] > 0 && waiting_passengers > capacity[0])
	{
		const sint64 overcrowded_proporion_passengers = waiting_passengers * 10ll / capacity[0];
		transfer_time = (uint32)std::min(std::max((sint64)transfer_time, (overcrowded_proporion_passengers * (2ll * overcrowded_proporion_passengers)) / 10ll), (sint64)transfer_time * 10ll);
	}

	if(capacity[2] > 0 && waiting_goods > capacity[2])
	{
		const sint64 overcrowded_proportion_goods = waiting_goods * 10ll / capacity[2];
		transshipment_time = (uint32)std::min(std::max((sint64)transshipment_time, (overcrowded_proportion_goods * (2ll *overcrowded_proportion_goods)) / 10ll), (sint64)transshipment_time * 10ll);
	}

	// For reference, with a transshipment speed of 1 km/h and a walking speed of 5 km/h,
	// and 125 meters per tile, a 1x2 station has a 1:48 transfer penalty for freight
	// and an 18 second transfer penalty for passengers.  A 1x6 station has
	// a 1:48 transfer penalty for passengers and a 9:18 transfer penalty for freight.
	// A large 8 x 4 station has an 18:42 penalty for freight and a 3:42 penalty for passengers.

	// TODO: Consider more sophisticated things here, such as allowing certain extensions to
	// reduce this transshipment time (convyer belts, travellators, etc.).
}

void haltestelle_t::add_waiting_time(uint32 time, halthandle_t halt, uint8 category, uint8 g_class, bool do_not_reset_month)
{
	if(halt.is_bound())
	{
		const waiting_time_map *wt = &waiting_times[category][g_class];

		if(!wt->is_contained(halt.get_id()))
		{
			fixed_list_tpl<uint32, 32> tmp;
			waiting_time_set set;
			set.times = tmp;
			set.month = 0;
			waiting_times[category][g_class].put(halt.get_id(), set);
		}
		waiting_times[category][g_class].access(halt.get_id())->times.add_to_tail(time);
		if(!do_not_reset_month)
		{
			waiting_times[category][g_class].access(halt.get_id())->month = 0;
		}
	}
}

void haltestelle_t::set_estimated_arrival_time(uint16 convoy_id, sint64 time)
{
	estimated_convoy_arrival_times.set(convoy_id, time);
}


void haltestelle_t::set_estimated_departure_time(uint16 convoy_id, sint64 time)
{
	estimated_convoy_departure_times.set(convoy_id, time);
}

void haltestelle_t::clear_estimated_timings(uint16 convoy_id)
{
	estimated_convoy_arrival_times.remove(convoy_id);
	estimated_convoy_departure_times.remove(convoy_id);
}

void haltestelle_t::add_line(linehandle_t line)
{
	registered_lines.append_unique(line);
#ifdef ALWAYS_CACHE_SERVICE_INTERVAL
	clear_service_intervals(line->get_schedule());
#else
	update_service_intervals(line->get_schedule());
#endif
}
void haltestelle_t::remove_line(linehandle_t line)
{
	registered_lines.remove(line);
	if(registered_convoys.empty() && registered_lines.empty() && !welt->is_destroying())
	{
		const uint8 max_categories = goods_manager_t::get_max_catg_index();
		for(uint8 i = 0; i < max_categories; i++)
		{
			connexions[i]->clear();
		}
	}
	update_service_intervals(line->get_schedule());
}

void haltestelle_t::add_convoy(convoihandle_t convoy)
{
	registered_convoys.append_unique(convoy);
#ifdef ALWAYS_CACHE_SERVICE_INTERVAL
	clear_service_intervals(convoy->get_schedule());
#else
	update_service_intervals(convoy->get_schedule());
#endif
}

void haltestelle_t::remove_convoy(convoihandle_t convoy)
{
	registered_convoys.remove(convoy);
	if(registered_convoys.empty() && registered_lines.empty() && !welt->is_destroying())
	{
		const uint8 max_categories = goods_manager_t::get_max_catg_index();
		for(uint8 i = 0; i < max_categories; i++)
		{
			connexions[i]->clear();
		}
	}
	if (!welt->is_destroying())
	{
		update_service_intervals(convoy->get_schedule());
	}
}

void haltestelle_t::update_service_intervals(schedule_t* sch)
{
	halthandle_t halt;
	for (uint8 i = 0; i < sch->get_count(); i++)
	{
		halt = haltestelle_t::get_halt(sch->entries[i].pos, world()->get_public_player());
		if (halt.is_bound())
		{
			service_frequency_specifier spec;
			spec.y = halt.get_id();
			uint32 freq;
			for(uint8 c = 0; c < goods_manager_t::get_max_catg_index(); c ++)
			{
				freq = calc_service_frequency(halt, c);
				spec.x = c;
				service_frequencies.set(spec, freq);
			}
		}
	}
}

#ifdef ALWAYS_CACHE_SERVICE_INTERVAL
void haltestelle_t::clear_service_intervals(schedule_t* sch)
{
	halthandle_t halt;
	for (uint8 i = 0; i < sch->get_count(); i++)
	{
		halt = haltestelle_t::get_halt(sch->entries[i].pos, world()->get_public_player());
		if (halt.is_bound())
		{
			service_frequency_specifier spec;
			spec.y = halt.get_id();
			for (uint8 c = 0; c < goods_manager_t::get_max_catg_index(); c++)
			{
				spec.x = c;
				service_frequencies.remove(spec);
			}
		}
	}
}
#endif

sint64 haltestelle_t::calc_earliest_arrival_time_at(halthandle_t halt, convoihandle_t &convoy, uint8 catg_index, uint8 g_class) const
{
	const arrival_times_map& next_transfer_arrivals = halt->get_estimated_convoy_arrival_times();
	sint64 best_arrival_time = SINT64_MAX_VALUE;
	sint64 current_time;
	convoihandle_t arrival_convoy;
	FOR(arrival_times_map, const& iter, estimated_convoy_departure_times)
	{
		arrival_convoy.set_id(iter.key);
		if(!arrival_convoy.is_bound())
		{
			continue;
		}

		if(arrival_convoy->get_no_load())
		{
			// Do not wait for a convoy that is set not to load.
			continue;
		}

		if (!arrival_convoy->get_goods_catg_index().is_contained(catg_index) || !arrival_convoy->carries_this_or_lower_class(catg_index, g_class))
		{
			// Do not wait for a convoy that cannot convey this type of load.
			continue;
		}

		if(next_transfer_arrivals.is_contained(iter.key))
		{
			// Potentially relevant convoy - stops at the next transfer
			current_time = next_transfer_arrivals.get(iter.key);
			// Check to see whether it has already left this stop but has not arrived at the destination yet.
			if(current_time < iter.value)
			{
				// It is possible that this is faster even so.
				// TODO: Add calculation code here based on the point to point times rather than skipping.
				continue;
			}

			// Check to see whether the convoy is running late.
			const sint64 this_stop_arrival = estimated_convoy_arrival_times.get(iter.key);
			if(this_stop_arrival <= welt->get_ticks() && !loading_here.is_contained(arrival_convoy))
			{
				// Assume that it will be as late again as it already is (e.g., if it is 2 minutes late so far, assume a total delay of 4 minutes)
				const sint64 estimated_total_delay = (welt->get_ticks() - this_stop_arrival) * waiting_multiplication_factor;
				current_time += estimated_total_delay;
			}

			if(current_time < best_arrival_time)
			{
				best_arrival_time = current_time;
				convoy = arrival_convoy;
			}
		}
	}
	return best_arrival_time;
}

#ifdef MULTI_THREAD
uint32 haltestelle_t::get_transferring_cargoes_count() const
{
	uint32 count = 0;
	for (sint32 i = 0; i < world()->get_parallel_operations(); i++)
	{
		count += transferring_cargoes[i].get_count();
	}
	return count;
}
#endif
