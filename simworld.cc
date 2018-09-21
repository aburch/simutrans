/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

/*
 * Hauptklasse fuer Simutrans, Datenstruktur die alles Zusammenhaelt
 * Hj. Malthaner, 1997
 */

#include <algorithm>
#include <limits>
#include <functional>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#include "path_explorer.h"

#include "simcity.h"
#include "simcolor.h"
#include "simconvoi.h"
#include "simdebug.h"
#include "simdepot.h"
#include "simfab.h"
#include "display/simgraph.h"
#include "display/viewport.h"
#include "simhalt.h"
#include "display/simimg.h"
#include "siminteraction.h"
#include "simintr.h"
#include "simio.h"
#include "simlinemgmt.h"
#include "simloadingscreen.h"
#include "simmenu.h"
#include "simmesg.h"
#include "simskin.h"
#include "simsound.h"
#include "simsys.h"
#include "simticker.h"
#include "utils/simrandom.h"
#include "simunits.h"
#include "simversion.h"
#include "display/simview.h"
#include "simtool.h"
#include "gui/simwin.h"
#include "simworld.h"

#include "vehicle/simpeople.h"

#include "tpl/vector_tpl.h"
#include "tpl/binary_heap_tpl.h"
#include "tpl/ordered_vector_tpl.h"
#include "tpl/stringhashtable_tpl.h"

#include "boden/boden.h"
#include "boden/wasser.h"

#include "old_blockmanager.h"
#include "vehicle/simvehicle.h"
#include "vehicle/simroadtraffic.h"
#include "vehicle/movingobj.h"
#include "boden/wege/schiene.h"

#include "obj/zeiger.h"
#include "obj/baum.h"
#include "obj/signal.h"
#include "obj/roadsign.h"
#include "obj/wayobj.h"
#include "obj/groundobj.h"
#include "obj/gebaeude.h"
#include "obj/leitung2.h"

#include "gui/password_frame.h"
#include "gui/messagebox.h"
#include "gui/help_frame.h"
#include "gui/karte.h"
#include "gui/player_frame_t.h"
#include "gui/components/gui_convoy_assembler.h"

#include "network/network.h"
#include "network/network_file_transfer.h"
#include "network/network_socket_list.h"
#include "network/network_cmd_ingame.h"
#include "dataobj/height_map_loader.h"
#include "dataobj/ribi.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "dataobj/scenario.h"
#include "dataobj/settings.h"
#include "dataobj/environment.h"
#include "dataobj/powernet.h"
#include "dataobj/records.h"
#include "dataobj/marker.h"

#include "utils/cbuffer_t.h"
#include "utils/simstring.h"
#include "network/memory_rw.h"

#include "bauer/brueckenbauer.h"
#include "bauer/tunnelbauer.h"
#include "bauer/fabrikbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"
#include "bauer/vehikelbauer.h"
#include "bauer/hausbauer.h"
#include "bauer/goods_manager.h"

#include "descriptor/ground_desc.h"
#include "descriptor/sound_desc.h"
#include "descriptor/tunnel_desc.h"
#include "descriptor/bridge_desc.h"
#include "descriptor/citycar_desc.h"
#include "descriptor/building_desc.h"

#include "player/simplay.h"
#include "player/finance.h"
#include "player/ai_passenger.h"
#include "player/ai_goods.h"

#include "dataobj/tabfile.h" // For reload of simuconf.tab to override savegames

#ifdef MULTI_THREAD
#include "utils/simthread.h"

static vector_tpl<pthread_t> private_car_route_threads;
static vector_tpl<pthread_t> unreserve_route_threads;
static vector_tpl<pthread_t> step_passengers_and_mail_threads;
static vector_tpl<pthread_t> individual_convoy_step_threads;
static vector_tpl<pthread_t> path_explorer_threads;
static pthread_t convoy_step_master_thread;
static pthread_t path_explorer_thread;

static pthread_attr_t thread_attributes;
static pthread_mutexattr_t mutex_attributes;

//static pthread_mutex_t private_car_route_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t karte_t::step_passengers_and_mail_mutex = PTHREAD_MUTEX_INITIALIZER;
//static pthread_mutex_t path_explorer_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t karte_t::unreserve_route_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t private_car_route_mutex;
pthread_mutex_t karte_t::step_passengers_and_mail_mutex;
static pthread_mutex_t path_explorer_mutex;
pthread_mutex_t karte_t::unreserve_route_mutex;

static simthread_barrier_t private_car_barrier;
simthread_barrier_t karte_t::unreserve_route_barrier;
static simthread_barrier_t step_passengers_and_mail_barrier;
static simthread_barrier_t start_path_explorer_barrier;
static simthread_barrier_t step_convoys_barrier_internal;
simthread_barrier_t karte_t::step_convoys_barrier_external;

static pthread_cond_t path_explorer_conditional_end = PTHREAD_COND_INITIALIZER;

bool karte_t::threads_initialised = false;

thread_local uint32 karte_t::passenger_generation_thread_number;
thread_local uint32 karte_t::marker_index = UINT32_MAX_VALUE;

sint32 karte_t::cities_to_process = 0;
vector_tpl<convoihandle_t> convoys_next_step;
sint32 karte_t::path_explorer_step_progress = -1;

vector_tpl<pedestrian_t*> *karte_t::pedestrians_added_threaded;
vector_tpl<private_car_t*> *karte_t::private_cars_added_threaded;
#endif
#ifdef MULTI_THREAD
vector_tpl<nearby_halt_t> *karte_t::start_halts;
vector_tpl<halthandle_t> *karte_t::destination_list;
#else
vector_tpl<nearby_halt_t> karte_t::start_halts;
vector_tpl<halthandle_t> karte_t::destination_list;
#endif

#ifdef DEBUG_SIMRAND_CALLS
bool karte_t::print_randoms = true;
int karte_t::random_calls = 0;
#endif
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
static uint32 halt_index = 9999999;
static const char *station_name = "Newton Abbot Railway Station";
static uint32 old_menge = -1;
void station_check(const char *who, karte_t *welt)
{
	player_t *player = welt->get_active_player();
	if (halt_index >= (uint32)player->get_haltcount() || 
		strcmp(player->get_halt(halt_index)->get_name(), station_name))
	{
		old_menge = -1;
		for (halt_index = 0; halt_index < (uint32) player->get_haltcount(); ++halt_index)
			if (!strcmp(player->get_halt(halt_index)->get_name(), station_name))
				break;
	}
	if (halt_index < (uint32) player->get_haltcount())
	{
		const halthandle_t &station = player->get_halt(halt_index);
		uint32 menge = station->get_warray(0)->get_element(2198).menge;
		if (old_menge != menge)
		{
			dbg->warning(who, "station \"%s\" goods[0][2198].menge %u -> %u", station->get_name(), old_menge, menge);
			old_menge = menge;
		}
	}
}
#endif
#endif

// advance 201 ms per sync_step in fast forward mode
#define MAGIC_STEP (201)

// frame per second for fast forward
#define FF_PPS (10)


static uint32 last_clients = -1;
static uint8 last_active_player_nr = 0;
static std::string last_network_game;

karte_t* karte_t::world = NULL;

stringhashtable_tpl<karte_t::missing_level_t>missing_pak_names;

#ifdef MULTI_THREAD
#include "utils/simthread.h"
#include <semaphore.h>

bool spawned_world_threads=false; // global job indicator array
static simthread_barrier_t world_barrier_start;
static simthread_barrier_t world_barrier_end;


// to start a thread
typedef struct{
	karte_t *welt;
	int thread_num;
	sint16 x_step;
	sint16 x_world_max;
	sint16 y_min;
	sint16 y_max;
	sem_t* wait_for_previous;
	sem_t* signal_to_next;
	xy_loop_func function;
	bool keep_running;
} world_thread_param_t;


// now the parameters
static world_thread_param_t world_thread_param[MAX_THREADS];

void *karte_t::world_xy_loop_thread(void *ptr)
{
	world_thread_param_t *param = reinterpret_cast<world_thread_param_t *>(ptr);
	while(true) {
		if(param->keep_running) {
			simthread_barrier_wait( &world_barrier_start );	// wait for all to start
		}

		sint16 x_min = 0;
		sint16 x_max = param->x_step;

		while(  x_min < param->x_world_max  ) {
			// wait for predecessor to finish its block
			if(  param->wait_for_previous  ) {
				sem_wait( param->wait_for_previous );
			}
			(param->welt->*(param->function))(x_min, x_max, param->y_min, param->y_max);

			// signal to next thread that we finished one block
			if(  param->signal_to_next  ) {
				sem_post( param->signal_to_next );
			}
			x_min = x_max;
			x_max = min(x_max + param->x_step, param->x_world_max);
		}

		if(param->keep_running) {
			simthread_barrier_wait( &world_barrier_end );	// wait for all to finish
		}
		else {
			return NULL;
		}
	}

	return ptr;
}
#endif


void karte_t::world_xy_loop(xy_loop_func function, uint8 flags)
{
	const bool use_grids = (flags & GRIDS_FLAG) == GRIDS_FLAG;
	uint16 max_x = use_grids?(cached_grid_size.x+1):cached_grid_size.x;
	uint16 max_y = use_grids?(cached_grid_size.y+1):cached_grid_size.y;
#ifdef MULTI_THREAD
	set_random_mode( INTERACTIVE_RANDOM ); // do not allow simrand() here!

	const bool sync_x_steps = (flags & SYNCX_FLAG) == SYNCX_FLAG;

	// semaphores to synchronize progress in x direction
	sem_t sems[MAX_THREADS-1];

	for(  int t = 0;  t < env_t::num_threads;  t++  ) {
		if(  sync_x_steps  &&  t < env_t::num_threads - 1  ) {
			sem_init(&sems[t], 0, 0);
		}

   		world_thread_param[t].welt = this;
   		world_thread_param[t].thread_num = t;
		world_thread_param[t].x_step = min( 64, max_x / env_t::num_threads );
		world_thread_param[t].x_world_max = max_x;
		world_thread_param[t].y_min = (t * max_y) / env_t::num_threads;
		world_thread_param[t].y_max = ((t + 1) * max_y) / env_t::num_threads;
		world_thread_param[t].function = function;

		world_thread_param[t].wait_for_previous = sync_x_steps  &&  t > 0 ? &sems[t-1] : NULL;
		world_thread_param[t].signal_to_next    = sync_x_steps  &&  t < env_t::num_threads - 1 ? &sems[t] : NULL;

		world_thread_param[t].keep_running = t < env_t::num_threads - 1;
	}

	if(  !spawned_world_threads  ) {
		// we can do the parallel display using posix threads ...
		pthread_t thread[MAX_THREADS];
		/* Initialize and set thread detached attribute */
		pthread_attr_t attr;
		pthread_attr_init( &attr );
		pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
		// init barrier
		simthread_barrier_init( &world_barrier_start, NULL, env_t::num_threads );
		simthread_barrier_init( &world_barrier_end, NULL, env_t::num_threads );

		for(  int t = 0;  t < env_t::num_threads - 1;  t++  ) {
			if(  pthread_create( &thread[t], &attr, world_xy_loop_thread, (void *)&world_thread_param[t] )  ) {
				dbg->fatal( "karte_t::world_xy_loop()", "cannot multithread, error at thread #%i", t+1 );
				return;
			}
		}
		spawned_world_threads = true;
		pthread_attr_destroy( &attr );
	}

	// and start processing
	simthread_barrier_wait( &world_barrier_start );

	// the last we can run ourselves
	world_xy_loop_thread(&world_thread_param[env_t::num_threads-1]);

	simthread_barrier_wait( &world_barrier_end );

	// return from thread
	for(  int t = 0;  t < env_t::num_threads - 1;  t++  ) {
		if(  sync_x_steps  ) {
			sem_destroy(&sems[t]);
		}
	}

	clear_random_mode( INTERACTIVE_RANDOM ); // do not allow simrand() here!

#else
	// slow serial way of display
	(this->*function)( 0, max_x, 0, max_y );
#endif
}


checklist_t::checklist_t(uint32 _ss, uint32 _st, uint8 _nfc, uint32 _random_seed, uint16 _halt_entry, uint16 _line_entry, uint16 _convoy_entry, uint32 *_rands)
	: ss(_ss), st(_st), nfc(_nfc), random_seed(_random_seed), halt_entry(_halt_entry), line_entry(_line_entry), convoy_entry(_convoy_entry)
{
	for(  uint8 i = 0;  i < CHK_RANDS; i++  ) {
		rand[i]	 = _rands[i];
	}
}


void checklist_t::rdwr(memory_rw_t *buffer)
{
	buffer->rdwr_long(ss);
	buffer->rdwr_long(st);
	buffer->rdwr_byte(nfc);
	buffer->rdwr_long(random_seed);
	buffer->rdwr_short(halt_entry);
	buffer->rdwr_short(line_entry);
	buffer->rdwr_short(convoy_entry);

	// desync debug
	for(  uint8 i = 0;  i < CHK_RANDS;  i++  ) {
		buffer->rdwr_long(rand[i]);
	}
}



int checklist_t::print(char *buffer, const char *entity) const
{
	return sprintf(buffer, "%s=[ss=%u st=%u nfc=%u rand=%u halt=%u line=%u cnvy=%u ssr=%u,%u,%u,%u,%u,%u,%u,%u str=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u exr=%u,%u,%u,%u,%u,%u,%u,%u  ",
		entity, ss, st, nfc, random_seed, halt_entry, line_entry, convoy_entry,
		rand[0], rand[1], rand[2], rand[3], rand[4], rand[5], rand[6], rand[7],
		rand[8], rand[9], rand[10], rand[11], rand[12], rand[13], rand[14], rand[15], rand[16], rand[17], rand[18], rand[19], rand[20], rand[21], rand[22], rand[23],
		rand[24], rand[25], rand[26], rand[27], rand[28], rand[29], rand[30], rand[31]
	);
}


void karte_t::recalc_season_snowline(bool set_pending)
{
	static const sint8 mfactor[12] = { 99, 95, 80, 50, 25, 10, 0, 5, 20, 35, 65, 85 };
	static const uint8 month_to_season[12] = { 2, 2, 2, 3, 3, 0, 0, 0, 0, 1, 1, 2 };

	// calculate snowline with day precision
	// use linear interpolation
	const sint32 ticks_this_month = get_ticks() & (karte_t::ticks_per_world_month - 1);
	const sint32 factor = mfactor[last_month] + (  ( (mfactor[(last_month + 1) % 12] - mfactor[last_month]) * (ticks_this_month >> 12) ) >> (karte_t::ticks_per_world_month_shift - 12) );

	// just remember them
	const uint8 old_season = season;
	const sint16 old_snowline = snowline;

	// and calculate new values
	season = month_to_season[last_month];   //  (2+last_month/3)&3; // summer always zero
	if(  old_season != season  && set_pending  ) {
		pending_season_change++;
	}

	const sint16 winterline = settings.get_winter_snowline();
	const sint16 summerline = settings.get_climate_borders()[arctic_climate] + 1;
	snowline = summerline - (sint16)(((summerline-winterline)*factor)/100) + groundwater;
	if(  old_snowline != snowline  && set_pending  ) {
		pending_snowline_change++;
	}
}

void karte_t::perlin_hoehe_loop( sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max )
{
	for(  int y = y_min;  y < y_max;  y++  ) {
		for(  int x = x_min; x < x_max;  x++  ) {
			// loop all tiles
			koord k(x,y);
			sint16 const h = perlin_hoehe(&settings, k, koord(0, 0));
			set_grid_hgt( k, (sint8) h);
		}
	}
}


/**
 * Height one point in the map with "perlin noise"
 *
 * @param frequency in 0..1.0 roughness, the higher the rougher
 * @param amplitude in 0..160.0 top height of mountains, may not exceed 160.0!!!
 * @author Hj. Malthaner
 */
sint32 karte_t::perlin_hoehe(settings_t const* const sets, koord k, koord const size, sint32 map_size_max)
{
	// Hajo: to Markus: replace the fixed values with your
	// settings. Amplitude is the top highness of the
	// mountains, frequency is something like landscape 'roughness'
	// amplitude may not be greater than 160.0 !!!
	// please don't allow frequencies higher than 0.8 it'll
	// break the AI's pathfinding. Frequency values of 0.5 .. 0.7
	// seem to be ok, less is boring flat, more is too crumbled
	// the old defaults are given here: f=0.6, a=160.0
	switch( sets->get_rotation() ) {
		// 0: do nothing
		case 1: k = koord(k.y,size.x-k.x); break;
		case 2: k = koord(size.x-k.x,size.y-k.y); break;
		case 3: k = koord(size.y-k.y,k.x); break;
	}
//    double perlin_noise_2D(double x, double y, double persistence);
//    return ((int)(perlin_noise_2D(x, y, 0.6)*160.0)) & 0xFFFFFFF0;
	k = k + koord(sets->get_origin_x(), sets->get_origin_y());
	return ((int)(perlin_noise_2D(k.x, k.y, sets->get_map_roughness(), map_size_max)*(double)sets->get_max_mountain_height())) / 16;
}

sint32 karte_t::perlin_hoehe(settings_t const* const sets, koord k, koord const size)
{
	return perlin_hoehe(sets, k, size, cached_size_max);
}


void karte_t::cleanup_grounds_loop( sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max )
{
	for(  int y = y_min;  y < y_max;  y++  ) {
		for(  int x = x_min; x < x_max;  x++  ) {
			planquadrat_t *pl = access_nocheck(x,y);
			grund_t *gr = pl->get_kartenboden();
			koord k(x,y);
			uint8 slope = calc_natural_slope(k);
			sint8 height = min_hgt_nocheck(k);
			sint8 water_hgt = get_water_hgt_nocheck(k);

			if(  height < water_hgt) {
				const sint8 disp_hn_sw = max( height + corner_sw(slope), water_hgt );
				const sint8 disp_hn_se = max( height + corner_se(slope), water_hgt );
				const sint8 disp_hn_ne = max( height + corner_ne(slope), water_hgt );
				const sint8 disp_hn_nw = max( height + corner_nw(slope), water_hgt );
				height = water_hgt;
				slope = (disp_hn_sw - height) + ((disp_hn_se - height) * 3) + ((disp_hn_ne - height) * 9) + ((disp_hn_nw - height) * 27);
			}

			gr->set_pos( koord3d( k, height) );
			if(  gr->get_typ() != grund_t::wasser  &&  max_hgt_nocheck(k) <= water_hgt  ) {
				// below water but ground => convert
				pl->kartenboden_setzen( new wasser_t(gr->get_pos()) );
			}
			else if(  gr->get_typ() == grund_t::wasser  &&  max_hgt_nocheck(k) > water_hgt  ) {
				// water above ground => to ground
				pl->kartenboden_setzen( new boden_t(gr->get_pos(), slope ) );
			}
			else {
				gr->set_grund_hang( slope );
			}

			if(  max_hgt_nocheck(k) > water_hgt  ) {
				set_water_hgt(k, groundwater-4);
			}
		}
	}
}


void karte_t::cleanup_karte( int xoff, int yoff )
{
	// we need a copy to smooth the map to a realistic level
	const sint32 grid_size = (get_size().x+1)*(sint32)(get_size().y+1);
	sint8 *grid_hgts_cpy = new sint8[grid_size];
	memcpy( grid_hgts_cpy, grid_hgts, grid_size );

	// the trick for smoothing is to raise each tile by one
	sint32 i,j;
	for(j=0; j<=get_size().y; j++) {
		for(i=j>=yoff?0:xoff; i<=get_size().x; i++) {
			raise_grid_to(i,j, grid_hgts_cpy[i+j*(get_size().x+1)] + 1);
		}
	}
	delete [] grid_hgts_cpy;

	// but to leave the map unchanged, we lower the height again
	for(j=0; j<=get_size().y; j++) {
		for(i=j>=yoff?0:xoff; i<=get_size().x; i++) {
			grid_hgts[i+j*(get_size().x+1)] --;
		}
	}

	if(  xoff==0 && yoff==0  ) {
		world_xy_loop(&karte_t::cleanup_grounds_loop, 0);
	}
	else {
		cleanup_grounds_loop( 0, get_size().x, yoff, get_size().y );
		cleanup_grounds_loop( xoff, get_size().x, 0, yoff );
	}
}


void karte_t::destroy()
{
	is_sound = false; // karte_t::play_sound_area_clipped needs valid zeiger (pointer/drawer)
	vehicle_t::sound_ticks = 0;
	destroying = true;
	DBG_MESSAGE("karte_t::destroy()", "destroying world");

#ifdef MULTI_THREAD
	destroy_threads();
	DBG_MESSAGE("karte_t::destroy()", "threads destroyed");
#else
	delete[] transferring_cargoes;
	transferring_cargoes = NULL;
#endif

	passenger_origins.clear();
	mail_origins_and_targets.clear();

	for (uint8 i = 0; i < goods_manager_t::passengers->get_number_of_classes(); i++)
	{
		commuter_targets[i].clear();
		visitor_targets[i].clear();
	}

	uint32 max_display_progress = 256+stadt.get_count()*10 + haltestelle_t::get_alle_haltestellen().get_count() + convoi_array.get_count() + (cached_size.x*cached_size.y)*2;
	uint32 old_progress = 0;

	loadingscreen_t ls( translator::translate("Destroying map ..."), max_display_progress, true );

	// rotate the map until it can be saved
	nosave_warning = false;
	if(  nosave  ) {
		max_display_progress += 256;
		for( int i=0;  i<4  &&  nosave;  i++  ) {
	DBG_MESSAGE("karte_t::destroy()", "rotating");
			rotate90();
		}
		old_progress += 256;
		ls.set_max( max_display_progress );
		ls.set_progress( old_progress );
	}
	if(nosave) {
		dbg->fatal( "karte_t::destroy()","Map cannot be cleanly destroyed in any rotation!" );
	}

	DBG_MESSAGE("karte_t::destroy()", "label clear");
	labels.clear();

	if(zeiger) {
		zeiger->set_pos(koord3d::invalid);
		delete zeiger;
		zeiger = NULL;
	}

	old_progress += 256;
	ls.set_progress( old_progress );

	// removes all moving stuff from the sync_step
	sync.clear();
	sync_eyecandy.clear();
	sync_way_eyecandy.clear();
	old_progress += cached_size.x*cached_size.y;
	ls.set_progress( old_progress );
	DBG_MESSAGE("karte_t::destroy()", "sync list cleared");

	// all convois aufraeumen
	while (!convoi_array.empty()) {
		convoihandle_t cnv = convoi_array.back();
		cnv->destroy();
		old_progress ++;
		if(  (old_progress&0x00FF) == 0  ) {
			ls.set_progress( old_progress );
		}
	}
	convoi_array.clear();
	DBG_MESSAGE("karte_t::destroy()", "convois destroyed");

	// all haltestellen aufraeumen
	old_progress += haltestelle_t::get_alle_haltestellen().get_count();
	haltestelle_t::destroy_all();
	DBG_MESSAGE("karte_t::destroy()", "stops destroyed");
	ls.set_progress( old_progress );

	// delete towns first (will also delete all their houses)
	// for the next game we need to remember the desired number ...
	sint32 const no_of_cities = settings.get_city_count();
	for(  uint32 i=0;  !stadt.empty();  i++  ) {
		remove_city(stadt.front());
		old_progress += 10;
		if(  (i&0x00F) == 0  ) {
			ls.set_progress( old_progress );
		}
	}
	settings.set_city_count(no_of_cities);

	DBG_MESSAGE("karte_t::destroy()", "towns destroyed");

	ls.set_progress( old_progress );

	// dinge aufraeumen
	cached_grid_size.x = cached_grid_size.y = 1;
	cached_size.x = cached_size.y = 0;
	delete[] plan;
	plan = NULL;
	DBG_MESSAGE("karte_t::destroy()", "planquadrat destroyed");

	old_progress += (cached_size.x*cached_size.y)/2;
	ls.set_progress( old_progress );

	// gitter aufraeumen
	delete [] grid_hgts;
	grid_hgts = NULL;

	delete [] water_hgts;
	water_hgts = NULL;

	// players aufraeumen
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		delete players[i];
		players[i] = NULL;
	}
	DBG_MESSAGE("karte_t::destroy()", "player destroyed");

	old_progress += (cached_size.x*cached_size.y)/4;
	ls.set_progress( old_progress );

	// all fabriken aufraeumen
	// Clean up all factories
	FOR(vector_tpl<fabrik_t*>, const f, fab_list) {
		delete f;
	}
	fab_list.clear();
	DBG_MESSAGE("karte_t::destroy()", "factories destroyed");

	// hier nur entfernen, aber nicht loeschen
	world_attractions.clear();
	DBG_MESSAGE("karte_t::destroy()", "attraction list destroyed");

	weg_t::clear_list_of__ways();
	DBG_MESSAGE("karte_t::destroy()", "way list destroyed");

	delete scenario;
	scenario = NULL;

	senke_t::new_world();
	pumpe_t::new_world();

	bool empty_depot_list = depot_t::get_depot_list().empty();
	assert( empty_depot_list );

	DBG_MESSAGE("karte_t::destroy()", "world destroyed");

	// Added by : B.Gabriel
	route_t::TERM_NODES();

	// Added by : Knightly
	path_explorer_t::finalise();

	dbg->important("World destroyed.");
	destroying = false;
#ifdef MULTI_THREAD
	cities_to_process = 0;
	terminating_threads = false;
#endif
}


void karte_t::add_convoi(convoihandle_t const &cnv)
{
	assert(cnv.is_bound());
	convoi_array.append_unique(cnv);
}


void karte_t::rem_convoi(convoihandle_t const &cnv)
{
	convoi_array.remove(cnv);
}


void karte_t::add_city(stadt_t *s)
{
	settings.set_city_count(settings.get_city_count() + 1);
	stadt.append(s, s->get_einwohner());
}


bool karte_t::remove_city(stadt_t *s)
{
	if(s == NULL  ||  stadt.empty()) {
		// no town there to delete ...
		return false;
	}

	// reduce number of towns
	if(s->get_name()) {
		DBG_MESSAGE("karte_t::remove_city()", "%s", s->get_name());
	}
	stadt.remove(s);
	DBG_DEBUG4("karte_t::remove_city()", "reduce city to %i", settings.get_city_count() - 1);
	settings.set_city_count(settings.get_city_count() - 1);

	// ok, we can delete this
	DBG_MESSAGE("karte_t::remove_city()", "delete" );
	delete s;

	return true;
}


// just allocates space;
void karte_t::init_tiles()
{
	assert(plan==0);

	uint32 const x = get_size().x;
	uint32 const y = get_size().y;
	plan      = new planquadrat_t[x * y];
	grid_hgts = new sint8[(x + 1) * (y + 1)];
	max_height = min_height = 0;
	MEMZERON(grid_hgts, (x + 1) * (y + 1));
	water_hgts = new sint8[x * y];
	MEMZERON(water_hgts, x * y);

	win_set_world( this );
	reliefkarte_t::get_karte()->init();

	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		// old default: AI 3 passenger, other goods
		players[i] = (i<2) ? new player_t(this,i) : NULL;
	}
	active_player = players[0];
	active_player_nr = 0;

	// clear world records
	records->clear_speed_records();

	// make timer loop invalid
	for( int i=0;  i<32;  i++ ) {
		last_frame_ms[i] = 0x7FFFFFFFu;
		last_step_nr[i] = 0xFFFFFFFFu;
	}
	last_frame_idx = 0;
	pending_season_change = 0;
	pending_snowline_change = 0;

	// init global history
	for (int year=0; year<MAX_WORLD_HISTORY_YEARS; year++) {
		for (int cost_type=0; cost_type<MAX_WORLD_COST; cost_type++) {
			finance_history_year[year][cost_type] = 0;
		}
	}
	for (int month=0; month<MAX_WORLD_HISTORY_MONTHS; month++) {
		for (int cost_type=0; cost_type<MAX_WORLD_COST; cost_type++) {
			finance_history_month[month][cost_type] = 0;
		}
	}
	last_month_bev = 0;

	tile_counter = 0;

	convoihandle_t::init( 1024 );
	linehandle_t::init( 1024 );

	halthandle_t::init( 1024 );

	vehicle_base_t::set_overtaking_offsets( get_settings().is_drive_left() );

	scenario = new scenario_t(this);

	nosave_warning = nosave = false;

	if (env_t::server) {
		nwc_auth_player_t::init_player_lock_server(this);
	}
}


void karte_t::set_scenario(scenario_t *s)
{
	if (scenario != s) {
		delete scenario;
	}
	scenario = s;
}


void karte_t::create_rivers( sint16 number )
{
	// First check, whether there is a canal:
	const way_desc_t* river_desc = way_builder_t::get_desc( env_t::river_type[env_t::river_types-1], 0 );
	if(  river_desc == NULL  ) {
		// should never reaching here ...
		dbg->warning("karte_t::create_rivers()","There is no river defined!\n");
		return;
	}

	// create a vector of the highest points
	vector_tpl<koord> water_tiles;
	weighted_vector_tpl<koord> mountain_tiles;

	koord last_koord(0,0);

	// trunk of 16 will ensure that rivers are long enough apart ...
	for(  sint16 y = 8;  y < cached_size.y;  y+=16  ) {
		for(  sint16 x = 8;  x < cached_size.x;  x+=16  ) {
			koord k(x,y);
			grund_t *gr = lookup_kartenboden_nocheck(k);
			const sint8 h = gr->get_hoehe() - get_water_hgt_nocheck(k);
			if(  gr->is_water()  ) {
				// may be good to start a river here
				water_tiles.append(k);
			}
			else {
				mountain_tiles.append( k, h * h );
			}
		}
	}
	if (water_tiles.empty()) {
		dbg->message("karte_t::create_rivers()","There aren't any water tiles!\n");
		return;
	}

	// now make rivers
	int river_count = 0;
	sint16 retrys = number*2;
	while(  number > 0  &&  !mountain_tiles.empty()  &&  retrys>0  ) {

		// start with random coordinates
		koord const start = pick_any_weighted(mountain_tiles);
		mountain_tiles.remove( start );

		// build a list of matching targets
		vector_tpl<koord> valid_water_tiles;

		for(  uint32 i=0;  i<water_tiles.get_count();  i++  ) {
			sint16 dist = koord_distance(start,water_tiles[i]);
			if(  settings.get_min_river_length() < dist  &&  dist < settings.get_max_river_length()  ) {
				valid_water_tiles.append( water_tiles[i] );
			}
		}

		// now try 512 random locations
		for(  sint32 i=0;  i<512  &&  !valid_water_tiles.empty();  i++  ) {
			koord const end = pick_any(valid_water_tiles);
			valid_water_tiles.remove( end );
			way_builder_t riverbuilder(players[1]);
			riverbuilder.init_builder(way_builder_t::river, river_desc);
			sint16 dist = koord_distance(start,end);
			riverbuilder.set_maximum( dist*50 );
			riverbuilder.calc_route( lookup_kartenboden(end)->get_pos(), lookup_kartenboden(start)->get_pos() );
			if(  riverbuilder.get_count() >= (uint32)settings.get_min_river_length()  ) {
				// do not built too short rivers
				riverbuilder.build();
				river_count++;
				number--;
				retrys++;
				break;
			}
		}

		retrys--;
	}
	// we gave up => tell the user
	if(  number>0  ) {
		dbg->warning( "karte_t::create_rivers()","Too many rivers requested! (only %i rivers placed)", river_count );
	}
}

void karte_t::remove_queued_city(stadt_t* city)
{
	cities_awaiting_private_car_route_check.remove(city);
}

void karte_t::add_queued_city(stadt_t* city)
{
	cities_awaiting_private_car_route_check.append(city);
}

void karte_t::distribute_cities(settings_t const * const sets, sint16 old_x, sint16 old_y)
{
	sint32 new_city_count = abs(sets->get_city_count());

	const uint32 number_of_big_cities = env_t::number_of_big_cities;

	dbg->important("Creating cities ...");
	DBG_DEBUG("karte_t::distribute_groundobjs_cities()", "prepare cities sizes");

	const sint32 city_population_target_count = stadt.empty() ? new_city_count : new_city_count + stadt.get_count() + 1;

	vector_tpl<sint32> city_population(city_population_target_count);
	sint32 median_population = abs(sets->get_mean_einwohnerzahl());

	const uint32 max_city_size = sets->get_max_city_size();
	const uint32 max_small_city_size = max(sets->get_max_small_city_size(), median_population / 2);

	// Generate random sizes to fit a Pareto distribution: P(x) = x_m / x^2 dx.
	// This ensures that Zipf's law is satisfied in a random fashion, and
	// arises from the observation that city distribution is self-similar.
	// The median of a Pareto distribution is twice the lower cut-off, x_m.
	// We can generate a Pareto deviate from a uniform deviate on range [0,1)
	// by taking m_x/u where u is the uniform deviate.

	while (city_population.get_count() < city_population_target_count) {
		uint32 population;
		do {
			uint32 rand;
			do {
				rand = simrand_plain();
			} while (rand == 0);

			population = ((double)median_population / 2) / ((double)rand / 0xffffffff);
		} while (city_population.get_count() < number_of_big_cities && (population <= max_small_city_size || population > max_city_size) ||
			city_population.get_count() >= number_of_big_cities &&  population > max_small_city_size);

		city_population.insert_ordered(population, std::greater<sint32>());
	}


#ifdef DEBUG
	for (sint32 i = 0; i < city_population_target_count; i++)
	{
		DBG_DEBUG("karte_t::distribute_groundobjs_cities()", "City rank %d -- %d", i, city_population[i]);
	}

	DBG_DEBUG("karte_t::distribute_groundobjs_cities()", "prepare cities");
#endif

	vector_tpl<koord> *pos = stadt_t::random_place(this, &city_population, old_x, old_y);

	if (pos->empty()) {
		// could not generate any town
		if (pos) {
			delete pos;
		}
		settings.set_city_count(stadt.get_count()); // new number of towns (if we did not find enough positions)
		return;
	}
	// Extra indentation here is to allow for better diff files; it used to be in a block

	const sint32 old_city_count = stadt.get_count();
	if (pos->get_count() < new_city_count) {
		new_city_count = pos->get_count();
		// Under no circumstances increase the number of new cities!
	}
	dbg->important("Creating cities: %d", new_city_count);

	// prissi if we could not generate enough positions ...
	settings.set_city_count(old_city_count);
	int old_progress = 16;

	// Ansicht auf erste City zentrieren
	if (old_x + old_y == 0) {
		viewport->change_world_position(koord3d((*pos)[0], min_hgt((*pos)[0])));
	}
	uint32 max_progress = 16 + 2 * (old_city_count + new_city_count) + 2 * new_city_count + (old_x == 0 ? settings.get_factory_count() : 0);
	loadingscreen_t ls(translator::translate("distributing cities"), max_progress, true, true);

	{
		// Loop only new cities:
		uint32 tbegin = dr_time();
		for (unsigned i = 0; i < new_city_count; i++) {
			stadt_t* s = new stadt_t(players[1], (*pos)[i], 1);
			DBG_DEBUG("karte_t::distribute_groundobjs_cities()", "Erzeuge stadt %i with %ld inhabitants", i, (s->get_city_history_month())[HIST_CITICENS]);
			if (s->get_buildings() > 0) {
				add_city(s);
			}
			else {
				delete(s);
			}
		}

		delete pos;
#ifdef DEBUG
		dbg->message("karte_t::distribute_groundobjs_cities()", "took %lu ms for all towns", dr_time() - tbegin);
#endif

		uint32 game_start = current_month;
		// townhalls available since?
		FOR(vector_tpl<building_desc_t const*>, const desc, *hausbauer_t::get_list(building_desc_t::townhall)) {
			uint32 intro_year_month = desc->get_intro_year_month();
			if (intro_year_month < game_start) {
				game_start = intro_year_month;
			}
		}
		// streets since when?
		game_start = max(game_start, way_builder_t::get_earliest_way(road_wt)->get_intro_year_month());

		uint32 original_start_year = current_month;
		uint32 original_industry_growth = settings.get_industry_increase_every();
		settings.set_industry_increase_every(0);

		for (uint32 i = old_city_count; i < stadt.get_count(); i++) {
			// Hajo: do final init after world was loaded/created
			stadt[i]->finish_rd();

			const uint32 citizens = city_population.get_count() > i ? city_population[i] : city_population.get_element(simrand(city_population.get_count() - 1, "void karte_t::distribute_groundobjs_cities"));

			sint32 diff = (original_start_year - game_start) / 2;
			sint32 growth = 32;
			sint32 current_bev = stadt[i]->get_einwohner();

			/* grow gradually while aging
			 * the difference to the current end year will be halved,
			 * while the growth step is doubled
			 */
			current_month = game_start;
			bool not_updated = false;
			bool new_town = true;
			while (current_bev < citizens) {
				growth = min(citizens - current_bev, growth * 2);
				current_bev = stadt[i]->get_einwohner();
				stadt[i]->change_size(growth, new_town, true);
				// Only "new" for the first change_size call
				new_town = false;
				if (current_bev > citizens / 2 && not_updated) {
					ls.set_progress(++old_progress);
					not_updated = true;
				}
				current_month += diff;
				diff >>= 1;
			}

			// the growth is slow, so update here the progress bar
			ls.set_progress(++old_progress);
		}

		current_month = original_start_year;
		settings.set_industry_increase_every(original_industry_growth);
		msg->clear();
	}

	finance_history_year[0][WORLD_TOWNS] = finance_history_month[0][WORLD_TOWNS] = stadt.get_count();
	finance_history_year[0][WORLD_CITICENS] = finance_history_month[0][WORLD_CITICENS] = 0;
	finance_history_year[0][WORLD_JOBS] = finance_history_month[0][WORLD_JOBS] = 0;
	finance_history_year[0][WORLD_VISITOR_DEMAND] = finance_history_month[0][WORLD_VISITOR_DEMAND] = 0;

	FOR(weighted_vector_tpl<stadt_t*>, const city, stadt)
	{
		finance_history_year[0][WORLD_CITICENS] += city->get_finance_history_month(0, HIST_CITICENS);
		finance_history_month[0][WORLD_CITICENS] += city->get_finance_history_year(0, HIST_CITICENS);

		finance_history_month[0][WORLD_JOBS] += city->get_finance_history_month(0, HIST_JOBS);
		finance_history_year[0][WORLD_JOBS] += city->get_finance_history_year(0, HIST_JOBS);

		finance_history_month[0][WORLD_VISITOR_DEMAND] += city->get_finance_history_month(0, HIST_VISITOR_DEMAND);
		finance_history_year[0][WORLD_VISITOR_DEMAND] += city->get_finance_history_year(0, HIST_VISITOR_DEMAND);
	}



	// Hajo: connect some cities with roads
	ls.set_what(translator::translate("Connecting cities ..."));
	way_desc_t const* desc = settings.get_intercity_road_type(get_timeline_year_month());
	if (desc == NULL || !settings.get_use_timeline()) {
		// Hajo: try some default (might happen with timeline ... )
		desc = way_builder_t::weg_search(road_wt, 80, get_timeline_year_month(), type_flat);
	}

	way_builder_t bauigel(NULL);
	bauigel.init_builder(way_builder_t::strasse | way_builder_t::terraform_flag, desc, tunnel_builder_t::get_tunnel_desc(road_wt, desc->get_topspeed(), get_timeline_year_month()), bridge_builder_t::find_bridge(road_wt, desc->get_topspeed(), get_timeline_year_month(), desc->get_max_axle_load() * 2));
	bauigel.set_keep_existing_ways(true);
	bauigel.set_maximum(env_t::intercity_road_length);

	// **** intercity road construction
	int count = 0;
	sint32 const n_cities = settings.get_city_count();
	int    const max_count = n_cities * (n_cities - 1) / 2 - old_city_count * (old_city_count - 1) / 2;
	// something to do??
	if (max_count > 0) {
		// print("Building intercity roads ...\n");
		ls.set_max(16 + 2 * (old_city_count + new_city_count) + 2 * new_city_count + (old_x == 0 ? settings.get_factory_count() : 0));
		// find townhall of city i and road in front of it
		vector_tpl<koord3d> k;
		for (int i = 0; i < settings.get_city_count(); ++i) {
			koord k1(stadt[i]->get_townhall_road());
			if (lookup_kartenboden(k1) && lookup_kartenboden(k1)->hat_weg(road_wt)) {
				k.append(lookup_kartenboden(k1)->get_pos());
			}
			else {
				// look for a road near the townhall
				gebaeude_t const* const gb = obj_cast<gebaeude_t>(lookup_kartenboden(stadt[i]->get_pos())->first_obj());
				bool ok = false;
				if (gb  &&  gb->is_townhall()) {
					koord k_check = stadt[i]->get_pos() + koord(-1, -1);
					const koord size = gb->get_tile()->get_desc()->get_size(gb->get_tile()->get_layout());
					koord inc(1, 0);
					// scan all adjacent tiles, take the first that has a road
					for (sint32 i = 0; i < 2 * size.x + 2 * size.y + 4 && !ok; i++) {
						grund_t *gr = lookup_kartenboden(k_check);
						if (gr  &&  gr->hat_weg(road_wt)) {
							k.append(gr->get_pos());
							ok = true;
						}
						k_check = k_check + inc;
						if (i == size.x + 1) {
							inc = koord(0, 1);
						}
						else if (i == size.x + size.y + 2) {
							inc = koord(-1, 0);
						}
						else if (i == 2 * size.x + size.y + 3) {
							inc = koord(0, -1);
						}
					}
				}
				if (!ok) {
					k.append(koord3d::invalid);
				}
			}
		}
		// compute all distances
		uint8 conn_comp = 1; // current connection component for phase 0
		vector_tpl<uint8> city_flag; // city already connected to the graph? >0 nr of connection component
		array2d_tpl<sint32> city_dist(settings.get_city_count(), settings.get_city_count());
		for (sint32 i = 0; i < settings.get_city_count(); ++i) {
			city_dist.at(i, i) = 0;
			for (sint32 j = i + 1; j < settings.get_city_count(); ++j) {
				city_dist.at(i, j) = koord_distance(k[i], k[j]);
				city_dist.at(j, i) = city_dist.at(i, j);
				// count unbuildable connections to new cities
				if (j >= old_city_count && city_dist.at(i, j) >= env_t::intercity_road_length) {
					count++;
				}
			}
			city_flag.append(i < old_city_count ? conn_comp : 0);

			// progress bar stuff
			ls.set_progress(16 + 2 * new_city_count + count * settings.get_city_count() * 2 / max_count);
		}
		// mark first town as connected
		if (old_city_count == 0) {
			city_flag[0] = conn_comp;
		}

		// get a default vehicle
		route_t verbindung;
		vehicle_t* test_driver;
		vehicle_desc_t test_drive_desc(road_wt, 500, vehicle_desc_t::diesel);
		test_driver = vehicle_builder_t::build(koord3d(), players[1], NULL, &test_drive_desc);
		test_driver->set_flag(obj_t::not_on_map);

		bool ready = false;
		uint8 phase = 0;
		// 0 - first phase: built minimum spanning tree (edge weights: city distance)
		// 1 - second phase: try to complete the graph, avoid edges that
		// == have similar length then already existing connection
		// == lead to triangles with an angle >90 deg

		while (phase < 2) {
			ready = true;
			koord conn = koord::invalid;
			sint32 best = env_t::intercity_road_length;

			if (phase == 0) {
				// loop over all unconnected cities
				for (int i = 0; i < settings.get_city_count(); ++i) {
					if (city_flag[i] == conn_comp) {
						// loop over all connections to connected cities
						for (int j = old_city_count; j < settings.get_city_count(); ++j) {
							if (city_flag[j] == 0) {
								ready = false;
								if (city_dist.at(i, j) < best) {
									best = city_dist.at(i, j);
									conn = koord(i, j);
								}
							}
						}
					}
				}
				// did we completed a connection component?
				if (!ready  &&  best == env_t::intercity_road_length) {
					// next component
					conn_comp++;
					// try the first not connected city
					ready = true;
					for (int i = old_city_count; i < settings.get_city_count(); ++i) {
						if (city_flag[i] == 0) {
							city_flag[i] = conn_comp;
							ready = false;
							break;
						}
					}
				}
			}
			else {
				// loop over all unconnected cities
				for (int i = 0; i < settings.get_city_count(); ++i) {
					for (int j = max(old_city_count, i + 1); j < settings.get_city_count(); ++j) {
						if (city_dist.at(i, j) < best  &&  city_flag[i] == city_flag[j]) {
							bool ok = true;
							// is there a connection i..l..j ? forbid stumpfe winkel
							for (int l = 0; l < settings.get_city_count(); ++l) {
								if (city_flag[i] == city_flag[l] && city_dist.at(i, l) == env_t::intercity_road_length  &&  city_dist.at(j, l) == env_t::intercity_road_length) {
									// cosine < 0 ?
									koord3d d1 = k[i] - k[l];
									koord3d d2 = k[j] - k[l];
									if (d1.x*d2.x + d1.y*d2.y < 0) {
										city_dist.at(i, j) = env_t::intercity_road_length + 1;
										city_dist.at(j, i) = env_t::intercity_road_length + 1;
										ok = false;
										count++;
										break;
									}
								}
							}
							if (ok) {
								ready = false;
								best = city_dist.at(i, j);
								conn = koord(i, j);
							}
						}
					}
				}
			}
			// valid connection?
			if (conn.x >= 0) {
				// is there a connection already
				const bool connected = (phase == 1 && verbindung.calc_route(this, k[conn.x], k[conn.y], test_driver, 0, 0, false, 0)) == route_t::valid_route;
				// build this connection?
				bool build = false;
				// set appropriate max length for way builder
				if (connected) {
					if (2 * verbindung.get_count() > (uint32)city_dist.at(conn)) {
						bauigel.set_maximum(verbindung.get_count() / 2);
						build = true;
					}
				}
				else {
					bauigel.set_maximum(env_t::intercity_road_length);
					build = true;
				}

				if (build) {
					bauigel.calc_route(k[conn.x], k[conn.y]);
				}

				if (build  &&  bauigel.get_count() >= 2) {
					bauigel.build();
					if (phase == 0) {
						city_flag[conn.y] = conn_comp;
					}
					// mark as built
					city_dist.at(conn) = env_t::intercity_road_length;
					city_dist.at(conn.y, conn.x) = env_t::intercity_road_length;
					count++;
				}
				else {
					// do not try again
					city_dist.at(conn) = env_t::intercity_road_length + 1;
					city_dist.at(conn.y, conn.x) = env_t::intercity_road_length + 1;
					count++;

					if (phase == 0) {
						// do not try to connect to this connected component again
						for (int i = 0; i < settings.get_city_count(); ++i) {
							if (city_flag[i] == conn_comp  && city_dist.at(i, conn.y) < env_t::intercity_road_length) {
								city_dist.at(i, conn.y) = env_t::intercity_road_length + 1;
								city_dist.at(conn.y, i) = env_t::intercity_road_length + 1;
								count++;
							}
						}
					}
				}
			}

			// progress bar stuff
			ls.set_progress(16 + 2 * new_city_count + count * settings.get_city_count() * 2 / max_count);

			// next phase?
			if (ready) {
				phase++;
				ready = false;
			}
		}
		delete test_driver;
	}
}

void karte_t::distribute_groundobjs_cities( settings_t const * const sets, sint16 old_x, sint16 old_y)
{
	DBG_DEBUG("karte_t::distribute_groundobjs_cities()","distributing groundobjs");

	if (env_t::river_types > 0 && settings.get_river_number() > 0) {
		create_rivers(settings.get_river_number());
	}

	sint32 new_city_count = abs(sets->get_city_count());
	// Do city and road creation if (and only if) cities were requested.
	if (new_city_count > 0) {
		this->distribute_cities(sets, old_x, old_y);
	}

	DBG_DEBUG("karte_t::distribute_groundobjs_cities()","distributing groundobjs");
	if(  env_t::ground_object_probability > 0  ) {
		// add eyecandy like rocky, moles, flowers, ...
		koord k;
		sint32 queried = simrand(env_t::ground_object_probability*2-1, "karte_t::distribute_groundobjs_cities(), distributing groundobjs - 1st instance");
		for(  k.y=0;  k.y<get_size().y;  k.y++  ) {
			for(  k.x=(k.y<old_y)?old_x:0;  k.x<get_size().x;  k.x++  ) {
				grund_t *gr = lookup_kartenboden_nocheck(k);
				if(  gr->get_typ()==grund_t::boden  &&  !gr->hat_wege()  ) {
					queried --;
					if(  queried<0  ) {
						// test for beach
						bool neighbour_water = false;
						for(int i=0; i<8; i++) {
							if(  is_within_limits(k + koord::neighbours[i])  &&  get_climate( k + koord::neighbours[i] ) == water_climate  ) {
								neighbour_water = true;
								break;
							}
						}
						const climate_bits cl = neighbour_water ? water_climate_bit : (climate_bits)(1<<get_climate(k));
						const groundobj_desc_t *desc = groundobj_t::random_groundobj_for_climate( cl, gr->get_grund_hang() );
						if(desc) {
							queried = simrand(env_t::ground_object_probability*2-1, "karte_t::distribute_groundobjs_cities(), distributing groundobjs - 2nd instance");
							gr->obj_add( new groundobj_t( gr->get_pos(), desc ) );
						}
					}
				}
			}
		}
	}


DBG_DEBUG("karte_t::distribute_groundobjs_cities()","distributing movingobjs");
	if(  env_t::moving_object_probability > 0  ) {
		// add animals and so on (must be done after growing and all other objects, that could change ground coordinates)
		koord k;

		bool has_water = movingobj_t::random_movingobj_for_climate( water_climate )!=NULL;	
		const uint32 max_queried = env_t::moving_object_probability*2-1; 
		sint32 queried = simrand(max_queried, "karte_t::distribute_groundobjs_cities()");
		// no need to test the borders, since they are mostly slopes anyway
		for(k.y=1; k.y<get_size().y-1; k.y++) {
			for(k.x=(k.y<old_y)?old_x:1; k.x<get_size().x-1; k.x++) {
				grund_t *gr = lookup_kartenboden_nocheck(k);
				// flat ground or open water
				if (gr->get_top() == 0 && ((gr->get_typ() == grund_t::boden  &&  gr->get_grund_hang() == slope_t::flat) || (has_water  &&  gr->is_water()))) {
					queried --;
					if(  queried<0  ) {
						const groundobj_desc_t *desc = movingobj_t::random_movingobj_for_climate( get_climate(k) );
						if(  desc  &&  ( desc->get_waytype() != water_wt  ||  gr->get_hoehe() <= get_water_hgt_nocheck(k) )  ) {
							if(desc->get_speed()!=0) {
								queried = simrand(max_queried, "karte_t::distribute_groundobjs_cities()");
								gr->obj_add( new movingobj_t( gr->get_pos(), desc ) );
							}
						}
					}
				}
			}
		}
	}
}


void karte_t::init(settings_t* const sets, sint8 const* const h_field)
{
	clear_random_mode( 7 );
	mute_sound(true);
	if (env_t::networkmode) {
		if (env_t::server) {
			network_reset_server();
		}
		else {
			network_core_shutdown();
		}
	}
	step_mode  = PAUSE_FLAG;
	intr_disable();

	if(plan) {
		destroy();

		// Added by : Knightly
		path_explorer_t::initialise(this);
	}

	for(  uint i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		set_tool(tool_t::general_tool[TOOL_QUERY], get_player(i));
	}
	if(is_display_init()) {
		display_show_pointer(false);
	}
	viewport->change_world_position( koord(0,0), 0, 0 );

	settings = *sets;
	// names during creation time
	settings.set_name_language_iso(env_t::language_iso);
	settings.set_use_timeline(settings.get_use_timeline() & 1);

	ticks = 0;
	last_step_ticks = ticks;
	// ticks = 0x7FFFF800;  // Testing the 31->32 bit step

	last_month = 0;
	last_year = settings.get_starting_year();
	current_month = last_month + (last_year*12);
	set_ticks_per_world_month_shift(settings.get_bits_per_month());
	next_month_ticks =  karte_t::ticks_per_world_month;
	season=(2+last_month/3)&3; // summer always zero
	steps = 0;
	network_frame_count = 0;
	sync_steps = 0;
	sync_steps_barrier = sync_steps;
	map_counter = 0;
	recalc_average_speed();	// resets timeline

	groundwater = (sint8)sets->get_groundwater();      //29-Nov-01     Markus Weber    Changed

	init_height_to_climate();
	snowline = sets->get_winter_snowline() + groundwater;

	if(sets->get_beginner_mode()) {
		goods_manager_t::set_multiplier(settings.get_beginner_price_factor(), settings.get_meters_per_tile());
		settings.set_just_in_time( 0 );
	}
	else {
		goods_manager_t::set_multiplier(1000, settings.get_meters_per_tile());
	}

	recalc_season_snowline(false);

	stadt.clear();

DBG_DEBUG("karte_t::init()","hausbauer_t::new_world()");
	// Call this before building cities
	hausbauer_t::new_world();

	cached_grid_size.x = 0;
	cached_grid_size.y = 0;

DBG_DEBUG("karte_t::init()","init_tiles");
	init_tiles();

	enlarge_map(&settings, h_field);

DBG_DEBUG("karte_t::init()","distributing trees");
	if (!settings.get_no_trees()) {
		baum_t::distribute_trees(3);
	}

DBG_DEBUG("karte_t::init()","built timeline");
	private_car_t::build_timeline_list(this);

	nosave_warning = nosave = false;

	dbg->important("Creating factories ...");
	factory_builder_t::new_world();

	int consecutive_build_failures = 0;

	loadingscreen_t ls( translator::translate("distributing factories"), 16 + settings.get_city_count() * 4 + settings.get_factory_count(), true, true );

	while(  fab_list.get_count() < (uint32)settings.get_factory_count()  ) {
		if(  !factory_builder_t::increase_industry_density( false, false, false, true )  ) {
			if(  ++consecutive_build_failures > 3  ) {
				// Industry chain building starts failing consecutively as map approaches full.
				break;
			}
		}
		else {
			consecutive_build_failures = 0;
		}
		ls.set_progress( 16 + settings.get_city_count() * 4 + min(fab_list.get_count(),settings.get_factory_count()) );
	}

	settings.set_factory_count( fab_list.get_count() );
	finance_history_year[0][WORLD_FACTORIES] = finance_history_month[0][WORLD_FACTORIES] = fab_list.get_count();

	// tourist attractions
	ls.set_what(translator::translate("Placing attractions ..."));
	// Not worth actually constructing a progress bar, very fast
	factory_builder_t::distribute_attractions(settings.get_tourist_attractions());

	ls.set_what(translator::translate("Finalising ..."));
	// Not worth actually constructing a progress bar, very fast
	dbg->important("Preparing startup ...");
	if(zeiger == 0) {
		zeiger = new zeiger_t(koord3d::invalid, NULL );
	}

	// finishes the line preparation and sets id 0 to invalid ...
	players[0]->simlinemgmt.finish_rd();

	set_tool( tool_t::general_tool[TOOL_QUERY], get_active_player() );

	recalc_average_speed();

	// @author: jamespetts
	calc_generic_road_time_per_tile_city();
	calc_generic_road_time_per_tile_intercity();
	calc_max_road_check_depth();

	for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
		if(  players[i]  ) {
			players[i]->set_active(settings.player_active[i]);
		}
	}

	active_player_nr = 0;
	active_player = players[0];
	tool_t::update_toolbars();

	set_dirty();
	step_mode = PAUSE_FLAG;
	simloops = 60;
	reset_timer();

	if(is_display_init()) {
		display_show_pointer(true);
	}
	mute_sound(false);

	// Added by : Knightly
	path_explorer_t::full_instant_refresh();

	// Set the actual industry density and industry density proportion
	actual_industry_density = 0;
	uint32 weight;
	FOR(vector_tpl<fabrik_t*>, factory, fab_list)
	{
		const factory_desc_t* factory_type = factory->get_desc();
		if(!factory_type->is_electricity_producer())
		{
			// Power stations are excluded from the target weight:
			// a different system is used for them.
			weight = factory_type->get_distribution_weight();
			actual_industry_density += (100 / weight);
		}
	}
	// The population is not counted at this point, so cannot set this here.
	industry_density_proportion = 0;

	recalc_passenger_destination_weights();

	pedestrian_t::check_timeline_pedestrians();

#ifdef MULTI_THREAD
	init_threads();
	first_step = 1;
#else
	transferring_cargoes = new vector_tpl<transferring_cargo_t>[1];
#endif
}

void karte_t::recalc_passenger_destination_weights()
{
	// This averages the weight over all classes.
	// TODO: Consider whether to have separate numbers of alternative destinations
	// for each different class.
	uint64 total_sum_wieght_commuter_targets = 0;
	uint64 total_sum_weight_visitor_targets = 0;

	for (uint8 i = 0; i < goods_manager_t::passengers->get_number_of_classes(); i++)
	{
		total_sum_wieght_commuter_targets += commuter_targets[i].get_sum_weight();
		total_sum_weight_visitor_targets += visitor_targets[i].get_sum_weight();
	}

	total_sum_wieght_commuter_targets /= goods_manager_t::passengers->get_number_of_classes();
	total_sum_weight_visitor_targets /= goods_manager_t::passengers->get_number_of_classes();

	settings.update_min_alternative_destinations_commuting(total_sum_wieght_commuter_targets);
	settings.update_min_alternative_destinations_visiting(total_sum_weight_visitor_targets);

	settings.update_max_alternative_destinations_commuting(total_sum_wieght_commuter_targets);
	settings.update_max_alternative_destinations_visiting(total_sum_weight_visitor_targets);
}

#ifdef MULTI_THREAD
void *check_road_connexions_threaded(void *args)
{
	const uint32* thread_number_ptr = (const uint32*)args;
	const uint32 thread_number = *thread_number_ptr;
	delete thread_number_ptr;

	karte_t::marker_index = thread_number + world()->get_parallel_operations(); 

	do
	{
		if (world()->is_terminating_threads())
		{
			break;
		}
		int error = pthread_mutex_lock(&private_car_route_mutex);
		assert(error == 0);
		if (karte_t::cities_to_process > 0)
		{
			stadt_t* city;
			city = world()->cities_awaiting_private_car_route_check.remove_first();
			karte_t::cities_to_process--;
			int error = pthread_mutex_unlock(&private_car_route_mutex);
			assert(error == 0);

			if (!city)
			{
				continue;
			}

			city->check_all_private_car_routes();
			city->set_check_road_connexions(false);

			simthread_barrier_wait(&private_car_barrier);
		}
		else
		{
			int error = pthread_mutex_unlock(&private_car_route_mutex);
			assert(error == 0);
		}
		// Having two barrier waits here is intentional.
		simthread_barrier_wait(&private_car_barrier);
	} while (!world()->is_terminating_threads());

	// New thread local nodes are created on the heap automatically when this is used,
	// so this must be released explicitly when this thread is terminated.
	route_t::TERM_NODES(); 

	pthread_exit(NULL);
	return args;
}

void *step_passengers_and_mail_threaded(void* args)
{
	const uint32* thread_number_ptr = (const uint32*)args;
	karte_t::passenger_generation_thread_number = *thread_number_ptr;
	const uint32 seed_base = karte_t::world->get_settings().get_random_counter();
	
	// This may easily overflow, but this is irrelevant for the purposes of a random seed
	// (so long as both server and client are using the same size of integer)

	const uint32 seed = seed_base * karte_t::passenger_generation_thread_number;

	delete thread_number_ptr;

	// The random seed is now thread local, so this must be initialised here
	// with values that will be deterministic between different clients in
	// a networked multi-player setup.
	setsimrand(seed, 0xFFFFFFFFu);
	set_random_mode(STEP_RANDOM);

	sint32 next_step_passenger_this_thread;
	sint32 next_step_mail_this_thread;

	sint32 total_units_passenger;
	sint32 total_units_mail;

	do
	{
	top:
		simthread_barrier_wait(&step_passengers_and_mail_barrier);
		if (karte_t::world->is_terminating_threads())
		{
			break;
		}

		// The generate passengers function is called many times (often well > 100) each step; the mail version is called only once or twice each step, sometimes not at all.
		sint32 units_this_step = 0;
		total_units_passenger = 0;
		total_units_mail = 0;

#ifndef FIXED_PASSENGER_NUMBERS_PER_STEP_FOR_TESTING
		next_step_passenger_this_thread = karte_t::world->next_step_passenger / (karte_t::world->get_parallel_operations());

		next_step_mail_this_thread = karte_t::world->next_step_mail / (karte_t::world->get_parallel_operations());

#ifdef FORBID_PARALLELL_PASSENGER_GENERATION_IN_NETWORK_MODE
		if (env_t::networkmode)
		{
			if (karte_t::passenger_generation_thread_number == 0)
			{
				next_step_passenger_this_thread = karte_t::world->next_step_passenger;
			}
			else
			{
				next_step_passenger_this_thread = 0;
			}
		}
		else
		{
#else

			if (next_step_passenger_this_thread < karte_t::world->passenger_step_interval && karte_t::world->next_step_passenger > karte_t::world->passenger_step_interval)
			{
				if (karte_t::passenger_generation_thread_number == 0)
				{
					// In case of very small numbers, make this effectively single threaded, or else rounding errors will prevent any passenger generation.
					next_step_passenger_this_thread = karte_t::world->next_step_passenger;
				}
				else
				{
					next_step_passenger_this_thread = 0;
				}
			}
			else if (karte_t::passenger_generation_thread_number == 0)
			{
				next_step_passenger_this_thread += karte_t::world->next_step_passenger % (karte_t::world->get_parallel_operations());
			}

			if (next_step_mail_this_thread < karte_t::world->mail_step_interval && karte_t::world->next_step_mail > karte_t::world->mail_step_interval)
			{
				if (karte_t::passenger_generation_thread_number == 0)
				{
					// In case of very small numbers, make this effectively single threaded, or else rounding errors will prevent any mail generation.
					next_step_mail_this_thread = karte_t::world->next_step_mail;
				}
				else
				{
					next_step_mail_this_thread = 0;
				}
			}
			else if (karte_t::passenger_generation_thread_number == 0)
			{
				next_step_mail_this_thread += karte_t::world->next_step_mail % (karte_t::world->get_parallel_operations());
			}
#endif

#ifdef FORBID_PARALLELL_PASSENGER_GENERATION_IN_NETWORK_MODE
		}
#endif
			
		if (karte_t::world->passenger_step_interval <= next_step_passenger_this_thread)
		{
			do
			{
				if (karte_t::world->passenger_origins.get_count() == 0)
				{
					goto top;
				}
				units_this_step = karte_t::world->generate_passengers_or_mail(goods_manager_t::passengers);
				total_units_passenger += units_this_step;
				next_step_passenger_this_thread -= (karte_t::world->passenger_step_interval * units_this_step);

			} while (karte_t::world->passenger_step_interval <= next_step_passenger_this_thread);
		}
	
		if (karte_t::world->mail_step_interval <= next_step_mail_this_thread)
		{
			do
			{
				if (karte_t::world->mail_origins_and_targets.get_count() == 0)
				{
					goto top;
				}
				units_this_step = karte_t::world->generate_passengers_or_mail(goods_manager_t::mail);
				total_units_mail += units_this_step;
				next_step_mail_this_thread -= (karte_t::world->mail_step_interval * units_this_step);

			} while (karte_t::world->mail_step_interval <= next_step_mail_this_thread);
		}
#else
		for (uint32 i = 0; i < 2; i++)
		{	
			karte_t::world->generate_passengers_or_mail(goods_manager_t::passengers);
			karte_t::world->generate_passengers_or_mail(goods_manager_t::mail);
		}
#endif

		simthread_barrier_wait(&step_passengers_and_mail_barrier); // Having three of these is intentional.
		int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
		karte_t::world->next_step_passenger -= (total_units_passenger * karte_t::world->passenger_step_interval);
		karte_t::world->next_step_mail -= (total_units_mail * karte_t::world->mail_step_interval);
		mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
		simthread_barrier_wait(&step_passengers_and_mail_barrier);
		assert(mutex_error == 0);
	} while (!karte_t::world->is_terminating_threads());

	pthread_exit(NULL);
	return args;
}

void *step_convoys_threaded(void* args)
{
	karte_t* world = (karte_t*)args;
	const sint32 parallel_operations = world->get_parallel_operations();

	do
	{	
		simthread_barrier_wait(&karte_t::step_convoys_barrier_external);
		if (world->is_terminating_threads())
		{
			break;
		}
		// since convois will be deleted during stepping, we need to step backwards
		for (uint32 i = world->convoi_array.get_count(); i-- != 0;)
		{
			convoihandle_t cnv = world->convoi_array[i];
			convoys_next_step.append(cnv);

			//  Needed here as the new method does not do this.
			if (world->is_terminating_threads())
			{
				simthread_barrier_wait(&step_convoys_barrier_internal);
				simthread_barrier_wait(&step_convoys_barrier_internal); // The multiples of these is intentional: we must stop the individual threads before the clear() command is executed.
				convoys_next_step.clear();
				break;
			}
			
			// Old method: less efficient, as only processed 4 convoys at a time.
			/*
			if ((convoys_next_step.get_count() == parallel_operations - 2) || (i == world->convoi_array.get_count() - 1))
			{
				simthread_barrier_wait(&step_convoys_barrier_internal); 
				simthread_barrier_wait(&step_convoys_barrier_internal); // The multiples of these is intentional: we must stop the individual threads before the clear() command is executed.
				convoys_next_step.clear();
				if (world->is_terminating_threads())
				{
					break;
				}
			}*/
		}

		// New method: process all convoys at once in all threads.
		if (!world->is_terminating_threads())
		{
			simthread_barrier_wait(&step_convoys_barrier_internal);
			simthread_barrier_wait(&step_convoys_barrier_internal); // The multiples of these is intentional: we must stop the individual threads before the clear() command is executed.
			convoys_next_step.clear();
		}

		simthread_barrier_wait(&karte_t::step_convoys_barrier_external);
	} while (!world->is_terminating_threads());

	pthread_exit(NULL);
	return args;
}

void* step_individual_convoy_threaded(void* args)
{
	//pthread_cleanup_push(&route_t::TERM_NODES, NULL);
	const uint32* thread_number_ptr = (const uint32*)args;
	const uint32 thread_number = *thread_number_ptr; 
	karte_t::marker_index = thread_number;
	delete thread_number_ptr;

	do
	{
		simthread_barrier_wait(&step_convoys_barrier_internal);
		if (karte_t::world->is_terminating_threads())
		{
			break;
		}
		
		const uint32 convoys_next_step_count = convoys_next_step.get_count();
		uint32 offset_counter = karte_t::world->get_parallel_operations();
		bool start_counting = false;

		for (uint32 i = 0; i < convoys_next_step_count; i++)
		{
			if (i == thread_number || offset_counter == 0)
			{
				start_counting = true;
				offset_counter = karte_t::world->get_parallel_operations();
				convoihandle_t cnv = convoys_next_step[i];
				if (cnv.is_bound())
				{
					cnv->threaded_step();
				}
			}

			if (start_counting)
			{
				offset_counter --;
			}
		}

		/*
		// Old method
		if (convoys_next_step.get_count() > thread_number)
		{
			convoihandle_t cnv = convoys_next_step[thread_number];
			if (cnv.is_bound())
			{
				cnv->threaded_step();
			}
		} 
		*/

		simthread_barrier_wait(&step_convoys_barrier_internal);
	} while (!karte_t::world->is_terminating_threads());
	
	//pthread_cleanup_pop(1); 
	route_t::TERM_NODES(); 

	pthread_exit(NULL);
	return args;
}

void* path_explorer_threaded(void* args)
{
	karte_t* world = (karte_t*)args;
	path_explorer_t::allow_path_explorer_on_this_thread = true;
	karte_t::path_explorer_step_progress = 2;

	do
	{
		simthread_barrier_wait(&start_path_explorer_barrier);
		karte_t::path_explorer_step_progress = 0;
		simthread_barrier_wait(&start_path_explorer_barrier);
		int error = pthread_mutex_lock(&path_explorer_mutex);
		assert(error == 0);

		if (karte_t::world->is_terminating_threads())
		{
			karte_t::path_explorer_step_progress = -1;
			error = pthread_mutex_unlock(&path_explorer_mutex);
			assert(error == 0);
			break;
		}

		path_explorer_t::step();

		karte_t::path_explorer_step_progress = 1;

		pthread_cond_signal(&path_explorer_conditional_end);
		karte_t::path_explorer_step_progress = 2;
		error = pthread_mutex_unlock(&path_explorer_mutex);
		assert(error == 0);
	} while (!karte_t::world->is_terminating_threads());

	karte_t::path_explorer_step_progress = -1;
	pthread_exit(NULL);
	return args;
}


void karte_t::stop_path_explorer()
{
#ifdef MULTI_THREAD_PATH_EXPLORER
	if (path_explorer_step_progress == -1)
	{
		return;
	}

	int error = pthread_mutex_lock(&path_explorer_mutex);
	assert(error == 0);

	while (path_explorer_step_progress == 0 || path_explorer_step_progress == 1)
	{
		pthread_cond_wait(&path_explorer_conditional_end, &path_explorer_mutex);
	}

	error = pthread_mutex_unlock(&path_explorer_mutex);
	assert(error == 0);

#endif
}

void karte_t::start_path_explorer()
{
#ifdef MULTI_THREAD_PATH_EXPLORER
	if (path_explorer_step_progress == -1)
	{
		// The threaded path explorer has been terminated, so do not wait
		// or else we will get a thread deadlock.
		return;
	}
	int error = pthread_mutex_lock(&path_explorer_mutex);
	assert(error == 0); 
	if (path_explorer_step_progress > 0)
	{
		simthread_barrier_wait(&start_path_explorer_barrier);
	}
	if (path_explorer_step_progress > -1)
	{
		simthread_barrier_wait(&start_path_explorer_barrier);
	}
	error = pthread_mutex_unlock(&path_explorer_mutex);
	assert(error == 0); 
#endif 
}

void* unreserve_route_threaded(void* args)
{
	const uint32* thread_number_ptr = (const uint32*)args;
	const uint32 thread_number = *thread_number_ptr;
	delete thread_number_ptr;

	do
	{
		simthread_barrier_wait(&karte_t::unreserve_route_barrier);
		
		if (karte_t::world->is_terminating_threads())
		{
			break;
		}
		if (convoi_t::current_unreserver == 0)
		{
			int error = pthread_mutex_unlock(&karte_t::unreserve_route_mutex);
			assert(error == 0);
			continue;
		}

		const uint32 max_count = weg_t::get_all_ways_count() - 1;
		const uint32 fraction = max_count / karte_t::world->get_parallel_operations();

		route_range_specification range;

		range.start = thread_number * fraction;
		if (thread_number == karte_t::world->get_parallel_operations() - 1)
		{
			range.end = max_count;
		}
		else
		{
			range.end = min(((thread_number + 1) * fraction - 1), max_count);
		}

		convoi_t::unreserve_route_range(range);

		simthread_barrier_wait(&karte_t::unreserve_route_barrier);
		
	} while (!karte_t::world->is_terminating_threads());

	pthread_exit(NULL);
	return args;
}

void karte_t::init_threads()
{
	marker_index = UINT32_MAX_VALUE;

	sint32 rc;

	const sint32 parallel_operations = get_parallel_operations();

	private_cars_added_threaded = new vector_tpl<private_car_t*>[parallel_operations + 2];
	pedestrians_added_threaded = new vector_tpl<pedestrian_t*>[parallel_operations + 2];
	transferring_cargoes = new vector_tpl<transferring_cargo_t>[parallel_operations + 2];
	marker_t::markers = new marker_t[parallel_operations * 2]; 

	start_halts = new vector_tpl<nearby_halt_t>[parallel_operations + 2];
	destination_list = new vector_tpl<halthandle_t>[parallel_operations + 2];

	pthread_attr_init(&thread_attributes);
	pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_JOINABLE);

	simthread_barrier_init(&private_car_barrier, NULL, parallel_operations + 1);
	simthread_barrier_init(&karte_t::unreserve_route_barrier, NULL, parallel_operations + 2); // This and the next does not run concurrently with anything significant on the main thread, so the number of parallel operations need to be +1 compared to the others.
	simthread_barrier_init(&step_passengers_and_mail_barrier, NULL, parallel_operations + 2); 
	simthread_barrier_init(&step_convoys_barrier_external, NULL, 2);
	simthread_barrier_init(&step_convoys_barrier_internal, NULL, parallel_operations + 1);	
	simthread_barrier_init(&start_path_explorer_barrier, NULL, 2);

	// Initialise mutexes
	pthread_mutexattr_init(&mutex_attributes);
	pthread_mutexattr_settype(&mutex_attributes, PTHREAD_MUTEX_ERRORCHECK);

	pthread_mutex_init(&private_car_route_mutex, &mutex_attributes);
	pthread_mutex_init(&step_passengers_and_mail_mutex, &mutex_attributes);
	pthread_mutex_init(&path_explorer_mutex, &mutex_attributes);
	pthread_mutex_init(&unreserve_route_mutex, &mutex_attributes);

	pthread_t thread;
	
	for (uint32 i = 0; i < parallel_operations + 1; i++)
	{
		if (i < parallel_operations)
		{
			uint32* thread_number_checker = new uint32;
			*thread_number_checker = i;
			rc = pthread_create(&thread, &thread_attributes, &check_road_connexions_threaded, (void*)thread_number_checker);
			if (rc)
			{
				dbg->fatal("void karte_t::init_threads()", "Failed to create private car thread, error %d. See here for a translation of the error numbers: http://epydoc.sourceforge.net/stdlib/errno-module.html", rc);
			}
			else
			{
				private_car_route_threads.append(thread);
			}
		}

		// The next two need an extra thread compared with the others, as they do not run concurrently with anything non-trivial on the main thread
		sint32* thread_number_unres = new sint32;
		*thread_number_unres = i;
		rc = pthread_create(&thread, &thread_attributes, &unreserve_route_threaded, (void*)thread_number_unres);
		if (rc)
		{
			dbg->fatal("void karte_t::init_threads()", "Failed to create private car thread, error %d. See here for a translation of the error numbers: http://epydoc.sourceforge.net/stdlib/errno-module.html", rc);
		}
		else
		{
			unreserve_route_threads.append(thread);
		}
		
#ifdef MULTI_THREAD_PASSENGER_GENERATION
		sint32* thread_number_pass = new sint32;
		*thread_number_pass = i + 1; // +1 because we need thread number 0 to represent the main thread.
		rc = pthread_create(&thread, &thread_attributes, &step_passengers_and_mail_threaded, (void*)thread_number_pass);
		if (rc)
		{
			dbg->fatal("void karte_t::init_threads()", "Failed to create step passengers and mail thread, error %d. See here for a translation of the error numbers: http://epydoc.sourceforge.net/stdlib/errno-module.html", rc);
		}
		else
		{
			step_passengers_and_mail_threads.append(thread);
		}
#endif
		if (i == parallel_operations)
		{
			break;
		}

#ifdef MULTI_THREAD_CONVOYS
		uint32* thread_number_cnv = new uint32;
		*thread_number_cnv = i; 
		rc = pthread_create(&thread, &thread_attributes, &step_individual_convoy_threaded, (void*)thread_number_cnv);
		if (rc)
		{
			dbg->fatal("void karte_t::init_threads()", "Failed to create individual convoy thread, error %d. See here for a translation of the error numbers: http://epydoc.sourceforge.net/stdlib/errno-module.html", rc);
		}
		else
		{
			individual_convoy_step_threads.append(thread);
		}
#endif 
	}
#ifdef MULTI_THREAD_CONVOYS
	rc = pthread_create(&convoy_step_master_thread, &thread_attributes, &step_convoys_threaded, (void*)this);
	if (rc)
	{
		dbg->fatal("void karte_t::init_threads()", "Failed to create convoy master thread, error %d. See here for a translation of the error numbers: http://epydoc.sourceforge.net/stdlib/errno-module.html", rc);
	}
#endif

#ifdef MULTI_THREAD_PATH_EXPLORER

	rc = pthread_create(&path_explorer_thread, &thread_attributes, &path_explorer_threaded, (void*)this); 
	if (rc)
	{
		dbg->fatal("void karte_t::init_threads()", "Failed to create path explorer thread, error %d. See here for a translation of the error numbers: http://epydoc.sourceforge.net/stdlib/errno-module.html", rc);
	}
#endif

	threads_initialised = true;
}

void karte_t::destroy_threads()
{
	if (threads_initialised)
	{
		pthread_attr_destroy(&thread_attributes);

		// Send waiting threads to their doom

		// HACK: The below sleep allows the convoy threads to synchronise to avoid
		// thread deadlocks that otherwise might result.
		dr_sleep(100);

		terminating_threads = true;
#ifdef MULTI_THREAD_CONVOYS
		simthread_barrier_wait(&step_convoys_barrier_external);
		simthread_barrier_wait(&step_convoys_barrier_internal);
#endif
#ifdef MULTI_THREAD_PASSENGER_GENERATION
		simthread_barrier_wait(&step_passengers_and_mail_barrier);
#endif
		simthread_barrier_wait(&private_car_barrier);
		simthread_barrier_wait(&unreserve_route_barrier);
#ifdef MULTI_THREAD_PATH_EXPLORER
		start_path_explorer();
#endif
#ifdef MULTI_THREAD_PATH_EXPLORER
		pthread_join(path_explorer_thread, 0);
#endif
#ifdef MULTI_THREAD_CONVOYS
		pthread_join(convoy_step_master_thread, 0);
		clean_threads(&individual_convoy_step_threads);
		individual_convoy_step_threads.clear();
#endif

		clean_threads(&private_car_route_threads);
		private_car_route_threads.clear();
#ifdef MULTI_THREAD_PASSENGER_GENERATION
		clean_threads(&step_passengers_and_mail_threads);
		step_passengers_and_mail_threads.clear();
#endif

		clean_threads(&unreserve_route_threads);
		unreserve_route_threads.clear();
#ifdef MULTI_THREAD_CONVOYS
		simthread_barrier_destroy(&step_convoys_barrier_external);
		simthread_barrier_destroy(&step_convoys_barrier_internal);
#endif
#ifdef MULTI_THREAD_PASSENGER_GENERATION
		simthread_barrier_destroy(&step_passengers_and_mail_barrier);
#endif
		simthread_barrier_destroy(&private_car_barrier);
		simthread_barrier_destroy(&unreserve_route_barrier);
#ifdef MULTI_THREAD_PATH_EXPLORER
		simthread_barrier_destroy(&start_path_explorer_barrier);
#endif 

		// Destroy mutexes
		pthread_mutex_destroy(&private_car_route_mutex);
		pthread_mutex_destroy(&step_passengers_and_mail_mutex);
		pthread_mutex_destroy(&path_explorer_mutex);
		pthread_mutex_destroy(&unreserve_route_mutex);

		pthread_mutexattr_destroy(&mutex_attributes);
	}

	first_step = 1;

	delete[] private_cars_added_threaded; 
	private_cars_added_threaded = NULL;
	delete[] pedestrians_added_threaded;
	pedestrians_added_threaded = NULL;
	delete[] transferring_cargoes;
	transferring_cargoes = NULL;
	delete[] marker_t::markers;
	marker_t::markers = NULL;
	delete[]start_halts;
	start_halts = NULL;
	delete[] destination_list;
	destination_list = NULL;

	threads_initialised = false;
	terminating_threads = false;
}

void karte_t::clean_threads(vector_tpl<pthread_t> *thread)
{
	FOR(vector_tpl<pthread_t>, this_thread, *thread)
	{
		pthread_join(this_thread, 0);
	}
}

#endif

sint32 karte_t::get_parallel_operations() const
{
#ifndef MULTI_THREAD
	return 0;
#else
	sint32 po;
	if(parallel_operations > 0 && (threads_initialised || (env_t::networkmode && !env_t::server)))
	{
		po = parallel_operations;
	}
	else
	{
		po = env_t::num_threads - 1;
	}

	return po;
#endif
}

#define array_koord(px,py) (px + py * get_size().x)


/* Lakes:
 * For each height from groundwater+1 to max_lake_height we loop over
 * all tiles in the map trying to increase water height to this value
 * To start with every tile in the map is checked - but when we fail for
 * a tile then it is excluded from subsequent checks
 */
void karte_t::create_lakes(  int xoff, int yoff  )
{
	if(  xoff > 0  ||  yoff > 0  ) {
		// too complicated to add lakes to an already existing world...
		return;
	}

	const sint8 max_lake_height = groundwater + 8;
	const uint16 size_x = get_size().x;
	const uint16 size_y = get_size().y;

	sint8 *max_water_hgt = new sint8[size_x * size_y];
	memset( max_water_hgt, 1, sizeof(sint8) * size_x * size_y );

	sint8 *stage = new sint8[size_x * size_y];
	sint8 *new_stage = new sint8[size_x * size_y];
	sint8 *local_stage = new sint8[size_x * size_y];

	for(  sint8 h = groundwater+1; h<max_lake_height; h++  ) {
		bool need_to_flood = false;
		memset( stage, -1, sizeof(sint8) * size_x * size_y );
		for(  uint16 y = 1;  y < size_y-1;  y++  ) {
			for(  uint16 x = 1;  x < size_x-1;  x++  ) {
				uint32 offset = array_koord(x,y);
				if(  max_water_hgt[offset]==1  &&  stage[offset]==-1  ) {

					sint8 hgt = lookup_hgt_nocheck( x, y );
					const sint8 water_hgt = water_hgts[offset]; // optimised <- get_water_hgt_nocheck(x, y);
					const sint8 new_water_hgt = max(hgt, water_hgt);
					if(  new_water_hgt>max_lake_height  ) {
						max_water_hgt[offset] = 0;
					}
					else if(  h>new_water_hgt  ) {
						koord k(x,y);
						memcpy( new_stage, stage, sizeof(sint8) * size_x * size_y );
						if(can_flood_to_depth(  k, h, new_stage, local_stage )) {
							sint8 *tmp_stage = new_stage;
							new_stage = stage;
							stage = tmp_stage;
							need_to_flood = true;
						}
						else {
							for(  uint16 iy = 1;  iy<size_y - 1;  iy++  ) {
								uint32 offset_end = array_koord(size_x - 1,iy);
								for(  uint32 local_offset = array_koord(0,iy);  local_offset<offset_end;  local_offset++  ) {
									if(  local_stage[local_offset] > -1  ) {
										max_water_hgt[local_offset] = 0;
									}
								}
							}
						}
					}
				}
			}
		}
		if(need_to_flood) {
			flood_to_depth(  h, stage  );
		}
		else {
			break;
		}
	}

	delete [] max_water_hgt;
	delete [] stage;
	delete [] new_stage;
	delete [] local_stage;

	for (planquadrat_t *pl = plan; pl < (plan + size_x * size_y); pl++) {
		pl->correct_water();
	}
}


bool karte_t::can_flood_to_depth(  koord k, sint8 new_water_height, sint8 *stage, sint8 *our_stage  ) const
{
	bool succeeded = true;
	if(  k == koord::invalid  ) {
		return false;
	}

	if(  new_water_height < get_groundwater() - 3  ) {
		return false;
	}

	// make a list of tiles to change
	// cannot use a recursive method as stack is not large enough!

	sint8 *from_dir = new sint8[get_size().x * get_size().y];
	bool local_stage = (our_stage==NULL);

	if(  local_stage  ) {
		our_stage = new sint8[get_size().x * get_size().y];
	}

	memset( from_dir, -1, sizeof(sint8) * get_size().x * get_size().y );
	memset( our_stage, -1, sizeof(sint8) * get_size().x * get_size().y );
	uint32 offset = array_koord(k.x,k.y);
	stage[offset]=0;
	our_stage[offset]=0;
	do {
		for(  int i = our_stage[offset];  i < 8;  i++  ) {
			koord k_neighbour = k + koord::neighbours[i];
			if(  is_within_limits(k_neighbour)  ) {
				const uint32 neighbour_offset = array_koord(k_neighbour.x,k_neighbour.y);

				// already visited
				if(our_stage[neighbour_offset] != -1) goto next_neighbour;

				// water height above
				if(water_hgts[neighbour_offset] >= new_water_height) goto next_neighbour;

				grund_t *gr2 = lookup_kartenboden_nocheck(k_neighbour);
				if(  !gr2  ) goto next_neighbour;

				sint8 neighbour_height = gr2->get_hoehe();

				// land height above
				if(neighbour_height >= new_water_height) goto next_neighbour;

				//move on to next tile
				from_dir[neighbour_offset] = i;
				stage[neighbour_offset] = 0;
				our_stage[neighbour_offset] = 0;
				our_stage[offset] = i;
				k = k_neighbour;
				offset = array_koord(k.x,k.y);
				break;
			}
			else {
				// edge of map - we keep iterating so we can mark all connected tiles as failing
				succeeded = false;
			}
			next_neighbour:
			//return back to previous tile
			if(  i==7  ) {
				k = k - koord::neighbours[from_dir[offset]];
			}
		}
		offset = array_koord(k.x,k.y);
	} while(  from_dir[offset] != -1  );

	delete [] from_dir;

	if(  local_stage  ) {
		delete [] our_stage;
	}

	return succeeded;
}


void karte_t::flood_to_depth( sint8 new_water_height, sint8 *stage )
{
	const uint16 size_x = get_size().x;
	const uint16 size_y = get_size().y;

	uint32 offset_max = size_x*size_y;
	for(  uint32 offset = 0;  offset < offset_max;  offset++  ) {
		if(  stage[offset] == -1  ) {
			continue;
		}
		water_hgts[offset] = new_water_height;
	}
}


void karte_t::create_beaches(  int xoff, int yoff  )
{
	const uint16 size_x = get_size().x;
	const uint16 size_y = get_size().y;

	// bays have wide beaches
	for(  uint16 iy = 0;  iy < size_y;  iy++  ) {
		for(  uint16 ix = (iy >= yoff - 19) ? 0 : max( xoff - 19, 0 );  ix < size_x;  ix++  ) {
			grund_t *gr = lookup_kartenboden_nocheck(ix,iy);
			if(  gr->is_water()  &&  gr->get_hoehe()==groundwater  &&  gr->kann_alle_obj_entfernen(NULL)==NULL) {
				koord k( ix, iy );
				uint8 neighbour_water = 0;
				bool water[8];
				// check whether nearby tiles are water
				for(  int i = 0;  i < 8;  i++  ) {
					grund_t *gr2 = lookup_kartenboden( k + koord::neighbours[i] );
					water[i] = (!gr2  ||  gr2->is_water());
				}

				// make a count of nearby tiles - where tiles on opposite (+-1 direction) sides are water these count much more so we don't block straits
				for(  int i = 0;  i < 8;  i++  ) {
					if(  water[i]  ) {
						neighbour_water++;
						if(  water[(i + 3) & 7]  ||  water[(i + 4) & 7]  ||  water[(i + 5) & 7]  ) {
							neighbour_water++;
						}
					}
				}

				// if not much nearby water then turn into a beach
				if(  neighbour_water < 4  ) {
					set_water_hgt( k, gr->get_hoehe() - 1 );
					raise_grid_to( ix, iy, gr->get_hoehe() );
					raise_grid_to( ix + 1, iy, gr->get_hoehe() );
					raise_grid_to( ix, iy + 1, gr->get_hoehe() );
					raise_grid_to( ix + 1, iy + 1 , gr->get_hoehe() );
					access_nocheck(k)->correct_water();
					access_nocheck(k)->set_climate( desert_climate );
				}
			}
		}
	}

	// headlands should not have beaches at all
	for(  uint16 iy = 0;  iy < size_y;  iy++  ) {
		for(  uint16 ix = (iy >= yoff - 19) ? 0 : max( xoff - 19, 0 );  ix < size_x;  ix++  ) {
			koord k( ix, iy );
			grund_t *gr = lookup_kartenboden_nocheck(k);
			if(  !gr->is_water()  &&  gr->get_pos().z == groundwater  ) {
				uint8 neighbour_water = 0;
				for(  int i = 0;  i < 8;  i++  ) {
					grund_t *gr2 = lookup_kartenboden( k + koord::neighbours[i] );
					if(  !gr2  ||  gr2->is_water()  ) {
						neighbour_water++;
					}
				}
				// if a lot of water nearby we are a headland
				if(  neighbour_water > 3  ) {
					access_nocheck(k)->set_climate( get_climate_at_height( groundwater + 1 ) );
				}
			}
		}
	}

	// remove any isolated 1 tile beaches
	for(  uint16 iy = 0;  iy < size_y;  iy++  ) {
		for(  uint16 ix = (iy >= yoff - 19) ? 0 : max( xoff - 19, 0 );  ix < size_x;  ix++  ) {
			koord k( ix, iy );
			if(  access_nocheck(k)->get_climate()  ==  desert_climate  ) {
				uint8 neighbour_beach = 0;
				//look up neighbouring climates
				climate neighbour_climate[8];
				for(  int i = 0;  i < 8;  i++  ) {
					koord k_neighbour = k + koord::neighbours[i];
					if(  !is_within_limits(k_neighbour)  ) {
						k_neighbour = get_closest_coordinate(k_neighbour);
					}
					neighbour_climate[i] = get_climate( k_neighbour );
				}

				// get transition climate - look for each corner in turn
				for( int i = 0;  i < 4;  i++  ) {
					climate transition_climate = (climate) max( max( neighbour_climate[(i * 2 + 1) & 7], neighbour_climate[(i * 2 + 3) & 7] ), neighbour_climate[(i * 2 + 2) & 7] );
					climate min_climate = (climate) min( min( neighbour_climate[(i * 2 + 1) & 7], neighbour_climate[(i * 2 + 3) & 7] ), neighbour_climate[(i * 2 + 2) & 7] );
					if(  min_climate <= desert_climate  &&  transition_climate == desert_climate  ) {
						neighbour_beach++;
					}
				}
				if(  neighbour_beach == 0  ) {
					access_nocheck(k)->set_climate( get_climate_at_height( groundwater + 1 ) );
				}
			}
		}
	}
}


void karte_t::init_height_to_climate()
{
	// create height table
	sint16 climate_border[MAX_CLIMATES];
	memcpy(climate_border, get_settings().get_climate_borders(), sizeof(climate_border));
	// set climate_border[0] to sea level
	climate_border[0] = groundwater;
	for( int cl=0;  cl<MAX_CLIMATES-1;  cl++ ) {
		if(climate_border[cl]>climate_border[arctic_climate]) {
			// unused climate
			climate_border[cl] = 0;
		}
	}
	// now arrange the remaining ones
	for( int h=0;  h<32;  h++  ) {
		sint16 current_height = 999;	      // current maximum
		sint16 current_cl = arctic_climate;	// and the climate
		for( int cl=0;  cl<MAX_CLIMATES;  cl++ ) {
			if (climate_border[cl] >= h + groundwater  &&  climate_border[cl] < current_height) {
				current_height = climate_border[cl];
				current_cl = cl;
			}
		}
		height_to_climate[h] = (uint8)current_cl;
	}
}


void karte_t::enlarge_map(settings_t const* sets, sint8 const* const h_field)
{
	sint16 new_size_x = sets->get_size_x();
	sint16 new_size_y = sets->get_size_y();
	//const sint32 map_size = max (new_size_x, new_size_y);

	if(  cached_grid_size.y>0  &&  cached_grid_size.y!=new_size_y  ) {
		// to keep the labels
		grund_t::enlarge_map( new_size_x, new_size_y );
	}

	planquadrat_t *new_plan = new planquadrat_t[new_size_x*new_size_y];
	sint8 *new_grid_hgts = new sint8[(new_size_x + 1) * (new_size_y + 1)];
	sint8 *new_water_hgts = new sint8[new_size_x * new_size_y];

	memset( new_grid_hgts, groundwater, sizeof(sint8) * (new_size_x + 1) * (new_size_y + 1) );
	memset( new_water_hgts, groundwater, sizeof(sint8) * new_size_x * new_size_y );

	sint16 old_x = cached_grid_size.x;
	sint16 old_y = cached_grid_size.y;

	settings.set_size_x(new_size_x);
	settings.set_size_y(new_size_y);
	cached_grid_size.x = new_size_x;
	cached_grid_size.y = new_size_y;
	cached_size_max = max(cached_grid_size.x,cached_grid_size.y);
	cached_size.x = cached_grid_size.x-1;
	cached_size.y = cached_grid_size.y-1;

	intr_disable();

	bool reliefkarte = reliefkarte_t::is_visible;

	uint32 max_display_progress;

	// If this is not called by karte_t::init
	if(  old_x != 0  ) {
		mute_sound(true);
		reliefkarte_t::is_visible = false;

		if(is_display_init()) {
			display_show_pointer(false);
		}

// Copy old values:
		for (sint16 iy = 0; iy<old_y; iy++) {
			for (sint16 ix = 0; ix<old_x; ix++) {
				uint32 nr = ix+(iy*old_x);
				uint32 nnr = ix+(iy*new_size_x);
				swap(new_plan[nnr], plan[nr]);
				new_water_hgts[nnr] = water_hgts[nr];
			}
		}
		for (sint16 iy = 0; iy<=old_y; iy++) {
			for (sint16 ix = 0; ix<=old_x; ix++) {
				uint32 nr = ix+(iy*(old_x+1));
				uint32 nnr = ix+(iy*(new_size_x+1));
				new_grid_hgts[nnr] = grid_hgts[nr];
			}
		}
		max_display_progress = 16 + sets->get_city_count()*2 + stadt.get_count()*4;
	}
	else {
		max_display_progress = 16 + sets->get_city_count() * 4 + settings.get_factory_count();
	}
	loadingscreen_t ls( translator::translate( old_x ? "enlarge map" : "Init map ..."), max_display_progress, true, true );

	delete [] plan;
	plan = new_plan;
	delete [] grid_hgts;
	grid_hgts = new_grid_hgts;
	delete [] water_hgts;
	water_hgts = new_water_hgts;

	if (old_x == 0) {
		// init max and min with defaults
		max_height = groundwater;
		min_height = groundwater;
	}

	setsimrand(0xFFFFFFFF, settings.get_map_number());
	clear_random_mode( 0xFFFF );
	set_random_mode( MAP_CREATE_RANDOM );

	if(  old_x == 0  &&  !settings.heightfield.empty()  ) {
		// init from file
		for(int y=0; y<cached_grid_size.y; y++) {
			for(int x=0; x<cached_grid_size.x; x++) {
				grid_hgts[x + y*(cached_grid_size.x+1)] = h_field[x+(y*(sint32)cached_grid_size.x)]+1;
			}
			grid_hgts[cached_grid_size.x + y*(cached_grid_size.x+1)] = grid_hgts[cached_grid_size.x-1 + y*(cached_grid_size.x+1)];
		}
		// lower border
		memcpy( grid_hgts+(cached_grid_size.x+1)*(sint32)cached_grid_size.y, grid_hgts+(cached_grid_size.x+1)*(sint32)(cached_grid_size.y-1), cached_grid_size.x+1 );
		ls.set_progress(2);
	}
	else {
		if(  sets->get_rotation()==0  &&  sets->get_origin_x()==0  &&  sets->get_origin_y()==0) {
			// otherwise negative offsets may occur, so we cache only non-rotated maps
			init_perlin_map(new_size_x,new_size_y);
		}
		if (  old_x > 0  &&  old_y > 0  ) {
			// loop only new tiles:
			for(  sint16 y = 0;  y<=new_size_y;  y++  ) {
				for(  sint16 x = (y>old_y) ? 0 : old_x+1;  x<=new_size_x;  x++  ) {
					koord k(x,y);
					sint16 const h = perlin_hoehe(&settings, k, koord(old_x, old_y));
					set_grid_hgt( k, (sint8) h);
				}
				ls.set_progress( (y*16)/new_size_y );
			}
		}
		else {
			world_xy_loop(&karte_t::perlin_hoehe_loop, GRIDS_FLAG);
			ls.set_progress(2);
		}
		exit_perlin_map();
	}

	/** @note First we'll copy the border heights to the adjacent tile.
	 * The best way I could find is raising the first new grid point to
	 * the same height the adjacent old grid point was and lowering to the
	 * same height again. This doesn't preserve the old area 100%, but it respects it
	 * somehow.
	 */

	sint32 i;
	grund_t *gr;
	sint8 h;

	if ( old_x > 0  &&  old_y > 0){
		for(i=0; i<old_x; i++) {
			gr = lookup_kartenboden_nocheck(i, old_y-1);
			if (!gr->is_water()) {
				h = gr->get_hoehe(slope4_t::corner_SW);
				raise_grid_to(i, old_y+1, h);
				lower_grid_to(i, old_y+1, h );
			}
		}
		for(i=0; i<old_y; i++) {
			gr = lookup_kartenboden_nocheck(old_x-1, i);
			if (!gr->is_water()) {
				h = gr->get_hoehe(slope4_t::corner_NE);
				raise_grid_to(old_x+1, i, h);
				lower_grid_to(old_x+1, i, h);
			}
		}
		gr = lookup_kartenboden_nocheck(old_x-1, old_y -1);
		if (!gr->is_water()) {
			h = gr->get_hoehe(slope4_t::corner_SE);
			raise_grid_to(old_x+1, old_y+1, h);
			lower_grid_to(old_x+1, old_y+1, h);
		}
	}

	if (  old_x > 0  &&  old_y > 0  ) {
		// create grounds on new part
		for (sint16 iy = 0; iy<new_size_y; iy++) {
			for (sint16 ix = (iy>=old_y)?0:old_x; ix<new_size_x; ix++) {
				koord k(ix,iy);
				access_nocheck(k)->kartenboden_setzen( new boden_t( koord3d( ix, iy, max( min_hgt_nocheck(k), get_water_hgt_nocheck(k) ) ), 0 ) );
			}
		}
	}
	else {
		world_xy_loop(&karte_t::create_grounds_loop, 0);
		ls.set_progress(3);
	}

	// smooth the new part, reassign slopes on new part
	cleanup_karte( old_x, old_y );
	if (  old_x == 0  &&  old_y == 0  ) {
		ls.set_progress(4);
	}

	if(  sets->get_lake()  ) {
		create_lakes( old_x, old_y );
	}

	if (  old_x == 0  &&  old_y == 0  ) {
		ls.set_progress(13);
	}

	// set climates in new area and old map near seam
	for(  sint16 iy = 0;  iy < new_size_y;  iy++  ) {
		for(  sint16 ix = (iy >= old_y - 19) ? 0 : max( old_x - 19, 0 );  ix < new_size_x;  ix++  ) {
			calc_climate( koord( ix, iy ), false );
		}
	}
	if (  old_x == 0  &&  old_y == 0  ) {
		ls.set_progress(14);
	}

	create_beaches( old_x, old_y );
	if (  old_x == 0  &&  old_y == 0  ) {
		ls.set_progress(15);
	}

	if (  old_x > 0  &&  old_y > 0  ) {
		// and calculate transitions in a 1 tile larger area
		for(  sint16 iy = 0;  iy < new_size_y;  iy++  ) {
			for(  sint16 ix = (iy >= old_y - 20) ? 0 : max( old_x - 20, 0 );  ix < new_size_x;  ix++  ) {
				recalc_transitions( koord( ix, iy ) );
			}
		}
	}
	else {
		// new world -> calculate all transitions
		world_xy_loop(&karte_t::recalc_transitions_loop, 0);

		ls.set_progress(16);
	}

	// now recalc the images of the old map near the seam ...
	for(  sint16 y = 0;  y < old_y - 20;  y++  ) {
		for(  sint16 x = max( old_x - 20, 0 );  x < old_x;  x++  ) {
			lookup_kartenboden_nocheck(x,y)->calc_image();
		}
	}
	for(  sint16 y = max( old_y - 20, 0 );  y < old_y;  y++) {
		for(  sint16 x = 0;  x < old_x;  x++  ) {
			lookup_kartenboden_nocheck(x,y)->calc_image();
		}
	}

	// eventual update origin
	switch(  settings.get_rotation()  ) {
		case 1: {
			settings.set_origin_y( settings.get_origin_y() - new_size_y + old_y );
			break;
		}
		case 2: {
			settings.set_origin_x( settings.get_origin_x() - new_size_x + old_x );
			settings.set_origin_y( settings.get_origin_y() - new_size_y + old_y );
			break;
		}
		case 3: {
			settings.set_origin_x( settings.get_origin_x() - new_size_y + old_y );
			break;
		}
	}

	distribute_groundobjs_cities(sets, old_x, old_y);

	// Now add all the buildings to the world list.
	// This is not done in distribute_groundobjs_cities
	// to save the time taken by constantly adding and
	// removing them during the iterative renovation that
	// is involved in map generation/enlargement.
	FOR(weighted_vector_tpl<stadt_t*>, const city, stadt)
	{
		city->add_all_buildings_to_world_list();
	}

	// hausbauer_t::new_world(); <- this would reinit monuments! do not do this!
	factory_builder_t::new_world();

#ifdef MULTI_THREAD
	stop_path_explorer();
#endif
	// Modified by : Knightly
	path_explorer_t::refresh_all_categories(true);

	set_schedule_counter();

	// Refresh the haltlist for the affected tiles / stations.
	// It is enough to check the tile just at the border ...
	uint16 const cov = settings.get_station_coverage();
	if (old_y < new_size_y) {
		for (sint16 y = 0; y<old_y; y++) {
			for (sint16 x = max(0, old_x - cov); x<old_x; x++) {
				const planquadrat_t* pl = access_nocheck(x, y);
				for (uint8 i = 0; i < pl->get_boden_count(); i++) {
					// update limits
					if (min_height > pl->get_boden_bei(i)->get_hoehe()) {
						min_height = pl->get_boden_bei(i)->get_hoehe();
					}
					else if (max_height < pl->get_boden_bei(i)->get_hoehe()) {
						max_height = pl->get_boden_bei(i)->get_hoehe();
					}
					// update halt
					halthandle_t h = pl->get_boden_bei(i)->get_halt();
					if (h.is_bound()) {
						for (sint16 xp = max(0, x - cov); xp<min(new_size_x, x + cov + 1); xp++) {
							for (sint16 yp = y; yp<min(new_size_y, y + cov + 1); yp++) {
								access_nocheck(xp, yp)->add_to_haltlist(h);
							}
						}
					}
				}
			}
		}
	}
	if (old_x < new_size_x) {
		for (sint16 y = max(0, old_y - cov); y<old_y; y++) {
			for (sint16 x = 0; x<old_x; x++) {
				const planquadrat_t* pl = access_nocheck(x, y);
				for (uint8 i = 0; i < pl->get_boden_count(); i++) {
					// update limits
					if (min_height > pl->get_boden_bei(i)->get_hoehe()) {
						min_height = pl->get_boden_bei(i)->get_hoehe();
					}
					else if (max_height < pl->get_boden_bei(i)->get_hoehe()) {
						max_height = pl->get_boden_bei(i)->get_hoehe();
					}
					// update halt
					halthandle_t h = pl->get_boden_bei(i)->get_halt();
					if (h.is_bound()) {
						for (sint16 xp = x; xp<min(new_size_x, x + cov + 1); xp++) {
							for (sint16 yp = max(0, y - cov); yp<min(new_size_y, y + cov + 1); yp++) {
								access_nocheck(xp, yp)->add_to_haltlist(h);
							}
						}
					}
				}
			}
		}
	}
	// After refreshing the haltlists for the map,
	// refresh the haltlist for all factories.
	// Don't try to be clever; we don't do map enlargements often.
	FOR(vector_tpl<fabrik_t*>, const fab, fab_list)
	{
		fab->recalc_nearby_halts();
	}
	clear_random_mode( MAP_CREATE_RANDOM );

	if ( old_x != 0 ) {
		if(is_display_init()) {
			display_show_pointer(true);
		}
		mute_sound(false);

		reliefkarte_t::is_visible = reliefkarte;
		reliefkarte_t::get_karte()->init();
		reliefkarte_t::get_karte()->calc_map();
		reliefkarte_t::get_karte()->set_mode( reliefkarte_t::get_karte()->get_mode() );

		set_dirty();
		reset_timer();
	}
	// update main menu
	tool_t::update_toolbars();
}



karte_t::karte_t() :
	settings(env_t::default_settings),
	convoi_array(0),
	world_attractions(16),
	stadt(0),
	idle_time(0),
	speed_factors_are_set(false)
{
	// length of day and other time stuff
	ticks_per_world_month_shift = 20;
	ticks_per_world_month = (1LL << ticks_per_world_month_shift);
	last_step_ticks = 0;
	server_last_announce_time = 0;
	last_interaction = dr_time();
	step_mode = PAUSE_FLAG;
	time_multiplier = 16;
	next_step_time = last_step_time = 0;
	fix_ratio_frame_time = 200;
	idle_time = 0;
	network_frame_count = 0;
	sync_steps = 0;
	sync_steps_barrier = sync_steps;
	next_step_passenger = 0;
	next_step_mail = 0;
	destroying = false;
	transferring_cargoes = NULL;
#ifdef MULTI_THREAD
	cities_to_process = 0;
	terminating_threads = false;
#endif

	for(  uint i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		selected_tool[i] = tool_t::general_tool[TOOL_QUERY];
	}

	viewport = new viewport_t(this);

	set_dirty();

	// for new world just set load version to current savegame version
	load_version = loadsave_t::int_version( env_t::savegame_version_str, NULL, NULL );

	// standard prices
	goods_manager_t::set_multiplier( 1000, settings.get_meters_per_tile() );

	zeiger = 0;
	plan = 0;

	grid_hgts = 0;
	water_hgts = 0;
	schedule_counter = 0;
	nosave_warning = nosave = false;

	recheck_road_connexions = true;
	actual_industry_density = industry_density_proportion = 0;

	loaded_rotation = 0;
	last_year = 1930;
	last_month = 0;

	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		players[i] = NULL;
		MEMZERO(player_password_hash[i]);
	}

	// no distance to show at first ...
	show_distance = koord3d::invalid;
	scenario = NULL;

	map_counter = 0;

	msg = new message_t();
	cached_size.x = 0;
	cached_size.y = 0;

	base_pathing_counter = 0;

	citycar_speed_average = 50;

	city_road = NULL;

	// @author: jamespetts
	set_scale();

	// Added by : Knightly
	path_explorer_t::initialise(this);
	records = new records_t(this->msg);

	// generate ground textures once
	ground_desc_t::init_ground_textures(this);

	// set single instance
	world = this;

	parallel_operations = -1;

	const uint8 number_of_passenger_classes = goods_manager_t::passengers->get_number_of_classes();
	commuter_targets = new weighted_vector_tpl<gebaeude_t*>[number_of_passenger_classes];
	visitor_targets = new weighted_vector_tpl<gebaeude_t*>[number_of_passenger_classes];

#ifdef MULTI_THREAD
	first_step = 1;
#endif
}

#ifdef DEBUG_SIMRAND_CALLS
	vector_tpl<const char*> karte_t::random_callers;
#endif


karte_t::~karte_t()
{
	is_sound = false;

	destroy();

	// not deleting the tools of this map ...
	delete viewport;
	delete msg;
	delete records;

	delete[] commuter_targets;
	delete[] visitor_targets;

	// unset single instance
	if (world == this) {
		world = NULL;
	}
}

void karte_t::set_scale()
{
	const uint16 scale_factor = get_settings().get_meters_per_tile();
	
	// Vehicles
	for(int i = road_wt; i <= air_wt; i++) 
	{
		if(&vehicle_builder_t::get_info((waytype_t)i) != NULL)
		{
			FOR(slist_tpl<vehicle_desc_t*>, const & info, vehicle_builder_t::get_info((waytype_t)i))
			{
				info->set_scale(scale_factor, get_settings().get_way_wear_power_factor_rail_type(), get_settings().get_way_wear_power_factor_road_type(), get_settings().get_standard_axle_load());
			}
		}
	}

	// Ways
	stringhashtable_tpl <way_desc_t *> * ways = way_builder_t::get_all_ways();

	if(ways != NULL)
	{
		FOR(stringhashtable_tpl<way_desc_t *>, & info, *ways)
		{
			info.value->set_scale(scale_factor);
		}
	}

	// Tunnels
	stringhashtable_tpl <tunnel_desc_t *> * tunnels = tunnel_builder_t::get_all_tunnels();

	if(tunnels != NULL)
	{
		FOR(stringhashtable_tpl<tunnel_desc_t *>, & info, *tunnels)
		{
			info.value->set_scale(scale_factor);
		}
	}

	// Bridges
	stringhashtable_tpl <bridge_desc_t *> * bridges = bridge_builder_t::get_all_bridges();

	if(bridges != NULL)
	{
		FOR(stringhashtable_tpl<bridge_desc_t *>, & info, *bridges)
		{
			info.value->set_scale(scale_factor);
		}
	}

	// Way objects
	FOR(stringhashtable_tpl<way_obj_desc_t *>, & info, *wayobj_t::get_all_wayobjects())
	{
		info.value->set_scale(scale_factor);
	}

	// Stations
	ITERATE(hausbauer_t::modifiable_station_buildings, n)
	{
		hausbauer_t::modifiable_station_buildings[n]->set_scale(scale_factor); 
	}

	// Goods
	const uint16 goods_count = goods_manager_t::get_count();
	for(uint16 i = 0; i < goods_count; i ++)
	{
		goods_manager_t::get_modifiable_info(i)->set_scale(scale_factor);
	}

	// Industries
	FOR(stringhashtable_tpl<factory_desc_t*>, & info, factory_builder_t::modifiable_table)
	{
		info.value->set_scale(scale_factor);
	}

	// Signs and signals
	roadsign_t::set_scale(scale_factor);

	// Settings
	settings.set_scale();

	// Cached speed factors need recalc
	speed_factors_are_set = false;
}


const char* karte_t::can_lower_plan_to(const player_t *player, sint16 x, sint16 y, sint8 h) const
{
	const planquadrat_t *plan = access(x,y);

	if(  plan==NULL  ) {
		return "";
	}

	if(  h < groundwater - 3  ) {
		return "";
	}

	const sint8 hmax = plan->get_kartenboden()->get_hoehe();
	if(  (hmax == h  ||  hmax == h - 1)  &&  (plan->get_kartenboden()->get_grund_hang() == 0  ||  is_plan_height_changeable( x, y ))  ) {
		return NULL;
	}

	if(  !is_plan_height_changeable(x, y)  ) {
		return "";
	}

	// tunnel slope below?
	grund_t *gr = plan->get_boden_in_hoehe( h - 1 );
	if(  !gr  ) {
		gr = plan->get_boden_in_hoehe( h - 2 );
	}
	if(  !gr  && settings.get_way_height_clearance()==2  ) {
		gr = plan->get_boden_in_hoehe( h - 3 );
	}
	if (gr  &&  h < gr->get_pos().z + slope_t::max_diff(gr->get_weg_hang()) + settings.get_way_height_clearance()) {
		return "";
	}

	// tunnel below?
	while(h < hmax) {
		if(plan->get_boden_in_hoehe(h)) {
			return "";
		}
		h ++;
	}

	// check allowance by scenario
	if (get_scenario()->is_scripted()) {
		return get_scenario()->is_work_allowed_here(player, TOOL_LOWER_LAND|GENERAL_TOOL, ignore_wt, plan->get_kartenboden()->get_pos());
	}

	return NULL;
}


const char* karte_t::can_raise_plan_to(const player_t *player, sint16 x, sint16 y, sint8 h) const
{
	const planquadrat_t *plan = access(x,y);
	if(  plan == 0  ||  !is_plan_height_changeable(x, y)  ) {
		return "";
	}

	// irgendwo eine Bruecke im Weg?
	int hmin = plan->get_kartenboden()->get_hoehe();
	while(h > hmin) {
		if(plan->get_boden_in_hoehe(h)) {
			return "";
		}
		h --;
	}

	// check allowance by scenario
	if (get_scenario()->is_scripted()) {
		return get_scenario()->is_work_allowed_here(player, TOOL_RAISE_LAND|GENERAL_TOOL, ignore_wt, plan->get_kartenboden()->get_pos());
	}

	return NULL;
}


bool karte_t::is_plan_height_changeable(sint16 x, sint16 y) const
{
	const planquadrat_t *plan = access(x,y);
	bool ok = true;

	if(plan != NULL) {
		grund_t *gr = plan->get_kartenboden();

		ok = (gr->ist_natur() || gr->is_water())  &&  !gr->hat_wege()  &&  !gr->is_halt();

		for(  int i=0; ok  &&  i<gr->get_top(); i++  ) {
			const obj_t *obj = gr->obj_bei(i);
			assert(obj != NULL);
			ok =
				obj->get_typ() == obj_t::baum  ||
				obj->get_typ() == obj_t::zeiger  ||
				obj->get_typ() == obj_t::wolke  ||
				obj->get_typ() == obj_t::sync_wolke  ||
				obj->get_typ() == obj_t::async_wolke  ||
				obj->get_typ() == obj_t::groundobj;
		}
	}

	return ok;
}


bool karte_t::terraformer_t::node_t::comp(const karte_t::terraformer_t::node_t& a, const karte_t::terraformer_t::node_t& b)
{
	int diff = a.x- b.x;
	if (diff == 0) {
		diff = a.y - b.y;
	}
	return diff<0;
}


void karte_t::terraformer_t::add_node(bool raise, sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw)
{
	if (!welt->is_within_limits(x,y)) {
		return;
	}
	node_t test(x, y, hsw, hse, hne, hnw, actual_flag^3);
	node_t *other = list.insert_unique_ordered(test, node_t::comp);

	sint8 factor = raise ? +1 : -1;

	if (other) {
		for(int i=0; i<4; i++) {
			if (factor*other->h[i] < factor*test.h[i]) {
				other->h[i] = test.h[i];
				other->changed |= actual_flag^3;
				ready = false;
			}
		}
	}
	else {
		ready = false;
	}
}

void karte_t::terraformer_t::add_raise_node(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw)
{
	add_node(true, x, y, hsw, hse, hne, hnw);
}

void karte_t::terraformer_t::add_lower_node(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw)
{
	add_node(false, x, y, hsw, hse, hne, hnw);
}

void karte_t::terraformer_t::iterate(bool raise)
{
	while( !ready) {
		actual_flag ^= 3; // flip bits
		// clear new_flag bit
		FOR(vector_tpl<node_t>, &i, list) {
			i.changed &= actual_flag;
		}
		// process nodes with actual_flag set
		ready = true;
		for(uint32 j=0; j < list.get_count(); j++) {
			node_t& i = list[j];
			if (i.changed & actual_flag) {
				i.changed &= ~ actual_flag;
				if (raise) {
					welt->prepare_raise(*this, i.x, i.y, i.h[0], i.h[1], i.h[2], i.h[3]);
				}
				else {
					welt->prepare_lower(*this, i.x, i.y, i.h[0], i.h[1], i.h[2], i.h[3]);
				}
			}
		}
	}
}


const char* karte_t::terraformer_t::can_raise_all(const player_t *player, bool allow_deep_water, bool keep_water) const
{
	const char* err = NULL;
	FOR(vector_tpl<node_t>, const &i, list) {
		err = welt->can_raise_to(player, i.x, i.y, keep_water, allow_deep_water, i.h[0], i.h[1], i.h[2], i.h[3]);
		if (err) return err;
	}
	return NULL;
}

const char* karte_t::terraformer_t::can_lower_all(const player_t *player, bool allow_deep_water) const
{
	const char* err = NULL;
	FOR(vector_tpl<node_t>, const &i, list) {
		err = welt->can_lower_to(player, i.x, i.y, i.h[0], i.h[1], i.h[2], i.h[3], allow_deep_water);
		if (err) {
			return err;
		}
	}
	return NULL;
}

int karte_t::terraformer_t::raise_all()
{
	int n=0;
	FOR(vector_tpl<node_t>, &i, list) {
		n += welt->raise_to(i.x, i.y, i.h[0], i.h[1], i.h[2], i.h[3]);
	}
	return n;
}

int karte_t::terraformer_t::lower_all()
{
	int n=0;
	FOR(vector_tpl<node_t>, &i, list) {
		n += welt->lower_to(i.x, i.y, i.h[0], i.h[1], i.h[2], i.h[3]);
	}
	return n;
}


const char* karte_t::can_raise_to(const player_t *player, sint16 x, sint16 y, bool keep_water, bool allow_deep_water, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw) const
{
	assert(is_within_limits(x,y));
	grund_t *gr = lookup_kartenboden_nocheck(x,y);
	const sint8 water_hgt = get_water_hgt_nocheck(x,y);

	const sint8 max_hgt = max(max(hsw,hse),max(hne,hnw));
	const sint8 min_hgt = min(min(hsw,hse),min(hne,hnw));

	if(  gr->is_water()  &&  keep_water  &&  max_hgt > water_hgt  ) {
		return "";
	}

	if(gr->is_water() && min_hgt < groundwater && !allow_deep_water) 
	{
		return "Cannot terraform in deep water";
	}

	const char* err = can_raise_plan_to(player, x, y, max_hgt);

	return err;
}


void karte_t::prepare_raise(terraformer_t& digger, sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw)
{
	assert(is_within_limits(x,y));
	grund_t *gr = lookup_kartenboden_nocheck(x,y);
	const sint8 water_hgt = get_water_hgt_nocheck(x,y);
	const sint8 h0 = gr->get_hoehe();
	// old height
	const sint8 h0_sw = gr->is_water() ? min(water_hgt, lookup_hgt_nocheck(x,y+1) )   : h0 + corner_sw( gr->get_grund_hang() );
	const sint8 h0_se = gr->is_water() ? min(water_hgt, lookup_hgt_nocheck(x+1,y+1) ) : h0 + corner_se( gr->get_grund_hang() );
	const sint8 h0_ne = gr->is_water() ? min(water_hgt, lookup_hgt_nocheck(x+1,y) )   : h0 + corner_ne( gr->get_grund_hang() );
	const sint8 h0_nw = gr->is_water() ? min(water_hgt, lookup_hgt_nocheck(x,y) )     : h0 + corner_nw( gr->get_grund_hang() );

	// new height
	const sint8 hn_sw = max(hsw, h0_sw);
	const sint8 hn_se = max(hse, h0_se);
	const sint8 hn_ne = max(hne, h0_ne);
	const sint8 hn_nw = max(hnw, h0_nw);
	// nothing to do?
	if(  !gr->is_water()  &&  h0_sw >= hsw  &&  h0_se >= hse  &&  h0_ne >= hne  &&  h0_nw >= hnw  ) {
		return;
	}
	// calc new height and slope
	const sint8 hneu = min( min( hn_sw, hn_se ), min( hn_ne, hn_nw ) );
	const sint8 hmaxneu = max( max( hn_sw, hn_se ), max( hn_ne, hn_nw ) );

	const uint8 max_hdiff = ground_desc_t::double_grounds ? 2 : 1;

	bool ok = (hmaxneu - hneu <= max_hdiff); // may fail on water tiles since lookup_hgt might be modified from previous raise_to calls
	if(  !ok  &&  !gr->is_water()  ) {
		assert(false);
	}

	// sw
	if (h0_sw < hsw) {
		digger.add_raise_node( x - 1, y + 1, hsw - max_hdiff, hsw - max_hdiff, hsw, hsw - max_hdiff );
	}
	// s
	if (h0_sw < hsw  ||  h0_se < hse) {
		const sint8 hs = max( hse, hsw ) - max_hdiff;
		digger.add_raise_node( x, y + 1, hs, hs, hse, hsw );
	}
	// se
	if (h0_se < hse) {
		digger.add_raise_node( x + 1, y + 1, hse - max_hdiff, hse - max_hdiff, hse - max_hdiff, hse );
	}
	// e
	if (h0_se < hse  ||  h0_ne < hne) {
		const sint8 he = max( hse, hne ) - max_hdiff;
		digger.add_raise_node( x + 1, y, hse, he, he, hne );
	}
	// ne
	if (h0_ne < hne) {
		digger.add_raise_node( x + 1,y - 1, hne, hne - max_hdiff, hne - max_hdiff, hne - max_hdiff );
	}
	// n
	if (h0_nw < hnw  ||  h0_ne < hne) {
		const sint8 hn = max( hnw, hne ) - max_hdiff;
		digger.add_raise_node( x, y - 1, hnw, hne, hn, hn );
	}
	// nw
	if (h0_nw < hnw) {
		digger.add_raise_node( x - 1, y - 1, hnw - max_hdiff, hnw, hnw - max_hdiff, hnw - max_hdiff );
	}
	// w
	if (h0_sw < hsw  ||  h0_nw < hnw) {
		const sint8 hw = max( hnw, hsw ) - max_hdiff;
		digger.add_raise_node( x - 1, y, hw, hsw, hnw, hw );
	}
}


int karte_t::raise_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw)
{
	int n=0;
	assert(is_within_limits(x,y));
	grund_t *gr = lookup_kartenboden_nocheck(x,y);
	const sint8 water_hgt = get_water_hgt_nocheck(x,y);
	const sint8 h0 = gr->get_hoehe();
	// old height
	const sint8 h0_sw = gr->is_water() ? min(water_hgt, lookup_hgt_nocheck(x,y+1) )   : h0 + corner_sw( gr->get_grund_hang() );
	const sint8 h0_se = gr->is_water() ? min(water_hgt, lookup_hgt_nocheck(x+1,y+1) ) : h0 + corner_se( gr->get_grund_hang() );
	const sint8 h0_ne = gr->is_water() ? min(water_hgt, lookup_hgt_nocheck(x+1,y) )   : h0 + corner_ne( gr->get_grund_hang() );
	const sint8 h0_nw = gr->is_water() ? min(water_hgt, lookup_hgt_nocheck(x,y) )     : h0 + corner_nw( gr->get_grund_hang() );

	// new height
	const sint8 hn_sw = max(hsw, h0_sw);
	const sint8 hn_se = max(hse, h0_se);
	const sint8 hn_ne = max(hne, h0_ne);
	const sint8 hn_nw = max(hnw, h0_nw);
	// nothing to do?
	if(  !gr->is_water()  &&  h0_sw >= hsw  &&  h0_se >= hse  &&  h0_ne >= hne  &&  h0_nw >= hnw  ) {
		return 0;
	}
	// calc new height and slope
	const sint8 hneu = min( min( hn_sw, hn_se ), min( hn_ne, hn_nw ) );
	const sint8 hmaxneu = max( max( hn_sw, hn_se ), max( hn_ne, hn_nw ) );

	const uint8 max_hdiff = ground_desc_t::double_grounds ? 2 : 1;
	const sint8 disp_hneu = max( hneu, water_hgt );
	const sint8 disp_hn_sw = max( hn_sw, water_hgt );
	const sint8 disp_hn_se = max( hn_se, water_hgt );
	const sint8 disp_hn_ne = max( hn_ne, water_hgt );
	const sint8 disp_hn_nw = max( hn_nw, water_hgt );
	const uint8 sneu = (disp_hn_sw - disp_hneu) + ((disp_hn_se - disp_hneu) * 3) + ((disp_hn_ne - disp_hneu) * 9) + ((disp_hn_nw - disp_hneu) * 27);

	bool ok = (hmaxneu - hneu <= max_hdiff); // may fail on water tiles since lookup_hgt might be modified from previous raise_to calls
	if (!ok && !gr->is_water()) {
		assert(false);
	}
	// change height and slope, for water tiles only if they will become land
	if(  !gr->is_water()  ||  (hmaxneu > water_hgt  ||  (hneu == water_hgt  &&  hmaxneu == water_hgt)  )  ) {
		gr->set_pos( koord3d( x, y, disp_hneu ) );
		gr->set_grund_hang( (slope_t::type)sneu );
		access_nocheck(x,y)->angehoben();
		set_water_hgt(x, y, groundwater-4);
	}

	// update north point in grid
	set_grid_hgt(x, y, hn_nw);
	calc_climate(koord(x,y), true);
	if ( x == cached_size.x ) {
		// update eastern grid coordinates too if we are in the edge.
		set_grid_hgt(x+1, y, hn_ne);
		set_grid_hgt(x+1, y+1, hn_se);
	}
	if ( y == cached_size.y ) {
		// update southern grid coordinates too if we are in the edge.
		set_grid_hgt(x, y+1, hn_sw);
		set_grid_hgt(x+1, y+1, hn_se);
	}

	n += hn_sw - h0_sw + hn_se - h0_se + hn_ne - h0_ne  + hn_nw - h0_nw;

	lookup_kartenboden_nocheck(x,y)->calc_image();
	if ( (x+1) < cached_size.x ) {
		lookup_kartenboden_nocheck(x+1,y)->calc_image();
	}
	if ( (y+1) < cached_size.y ) {
		lookup_kartenboden_nocheck(x,y+1)->calc_image();
	}

	return n;
}


// raise height in the hgt-array
void karte_t::raise_grid_to(sint16 x, sint16 y, sint8 h)
{
	if(is_within_grid_limits(x,y)) {
		const sint32 offset = x + y*(cached_grid_size.x+1);

		if(  grid_hgts[offset] < h  ) {
			grid_hgts[offset] = h;

			const sint8 hh = h - (ground_desc_t::double_grounds ? 2 : 1);

			// set new height of neighbor grid points
			raise_grid_to(x-1, y-1, hh);
			raise_grid_to(x  , y-1, hh);
			raise_grid_to(x+1, y-1, hh);
			raise_grid_to(x-1, y  , hh);
			raise_grid_to(x+1, y  , hh);
			raise_grid_to(x-1, y+1, hh);
			raise_grid_to(x  , y+1, hh);
			raise_grid_to(x+1, y+1, hh);
		}
	}
}


int karte_t::grid_raise(const player_t *player, koord k, bool allow_deep_water, const char*&err)
{
	int n = 0;

	if(is_within_grid_limits(k)) {

		const grund_t *gr = lookup_kartenboden_gridcoords(k);
		const slope_t::type corner_to_raise = get_corner_to_operate(k);

		const sint16 x = gr->get_pos().x;
		const sint16 y = gr->get_pos().y;
		const sint8 hgt = gr->get_hoehe(corner_to_raise);

		sint8 hsw, hse, hne, hnw;
		if(  !gr->is_water()  ) {
			const sint8 f = ground_desc_t::double_grounds ?  2 : 1;
			const sint8 o = ground_desc_t::double_grounds ?  1 : 0;

			hsw = hgt - o + scorner_sw( corner_to_raise ) * f;
			hse = hgt - o + scorner_se( corner_to_raise ) * f;
			hne = hgt - o + scorner_ne( corner_to_raise ) * f;
			hnw = hgt - o + scorner_nw( corner_to_raise ) * f;
		}
		else {
			hsw = hse = hne = hnw = hgt;
		}

		terraformer_t digger(this);
		digger.add_raise_node(x, y, hsw, hse, hne, hnw);
		digger.iterate(true);

		err = digger.can_raise_all(player, allow_deep_water);
		if (err) {
			return 0;
		}

		n = digger.raise_all();

		// force world full redraw, or background could be dirty.
		set_dirty();

		if (max_height < lookup_kartenboden_gridcoords(k)->get_hoehe()) {
			max_height = lookup_kartenboden_gridcoords(k)->get_hoehe();
		}
	}
	return (n+3)>>2;
}


void karte_t::prepare_lower(terraformer_t& digger, sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw)
{
	assert(is_within_limits(x,y));
	grund_t *gr = lookup_kartenboden_nocheck(x,y);
	const sint8 water_hgt = get_water_hgt_nocheck(x,y);
	const sint8 h0 = gr->get_hoehe();
	// which corners have to be raised?
	const sint8 h0_sw = gr->is_water() ? min( water_hgt, lookup_hgt_nocheck(x,y+1) )   : h0 + corner_sw( gr->get_grund_hang() );
	const sint8 h0_se = gr->is_water() ? min( water_hgt, lookup_hgt_nocheck(x+1,y+1) ) : h0 + corner_se( gr->get_grund_hang() );
	const sint8 h0_ne = gr->is_water() ? min( water_hgt, lookup_hgt_nocheck(x,y+1) )   : h0 + corner_ne( gr->get_grund_hang() );
	const sint8 h0_nw = gr->is_water() ? min( water_hgt, lookup_hgt_nocheck(x,y) )     : h0 + corner_nw( gr->get_grund_hang() );

	const uint8 max_hdiff = ground_desc_t::double_grounds ?  2 : 1;

	// sw
	if (h0_sw > hsw) {
		digger.add_lower_node(x - 1, y + 1, hsw + max_hdiff, hsw + max_hdiff, hsw, hsw + max_hdiff);
	}
	// s
	if (h0_se > hse || h0_sw > hsw) {
		const sint8 hs = min( hse, hsw ) + max_hdiff;
		digger.add_lower_node( x, y + 1, hs, hs, hse, hsw);
	}
	// se
	if (h0_se > hse) {
		digger.add_lower_node( x + 1, y + 1, hse + max_hdiff, hse + max_hdiff, hse + max_hdiff, hse);
	}
	// e
	if (h0_se > hse || h0_ne > hne) {
		const sint8 he = max( hse, hne ) + max_hdiff;
		digger.add_lower_node( x + 1,y, hse, he, he, hne);
	}
	// ne
	if (h0_ne > hne) {
		digger.add_lower_node( x + 1, y - 1, hne, hne + max_hdiff, hne + max_hdiff, hne + max_hdiff);
	}
	// n
	if (h0_nw > hnw  ||  h0_ne > hne) {
		const sint8 hn = min( hnw, hne ) + max_hdiff;
		digger.add_lower_node( x, y - 1, hnw, hne, hn, hn);
	}
	// nw
	if (h0_nw > hnw) {
		digger.add_lower_node( x - 1, y - 1, hnw + max_hdiff, hnw, hnw + max_hdiff, hnw + max_hdiff);
	}
	// w
	if (h0_nw > hnw || h0_sw > hsw) {
		const sint8 hw = min( hnw, hsw ) + max_hdiff;
		digger.add_lower_node( x - 1, y, hw, hsw, hnw, hw);
	}
}

// lower plan
// new heights for each corner given
// only test corners in ctest to avoid infinite loops
const char* karte_t::can_lower_to(const player_t* player, sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw, bool allow_deep_water) const
{
	assert(is_within_limits(x,y));

	grund_t *gr = lookup_kartenboden_nocheck(x,y);
	const sint8 water_hgt = get_water_hgt_nocheck(x,y);
	const sint8 min_hgt = min(min(hsw,hse),min(hne,hnw));

	const sint8 hneu = min( min( hsw, hse ), min( hne, hnw ) );
	// water heights
	// check if need to lower water height for higher neighbouring tiles
	for(  sint16 i = 0 ;  i < 8 ;  i++  ) {
		const koord neighbour = koord( x, y ) + koord::neighbours[i];
		if(  is_within_limits(neighbour)  &&  get_water_hgt_nocheck(neighbour) > hneu  ) {
			if (!is_plan_height_changeable( neighbour.x, neighbour.y )) {
				return "";
			}
		}
	}

	if(min_hgt < groundwater && !allow_deep_water) 
	{
		return "Cannot terraform in deep water";
	}

	return can_lower_plan_to(player, x, y, hneu );
}


int karte_t::lower_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw)
{
	int n=0;
	assert(is_within_limits(x,y));
	grund_t *gr = lookup_kartenboden_nocheck(x,y);
	const uint8 old_slope = gr->get_grund_hang();
	sint8 water_hgt = get_water_hgt_nocheck(x,y);
	const sint8 h0 = gr->get_hoehe();
	// old height
	const sint8 h0_sw = gr->is_water() ? min( water_hgt, lookup_hgt_nocheck(x,y+1) )   : h0 + corner_sw( gr->get_grund_hang() );
	const sint8 h0_se = gr->is_water() ? min( water_hgt, lookup_hgt_nocheck(x+1,y+1) ) : h0 + corner_se( gr->get_grund_hang() );
	const sint8 h0_ne = gr->is_water() ? min( water_hgt, lookup_hgt_nocheck(x+1,y) )   : h0 + corner_ne( gr->get_grund_hang() );
	const sint8 h0_nw = gr->is_water() ? min( water_hgt, lookup_hgt_nocheck(x,y) )     : h0 + corner_nw( gr->get_grund_hang() );
	// new height
	const sint8 hn_sw = min(hsw, h0_sw);
	const sint8 hn_se = min(hse, h0_se);
	const sint8 hn_ne = min(hne, h0_ne);
	const sint8 hn_nw = min(hnw, h0_nw);
	// nothing to do?
	if(  gr->is_water()  ) {
		if(  h0_nw <= hnw  ) {
			return 0;
		}
	}
	else {
		if(  h0_sw <= hsw  &&  h0_se <= hse  &&  h0_ne <= hne  &&  h0_nw <= hnw  ) {
			return 0;
		}
	}

	// calc new height and slope
	const sint8 hneu = min( min( hn_sw, hn_se ), min( hn_ne, hn_nw ) );
	const sint8 hmaxneu = max( max( hn_sw, hn_se ), max( hn_ne, hn_nw ) );

	// only slope could have been shores
	if(  old_slope  ) {
		// if there are any shore corners, then the new tile must be all water since it is lowered
		const bool make_water =
			get_water_hgt_nocheck(x,y) <= lookup_hgt_nocheck(x,y)  ||
			get_water_hgt_nocheck(x+1,y) <= lookup_hgt_nocheck(x+1,y)  ||
			get_water_hgt_nocheck(x,y+1) <= lookup_hgt_nocheck(x,y+1)  ||
			get_water_hgt_nocheck(x+1,y+1) <= lookup_hgt_nocheck(x+1,y+1);
		if(  make_water  ) {
			sint8 water_table = water_hgt >= hneu ? water_hgt : hneu;
			set_water_hgt(x, y, water_table );
		}
	}

	if(  hneu >= water_hgt  ) {
		// calculate water table from surrounding tiles - start off with height on this tile
		sint8 water_table = water_hgt >= h0 ? water_hgt : groundwater - 4;

		/* we test each corner in turn to see whether it is at the base height of the tile
			if it is we then mark the 3 surrounding tiles for that corner for checking
			surrounding tiles are indicated by bits going anti-clockwise from
			(binary) 00000001 for north-west through to (binary) 10000000 for north
			as this is the order of directions used by koord::neighbours[] */

		uint8 neighbour_flags = 0;

		if(  hn_nw == hneu  ) {
			neighbour_flags |= 0x83;
		}
		if(  hn_ne == hneu  ) {
			neighbour_flags |= 0xe0;
		}
		if(  hn_se == hneu  ) {
			neighbour_flags |= 0x38;
		}
		if(  hn_sw == hneu  ) {
			neighbour_flags |= 0x0e;
		}

		for(  sint16 i = 0;  i < 8 ;  i++  ) {
			const koord neighbour = koord( x, y ) + koord::neighbours[i];

			// here we look at the bit in neighbour_flags for this direction
			// we shift it i bits to the right and test the least significant bit

			if(  is_within_limits( neighbour )  &&  ((neighbour_flags >> i) & 1)  ) {
				grund_t *gr2 = lookup_kartenboden_nocheck( neighbour );
				const sint8 water_hgt_neighbour = get_water_hgt_nocheck( neighbour );
				if(  gr2  &&  (water_hgt_neighbour >= gr2->get_hoehe())  &&  water_hgt_neighbour <= hneu  ) {
					water_table = max( water_table, water_hgt_neighbour );
				}
			}
		}

		for(  sint16 i = 0;  i < 8 ;  i++  ) {
			const koord neighbour = koord( x, y ) + koord::neighbours[i];
			if(  is_within_limits( neighbour )  ) {
				grund_t *gr2 = lookup_kartenboden_nocheck( neighbour );
				if(  gr2  &&  gr2->get_hoehe() < water_table  ) {
					i = 8;
					water_table = groundwater - 4;
				}
			}
		}

		// only allow water table to be lowered (except for case of sea level)
		// this prevents severe (errors!
		if(  water_table < get_water_hgt_nocheck(x,y)  ) {
			water_hgt = water_table;
			set_water_hgt(x, y, water_table );
		}
	}

	// calc new height and slope
	const sint8 disp_hneu = max( hneu, water_hgt );
	const sint8 disp_hn_sw = max( hn_sw, water_hgt );
	const sint8 disp_hn_se = max( hn_se, water_hgt );
	const sint8 disp_hn_ne = max( hn_ne, water_hgt );
	const sint8 disp_hn_nw = max( hn_nw, water_hgt );
	const uint8 sneu = (disp_hn_sw - disp_hneu) + ((disp_hn_se - disp_hneu) * 3) + ((disp_hn_ne - disp_hneu) * 9) + ((disp_hn_nw - disp_hneu) * 27);

	// change height and slope for land tiles only
	if(  !gr->is_water()  ||  (hmaxneu > water_hgt)  ) {
		gr->set_pos( koord3d( x, y, disp_hneu ) );
		gr->set_grund_hang( (slope_t::type)sneu );
		access_nocheck(x,y)->abgesenkt();
	}
	// update north point in grid
	set_grid_hgt(x, y, hn_nw);
	if ( x == cached_size.x ) {
		// update eastern grid coordinates too if we are in the edge.
		set_grid_hgt(x+1, y, hn_ne);
		set_grid_hgt(x+1, y+1, hn_se);
	}
	if ( y == cached_size.y ) {
		// update southern grid coordinates too if we are in the edge.
		set_grid_hgt(x, y+1, hn_sw);
		set_grid_hgt(x+1, y+1, hn_se);
	}

	// water heights
	// lower water height for higher neighbouring tiles
	// find out how high water is
	for(  sint16 i = 0;  i < 8;  i++  ) {
		const koord neighbour = koord( x, y ) + koord::neighbours[i];
		if(  is_within_limits( neighbour )  ) {
			const sint8 water_hgt_neighbour = get_water_hgt_nocheck( neighbour );
			if(water_hgt_neighbour > hneu  ) {
				if(  min_hgt_nocheck( neighbour ) < water_hgt_neighbour  ) {
					// convert to flat ground before lowering water level
					raise_grid_to( neighbour.x, neighbour.y, water_hgt_neighbour );
					raise_grid_to( neighbour.x + 1, neighbour.y, water_hgt_neighbour );
					raise_grid_to( neighbour.x, neighbour.y + 1, water_hgt_neighbour );
					raise_grid_to( neighbour.x + 1, neighbour.y + 1, water_hgt_neighbour );
				}
				set_water_hgt( neighbour, hneu );
				access_nocheck(neighbour)->correct_water();
			}
		}
	}

	calc_climate( koord( x, y ), false );
	for(  sint16 i = 0;  i < 8;  i++  ) {
		const koord neighbour = koord( x, y ) + koord::neighbours[i];
		calc_climate( neighbour, false );
	}

	// recalc landscape images - need to extend 2 in each direction
	for(  sint16 j = y - 2;  j <= y + 2;  j++  ) {
		for(  sint16 i = x - 2;  i <= x + 2;  i++  ) {
			if(  is_within_limits( i, j )  /*&&  (i != x  ||  j != y)*/  ) {
				recalc_transitions( koord (i, j ) );
			}
		}
	}

	n += h0_sw-hn_sw + h0_se-hn_se + h0_ne-hn_ne + h0_nw-hn_nw;

	lookup_kartenboden_nocheck(x,y)->calc_image();
	if( (x+1) < cached_size.x ) {
		lookup_kartenboden_nocheck(x+1,y)->calc_image();
	}
	if( (y+1) < cached_size.y ) {
		lookup_kartenboden_nocheck(x,y+1)->calc_image();
	}
	return n;
}


void karte_t::lower_grid_to(sint16 x, sint16 y, sint8 h)
{
	if(is_within_grid_limits(x,y)) {
		const sint32 offset = x + y*(cached_grid_size.x+1);

		if(  grid_hgts[offset] > h  ) {
			grid_hgts[offset] = h;
			sint8 hh = h + 2;
			// set new height of neighbor grid points
			lower_grid_to(x-1, y-1, hh);
			lower_grid_to(x  , y-1, hh);
			lower_grid_to(x+1, y-1, hh);
			lower_grid_to(x-1, y  , hh);
			lower_grid_to(x+1, y  , hh);
			lower_grid_to(x-1, y+1, hh);
			lower_grid_to(x  , y+1, hh);
			lower_grid_to(x+1, y+1, hh);
		}
	}
}


int karte_t::grid_lower(const player_t *player, koord k, const char*&err)
{
	int n = 0;

	if(is_within_grid_limits(k)) {

		const grund_t *gr = lookup_kartenboden_gridcoords(k);
		const slope_t::type corner_to_lower = get_corner_to_operate(k);

		const sint16 x = gr->get_pos().x;
		const sint16 y = gr->get_pos().y;
		const sint8 hgt = gr->get_hoehe(corner_to_lower);

		const sint8 f = ground_desc_t::double_grounds ?  2 : 1;
		const sint8 o = ground_desc_t::double_grounds ?  1 : 0;
		const sint8 hsw = hgt + o - scorner_sw( corner_to_lower ) * f;
		const sint8 hse = hgt + o - scorner_se( corner_to_lower ) * f;
		const sint8 hne = hgt + o - scorner_ne( corner_to_lower ) * f;
		const sint8 hnw = hgt + o - scorner_nw( corner_to_lower ) * f;

		terraformer_t digger(this);
		digger.add_lower_node(x, y, hsw, hse, hne, hnw);
		digger.iterate(false);

		err = digger.can_lower_all(player, player->is_public_service());
		if (err) {
			return 0;
		}

		n = digger.lower_all();
		err = NULL;
	}
	return (n+3)>>2;
}


bool karte_t::can_flatten_tile(player_t *player, koord k, sint8 hgt, bool keep_water, bool make_underwater_hill)
{
	return flatten_tile(player, k, hgt, keep_water, make_underwater_hill, true /* just check */);
}


// make a flat level at this position 
bool karte_t::flatten_tile(player_t *player, koord k, sint8 hgt, bool keep_water, bool make_underwater_hill, bool justcheck)
{
	int n = 0;
	bool ok = true;
	const grund_t *gr = lookup_kartenboden(k);
	const slope_t::type slope = gr->get_grund_hang();
	const sint8 old_hgt = make_underwater_hill  &&  gr->is_water() ? min_hgt(k) : gr->get_hoehe();
	const sint8 max_hgt = old_hgt + slope_t::max_diff(slope);
	if(  max_hgt > hgt  ) {

		terraformer_t digger(this);
		digger.add_lower_node(k.x, k.y, hgt, hgt, hgt, hgt);
		digger.iterate(false);

		ok = digger.can_lower_all(player, player ? player->is_public_service() : true) == NULL;

		if (ok  &&  !justcheck) {
			n += digger.lower_all();
		}
	}
	if(  ok  &&  old_hgt < hgt  ) {

		terraformer_t digger(this);
		digger.add_raise_node(k.x, k.y, hgt, hgt, hgt, hgt);
		digger.iterate(true);

		ok = digger.can_raise_all(player, keep_water) == NULL;

		if (ok  &&  !justcheck) {
			n += digger.raise_all();
		}
	}
	// was changed => pay for it
	if(n>0) {
		n = (n+3) >> 2;
		player_t::book_construction_costs(player, n * settings.cst_alter_land, k, ignore_wt);
	}
	return ok;
}


void karte_t::store_player_password_hash( uint8 player_nr, const pwd_hash_t& hash )
{
	player_password_hash[player_nr] = hash;
}


void karte_t::clear_player_password_hashes()
{
	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		player_password_hash[i].clear();
		if (players[i]) {
			players[i]->check_unlock(player_password_hash[i]);
		}
	}
}


void karte_t::rdwr_player_password_hashes(loadsave_t *file)
{
	pwd_hash_t dummy;
	for(  int i=0;  i<PLAYER_UNOWNED; i++  ) {
		pwd_hash_t *p = players[i] ? &players[i]->access_password_hash() : &dummy;
		for(  uint8 j=0; j<20; j++) {
			file->rdwr_byte( (*p)[j] );
		}
	}
}


void karte_t::call_change_player_tool(uint8 cmd, uint8 player_nr, uint16 param, bool scripted_call)
{
	if (env_t::networkmode) {
		nwc_chg_player_t *nwc = new nwc_chg_player_t(sync_steps, map_counter, cmd, player_nr, param, scripted_call);

		network_send_server(nwc);
	}
	else {
		change_player_tool(cmd, player_nr, param, !get_public_player()->is_locked()  ||  scripted_call, true);
		// update the window
		ki_kontroll_t* playerwin = (ki_kontroll_t*)win_get_magic(magic_ki_kontroll_t);
		if (playerwin) {
			playerwin->update_data();
		}
	}
}


bool karte_t::change_player_tool(uint8 cmd, uint8 player_nr, uint16 param, bool public_player_unlocked, bool exec)
{
	switch(cmd) {
		case new_player: {
			// only public player can start AI
			if(  (param != player_t::HUMAN  &&  !public_player_unlocked)  ||  param >= player_t::MAX_AI  ) {
				return false;
			}
			// range check, player already existent?
			if(  player_nr >= PLAYER_UNOWNED  ||   get_player(player_nr)  ) {
				return false;
			}
			if(exec) {
				init_new_player( player_nr, (uint8) param );
				// activate/deactivate AI immediately
				player_t *player = get_player(player_nr);
				if (param != player_t::HUMAN  &&  player) {
					player->set_active(true);
					settings.set_player_active(player_nr, player->is_active());
				}
			}
			return true;
		}
		case toggle_freeplay: {
			// only public player can change freeplay mode
			if (!public_player_unlocked  ||  !settings.get_allow_player_change()) {
				return false;
			}
			if (exec) {
				settings.set_freeplay( !settings.is_freeplay() );
			}
			return true;
		}
		case delete_player: {
			// range check, player existent?
			if ( player_nr >= PLAYER_UNOWNED  ||   get_player(player_nr)==NULL ) {
				return false;
			}
			if (exec) {
				remove_player(player_nr);
			}
			return true;
		}
		// unknown command: delete
		default: ;
	}
	return false;
}


void karte_t::set_tool( tool_t *tool, player_t *player )
{
	if(  get_random_mode()&LOAD_RANDOM  ) {
		dbg->warning("karte_t::set_tool", "Ignored tool %i during loading.", tool->get_id() );
		return;
	}
	bool scripted_call = tool->is_scripted();
	// check for scenario conditions
	if(  !scripted_call  &&  (scenario && !scenario->is_tool_allowed(player, tool->get_id(), tool->get_waytype()))  ) {
		return;
	}

	player_t* action_player = player;
	if(tool->get_id() == (TOOL_ACCESS_TOOL | SIMPLE_TOOL))
	{
		uint16 id_setting_player;
		uint16 id_receiving_player;
		sint16 allow_access;

		if(3 != sscanf(tool->get_default_param(), "g%hi,%hi,%hi", &id_setting_player, &id_receiving_player, &allow_access)) 
		{
			dbg->error( "karte_t::set_tool", "could not perform (%s)", tool->get_default_param() );
			return;
		}
		action_player = this->get_player(id_setting_player);
	}

	// check for password-protected players
	if(  (!tool->is_init_network_save()  ||  !tool->is_work_network_save())  &&  !scripted_call  &&
		 !(tool->get_id()==(TOOL_CHANGE_PLAYER|SIMPLE_TOOL)  ||  tool->get_id()==(TOOL_ADD_MESSAGE|SIMPLE_TOOL))  &&
		 action_player  &&  action_player->is_locked()  ) {
		// player is currently password protected => request unlock first
		create_win( -1, -1, new password_frame_t(action_player), w_info, magic_pwd_t + action_player->get_player_nr() );
		return;
	}
	tool->flags |= event_get_last_control_shift();
	if(!env_t::networkmode  ||  tool->is_init_network_save()  ) {
		local_set_tool(tool, player);
	}
	else {
		// queue tool for network
		nwc_tool_t *nwc = new nwc_tool_t(player, tool, zeiger->get_pos(), steps, map_counter, true);
		network_send_server(nwc);
	}
}


// set a new tool on our client, calls init
void karte_t::local_set_tool( tool_t *tool, player_t * player )
{
	tool->flags |= tool_t::WFL_LOCAL;

	if (get_scenario())
	{
		if (get_scenario()->is_scripted() && !get_scenario()->is_tool_allowed(player, tool->get_id())) {
			tool->flags = 0;
			return;
		}
	}
	// now call init
	bool init_result = tool->init(player);
	// for unsafe tools init() must return false
	assert(tool->is_init_network_save()  ||  !init_result);

	if (player && init_result) {

		set_dirty();
		tool_t *sp_tool = selected_tool[player->get_player_nr()];
		if(tool != sp_tool) {

			// reinit same tool => do not play sound twice
			sound_play(SFX_SELECT);

			// only exit, if it is not the same tool again ...

			sp_tool->flags |= tool_t::WFL_LOCAL;
			sp_tool->exit(player);
			sp_tool->flags =0;
		}
		else {
			// init again, to interrupt dragging
			selected_tool[player->get_player_nr()]->init(active_player);
		}
		
		if(  player==active_player  ) {
			// reset pointer
			koord3d zpos = zeiger->get_pos();
			// remove marks
			zeiger->change_pos( koord3d::invalid );
			// set new cursor properties
			tool->init_cursor(zeiger);
			// .. and mark again (if the position is acceptable for the tool)
			if( tool->check_valid_pos(zpos.get_2d())) {
				zeiger->change_pos( zpos );
			}
			else {
				zeiger->change_pos( koord3d::invalid );
			}
		}
		selected_tool[player->get_player_nr()] = tool;
	}
	tool->flags = 0;
}


sint8 karte_t::min_hgt_nocheck(const koord k) const
{
	// more optimised version of min_hgt code
	const sint8 * p = &grid_hgts[k.x + k.y*(sint32)(cached_grid_size.x+1)];

	const int h1 = *p;
	const int h2 = *(p+1);
	const int h3 = *(p+get_size().x+2);
	const int h4 = *(p+get_size().x+1);

	return min(min(h1,h2), min(h3,h4));
}


sint8 karte_t::max_hgt_nocheck(const koord k) const
{
	// more optimised version of max_hgt code
	const sint8 * p = &grid_hgts[k.x + k.y*(sint32)(cached_grid_size.x+1)];

	const int h1 = *p;
	const int h2 = *(p+1);
	const int h3 = *(p+get_size().x+2);
	const int h4 = *(p+get_size().x+1);

	return max(max(h1,h2), max(h3,h4));
}


sint8 karte_t::min_hgt(const koord k) const
{
	const sint8 h1 = lookup_hgt(k);
	const sint8 h2 = lookup_hgt(k+koord(1, 0));
	const sint8 h3 = lookup_hgt(k+koord(1, 1));
	const sint8 h4 = lookup_hgt(k+koord(0, 1));

	return min(min(h1,h2), min(h3,h4));
}


sint8 karte_t::max_hgt(const koord k) const
{
	const sint8 h1 = lookup_hgt(k);
	const sint8 h2 = lookup_hgt(k+koord(1, 0));
	const sint8 h3 = lookup_hgt(k+koord(1, 1));
	const sint8 h4 = lookup_hgt(k+koord(0, 1));

	return max(max(h1,h2), max(h3,h4));
}


planquadrat_t *rotate90_new_plan;
sint8 *rotate90_new_water;

void karte_t::rotate90_plans(sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max)
{
	const int LOOP_BLOCK = 64;
	if(  (loaded_rotation + settings.get_rotation()) & 1  ) {  // 1 || 3
		for(  int yy = y_min;  yy < y_max;  yy += LOOP_BLOCK  ) {
			for(  int xx = x_min;  xx < x_max;  xx += LOOP_BLOCK  ) {
				for(  int y = yy;  y < min(yy + LOOP_BLOCK, y_max);  y++  ) {
					for(  int x = xx;  x < min(xx + LOOP_BLOCK, x_max);  x++  ) {
						const int nr = x + (y * cached_grid_size.x);
						const int new_nr = (cached_size.y - y) + (x * cached_grid_size.y);
						// first rotate everything on the ground(s)
						for(  uint i = 0;  i < plan[nr].get_boden_count();  i++  ) {
							plan[nr].get_boden_bei(i)->rotate90();
						}
						// rotate climate transitions
						rotate_transitions( koord( x, y ) );
						// now: rotate all things on the map
						swap(rotate90_new_plan[new_nr], plan[nr]);
					}
				}
			}
		}
	}
	else {
		// first: rotate all things on the map
		for(  int xx = x_min;  xx < x_max;  xx += LOOP_BLOCK  ) {
			for(  int yy = y_min;  yy < y_max;  yy += LOOP_BLOCK  ) {
				for(  int x = xx;  x < min(xx + LOOP_BLOCK, x_max);  x++  ) {
					for(  int y=yy;  y < min(yy + LOOP_BLOCK, y_max);  y++  ) {
						// rotate climate transitions
						rotate_transitions( koord( x, y ) );
						const int nr = x + (y * cached_grid_size.x);
						const int new_nr = (cached_size.y - y) + (x * cached_grid_size.y);
						swap(rotate90_new_plan[new_nr], plan[nr]);
					}
				}
			}
		}
		// now rotate everything on the ground(s)
		for(  int xx = x_min;  xx < x_max;  xx += LOOP_BLOCK  ) {
			for(  int yy = y_min;  yy < y_max;  yy += LOOP_BLOCK  ) {
				for(  int x = xx;  x < min(xx + LOOP_BLOCK, x_max);  x++  ) {
					for(  int y = yy;  y < min(yy + LOOP_BLOCK, y_max);  y++  ) {
						const int new_nr = (cached_size.y - y) + (x * cached_grid_size.y);
						for(  uint i = 0;  i < rotate90_new_plan[new_nr].get_boden_count();  i++  ) {
							rotate90_new_plan[new_nr].get_boden_bei(i)->rotate90();
						}
					}
				}
			}
		}
	}

	// rotate water
	for(  int xx = 0;  xx < cached_grid_size.x;  xx += LOOP_BLOCK  ) {
		for(  int yy = y_min;  yy < y_max;  yy += LOOP_BLOCK  ) {
			for(  int x = xx;  x < min( xx + LOOP_BLOCK, cached_grid_size.x );  x++  ) {
				int nr = x + (yy * cached_grid_size.x);
				int new_nr = (cached_size.y - yy) + (x * cached_grid_size.y);
				for(  int y = yy;  y < min( yy + LOOP_BLOCK, y_max );  y++  ) {
					rotate90_new_water[new_nr] = water_hgts[nr];
					nr += cached_grid_size.x;
					new_nr--;
				}
			}
		}
	}
}


void karte_t::rotate90()
{
DBG_MESSAGE( "karte_t::rotate90()", "called" );
	// assume we can save this rotation
	nosave_warning = nosave = false;

	//announce current target rotation
	settings.rotate90();

	// clear marked region
	zeiger->change_pos( koord3d::invalid );

	// preprocessing, detach stops from factories to prevent crash
	FOR(vector_tpl<halthandle_t>, const s, haltestelle_t::get_alle_haltestellen()) {
		s->release_factory_links();
	}

	//rotate plans in parallel posix thread ...
	rotate90_new_plan = new planquadrat_t[cached_grid_size.y * cached_grid_size.x];
	rotate90_new_water = new sint8[cached_grid_size.y * cached_grid_size.x];

	world_xy_loop(&karte_t::rotate90_plans, 0);

	grund_t::finish_rotate90();

	delete [] plan;
	plan = rotate90_new_plan;
	delete [] water_hgts;
	water_hgts = rotate90_new_water;

	// rotate heightmap
	sint8 *new_hgts = new sint8[(cached_grid_size.x+1)*(cached_grid_size.y+1)];
	const int LOOP_BLOCK = 64;
	for(  int yy=0;  yy<=cached_grid_size.y;  yy+=LOOP_BLOCK  ) {
		for(  int xx=0;  xx<=cached_grid_size.x;  xx+=LOOP_BLOCK  ) {
			for(  int x=xx;  x<=min(xx+LOOP_BLOCK,cached_grid_size.x);  x++  ) {
				for(  int y=yy;  y<=min(yy+LOOP_BLOCK,cached_grid_size.y);  y++  ) {
					const int nr = x+(y*(cached_grid_size.x+1));
					const int new_nr = (cached_grid_size.y-y)+(x*(cached_grid_size.y+1));
					new_hgts[new_nr] = grid_hgts[nr];
				}
			}
		}
	}
	delete [] grid_hgts;
	grid_hgts = new_hgts;

	// rotate borders
	sint16 xw = cached_size.x;
	cached_size.x = cached_size.y;
	cached_size.y = xw;

	int wx = cached_grid_size.x;
	cached_grid_size.x = cached_grid_size.y;
	cached_grid_size.y = wx;

	// now step all towns (to generate passengers)
	FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
		i->rotate90(cached_size.x);
	}

	//fixed order factory, halts, convois
	FOR(vector_tpl<fabrik_t*>, const f, fab_list) {
		f->rotate90(cached_size.x);
	}
	// after rotation of factories, rotate everything that holds freight: stations and convoys
	FOR(vector_tpl<halthandle_t>, const s, haltestelle_t::get_alle_haltestellen()) {
		s->rotate90(cached_size.x);
	}
	for (uint32 i = 0; i < get_parallel_operations(); i++)
	{
		vector_tpl<transferring_cargo_t>& tcarray = transferring_cargoes[i];
		for (size_t j = tcarray.get_count(); j-- > 0;)
		{
			transferring_cargo_t& tc = tcarray[j];
			if (tc.ware.menge > 0)
			{
				tc.ware.rotate90(cached_size.x);
			}
			else
			{
				// empty => remove
				tcarray.remove_at(j);
			}
		}
	}
	// Factories need their halt lists recalculated after the halts are rotated.  Yuck!
	FOR(vector_tpl<fabrik_t*>, const f, fab_list) {
		f->recalc_nearby_halts();
	}

	FOR(vector_tpl<convoihandle_t>, const i, convoi_array) {
		i->rotate90(cached_size.x);
	}

	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  players[i]  ) {
			players[i]->rotate90( cached_size.x );
			selected_tool[i]->rotate90(cached_size.x);
		}
	}

	// rotate label texts
	FOR(slist_tpl<koord>, & l, labels) {
		l.rotate90(cached_size.x);
	}

	// rotate view
	viewport->rotate90( cached_size.x );

	// rotate messages
	msg->rotate90( cached_size.x );

	// rotate view in dialog windows
	win_rotate90( cached_size.x );

	if( cached_grid_size.x != cached_grid_size.y ) {
		// the map must be reinit
		reliefkarte_t::get_karte()->init();
	}

	//  rotate map search array
	factory_builder_t::new_world();

	// update minimap
	if(reliefkarte_t::is_visible) {
		reliefkarte_t::get_karte()->set_mode( reliefkarte_t::get_karte()->get_mode() );
	}

	get_scenario()->rotate90( cached_size.x );

	// finally recalculate schedules for goods in transit ...
	// Modified by : Knightly
	path_explorer_t::refresh_all_categories(true);

	set_dirty();
}
// -------- Verwaltung von Fabriken -----------------------------


bool karte_t::add_fab(fabrik_t *fab)
{
//DBG_MESSAGE("karte_t::add_fab()","fab = %p",fab);
	assert(fab != NULL);
	//fab_list.insert( fab );
	fab_list.append(fab);
	goods_in_game.clear(); // Force rebuild of goods list
	return true;
}


// beware: must remove also links from stops and towns
bool karte_t::rem_fab(fabrik_t *fab)
{
	if(!fab_list.is_contained(fab))
	{
		return false;
	}
	else
	{
		fab_list.remove(fab);
	}

	// Force rebuild of goods list
	goods_in_game.clear();

	// now all the interwoven connections must be cleared
	// This is hairy; a cleaner method would be desirable --neroden
	vector_tpl<koord> tile_list;
	fab->get_tile_list(tile_list);
	FOR (vector_tpl<koord>, const k, tile_list) {
		planquadrat_t* tile = access(k);
		if(tile)
		{
			// we need a copy, since the verbinde fabriken will modify the list
			const uint8 count = tile->get_haltlist_count();
			vector_tpl<nearby_halt_t> tmp_list;
			// Make it an appropriate size.
			tmp_list.resize(count);
			for(  uint8 i = 0;  i < count;  i++  ) {
				tmp_list.append( tile->get_haltlist()[i] );
			};
			for(  uint8 i = 0;  i < count;  i++  ) {
				// first remove all the tiles that do not connect
				// This will only remove if it is no longer connected
				tile->remove_from_haltlist( tmp_list[i].halt );
			}
		}
	}

	// OK, now stuff where we need not check every tile
	// Still double-check in case we were not on the map (which should not happen)
	koord pos = fab->get_pos().get_2d();
 	const planquadrat_t* tile = access(pos);
	if (tile) {

		// finally delete it
		delete fab;

		// recalculate factory position map
		factory_builder_t::new_world();
	}
	return true;
}

/*----------------------------------------------------------------------------------------------------------------------*/
/* same procedure for tourist attractions */


void karte_t::add_attraction(gebaeude_t *gb)
{
	assert(gb != NULL);
	world_attractions.append(gb, gb->get_adjusted_visitor_demand());
}


void karte_t::remove_attraction(gebaeude_t *gb)
{
	assert(gb != NULL);
	world_attractions.remove(gb);
	stadt_t* city = get_city(gb->get_pos().get_2d());
	if(!city)
	{
		remove_building_from_world_list(gb);
	}
}


// -------- Verwaltung von Staedten -----------------------------
// "look for next city" (Babelfish)

stadt_t *karte_t::find_nearest_city(const koord k) const
{
	uint32 min_dist = 99999999;
	bool contains = false;
	stadt_t *best = NULL;	// within city limits

	if(  is_within_limits(k)  ) {
		FOR(  weighted_vector_tpl<stadt_t*>,  const s,  stadt  ) {
			if(  k.x >= s->get_linksoben().x  &&  k.y >= s->get_linksoben().y  &&  k.x < s->get_rechtsunten().x  &&  k.y < s->get_rechtsunten().y  ) {
				const uint32 dist = koord_distance( k, s->get_center() );
				if(  !contains  ) {
					// no city within limits => this is best
					best = s;
					min_dist = dist;
				}
				else if(  dist < min_dist  ) {
					best = s;
					min_dist = dist;
				}
				contains = true;
			}
			else if(  !contains  ) {
				// so far no cities found within its city limit
				const uint32 dist = koord_distance( k, s->get_center() );
				if(  dist < min_dist  ) {
					best = s;
					min_dist = dist;
				}
			}
		}
	}
	return best;
}


stadt_t *karte_t::get_city(const koord pos) const
{
	stadt_t* city = NULL;

	if(is_within_limits(pos)) 
	{
		int cities = 0;
		FOR(weighted_vector_tpl<stadt_t*>, const c, stadt) 
		{
			if(c->is_within_city_limits(pos))
			{
				cities ++;
				if(cities > 1)
				{
					// We have a city within a city. Make sure to return the *inner* city.
					if(city->is_within_city_limits(c->get_pos()))
					{
						// "c" is the inner city: c's town hall is within the city limits of "city".
						city = c;
					}
				}
				else
				{
					city = c;
				}
			}
		}
	}
	return city;
}

// -------- Verwaltung von synchronen Objekten ------------------

void karte_t::sync_list_t::add(sync_steppable *obj)
{
	//assert(!sync_step_running);
	list.append(obj);
}

void karte_t::sync_list_t::remove(sync_steppable *obj)
{
	if(sync_step_running) {
		if (obj == currently_deleting) {
			return;
		}
		assert(false);
	}
	else {
		list.remove(obj);
	}
}

void karte_t::sync_list_t::clear()
{
	list.clear();
	currently_deleting = NULL;
	sync_step_running = false;
}

void karte_t::sync_list_t::sync_step(uint32 delta_t)
{
	sync_step_running = true;
	currently_deleting = NULL;

	for(uint32 i=0; i<list.get_count();i++) {
		sync_steppable *ss = list[i];
		switch(ss->sync_step(delta_t)) {
			case SYNC_OK:
				break;
			case SYNC_DELETE:
				currently_deleting = ss;
				delete ss;
				currently_deleting = NULL;
				/* fall-through */
			case SYNC_REMOVE:
				ss = list.pop_back();
				if (i < list.get_count()) {
					list[i] = ss;
				}
		}
	}
	sync_step_running = false;
}


/*
 * this routine is called before an image is displayed
 * it moves vehicles and pedestrians
 * only time consuming thing are done in step()
 * everything else is done here
 */
void karte_t::sync_step(uint32 delta_t, bool do_sync_step, bool display )
{
rands[0] = get_random_seed();
rands[2] = 0;
rands[3] = 0;
rands[4] = 0;
rands[5] = 0;
rands[6] = 0;
rands[7] = 0;
	set_random_mode( SYNC_STEP_RANDOM );
	haltestelle_t::pedestrian_limit = 0;
	if(do_sync_step) {
		// only omitted, when called to display a new frame during fast forward

		// just for progress
		if(  delta_t > 10000  ) {
			dbg->error( "karte_t::sync_step()", "delta_t too large: %li", delta_t );
			delta_t = 10000;
		}
		ticks += delta_t;

		set_random_mode( INTERACTIVE_RANDOM );

		/* animations do not require exact sync
		 * foundations etc are added removed frequently during city growth
		 * => they are now in a hastable!
		 */
		sync_eyecandy.sync_step( delta_t );

		/* pedestrians do not require exact sync and are added/removed frequently
		 * => they are now in a hastable!
		 */
		sync_way_eyecandy.sync_step( delta_t );

		clear_random_mode( INTERACTIVE_RANDOM );

		sync.sync_step( delta_t );
	}
rands[1] = get_random_seed();

	if(display) {
		// only omitted in fast forward mode for the magic steps

		for(int x=0; x<MAX_PLAYER_COUNT-1; x++) {
			if(players[x]) {
				players[x]->age_messages(delta_t);
			}
		}

		// change view due to following a convoi?
		convoihandle_t follow_convoi = viewport->get_follow_convoi();
		if(follow_convoi.is_bound()  &&  follow_convoi->get_vehicle_count()>0) {
			vehicle_t const& v       = *follow_convoi->front();
			koord3d   const  new_pos = v.get_pos();
			if(new_pos!=koord3d::invalid) {
				const sint16 rw = get_tile_raster_width();
				int new_xoff = 0;
				int new_yoff = 0;
				v.get_screen_offset( new_xoff, new_yoff, get_tile_raster_width() );
				new_xoff -= tile_raster_scale_x(-v.get_xoff(), rw);
				new_yoff -= tile_raster_scale_y(-v.get_yoff(), rw) + tile_raster_scale_y(new_pos.z * TILE_HEIGHT_STEP, rw);
				viewport->change_world_position( new_pos.get_2d(), -new_xoff, -new_yoff );
			}
		}

		// display new frame with water animation
		intr_refresh_display( false );
		update_frame_sleep_time();
	}
	clear_random_mode( SYNC_STEP_RANDOM );
}


// does all the magic about frame timing
void karte_t::update_frame_sleep_time()
{
	// get average frame time
	uint32 last_ms = dr_time();
	last_frame_ms[last_frame_idx] = last_ms;
	last_frame_idx = (last_frame_idx+1) % 32;
	sint32 ms_diff = (sint32)( last_ms - last_frame_ms[last_frame_idx] );
	if(ms_diff > 0) {
		realFPS = (32000u) / ms_diff;
	}
	else {
		realFPS = env_t::fps;
		simloops = 60;
	}

	if(  step_mode&PAUSE_FLAG  ) {
		// not changing pauses
		next_step_time = dr_time()+100;
		idle_time = 100;
	}
	else if(  step_mode==FIX_RATIO) {
		simloops = realFPS;
	}
	else if(step_mode==NORMAL) {
		// calculate simloops
		uint16 last_step = (steps+31)%32;
		if(last_step_nr[last_step]>last_step_nr[steps%32]) {
			simloops = (10000*32l)/(last_step_nr[last_step]-last_step_nr[steps%32]);
		}
		// (de-)activate faster redraw
		env_t::simple_drawing = (env_t::simple_drawing_normal >= get_tile_raster_width());

		// calculate and activate fast redraw ..
		if(  realFPS > (env_t::fps*17/16)  ) {
			// decrease fast tile zoom by one
			if(  env_t::simple_drawing_normal > env_t::simple_drawing_default  ) {
				env_t::simple_drawing_normal --;
			}
		}
		else if(  realFPS < env_t::fps/2  ) {
			// activate simple redraw
			env_t::simple_drawing_normal = max( env_t::simple_drawing_normal, get_tile_raster_width()+1 );
		}
		else if(  realFPS < (env_t::fps*15)/16  )  {
			// increase fast tile redraw by one if below current tile size
			if(  env_t::simple_drawing_normal <= (get_tile_raster_width()*3)/2  ) {
				env_t::simple_drawing_normal ++;
			}
		}
		else if(  idle_time > 0  ) {
			// decrease fast tile zoom by one
			if(  env_t::simple_drawing_normal > env_t::simple_drawing_default  ) {
				env_t::simple_drawing_normal --;
			}
		}
		env_t::simple_drawing = (env_t::simple_drawing_normal >= get_tile_raster_width());

		// way too slow => try to increase time ...
		if(  last_ms-last_interaction > 100  ) {
			if(  last_ms-last_interaction > 500  ) {
				set_frame_time( 1+get_frame_time() );
				// more than 1s since last zoom => check if zoom out is a way to improve it
				if(  last_ms-last_interaction > 5000  &&  get_current_tile_raster_width() < 32  ) {
					zoom_factor_up();
					set_dirty();
					last_interaction = last_ms-1000;
				}
			}
			else {
				increase_frame_time();
				increase_frame_time();
				increase_frame_time();
				increase_frame_time();
			}
		}
		else {
			// change frame spacing ... (pause will be changed by step() directly)
			if(realFPS>(env_t::fps*17/16)) {
				increase_frame_time();
			}
			else if(realFPS<env_t::fps) {
				if(  1000u/get_frame_time() < 2*realFPS  ) {
					if(  realFPS < (env_t::fps/2)  ) {
						set_frame_time( get_frame_time()-1 );
						next_step_time = last_ms;
					}
					else {
						reduce_frame_time();
					}
				}
				else {
					// do not set time too short!
					set_frame_time( 500/max(1,realFPS) );
					next_step_time = last_ms;
				}
			}
		}
	}
	else  { // here only with fyst forward ...
		// try to get 10 fps or lower rate (if set)
		uint32 frame_intervall = max( 100, 1000/env_t::fps );
		if(get_frame_time()>frame_intervall) {
			reduce_frame_time();
		}
		else {
			increase_frame_time();
		}
		// (de-)activate faster redraw
		env_t::simple_drawing = env_t::simple_drawing_fast_forward  ||  (env_t::simple_drawing_normal >= get_tile_raster_width());
	}
}


// add an amount to a subcategory
void karte_t::buche(sint64 const betrag, player_cost const type)
{
	assert(type < MAX_WORLD_COST);
	finance_history_year[0][type] += betrag;
	finance_history_month[0][type] += betrag;
	// to do: check for dependencies
}


inline sint32 get_population(stadt_t const* const c)
{
	return c->get_einwohner();
}


void karte_t::new_month()
{
	update_history();

	// advance history ...
	last_month_bev = finance_history_month[0][WORLD_CITICENS];
	for(  int hist=0;  hist<karte_t::MAX_WORLD_COST;  hist++  ) {
		for( int y=MAX_WORLD_HISTORY_MONTHS-1; y>0;  y--  ) {
			finance_history_month[y][hist] = finance_history_month[y-1][hist];
		}
	}

	finance_history_month[0][WORLD_CITYCARS] = 0;

	current_month ++;
	last_month ++;
	if( last_month > 11 ) {
		last_month = 0;
	}
	DBG_MESSAGE("karte_t::new_month()","Month (%d/%d) has started", (last_month%12)+1, last_month/12 );

	// this should be done before a map update, since the map may want an update of the way usage
//	DBG_MESSAGE("karte_t::new_month()","ways");
	FOR(vector_tpl<weg_t*>, const w, weg_t::get_alle_wege()) {
		w->new_month();
	}

//	DBG_MESSAGE("karte_t::new_month()","depots");
	// Bernd Gabriel - call new month for depots	
	FOR(slist_tpl<depot_t *>, const dep, depot_t::get_depot_list()) {
		dep->new_month();
	}

	// recalc old settings (and maybe update the stops with the current values)
	reliefkarte_t::get_karte()->new_month();

	INT_CHECK("simworld 3042");

//	DBG_MESSAGE("karte_t::new_month()","convois");
	// hsiegeln - call new month for convois
	FOR(vector_tpl<convoihandle_t>, const cnv, convoi_array) {
		cnv->new_month();
	}

	base_pathing_counter ++;

	INT_CHECK("simworld 3053"); 
	 

//	DBG_MESSAGE("karte_t::new_month()","factories");
	uint32 total_electric_demand = 1;
	uint32 electric_productivity = 0;
	closed_factories_this_month.clear();
	uint32 closed_factories_count = 0;
	FOR(vector_tpl<fabrik_t*>, const fab, fab_list)
	{
		if(!closed_factories_this_month.is_contained(fab))
		{
			fab->new_month();
			// Check to see whether the factory has closed down - if so, the pointer will be dud.
			if(closed_factories_count == closed_factories_this_month.get_count())
			{
				if(fab->get_desc()->is_electricity_producer())
				{
					electric_productivity += fab->get_scaled_electric_demand();
				} 
				else 
				{
					total_electric_demand += fab->get_scaled_electric_demand();
				}
			}
			else
			{
				closed_factories_count = closed_factories_this_month.get_count();
			}
		}
	}

	FOR(vector_tpl<fabrik_t*>, const fab, closed_factories_this_month)
	{
		if(fab_list.is_contained(fab)) 
		{
			gebaeude_t* gb = fab->get_building();
			hausbauer_t::remove(get_public_player(), gb, false);
		}
	}

	// Check to see whether more factories need to be added
	// to replace ones that have closed.
	// @author: jamespetts

	if(industry_density_proportion == 0 && finance_history_month[0][WORLD_CITICENS] > 0)
	{
		// Set the industry density proportion for the first time when the number of citizens is populated.
		industry_density_proportion = (uint32)((sint64)actual_industry_density * 1000000ll) / finance_history_month[0][WORLD_CITICENS];
	}
	const uint32 target_industry_density = get_target_industry_density();
	if(actual_industry_density < target_industry_density)
	{
		// Only add one chain per month, and randomise (with a minimum of 8% distribution_weight to ensure that any industry deficiency is, on average, remedied in about a year).
		const uint32 percentage = max((((target_industry_density - actual_industry_density) * 100u) / target_industry_density), 8u);
		const uint32 distribution_weight = simrand(100u, "void karte_t::new_month()");
		if(distribution_weight < percentage)
		{
			factory_builder_t::increase_industry_density(true, true);
		}
	}

	INT_CHECK("simworld 3105");

	// Check attractions' road connexions
	FOR(weighted_vector_tpl<gebaeude_t*>, const &i, world_attractions)
	{
		i->check_road_tiles(false);
	}


	//	DBG_MESSAGE("karte_t::new_month()","cities");
	stadt.update_weights(get_population);
	FOR(weighted_vector_tpl<stadt_t*>, const s, stadt) 
	{
		if(recheck_road_connexions) 
		{
			cities_awaiting_private_car_route_check.append_unique(s);
		}
		s->new_month(recheck_road_connexions);
		//INT_CHECK("simworld 3117");
		total_electric_demand += s->get_power_demand();
	}
	recheck_road_connexions = false;

	if(factory_builder_t::power_stations_available() && total_electric_demand && (((sint64)electric_productivity * 4000l) / total_electric_demand) < (sint64)get_settings().get_electric_promille())
	{
		// Add industries if there is a shortage of electricity - power stations will be built.
		// Also, check whether power stations are available, or else large quantities of other industries will
		// be built instead every month.
		factory_builder_t::increase_industry_density(true, true, true);
	}

	INT_CHECK("simworld 3130");

	// players
	for(uint i=0; i<MAX_PLAYER_COUNT; i++) {
		if( last_month == 0  &&  !settings.is_freeplay() ) {
			// remove all player (but first and second) who went bankrupt during last year
			if(  players[i] != NULL  &&  players[i]->get_finance()->is_bankrupted()  )
			{
				remove_player(i);
			}
		}

		if(  players[i] != NULL  ) {
			// if returns false -> remove player
			if (!players[i]->new_month()) {
				remove_player(i);
			}
		}
	}
	// update the window
	ki_kontroll_t* playerwin = (ki_kontroll_t*)win_get_magic(magic_ki_kontroll_t);
	if(  playerwin  ) {
		playerwin->update_data();
	}

	INT_CHECK("simworld 3175");

//	DBG_MESSAGE("karte_t::new_month()","halts");
	FOR(vector_tpl<halthandle_t>, const s, haltestelle_t::get_alle_haltestellen()) {
		s->new_month();
		INT_CHECK("simworld 1877");
	}

	INT_CHECK("simworld 2522");
	FOR(slist_tpl<depot_t *>, const& iter, depot_t::get_depot_list())
	{
		iter->new_month();
	}

	scenario->new_month();

	// now switch year to get the right year for all timeline stuff ...
	if( last_month == 0 ) {
		new_year();
		INT_CHECK("simworld 1299");
	}

	way_builder_t::new_month();
	INT_CHECK("simworld 1299");

	hausbauer_t::new_month();
	INT_CHECK("simworld 1299");

	// Check whether downstream substations have become engulfed by
	// an expanding city.
	FOR(slist_tpl<senke_t *>, & senke_iter, senke_t::senke_list)
	{
		// This will add a city if the city has engulfed the substation, and remove a city if
		// the city has been deleted or become smaller. 
		senke_t* const substation = senke_iter;
		stadt_t* const city = get_city(substation->get_pos().get_2d());
		substation->set_city(city);
		if(city)
		{
			city->add_substation(substation);
		}
		else
		{		
			// Check whether an industry has placed itself near the substation.
			substation->check_industry_connexion();
		}
	}

	recalc_average_speed();
	INT_CHECK("simworld 1921");

	// update toolbars (e.g. new waytypes)
	tool_t::update_toolbars();


	// no autosave in networkmode or when the new world dialogue is shown
	if( !env_t::networkmode && env_t::autosave>0 && last_month%env_t::autosave==0 && !win_get_magic(magic_welt_gui_t) ) {
		char buf[128];
		sprintf( buf, "save/autosave%02i.sve", last_month+1 );
		save( buf, loadsave_t::autosave_mode, env_t::savegame_version_str, env_t::savegame_ex_version_str, env_t::savegame_ex_revision_str, true );
	}

	recalc_passenger_destination_weights();

	set_citycar_speed_average();
	calc_generic_road_time_per_tile_city();
	calc_generic_road_time_per_tile_intercity();
	calc_max_road_check_depth();

	pedestrian_t::check_timeline_pedestrians();

#ifdef MULTI_THREAD
	stop_path_explorer();
#endif

	// Added by : Knightly
	// Note		: This should be done after all lines and convoys have rolled their statistics
	path_explorer_t::refresh_all_categories(true);
}


void karte_t::new_year()
{
	last_year = current_month/12;

	// advance history ...
	for(  int hist=0;  hist<karte_t::MAX_WORLD_COST;  hist++  ) {
		for( int y=MAX_WORLD_HISTORY_YEARS-1; y>0;  y--  ) {
			finance_history_year[y][hist] = finance_history_year[y-1][hist];
		}
	}

	cbuffer_t buf;
	buf.printf( translator::translate("Year %i has started."), last_year );
	msg->add_message(buf,koord::invalid,message_t::general,COL_BLACK,skinverwaltung_t::neujahrsymbol->get_image_id(0));

	FOR(vector_tpl<convoihandle_t>, const cnv, convoi_array) {
		cnv->new_year();
	}

	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(  players[i] != NULL  ) {
			players[i]->new_year();
		}
	}

	for(weighted_vector_tpl<gebaeude_t *>::const_iterator a = world_attractions.begin(), end = world_attractions.end(); a != end; ++a)
	{
		(*a)->new_year();
	}

	FOR(vector_tpl<fabrik_t*>, const fab, fab_list)
	{
		fab->get_building()->new_year();
	}

	finance_history_year[0][WORLD_CITYCARS] = 0;

	scenario->new_year();
}


// recalculated speed boni for different vehicles
// and takes care of all timeline stuff
void karte_t::recalc_average_speed()
{
	// retire/allocate vehicles
	private_car_t::build_timeline_list(this);

	//	DBG_MESSAGE("karte_t::recalc_average_speed()","");
	if(use_timeline())
	{
		for(int i=road_wt; i<=air_wt; i++)
		{
			const char *vehicle_type=NULL;
			switch(i) {
				case road_wt:
					vehicle_type = "road vehicle";
					break;
				case track_wt:
					vehicle_type = "rail car";
					break;
				case water_wt:
					vehicle_type = "water vehicle";
					break;
				case monorail_wt:
					vehicle_type = "monorail vehicle";
					break;
				case tram_wt:
					vehicle_type = "street car";
					break;
				case air_wt:
					vehicle_type = "airplane";
					break;
				case maglev_wt:
					vehicle_type = "maglev vehicle";
					break;
				case narrowgauge_wt:
					vehicle_type = "narrowgauge vehicle";
					break;
				default:
					// this is not a valid waytype
					continue;
			}
			vehicle_type = translator::translate( vehicle_type );

			FOR(slist_tpl<vehicle_desc_t *>, const info, vehicle_builder_t::get_info((waytype_t)i)) 
			{
				const uint16 intro_month = info->get_intro_year_month();
				if(intro_month == current_month) 
				{
					if(info->is_available_only_as_upgrade())
					{
						cbuffer_t buf;
						buf.printf(translator::translate("Upgrade to %s now available:\n%s\n"), vehicle_type, translator::translate(info->get_name()));
						msg->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,info->get_base_image());
					}
					else
					{
						cbuffer_t buf;
						buf.printf( translator::translate("New %s now available:\n%s\n"), vehicle_type, translator::translate(info->get_name()) );
						msg->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,info->get_base_image());
					}
				}

				const uint16 retire_month = info->get_retire_year_month();
				if(retire_month == current_month) 
				{
					cbuffer_t buf;
					buf.printf( translator::translate("Production of %s has been stopped:\n%s\n"), vehicle_type, translator::translate(info->get_name()) );
					msg->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,info->get_base_image());
				}

				const uint16 obsolete_month = info->get_obsolete_year_month(this);
				if(obsolete_month == current_month) 
				{
					cbuffer_t buf;
					buf.printf(translator::translate("The following %s has become obsolete:\n%s\n"), vehicle_type, translator::translate(info->get_name()));
					msg->add_message(buf,koord::invalid,message_t::new_vehicle,COL_DARK_BLUE,info->get_base_image());
				}
			}
		}

		// city road (try to use always a timeline)
		if (way_desc_t const* city_road_test = settings.get_city_road_type(current_month) ) {
			city_road = city_road_test;
		}
		else {
			DBG_MESSAGE("karte_t::new_month()","Month %d has started", last_month);
			city_road = way_builder_t::weg_search(road_wt, 50, get_timeline_year_month(), type_flat);
		}
	}
	else {
		// defaults
		city_road = way_builder_t::weg_search(road_wt, 50, get_timeline_year_month(), 5, type_flat, 25000000);
	}
}


// returns the current speed record
sint32 karte_t::get_record_speed( waytype_t w ) const
{
	return records->get_record_speed(w);
}


// sets the new speed record
void karte_t::notify_record( convoihandle_t cnv, sint32 max_speed, koord k )
{
	records->notify_record(cnv, max_speed, k, current_month);
}


void karte_t::set_schedule_counter()
{
#ifndef MULTI_THREAD
	// do not call this from gui when playing in network mode!
	//  The below gives spurious assertion failures when multi-threaded.
	assert( (get_random_mode() & INTERACTIVE_RANDOM) == 0  );
#endif

	schedule_counter++;
}

void karte_t::step()
{
	rands[8] = get_random_seed();
	DBG_DEBUG4("karte_t::step", "start step");
	uint32 time = dr_time();

	// calculate delta_t before handling overflow in ticks
	const sint32 delta_t = (sint32)(ticks-last_step_ticks);

	// first: check for new month
	if(ticks > next_month_ticks) {

		// avoid overflow here ...
		// Should not overflow: now using 64-bit values.
		// @author: jamespetts
/*
		if(  next_month_ticks > next_month_ticks+karte_t::ticks_per_world_month  ) {
			// avoid overflow here ...
			dbg->warning("karte_t::step()", "Ticks were overflowing => reset");
			ticks %= karte_t::ticks_per_world_month;
			next_month_ticks %= karte_t::ticks_per_world_month;
		}
*/
		next_month_ticks += karte_t::ticks_per_world_month;

		DBG_DEBUG4("karte_t::step", "calling new_month");
		new_month();
	}
	rands[9] = get_random_seed();

	DBG_DEBUG4("karte_t::step", "time calculations");
	if(  step_mode==NORMAL  ) {
		/* Try to maintain a decent pause, with a step every 170-250 ms (~5,5 simloops/s)
		 * Also avoid too large or negative steps
		 */

		// needs plausibility check?!?
		if(delta_t>10000  || delta_t<0) {
			dbg->error( "karte_t::step()", "delta_t (%li) out of bounds!", delta_t );
			last_step_ticks = ticks;
			next_step_time = time+10;
			return;
		}
		idle_time = 0;
		last_step_nr[steps%32] = ticks;
		next_step_time = time+(3200/get_time_multiplier());
	}
	else if(  step_mode==FAST_FORWARD  ) {
		// fast forward first: get average simloops (i.e. calculate acceleration)
		last_step_nr[steps%32] = dr_time();
		int last_5_simloops = simloops;
		if(  last_step_nr[(steps+32-5)%32] < last_step_nr[steps%32]  ) {
			// since 5 steps=1s
			last_5_simloops = (1000) / (last_step_nr[steps%32]-last_step_nr[(steps+32-5)%32]);
		}
		if(  last_step_nr[(steps+1)%32] < last_step_nr[steps%32]  ) {
			simloops = (10000*32) / (last_step_nr[steps%32]-last_step_nr[(steps+1)%32]);
		}
		// now try to approach the target speed
		if(last_5_simloops<env_t::max_acceleration) {
			if(idle_time>0) {
				idle_time --;
			}
		}
		else if(simloops>8u*env_t::max_acceleration) {
			if(idle_time + 10u < get_frame_time()) {
				idle_time ++;
			}
		}
		// cap it ...
		if( idle_time + 10u >= get_frame_time()) {
			idle_time = get_frame_time()-10;
		}
		next_step_time = time+idle_time;
	}
	else {
		// network mode
	}
	// now do the step ...
	last_step_ticks = ticks;
	steps ++;

	// to make sure the tick counter will be updated
	INT_CHECK("karte_t::step");

	/** THREADING CAN START HERE **/

	// Check the private car routes. In multi-threaded mode, this can be running in the background whilst a number of other steps are processed.
	// This is computationally intensive, but intermittently.
	const bool check_city_routes = cities_awaiting_private_car_route_check.get_count() > 0 && (steps % 12) == 0;
	if (check_city_routes)
	{
		const sint32 parallel_operations = get_parallel_operations();
		
#ifdef MULTI_THREAD
		// This cannot be started at the end of the step, as we will not know at that point whether we need to call this at all.
		cities_to_process = min(cities_awaiting_private_car_route_check.get_count() - 1, parallel_operations);
		simthread_barrier_wait(&private_car_barrier); // One wait barrier to activate all the private car checker threads, the second to wait until they have all finished. This is the first.
#else			
		const uint32 cities_to_process = min(cities_awaiting_private_car_route_check.get_count() - 1, parallel_operations);
		for (uint32 j = 0; j < cities_to_process; j++)
		{
			stadt_t* city = cities_awaiting_private_car_route_check.remove_first();
			city->check_all_private_car_routes();
			city->set_check_road_connexions(false);

		}
#endif	
	}

	rands[10] = get_random_seed();

#ifdef MULTI_THREAD_CONVOYS
	if (first_step == 1)
	{
		simthread_barrier_wait(&step_convoys_barrier_external); // Start the threaded part of the convoys' steps: this is mainly route searches. Block reservation, etc., is in the single threaded part. 
	}
#endif

	/// check for pending seasons change
	// This is not very computationally intensive.
	const bool season_change = pending_season_change > 0;
	const bool snowline_change = pending_snowline_change > 0;
	if(  season_change  ||  snowline_change  ) {
		DBG_DEBUG4("karte_t::step", "pending_season_change");
		// process
		const uint32 end_count = min( cached_grid_size.x * cached_grid_size.y,  tile_counter + max( 16384, cached_grid_size.x * cached_grid_size.y / 16 ) );
		while(  tile_counter < end_count  ) {
			plan[tile_counter].check_season_snowline( season_change, snowline_change );
			tile_counter++;
			if(  (tile_counter & 0x3FF) == 0  ) {
				INT_CHECK("karte_t::step 1");
			}
		}

		if(  tile_counter >= (uint32)cached_grid_size.x * (uint32)cached_grid_size.y  ) {
			if(  season_change ) {
				pending_season_change--;
			}
			if(  snowline_change  ) {
				pending_snowline_change--;
			}
			tile_counter = 0;
		}
	}

	rands[11] = get_random_seed();

	// to make sure the tick counter will be updated
	INT_CHECK("karte_t::step 1");

#ifdef MULTI_THREAD_PATH_EXPLORER
	// Stop the path explorer before we use its results.
	stop_path_explorer();
#else
	// Knightly : calling global path explorer
	path_explorer_t::step();
#endif
	rands[12] = get_random_seed();
	
	INT_CHECK("karte_t::step 2");

#ifdef MULTI_THREAD_CONVOYS
	simthread_barrier_wait(&step_convoys_barrier_external); 
	// Finish the threaded part of the convoys' steps: this is mainly route searches. Block reservation, etc., is in the single threaded part. 
	if (first_step == 1)
	{
		first_step = 2;
	}
	else if (first_step == 0)
	{
		// We need this to know whether the convoy thread loop is at the top or bottom barrier.
		first_step = 3;
	}
#else
	for (uint32 i = convoi_array.get_count(); i-- != 0;)
	{
		convoihandle_t cnv = convoi_array[i];
		cnv->threaded_step();
	}
#endif
	
	// The more computationally intensive parts of this have been extracted and made multi-threaded.
	DBG_DEBUG4("karte_t::step 4", "step %d convois", convoi_array.get_count());
	// since convois will be deleted during stepping, we need to step backwards
	for (uint32 i = convoi_array.get_count(); i-- != 0;) {
		convoihandle_t cnv = convoi_array[i];
		cnv->step();
		if((i&7)==0) {
			INT_CHECK("karte_t::step 3");
		}
	}

	rands[13] = get_random_seed();	

	// NOTE: Original position of the start of multi-threaded convoy stepping

	// now step all towns 
	// This is not very computationally intensive at present, but might become more so when town growth is reworked.
	DBG_DEBUG4("karte_t::step 6", "step cities");
	FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
		i->step(delta_t);
		rands[21] += i->get_einwohner();
		rands[22] += i->get_buildings();
	}
	rands[14] = get_random_seed();

#ifdef MULTI_THREAD
	// The placement of this barrier must be before any code that in any way relies on the private car routes between cities, most especially the mail and passenger generation (step_passengers_and_mail(delta_t)).
	if (check_city_routes)
	{
		simthread_barrier_wait(&private_car_barrier); // One wait barrier to activate all the private car checker threads, the second to wait until they have all finished. This is the second.
	}
#endif	

	rands[24] = 0;
	rands[25] = 0;
	rands[26] = 0;
	rands[27] = 0;
	rands[28] = 0;
	rands[29] = 0;
	rands[30] = 0;
	rands[31] = 0;
	rands[23] = 0;

	// This is quite computationally intensive, but not as much as the path explorer. It can be more or less than the convoys, depending on the map.
	// Multi-threading the passenger and mail generation is currently not working well as dividing the number of passengers/mail to be generated per
	//step by the number of parallel operations introduces significant rounding errors.
#ifdef MULTI_THREAD_PASSENGER_GENERATION

#ifdef FORBID_MULTI_THREAD_PASSENGER_GENERATION_IN_NETWORK_MODE
	if (!env_t::networkmode)
	{
#endif
		// Start the passenger and mail generation.
		sint32 dt = delta_t;
		if (dt > world->ticks_per_world_month)
		{
			dt = 1;
		}
		next_step_passenger += dt;
		next_step_mail += dt;

		if (!speed_factors_are_set)
		{
			set_speed_factors();
			speed_factors_are_set = true;
		}

		simthread_barrier_wait(&step_passengers_and_mail_barrier);
#ifdef FORBID_MULTI_THREAD_PASSENGER_GENERATION_IN_NETWORK_MODE
	}
	else
	{
		step_passengers_and_mail(delta_t);
	}
#endif
#else
	step_passengers_and_mail(delta_t);
#endif
	DBG_DEBUG4("karte_t::step", "step generate passengers and mail");

	rands[15] = get_random_seed();

	// the inhabitants stuff
	finance_history_year[0][WORLD_CITICENS] = finance_history_month[0][WORLD_CITICENS] = 0;
	finance_history_year[0][WORLD_JOBS] = finance_history_month[0][WORLD_JOBS] = 0;
	finance_history_year[0][WORLD_VISITOR_DEMAND] = finance_history_month[0][WORLD_VISITOR_DEMAND] = 0;

	FOR(weighted_vector_tpl<stadt_t*>, const city, stadt) 
	{
		finance_history_year[0][WORLD_CITICENS] += city->get_finance_history_month(0, HIST_CITICENS);
		finance_history_month[0][WORLD_CITICENS] += city->get_finance_history_year(0, HIST_CITICENS);

		finance_history_month[0][WORLD_JOBS] += city->get_finance_history_month(0, HIST_JOBS);
		finance_history_year[0][WORLD_JOBS] += city->get_finance_history_year(0, HIST_JOBS);

		finance_history_month[0][WORLD_VISITOR_DEMAND] += city->get_finance_history_month(0, HIST_VISITOR_DEMAND);
		finance_history_year[0][WORLD_VISITOR_DEMAND] += city->get_finance_history_year(0, HIST_VISITOR_DEMAND);
	}

	rands[23] = get_random_seed();

	INT_CHECK("karte_t::step 4");

#ifdef MULTI_THREAD_PASSENGER_GENERATION
#ifdef FORBID_MULTI_THREAD_PASSENGER_GENERATION_IN_NETWORK_MODE
	if (!env_t::networkmode)
	{
#endif
		// Stop the passenger and mail generation.
		simthread_barrier_wait(&step_passengers_and_mail_barrier);
		// The extra barrier is needed to synchronise the generation figures.
		simthread_barrier_wait(&step_passengers_and_mail_barrier);
#ifdef FORBID_MULTI_THREAD_PASSENGER_GENERATION_IN_NETWORK_MODE
	}
#endif
#endif

#ifdef MULTI_THREAD
	// This is necessary in network mode to ensure that all cars set in motion
	// by passenger generation are added to the world list in the same order
	// even when the creation of those objects was multi-threaded. 
#ifndef FORBID_SYNC_OBJECTS
	for (uint32 i = 0; i < get_parallel_operations() + 2; i++)
	{
		FOR(vector_tpl<private_car_t*>, car, private_cars_added_threaded[i])
		{
			const koord3d pos_obj = car->get_pos();
			grund_t* const gr = lookup(pos_obj);
			if (gr)
			{
				gr->obj_add(car); 
			}
			else
			{
				car->set_flag(obj_t::not_on_map);
				car->set_time_to_life(0);
			}
			sync.add(car);
		}
		private_cars_added_threaded[i].clear();

		FOR(vector_tpl<pedestrian_t*>, ped, pedestrians_added_threaded[i])
		{
			const koord3d pos_obj = ped->get_pos();
			grund_t* const gr = lookup(pos_obj);
			bool ok = false;
			if (gr)
			{
				ok = gr->obj_add(ped);
			}
			else
			{
				ok = false;
			}
			if (ok)
			{
				sync.add(ped);

				if (i > 0)
				{
					// walk a little
					ped->sync_step((i & 3) * 64 * 24);
				}
			}
			else
			{
				ped->set_flag(obj_t::not_on_map);
				// do not try to delete it from sync-list
				ped->set_time_to_life(0);
				delete ped;
			}	
		}
		pedestrians_added_threaded[i].clear();
	}
#endif
#endif
	INT_CHECK("karte_t::step 5");

	DBG_DEBUG4("karte_t::step", "step factories");
	FOR(vector_tpl<fabrik_t*>, const f, fab_list) {
		f->step(delta_t);
	}
	rands[16] = get_random_seed();

	finance_history_year[0][WORLD_FACTORIES] = finance_history_month[0][WORLD_FACTORIES] = fab_list.get_count();

	// step powerlines - required order: pumpe, senke, then powernet
	// This is not computationally intensive.
	DBG_DEBUG4("karte_t::step", "step poweline stuff");
	pumpe_t::step_all( delta_t );
	senke_t::step_all( delta_t );
	powernet_t::step_all( delta_t );
	rands[17] = get_random_seed();

	INT_CHECK("karte_t::step 6");

	DBG_DEBUG4("karte_t::step", "step players");
	// then step all players
	// This is not computationally intensive (except possibly occasionally when liquidating a company)
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  players[i] != NULL  ) {
			players[i]->step();
		}
	}
	rands[18] = get_random_seed();

	INT_CHECK("karte_t::step 7");

	// This is not computationally intensive
	DBG_DEBUG4("karte_t::step", "step halts");
	haltestelle_t::step_all();
	rands[19] = get_random_seed();

	// Re-check paths if the time has come. 
	// Long months means that it might be necessary to do
	// this more than once per month to get up to date
	// routings for goods/passengers.
	// Default: 8192 ~ 1h (game time) at 125m/tile.

	// This is not the computationally intensive bit of the path explorer.
	if((steps % get_settings().get_reroute_check_interval_steps()) == 0)
	{
		path_explorer_t::refresh_all_categories(true);
	}
	
#ifdef MULTI_THREAD_CONVOYS
	// This must start after the multi-threaded path explorer starts

	// Start the convoys' route finding again immediately after the convoys have been stepped: this maximises efficiency and concurrency.
	// Since it is purely route finding in the multi-threaded convoy step, it is safe to have this concurrent with everything but the single-
	// threaded convoy step.
	if (first_step == 3 || first_step == 2)
	{
		simthread_barrier_wait(&step_convoys_barrier_external); // Start the threaded part of the convoys' steps: this is mainly route searches. Block reservation, etc., is in the single threaded part. This will end in the next step
	}

	if (first_step == 2 || first_step == 3)
	{
		// Convoys cannot be stepped concurrently with sync_step running in network mode because whether they need routing may depend on a command executed in sync_step,
		// and it will be entirely by distribution_weight whether that command is executed before or after the threads get to processing that particular convoy.
		first_step = 0;
	}
#endif

	INT_CHECK("karte_t::step 8");

	check_transferring_cargoes();

#ifdef MULTI_THREAD_PATH_EXPLORER
	// Start the path explorer ready for the next step. This can be very 
	// computationally intensive, but intermittently so.
	start_path_explorer();
#endif

	// ok, next step
	INT_CHECK("karte_t::step 9");

	if((steps%8)==0) {
		DBG_DEBUG4("karte_t::step", "checkmidi");
		check_midi();
	}

	recalc_season_snowline(true);

	// This is not particularly computationally intensive.
	step_time_interval_signals();

	/** END OF THREADABLE AREA **/

	// number of playing clients changed
	if(  env_t::server  &&  last_clients!=socket_list_t::get_playing_clients()  ) {
		if(  env_t::server_announce  ) {
			// inform the master server
			announce_server( 1 );
		}

		// check if player has left and send message
		for(uint32 i=0; i < socket_list_t::get_count(); i++) {
			socket_info_t& info = socket_list_t::get_client(i);
			if (info.state == socket_info_t::has_left) {
				nwc_nick_t::server_tools(this, i, nwc_nick_t::FAREWELL, NULL);
				info.state = socket_info_t::inactive;
			}
		}
		last_clients = socket_list_t::get_playing_clients();
		// add message via tool
		cbuffer_t buf;
		buf.printf("%d,", message_t::general | message_t::local_flag);
		buf.printf(translator::translate("Now %u clients connected.", settings.get_name_language_id()), last_clients);
		tool_t *tool = create_tool( TOOL_ADD_MESSAGE | SIMPLE_TOOL );
		tool->set_default_param( buf );
		set_tool( tool, NULL );
		// since init always returns false, it is safe to delete immediately
		delete tool;
#ifdef DEBUG_SIMRAND_CALLS
		if(/*last_clients == 0*/ true)
		{
			ITERATE(karte_t::random_callers, n)
			{
				get_message()->add_message(random_callers.get_element(n), koord::invalid, message_t::ai);
			}
			print_randoms = false;
		}
		else
		{
			print_randoms = true;
		}
		printf("Number of connected clients changed to %u", last_clients);
#endif
	}

	if(  get_scenario()->is_scripted() ) {
		get_scenario()->step();
	}

	DBG_DEBUG4("karte_t::step", "end");
	rands[20] = get_random_seed();
}

void karte_t::step_time_interval_signals()
{
	if (!time_interval_signals_to_check.empty())
	{
		const sint64 caution_interval_ticks = get_seconds_to_ticks(settings.get_time_interval_seconds_to_caution());
		const sint64 clear_interval_ticks = get_seconds_to_ticks(settings.get_time_interval_seconds_to_clear());

		for (vector_tpl<signal_t*>::iterator iter = time_interval_signals_to_check.begin(); iter != time_interval_signals_to_check.end();)
		{
			signal_t* sig = *iter;
			if (((sig->get_train_last_passed() + clear_interval_ticks) < ticks) && sig->get_no_junctions_to_next_signal())
			{
				iter = time_interval_signals_to_check.swap_erase(iter);
				sig->set_state(roadsign_t::clear_no_choose);
			}
			else if (sig->get_state() == roadsign_t::danger && ((sig->get_train_last_passed() + caution_interval_ticks) < ticks) && sig->get_no_junctions_to_next_signal())
			{
				if (sig->get_desc()->is_pre_signal())
				{
					sig->set_state(roadsign_t::clear_no_choose);
				}
				else
				{
					sig->set_state(roadsign_t::caution_no_choose);
				}

				++iter;
			}
			else
			{
				++iter;
			}
		}
	}
}

sint32 karte_t::calc_adjusted_step_interval(const uint32 weight, uint32 trips_per_month_hundredths) const
{
	const uint32 median_packet_size = (uint32)(get_settings().get_passenger_routing_packet_size() + 1) / 2;	
	const uint64 trips_per_month = std::max((((sint64)weight * (sint64)calc_adjusted_monthly_figure(trips_per_month_hundredths)) / 100ll) / (sint64)median_packet_size, 1ll);
		
	return (sint32)((uint64)ticks_per_world_month > trips_per_month ? (uint64) ticks_per_world_month / trips_per_month : 1);
}

void karte_t::step_passengers_and_mail(uint32 delta_t)
{
	if(delta_t > ticks_per_world_month) 
	{
		delta_t = 1;
	}

	next_step_passenger += delta_t;
	next_step_mail += delta_t;

	// The generate passengers function is called many times (often well > 100) each step; the mail version is called only once or twice each step, sometimes not at all.
	sint32 units_this_step;
	while(passenger_step_interval <= next_step_passenger) 
	{
		if(passenger_origins.get_count() == 0)
		{
			return;
		}
		units_this_step = generate_passengers_or_mail(goods_manager_t::passengers);
		next_step_passenger -= (passenger_step_interval * units_this_step);

	} 

	while(mail_step_interval <= next_step_mail) 
	{
		if(mail_origins_and_targets.get_count() == 0)
		{
			return;
		}
		units_this_step = generate_passengers_or_mail(goods_manager_t::mail);
		next_step_mail -= (mail_step_interval * units_this_step);
	} 
}

void karte_t::get_nearby_halts_of_tiles(const minivec_tpl<const planquadrat_t*> &tile_list, const goods_desc_t * wtyp, vector_tpl<nearby_halt_t> &halts) const
{
	// Suitable start search (public transport)
	FOR(minivec_tpl<const planquadrat_t*>, const& current_tile, tile_list)
	{
		const nearby_halt_t* halt_list = current_tile->get_haltlist();
		for(int h = current_tile->get_haltlist_count() - 1; h >= 0; h--) 
		{
			nearby_halt_t halt = halt_list[h];
			if (halt.halt->is_enabled(wtyp)) 
			{
				// Previous versions excluded overcrowded halts here, but we need to know which
				// overcrowded halt would have been the best start halt if it was not overcrowded,
				// so do that below.
				halts.append(halt);
			}
		}
	}
}

void karte_t::add_to_waiting_list(ware_t ware, koord origin_pos)
{
#ifdef DISABLE_GLOBAL_WAITING_LIST
	return;
#endif
	sint64 ready_time = calc_ready_time(ware, origin_pos);
	
	transferring_cargo_t tc;
	tc.ware = ware;
	tc.ready_time = ready_time;
#ifdef MULTI_THREAD
	transferring_cargoes[karte_t::passenger_generation_thread_number].append(tc);
#else
	transferring_cargoes[0].append(tc);
#endif
}

sint64 karte_t::calc_ready_time(ware_t ware, koord origin_pos) const
{
	sint64 ready_time = get_ticks();

	uint16 distance = shortest_distance(ware.get_zielpos(), origin_pos);

	if (ware.is_freight())
	{
		const uint32 tenths_of_minutes = walk_haulage_time_tenths_from_distance(distance);
		const sint64 carting_time = get_seconds_to_ticks(tenths_of_minutes * 6);
		ready_time += carting_time;
	}
	else
	{
		const uint32 seconds = walking_time_secs_from_distance(distance);
		const sint64 walking_time = get_seconds_to_ticks(seconds);
		ready_time += walking_time;
	}

	return ready_time;
}

void karte_t::check_transferring_cargoes()
{
	const sint64 current_time = ticks;
	ware_t ware;
#ifdef MULTI_THREAD
	sint32 po = get_parallel_operations();
#else
	sint32 po = 1;
#endif
	bool removed;
	for (sint32 i = 0; i < po; i++)
	{
		FOR(vector_tpl<transferring_cargo_t>, tc, transferring_cargoes[i])
		{
			/*const uint32 ready_seconds = ticks_to_seconds((tc.ready_time - current_time));
			const uint32 ready_minutes = ready_seconds / 60;
			const uint32 ready_hours = ready_minutes / 60;*/
			if (tc.ready_time <= current_time)
			{
				ware = tc.ware;
				removed = transferring_cargoes[i].remove(tc);
				if (removed)
				{
					deposit_ware_at_destination(ware);
				}
			}
		}
	}
}

void karte_t::deposit_ware_at_destination(ware_t ware)
{
	const grund_t* gr = lookup_kartenboden(ware.get_zielpos());
	gebaeude_t* gb_dest = gr->get_building();
	fabrik_t* const fab = gb_dest ? gb_dest->get_fabrik() : NULL;
	if (!gb_dest) 
	{
		gb_dest = gr->get_depot();
	}

	if (fab)
	{
		// Packet is headed to a factory;
		// the factory exists;
		if (fab_list.is_contained(fab) || !ware.is_freight())
		{
			if (!ware.is_passenger() || ware.is_commuting_trip)
			{
				// Only book arriving passengers for commuting trips.
				fab->liefere_an(ware.get_desc(), ware.menge);
			}
			else if(fab->is_end_consumer())
			{
				// Add visiting passengers as consumers
				fab->add_consuming_passengers(ware.menge);
			}
			gb_dest =lookup(fab->get_pos())->find<gebaeude_t>();
		}
		else
		{
			gb_dest = NULL;
		}
	}

	if (gb_dest)
	{
		if (ware.is_passenger())
		{
			if (ware.is_commuting_trip)
			{
				if (gb_dest && gb_dest->get_tile()->get_desc()->get_type() != building_desc_t::city_res)
				{
					// Do not record the passengers coming back home again.
					gb_dest->set_commute_trip(ware.menge);
				}
			}
			else if (gb_dest && gb_dest->get_tile()->get_desc()->get_type() != building_desc_t::city_res)
			{
				gb_dest->add_passengers_succeeded_visiting(ware.menge);
			}

			// Arriving passengers may create pedestrians
			// at the arriving halt or the building 
			// that they left (not their ultimate destination). 
			if (get_settings().get_show_pax())
			{
				const uint32 menge = ware.menge;
				koord3d pos_pedestrians;
				if (ware.get_zwischenziel().is_bound())
				{
					pos_pedestrians = ware.get_zwischenziel()->get_basis_pos3d();
				}
				else
				{
					pos_pedestrians = ware.get_origin().is_bound() ? ware.get_origin()->get_basis_pos3d() : koord3d::invalid;
				}
				if (pos_pedestrians != koord3d::invalid)
				{
					pedestrian_t::generate_pedestrians_at(pos_pedestrians, menge);
				}
			}
		}
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
		if (talk)
			dbg->message("haltestelle_t::liefere_an", "%d arrived at station \"%s\" goods[0].count %d", ware.menge, get_name(), get_warray(0)->get_count());
#endif
#endif
	}
}

sint32 karte_t::generate_passengers_or_mail(const goods_desc_t * wtyp)
{
	const city_cost history_type = (wtyp == goods_manager_t::passengers) ? HIST_PAS_TRANSPORTED : HIST_MAIL_TRANSPORTED;
	const uint32 units_this_step = simrand((uint32)settings.get_passenger_routing_packet_size(), "void karte_t::generate_passengers_and_mail(uint32 delta_t) passenger/mail packet size") + 1;
	// Pick the building from which to generate passengers/mail
	gebaeude_t* gb;
	if(wtyp == goods_manager_t::passengers)
	{
		// Pick a passenger building at random
		const uint32 weight = simrand(passenger_origins.get_sum_weight() - 1, "void karte_t::generate_passengers_and_mail(uint32 delta_t) pick origin building (passengers)");
		gb = passenger_origins.at_weight(weight);
	}
	else
	{
		// Pick a mail building at random
		const uint32 weight = simrand(mail_origins_and_targets.get_sum_weight() - 1, "void karte_t::generate_passengers_and_mail(uint32 delta_t) pick origin building (mail)");
		gb = mail_origins_and_targets.at_weight(weight);
	}

	stadt_t* city = gb->get_stadt();

	// We need this for recording statistics for onward journeys in the very original departure point.
	gebaeude_t* const first_origin = gb;

	if(city)
	{
		// Mail is generated in non-city buildings such as attractions.
		// That will be the only legitimate case in which this condition is not fulfilled.
#ifdef MULTI_THREAD
		int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
		city->set_generated_passengers(units_this_step, history_type + 1);
#ifdef MULTI_THREAD
		mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
	}
	
	koord3d origin_pos = gb->get_pos();
	minivec_tpl<const planquadrat_t*> const &tile_list = first_origin->get_tiles();

	// Suitable start search (public transport)
#ifdef MULTI_THREAD
	start_halts[passenger_generation_thread_number].clear();
#else
	start_halts.clear();
#endif

	//vector_tpl<nearby_halt_t> start_halts(tile_list.empty() ? 0 : tile_list[0]->get_haltlist_count() * tile_list.get_count());
#ifdef MULTI_THREAD
	get_nearby_halts_of_tiles(tile_list, wtyp, start_halts[passenger_generation_thread_number]);
#else
	get_nearby_halts_of_tiles(tile_list, wtyp, start_halts);
#endif

	// Initialise the class out of the loop, as the passengers remain the same class no matter what their trip.
	const uint8 g_class = first_origin->get_random_class(wtyp);

	// Check whether this batch of passengers has access to a private car each.	
	const sint16 private_car_percent = wtyp == goods_manager_t::passengers ? get_private_car_ownership(get_timeline_year_month(), g_class) : 0;
	// Only passengers have private cars
	// QUERY: Should people be taken to be able to deliver mail packets in their own cars?
	bool has_private_car = private_car_percent > 0 ? simrand(100, "karte_t::generate_passengers_and_mail() (has private car?)") <= (uint16)private_car_percent : false;
	
	// Record the most useful set of information about why passengers cannot reach their chosen destination:
	// Too slow > overcrowded > no route. Tiebreaker: higher destination preference.
	koord best_bad_destination;
	uint8 best_bad_start_halt;
	bool too_slow_already_set;
	bool overcrowded_already_set;

	const uint32 min_commuting_tolerance = settings.get_min_commuting_tolerance();
	const uint32 range_commuting_tolerance = max(0, settings.get_range_commuting_tolerance() - min_commuting_tolerance);

	const uint32 min_visiting_tolerance = settings.get_min_visiting_tolerance();
	const uint32 range_visiting_tolerance = max(0, settings.get_range_visiting_tolerance() - min_visiting_tolerance);

	const uint16 max_onward_trips = settings.get_max_onward_trips();
	trip_type trip = (wtyp == goods_manager_t::passengers) ?
			simrand(100, "karte_t::generate_passengers_and_mail() (commuting or visiting trip?)") < settings.get_commuting_trip_chance_percent() ?
		commuting_trip : visiting_trip : mail_trip;

	// Add 1 because the simuconf.tab setting is for maximum *alternative* destinations, whereas we need maximum *actual* desintations 
	// Mail does not have alternative destinations: people do not send mail to one place because they cannot reach another. Mail has specific desinations.
	const uint32 min_destinations = trip == commuting_trip ? settings.get_min_alternative_destinations_commuting() + 1: trip == visiting_trip ? settings.get_min_alternative_destinations_visiting() + 1 : 1;
	const uint32 max_destinations = trip == commuting_trip ? settings.get_max_alternative_destinations_commuting() : trip == visiting_trip ? settings.get_max_alternative_destinations_visiting() : 1;
	koord destination_pos;
	route_status_type route_status;
	destination current_destination;
	destination first_destination;
	first_destination.location = koord::invalid;
	uint32 time_per_tile;
	uint32 tolerance;

	halthandle_t start_halt;
	halthandle_t current_halt;
	halthandle_t ret_halt;
	//halthandle_t halt;

	// Find passenger destination

	// Mail does not make onward journeys.
	const uint16 onward_trips = simrand(100, "void stadt_t::generate_passengers_and_mail() (any onward trips?)") < settings.get_onward_trip_chance_percent() &&	wtyp == goods_manager_t::passengers ? simrand(max_onward_trips, "void stadt_t::step_passengers() (how many onward trips?)") + 1 : 1;

	route_status = initialising;

	for(uint32 trip_count = 0; trip_count < onward_trips && route_status != no_route && route_status != too_slow && route_status != overcrowded && route_status != destination_unavailable; trip_count ++)
	{
		// Permit onward journeys - but only for successful journeys
		const uint32 destination_count = simrand(max_destinations, "void stadt_t::generate_passengers_and_mail() (number of destinations?)") + min_destinations;
		uint32 extend_count = 0;
		// Split passengers between commuting trips and other trips.
		if(trip_count == 0)
		{
			// Set here because we deduct the previous journey time from the tolerance for onward trips.
			if(trip == mail_trip)
			{
				tolerance = UINT32_MAX_VALUE;
			}
			else if(trip == commuting_trip)
			{
				tolerance = simrand_normal(range_commuting_tolerance, settings.get_random_mode_commuting(), "karte_t::generate_passengers_and_mail (commuting tolerance?)") + (min_commuting_tolerance * onward_trips);
			}
			else
			{
				tolerance = simrand_normal(range_visiting_tolerance, settings.get_random_mode_visiting(), "karte_t::generate_passengers_and_mail (visiting tolerance?)") + (min_visiting_tolerance * onward_trips);
			}
		}
		else
		{
			// The trip is already set. Only re-set this for a commuting trip, as people making onward journeys
			// from a commuting trip will not be doing so as another commuting trip. 
			if(trip == commuting_trip)
			{
				trip = visiting_trip;
			}

			// Onward journey - set the initial point to the previous end point.
			const grund_t* gr = lookup_kartenboden(destination_pos);
			if(!gr)
			{
				continue;
			}
			gb = gr->get_building();
					
			if(!gb)
			{
				// This sometimes happens for unknown reasons. 
				continue;
			}
			city = get_city(destination_pos);

			// Added here as the original journey had its generated passengers set much earlier, outside the for loop.
			if(city)
			{
#ifdef MULTI_THREAD
				int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
				assert(mutex_error == 0);
#endif
				city->set_generated_passengers(units_this_step, history_type + 1);
#ifdef MULTI_THREAD
				mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
				assert(mutex_error == 0);
#endif
			}

			if(route_status != private_car)
			{
				// If passengers did not use a private car for the first leg, they cannot use one for subsequent legs.
				has_private_car = false;
			}

			// Regenerate the start halts information for this new onward trip.
			// We cannot reuse "destination_list" as this is a list of halthandles,
			// not nearby_halt_t objects.
			// TODO BG, 15.02.2014: first build a nearby_destination_list and then a destination_list from it.
			//  Should be faster than finding all nearby halts again.

			minivec_tpl<const planquadrat_t*> const &tile_list_2 = first_origin->get_tiles();

			// Suitable start search (public transport)
#ifdef MULTI_THREAD
			start_halts[passenger_generation_thread_number].clear();
			get_nearby_halts_of_tiles(tile_list_2, wtyp, start_halts[passenger_generation_thread_number]);
#else
			start_halts.clear();
			get_nearby_halts_of_tiles(tile_list_2, wtyp, start_halts);
#endif
		}

		ware_t pax(wtyp);
		pax.is_commuting_trip = trip == commuting_trip;
		start_halt.set_id(0);
		uint32 best_journey_time;
		uint32 walking_time;
		route_status = initialising;
		pax.g_class = g_class;
		if (wtyp == goods_manager_t::passengers)
		{
			pax.comfort_preference_percentage = simrand(settings.get_max_comfort_preference_percentage() - 100, "karte_t::generate_passengers_and_mail (comfort_preference_percentage)") + 100;
		}

		first_destination = find_destination(trip, pax.get_class());
		current_destination = first_destination;

		if(trip == commuting_trip)
		{
#ifdef MULTI_THREAD
			int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
			assert(mutex_error == 0);
#endif
			first_origin->add_passengers_generated_commuting(units_this_step);
#ifdef MULTI_THREAD
			mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
			assert(mutex_error == 0);
#endif
		}
			
		else if(trip == visiting_trip)
		{
#ifdef MULTI_THREAD
			int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
			assert(mutex_error == 0);
#endif
			first_origin->add_passengers_generated_visiting(units_this_step);
#ifdef MULTI_THREAD
			mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
			assert(mutex_error == 0);
#endif
		}

		// Do nothing if trip == mail_trip

		/**
		* Walking tolerance is necessary because mail can be delivered by hand. If it is delivered
		* by hand, the deliverer has a tolerance, but if it is sent through the postal system,
		* the mail packet itself does not have a tolerance.
		*
		* In addition, walking tolerance for very long distance journeys (with a journey time greater than
		* the maximum journey time for commuting passengers, as defined by threshold_tolerance, is divided
		* by two because passengers prefer not to walk for extremely long distances, as it is tiring,
		* especially with luggage. Formerly, this was applied to all walking journeys and termed,
		* "quasi_tolerance", but this did not balance well, especially for commuting trips in early years,
		* when workers did in reality walk long distances to work. 
		*/
		uint32 walking_tolerance = tolerance;
		const uint32 threshold_tolerance = range_commuting_tolerance + min_commuting_tolerance;

		// Above this journey time, passengers will prefer not to walk even if walking is the quickest way
		// of getting to their destination, provided that another means of transport is within their journey
		// time tolerance. This simulates laziness. 
		uint32 walking_time_preference_threshold;

		if(wtyp == goods_manager_t::mail)
		{
			// People will walk long distances with mail: it is not heavy.
			walking_tolerance = simrand_normal(range_visiting_tolerance, settings.get_random_mode_visiting(), "karte_t::generate_passengers_and_mail (walking tolerance)") + min_visiting_tolerance;
			walking_time_preference_threshold = walking_tolerance;
		}
		else
		{
			// Passengers
			if (tolerance > threshold_tolerance)
			{
				walking_tolerance = max(tolerance / 2, min(tolerance, threshold_tolerance));
			}
			walking_time_preference_threshold = simrand(walking_tolerance > min_commuting_tolerance ? walking_tolerance - min_commuting_tolerance : min_commuting_tolerance, "karte_t::generate_passengers_and_mail (walking walking_time_preference_threshold)") + min_commuting_tolerance;
		}

		uint32 car_minutes;

		best_bad_destination = first_destination.location;
		best_bad_start_halt = 0;
		too_slow_already_set = false;
		overcrowded_already_set = false;

		for (int n = 0; n < destination_count + extend_count && route_status != public_transport && route_status != private_car && route_status != on_foot; n++)
		{
			destination_pos = current_destination.location;
			if (trip == commuting_trip)
			{
				gebaeude_t* dest_building = current_destination.building;
#ifdef DISABLE_JOB_EFFECTS
				if (!dest_building)
#else
				if (!dest_building || !dest_building->jobs_available() || (dest_building->get_is_factory() && dest_building->get_fabrik()->is_input_empty()))
#endif
				{
					if (route_status == initialising)
					{
						// This is the lowest priority route status.
						route_status = destination_unavailable;
					}

					/**
					* As there are no jobs, this is not a destination for commuting
					*/
					if (n < destination_count - 1)
					{
						current_destination = find_destination(trip, pax.get_class());

						if (extend_count < destination_count * 4)
						{
							extend_count++;
						}
					}
					continue;
				}
			}
			else if (trip == visiting_trip)
			{
				gebaeude_t* dest_building = current_destination.building;
				if (!dest_building)
				{
					if (route_status == initialising)
					{
						// This is the lowest priority route status.
						route_status = destination_unavailable;
					}
				}
				else if (dest_building->get_is_factory() && dest_building->get_fabrik()->is_end_consumer())
				{
					// If the visiting passengers are bound for a shop that has run out of goods to sell, 
					// do not allow the passengers to go here.
					fabrik_t* fab = dest_building->get_fabrik();
					if (!fab || fab->out_of_stock_selective())
					{
						if (route_status == initialising)
						{
							// This is the lowest priority route status.
							route_status = destination_unavailable;
						}

						if (n < destination_count + extend_count - 1)
						{
							current_destination = find_destination(trip, pax.get_class());
						}
						continue;
					}
				}
			}

			if (route_status == initialising)
			{
				route_status = no_route;
			}

			const uint32 straight_line_distance = shortest_distance(origin_pos.get_2d(), destination_pos);
			// This number may be very long.
			walking_time = walking_time_tenths_from_distance(straight_line_distance);
			car_minutes = UINT32_MAX_VALUE;

			const bool can_walk = walking_time <= walking_tolerance;
#ifdef MULTI_THREAD 
			if (!has_private_car && !can_walk && start_halts[passenger_generation_thread_number].empty())
#else
			if (!has_private_car && !can_walk && start_halts.empty())
#endif
			{
				/**
				* If the passengers have no private car, are not in reach of any public transport
				* facilities and the journey is too long on foot, do not continue to check other things.
				*/
				if (n < destination_count + extend_count - 1)
				{
					current_destination = find_destination(trip, pax.get_class());
				}
				continue;
			}

			// Check for a suitable stop within walking distance of the destination.

			// Note that, although factories are only *connected* now if they are within the smaller factory radius
			// (default: 1), they can take passengers within the wider square of the passenger radius. This is intended,
			// and is as a result of using the below method for all destination types.

			//minivec_tpl<const planquadrat_t*> const &tile_list_3 = current_destination.building->get_tiles();

			// The below is not thread safe
			//if(tile_list_3.empty())
			//{
			//	tile_list_3.append(access(current_destination.location));
			//}
#ifdef MULTI_THREAD
			destination_list[passenger_generation_thread_number].clear();
#else
			destination_list.clear();
#endif
			//vector_tpl<halthandle_t> destination_list(tile_list_3[0]->get_haltlist_count() * tile_list_3.get_count());

			if (current_destination.building->get_tiles().empty())
			{
				const planquadrat_t* current_tile_3 = access(current_destination.location); 
				const nearby_halt_t* halt_list = current_tile_3->get_haltlist();
				for (int h = current_tile_3->get_haltlist_count() - 1; h >= 0; h--)
				{
					halthandle_t halt = halt_list[h].halt;
					if (halt->is_enabled(wtyp))
					{
						// Previous versions excluded overcrowded halts here, but we need to know which
						// overcrowded halt would have been the best start halt if it was not overcrowded,
						// so do that below.
#ifdef MULTI_THREAD
						destination_list[passenger_generation_thread_number].append(halt);
#else
						destination_list.append(halt);
#endif
					}
				}
			}
			else
			{
				FOR(minivec_tpl<const planquadrat_t*>, const& current_tile_3, current_destination.building->get_tiles())
				{
					const nearby_halt_t* halt_list = current_tile_3->get_haltlist();
					if (!halt_list)
					{
						continue;
					}
					for (int h = current_tile_3->get_haltlist_count() - 1; h >= 0; h--)
					{
						halthandle_t halt = halt_list[h].halt;
						if (halt->is_enabled(wtyp))
						{
							// Previous versions excluded overcrowded halts here, but we need to know which
							// overcrowded halt would have been the best start halt if it was not overcrowded,
							// so do that below.
#ifdef MULTI_THREAD
							destination_list[passenger_generation_thread_number].append(halt);
#else
							destination_list.append(halt);
#endif
						}
					}
				}
			}

			best_journey_time = UINT32_MAX_VALUE;
			uint32 current_journey_time;
#ifdef MULTI_THREAD
			if (start_halts[passenger_generation_thread_number].get_count() == 1 && destination_list[passenger_generation_thread_number].get_count() == 1 && start_halts[passenger_generation_thread_number].get_element(0).halt == destination_list[passenger_generation_thread_number].get_element(0))
#else
			if (start_halts.get_count() == 1 && destination_list.get_count() == 1 && start_halts[0].halt == destination_list.get_element(0))
#endif
			{
				/** There is no public transport route, as the only stop
				* for the origin is also the only stop for the destintation.
				*/
#ifdef MULTI_THREAD 
				start_halt = start_halts[passenger_generation_thread_number].get_element(0).halt;
#else
				start_halt = start_halts[0].halt;
#endif

				if (can_walk)
				{
					const grund_t* destination_gr = lookup_kartenboden(current_destination.location);
					if (destination_gr && !destination_gr->is_water())
					{
						// People cannot walk on water. This is relevant for fisheries and oil rigs in particular.
						route_status = on_foot;
					}
				}
			}
			else
			{
				// Check whether public transport can be used.
				// Journey start information needs to be added later.
				pax.reset();
				pax.set_zielpos(destination_pos);
				pax.menge = units_this_step;
				//"Menge" = volume (Google)

				// Search for a route using public transport. 

				uint32 best_start_halt = 0;
				uint32 best_non_crowded_start_halt = 0;
				uint32 best_journey_time_including_crowded_halts = UINT32_MAX_VALUE;

				sint32 i = 0;
#ifdef MULTI_THREAD
				FOR(vector_tpl<nearby_halt_t>, const& nearby_halt, start_halts[passenger_generation_thread_number])
#else
				FOR(vector_tpl<nearby_halt_t>, const& nearby_halt, start_halts)
#endif
				{
					current_halt = nearby_halt.halt;
#ifdef MULTI_THREAD
					current_journey_time = current_halt->find_route(destination_list[passenger_generation_thread_number], pax, best_journey_time, destination_pos);
#else 
					current_journey_time = current_halt->find_route(destination_list, pax, best_journey_time, destination_pos);
#endif
					// Because it is possible to walk between stops in the route finder, check to make sure that this is not an all walking journey.
					// We cannot test this recursively within a reasonable time, so check only for the first stop.
					if (pax.get_ziel() == pax.get_zwischenziel())
					{
						haltestelle_t::connexion* cnx = current_halt->get_connexions(wtyp->get_catg_index())->get(pax.get_zwischenziel());
			
						if (current_halt->is_within_walking_distance_of(pax.get_zwischenziel()) && (!cnx || (!cnx->best_convoy.is_bound() && !cnx->best_line.is_bound())))
						{
							// Do not treat this as a public transport route: if it is a viable walking route, it will be so treated elsewhere.
							current_journey_time = UINT32_MAX_VALUE;
						}
					}

					
					// Add walking time from the origin to the origin stop. 
					// Note that the walking time to the destination stop is already added by find_route.
					if (current_journey_time < UINT32_MAX_VALUE)
					{
						// The above check is needed to prevent an overflow.
#ifdef MULTI_THREAD
						current_journey_time += walking_time_tenths_from_distance(start_halts[passenger_generation_thread_number].get_element(i).distance);
#else
						current_journey_time += walking_time_tenths_from_distance(start_halts[i].distance);
#endif
					}
					// TODO: Add facility to check whether station/stop has car parking facilities, and add the possibility of a (faster) private car journey.
					// Use the private car journey time per tile from the passengers' origin to the city in which the stop is located.

					if(current_journey_time < best_journey_time)
					{
						if(!current_halt->is_overcrowded(wtyp->get_index()))
						{
							best_journey_time = current_journey_time;
							best_non_crowded_start_halt = i;
							if(pax.get_ziel().is_bound())
							{
								route_status = public_transport;
							}
						}
						best_journey_time_including_crowded_halts = current_journey_time;
						best_start_halt = i;
					}
					i ++; 
				}

				if(best_journey_time == 0)
				{
					best_journey_time = 1;
				}

				if(can_walk && walking_time < best_journey_time && (walking_time <= walking_time_preference_threshold || best_journey_time > tolerance))
				{
					// If walking is faster than public transport, passengers will walk, unless
					// the public transport journey is within passengers' tolerance and the walking
					// time exceeds passenger' walking time preference threshold.
					const grund_t* destination_gr = lookup_kartenboden(current_destination.location);
					if(destination_gr && !destination_gr->is_water())
					{
						// People cannot walk on water. This is relevant for fisheries and oil rigs in particular.
						route_status = on_foot;
					}
				}

				// Check first whether the best route is outside
				// the passengers' tolerance.

				if(best_journey_time_including_crowded_halts < tolerance && route_status != public_transport && walking_time > best_journey_time)
				{ 
					route_status = overcrowded;
					if(!overcrowded_already_set)
					{
						best_bad_destination = destination_pos;
						best_bad_start_halt = best_start_halt;
						overcrowded_already_set = true;
					}
				}
				else if((route_status == public_transport || route_status == no_route) && best_journey_time_including_crowded_halts >= tolerance && best_journey_time_including_crowded_halts < UINT32_MAX_VALUE)
				{
					route_status = too_slow;
				
					if(!too_slow_already_set && !overcrowded_already_set)
					{
						best_bad_destination = destination_pos;
						best_bad_start_halt = best_start_halt;
						too_slow_already_set = true;
					}
				}
				else
				{
					// All passengers will use the quickest route.
#ifdef MULTI_THREAD
					if(start_halts[passenger_generation_thread_number].get_count() > 0)
					{
						start_halt = start_halts[passenger_generation_thread_number].get_element(best_start_halt).halt;
#else
					if (start_halts.get_count() > 0)
					{
						start_halt = start_halts[best_start_halt].halt;
#endif
					}
				}
			}

			if(has_private_car) 
			{
				// time_per_tile here is in 100ths of minutes per tile.
				// 1/100th of a minute per tile = km/h * 6.
				time_per_tile = UINT32_MAX_VALUE;
				switch(current_destination.type)
				{
				case town:
					//Town
					if(city)
					{
						time_per_tile = city->check_road_connexion_to(current_destination.building->get_stadt());
					}
					else
					{
						// Going onward from an out of town attraction or industry to a city building - get route backwards.
						if(current_destination.type == attraction)
						{
							time_per_tile = current_destination.building->get_stadt()->check_road_connexion_to(current_destination.building);
						}
						else if(current_destination.type == factory)		
						{
							time_per_tile = current_destination.building->get_stadt()->check_road_connexion_to(current_destination.building->get_fabrik());
						}						
					}
					break;
				case factory:
					if(city) // Previous time per tile value used as default if the city is not available.
					{
						time_per_tile = city->check_road_connexion_to(current_destination.building->get_fabrik());
					}
					break;
				case attraction:
					if(city) // Previous time per tile value used as default if the city is not available.
					{
						time_per_tile = city->check_road_connexion_to(current_destination.building);
					}							
					break;
				default:
					//Some error - this should not be reached.
#ifdef MULTI_THREAD
					int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
					assert(mutex_error == 0);
#endif
					dbg->error("simworld.cc", "Incorrect destination type detected");
#ifdef MULTI_THREAD
					mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
					assert(mutex_error == 0);
#endif
				};

				if(time_per_tile < UINT32_MAX_VALUE)
				{
					// *Hundredths* of minutes used here for per tile times for accuracy.
					// Convert to tenths, but only after multiplying to preserve accuracy.
					// Use a uint32 intermediary to avoid overflow.
					const uint32 car_mins = (time_per_tile * straight_line_distance) / 10;
					car_minutes = car_mins + 30; // Add three minutes to represent 1:30m parking time at each end.

					// Now, adjust the timings for congestion (this is already taken into account if the route was
					// calculated using the route finder; note that journeys inside cities are not calculated using
					// the route finder). 
#ifndef FORBID_CONGESTION_EFFECTS
					if(settings.get_assume_everywhere_connected_by_road() || (current_destination.type == town && current_destination.building->get_stadt() == city))
					{
						// Congestion here is assumed to be on the percentage basis: i.e. the percentage of extra time that
						// a journey takes owing to congestion. This is the measure used by the TomTom congestion index,
						// compiled by the satellite navigation company of that name, which provides useful research data.
						// See: http://www.tomtom.com/lib/doc/trafficindex/2013-0129-TomTom%20Congestion-Index-2012Q3europe-km.pdf
							
						//Average congestion of origin and destination towns.
						uint16 congestion_total;
						if(current_destination.building->get_stadt() != NULL && current_destination.building->get_stadt() != city)
						{
							// Destination type is town and the destination town object can be found.
							congestion_total = (city->get_congestion() + current_destination.building->get_stadt()->get_congestion()) / 2;
						}
						else
						{
							congestion_total = city->get_congestion();
						}
					
						const uint32 congestion_extra_minutes = (car_minutes * congestion_total) / 100;

						car_minutes += congestion_extra_minutes;
					}
#endif
				}
			}

			// Cannot be <=, as mail has a tolerance of UINT32_MAX_VALUE, which is used as the car_minutes when
			// a private car journey is not possible.
			if(car_minutes < tolerance)
			{
				const uint32 private_car_chance = simrand(100, "void stadt_t::generate_passengers_and_mail() (private car distribution_weight?)");

				if(route_status != public_transport)
				{
					// The passengers can get to their destination by car but not by public transport.
					// Therefore, they will always use their car unless it is faster to walk and they 
					// are not people who always prefer to use the car.
					if(car_minutes > walking_time && can_walk && walking_time <= walking_time_preference_threshold && private_car_chance > settings.get_always_prefer_car_percent())
					{
						// If walking is faster than taking the car, and the walking time is below passengers'
						// walking time preference threshold, passengers will walk.
						route_status = on_foot;
					}
					else
					{
						route_status = private_car;
					}
				}
				else if(private_car_chance <= settings.get_always_prefer_car_percent() || car_minutes <= best_journey_time)
				{
					route_status = private_car;
				}
			}
			else if(car_minutes != UINT32_MAX_VALUE)
			{
				route_status = too_slow;

				if(!too_slow_already_set && !overcrowded_already_set)
				{
					best_bad_destination = destination_pos;
 					// too_slow_already_set = true;
					// Do not set too_slow_already_set here, as will
					// prevent the passengers showing up in a "too slow" 
					// graph on a subsequent station/stop.
				}
			}
				
			if((route_status == no_route || route_status == too_slow || route_status == overcrowded || route_status == destination_unavailable) && n < destination_count + extend_count - 1)
			{
				// Do not get a new destination if there is a good status,
				// or if this is the last destination to be assigned,
				// or else entirely the wrong information will be recorded
				// below!
				current_destination = find_destination(trip, pax.get_class());
			}

		} // For loop (route_status)

		bool set_return_trip = false;
		stadt_t* destination_town;

#ifdef MULTI_THREAD
		int mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif

		switch(route_status)
		{
		case public_transport:
#ifdef MULTI_THREAD
			mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
			assert(mutex_error == 0);
#endif
			if(tolerance < UINT32_MAX_VALUE)
			{
				tolerance -= best_journey_time;
				walking_tolerance -= best_journey_time;
			}
			pax.set_origin(start_halt);
			start_halt->starte_mit_route(pax, origin_pos.get_2d());
#ifdef MULTI_THREAD
			mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
			assert(mutex_error == 0);
#endif
			if(city && wtyp == goods_manager_t::passengers)
			{
				city->merke_passagier_ziel(destination_pos, COL_YELLOW);
			}
			set_return_trip = true;
			// create pedestrians in the near area?
			if(settings.get_random_pedestrians() && wtyp == goods_manager_t::passengers) 
			{
				pedestrian_t::generate_pedestrians_at(origin_pos, units_this_step);
			}
			// We cannot do this on arrival, as the ware packets do not remember their origin building.
			// However, as for the destination, this can be set when the passengers arrive.
			if(trip == commuting_trip && first_origin)
			{
				first_origin->add_passengers_succeeded_commuting(units_this_step);
			}
			else if(trip == visiting_trip && first_origin)
			{
				first_origin->add_passengers_succeeded_visiting(units_this_step);
			}
			// Do nothing if trip == mail: mail statistics are added on arrival.
			break;

		case private_car:  
					
			if(tolerance < UINT32_MAX_VALUE)
			{
				tolerance -= car_minutes;
				walking_tolerance -= car_minutes;
			}
					
			destination_town = current_destination.type == town ? current_destination.building->get_stadt() : NULL;
			if(city)
			{
#ifdef DESTINATION_CITYCARS

				city->generate_private_cars(origin_pos.get_2d(), car_minutes, destination_pos, units_this_step);
#endif
				if(wtyp == goods_manager_t::passengers)
				{
					city->set_private_car_trip(units_this_step, destination_town);
					city->merke_passagier_ziel(destination_pos, COL_TURQUOISE);
				}
				else
				{
					// Mail
					city->add_transported_mail(units_this_step);
				}
			}

			set_return_trip = true;
			pax.set_zielpos(current_destination.location);
			// We cannot do this on arrival, as the ware packets do not remember their origin building.
			if(trip == commuting_trip)
			{
				first_origin->add_passengers_succeeded_commuting(units_this_step);
			}
			else if(trip == visiting_trip)
			{
				first_origin->add_passengers_succeeded_visiting(units_this_step);
			}
#ifdef MULTI_THREAD
			mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
			assert(mutex_error == 0);
#endif
			add_to_waiting_list(pax, origin_pos.get_2d());
#ifdef MULTI_THREAD
			mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
			assert(mutex_error == 0);
#endif
			break;

		case on_foot: 
					
			pax.set_zielpos(current_destination.location);

			if(tolerance < UINT32_MAX_VALUE)
			{
				tolerance -= walking_time;
				walking_tolerance -= walking_time;
			}	

			// Walking passengers are not marked as "happy", as the player has not made them happy.

			if(settings.get_random_pedestrians() && wtyp == goods_manager_t::passengers) 
			{
				pedestrian_t::generate_pedestrians_at(origin_pos, units_this_step, get_seconds_to_ticks(walking_time * 6));
			}
				
			if(city)
			{
				if(wtyp == goods_manager_t::passengers)
				{
					city->merke_passagier_ziel(destination_pos, COL_DARK_YELLOW);
					city->add_walking_passengers(units_this_step);
				}
				else
				{
					// Mail
					city->add_transported_mail(units_this_step);
				}
			}
			set_return_trip = true;

			// We cannot do this on arrival, as the ware packets do not remember their origin building.
			if(trip == commuting_trip)
			{
				first_origin->add_passengers_succeeded_commuting(units_this_step);
			}
			else if(trip == visiting_trip)
			{
				first_origin->add_passengers_succeeded_visiting(units_this_step);
				
			}
#ifdef MULTI_THREAD
			mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
			assert(mutex_error == 0);

			// Prevent deadlocks because of double-locking:
			// there is already a mutex lock in the add_to_waiting_list
#endif
			add_to_waiting_list(pax, origin_pos.get_2d());
			// Do nothing if trip == mail.
#ifdef MULTI_THREAD
			mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
			assert(mutex_error == 0);
#endif
			break;

		case overcrowded:

			if(city && wtyp == goods_manager_t::passengers)
			{
				city->merke_passagier_ziel(best_bad_destination, COL_RED);
			}					
#ifdef MULTI_THREAD
			if(start_halts[passenger_generation_thread_number].get_count() > 0)
			{
				start_halt = start_halts[passenger_generation_thread_number].get_element(best_bad_start_halt).halt;
#else
			if (start_halts.get_count() > 0)
			{
				start_halt = start_halts[best_bad_start_halt].halt;
#endif
				if(start_halt.is_bound())
				{
					start_halt->add_pax_unhappy(units_this_step);
				}
			}

			break;

		case too_slow:
			if(city && wtyp == goods_manager_t::passengers)
			{
				if(car_minutes >= best_journey_time)
				{
					city->merke_passagier_ziel(best_bad_destination, COL_LIGHT_PURPLE);
				}
				else if(car_minutes < UINT32_MAX_VALUE)
				{
					city->merke_passagier_ziel(best_bad_destination, COL_PURPLE);
				}
				else
				{
					// This should not occur but occasionally does.
					goto no_route;
				}
			}
#ifdef MULTI_THREAD
			if(too_slow_already_set && !start_halts[passenger_generation_thread_number].empty())
			{
				// This will be dud for a private car trip.
				start_halt = start_halts[passenger_generation_thread_number].get_element(best_bad_start_halt).halt;
			}
#else
			if (too_slow_already_set && !start_halts.empty())
			{
				// This will be dud for a private car trip.
				start_halt = start_halts[best_bad_start_halt].halt;
			}
#endif
			if(start_halt.is_bound() && best_journey_time < UINT32_MAX_VALUE)
			{
				start_halt->add_pax_too_slow(units_this_step);
			}
			break;

		case no_route:
		case destination_unavailable:
		default:
no_route:
			if(city && wtyp == goods_manager_t::passengers)
			{
				if(route_status == destination_unavailable)
				{
					city->merke_passagier_ziel(first_destination.location, COL_DARK_RED);
				}
				else
				{
					city->merke_passagier_ziel(first_destination.location, COL_DARK_ORANGE);
				}
			}
#ifdef MULTI_THREAD
			if(route_status != destination_unavailable && start_halts[passenger_generation_thread_number].get_count() > 0)
			{
				start_halt = start_halts[passenger_generation_thread_number].get_element(best_bad_start_halt).halt;
#else
			if (route_status != destination_unavailable && start_halts.get_count() > 0)
			{
				start_halt = start_halts[best_bad_start_halt].halt;
#endif
				if(start_halt.is_bound())
				{
					start_halt->add_pax_no_route(units_this_step);
				}
			}
		};

#ifdef MULTI_THREAD
		mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
		assert(mutex_error == 0);
#endif
#ifdef FORBID_RETURN_TRIPS
		if(false)
#else
		if(set_return_trip)
#endif
		{
			// Calculate a return journey
			// This comes most of the time for free and also balances the flows of passengers to and from any given place.

			// NOTE: This currently does not re-do the whole start/end stop search done on the way out. This saves time, but might
			// cause anomalies with substantially asymmetric routes. Reconsider this some time.

			// Because passengers/mail now register as transported on delivery, these are needed 
			// here to keep an accurate record of the proportion transported.
			stadt_t* const destination_town = get_city(current_destination.location);
			if(destination_town)
			{
#ifndef FORBID_SET_GENERATED_PASSENGERS
#ifdef MULTI_THREAD
				mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
				assert(mutex_error == 0);
#endif
				destination_town->set_generated_passengers(units_this_step, history_type + 1);
#ifdef MULTI_THREAD
				mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
				assert(mutex_error == 0);
#endif
#endif
			}
			else if(city)
			{
#ifndef FORBID_SET_GENERATED_PASSENGERS
#ifdef MULTI_THREAD
				mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
				assert(mutex_error == 0);
#endif
				city->set_generated_passengers(units_this_step, history_type + 1);
#ifdef MULTI_THREAD
				mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
				assert(mutex_error == 0);
#endif
#endif
				// Cannot add success figures for buildings here as cannot get a building from a koord. 
				// However, this should not matter much, as equally not recording generated passengers
				// for all return journeys should still show accurate percentages overall. 
			}
			if(current_destination.type == factory && (trip == commuting_trip || trip == mail_trip))
			{
				// The only passengers generated by a factory are returning passengers who have already reached the factory somehow or another
				// from home (etc.).
#ifndef FORBID_RECORDING_RETURN_FACTORY_PASSENGERS
#ifdef MULTI_THREAD
				mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
				assert(mutex_error == 0);
#endif
				current_destination.building->get_fabrik()->book_stat(units_this_step, (wtyp == goods_manager_t::passengers ? FAB_PAX_GENERATED : FAB_MAIL_GENERATED));
#ifdef MULTI_THREAD
				mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
				assert(mutex_error == 0);
#endif
#endif
			}
		
			ret_halt = pax.get_ziel();
			// Those who have driven out have to take thier cars back regardless of whether public transport is better - do not check again.
			bool return_in_private_car = route_status == private_car;
			bool return_on_foot = route_status == on_foot;

			if (!return_in_private_car && !return_on_foot)
			{
				if (!ret_halt.is_bound())
				{
#ifdef FORBID_SWITCHING_TO_RETURN_ON_FOOT
					if (false)
#else
					if (walking_time <= tolerance)
#endif
					{
						return_on_foot = true;
						goto return_on_foot;
					}
					else
					{
						// If the passengers cannot return, do not record them as having returned.
						continue;
					}
				}

				bool found_alternative_return_route = false;				

				// Now try to add them to the target halt
				const bool return_halt_is_overcrowded = ret_halt->is_overcrowded(wtyp->get_index());

				bool direct_return_available = false;
				ware_t return_passengers(wtyp, ret_halt);

#ifndef FORBID_FIND_ROUTE_FOR_RETURNING_PASSENGERS_1
				return_passengers.menge = units_this_step;
				// Overcrowding at the origin stop does not prevent a return to this stop.
				// best_bad_start_halt is actually the best start halt irrespective of overcrowding:
				// if the start halt is not overcrowded, this will be the actual start halt.
#ifdef MULTI_THREAD
				return_passengers.set_ziel(start_halts[passenger_generation_thread_number].get_element(best_bad_start_halt).halt);
#else
				return_passengers.set_ziel(start_halts[best_bad_start_halt].halt);
#endif
				return_passengers.set_zielpos(origin_pos.get_2d());
				return_passengers.is_commuting_trip = trip == commuting_trip;
				return_passengers.comfort_preference_percentage = pax.comfort_preference_percentage;

				// Passengers will always use the same return route as the route out if available.
				// (Passengers in real life are lazy, and this reduces compuational load)
				// We still need to do this even if the return halt is overcrowded so that we can
				// have accurate statistics. 
				direct_return_available = ret_halt->find_route(return_passengers) < UINT32_MAX_VALUE;
#endif
				if (!direct_return_available)
				{
					// Try to return to one of the other halts near the origin (now the destination)
					uint32 return_journey_time = UINT32_MAX;
#ifdef MULTI_THREAD
					FOR(vector_tpl<nearby_halt_t>, const nearby_halt, start_halts[passenger_generation_thread_number])
#else
					FOR(vector_tpl<nearby_halt_t>, const nearby_halt, start_halts)
#endif
					{
						halthandle_t test_halt = nearby_halt.halt;
						haltestelle_t::connexion* cnx = test_halt->get_connexions(wtyp->get_catg_index())->get(ret_halt);
						const uint32 jt = cnx ? cnx->journey_time : UINT32_MAX_VALUE;

						if (test_halt->is_enabled(wtyp) && (ret_halt == test_halt || jt < return_journey_time))
						{
							found_alternative_return_route = true;
							return_journey_time = jt;
							start_halt = test_halt;
						}
					}
				}
				
				bool can_return = direct_return_available;

				// Only mark the passengers as being unable to get to their 
				// destination due to overcrowding if they could get to 
				// their destination if the stop were not overcrowded.
				if(direct_return_available || found_alternative_return_route)
				{					
					if (!direct_return_available)
					{
						return_passengers.set_ziel(start_halt);

#ifndef FORBID_FIND_ROUTE_FOR_RETURNING_PASSENGERS_2
						can_return = ret_halt->find_route(return_passengers) < UINT32_MAX_VALUE;
					}
#endif
					if (can_return)
					{
						if (!return_halt_is_overcrowded)
						{
#ifndef FORBID_STARTE_MIT_ROUTE_FOR_RETURNING_PASSENGERS
							ret_halt->starte_mit_route(return_passengers, pax.get_zielpos());
#endif
							if (current_destination.type == factory && (trip == commuting_trip || trip == mail_trip))
							{
								// This is somewhat anomalous, as we are recording that the passengers have departed, not arrived, whereas for cities, we record
								// that they have successfully arrived. However, this is not easy to implement for factories, as passengers do not store their ultimate
								// origin, so the origin factory is not known by the time that the passengers reach the end of their journey.
#ifdef MULTI_THREAD
								mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
								assert(mutex_error == 0);
#endif
								current_destination.building->get_fabrik()->book_stat(units_this_step, (wtyp == goods_manager_t::passengers ? FAB_PAX_DEPARTED : FAB_MAIL_DEPARTED));
#ifdef MULTI_THREAD
								mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
								assert(mutex_error == 0);
#endif
							}
						}
						else
						{
							// Return halt crowded. Either return on foot or mark unhappy.
							if (walking_time <= tolerance)
							{
								return_on_foot = true;
							}
							else
							{
#ifdef MULTI_THREAD
								mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
								assert(mutex_error == 0);
#endif
								ret_halt->add_pax_unhappy(units_this_step);
#ifdef MULTI_THREAD
								mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
								assert(mutex_error == 0);
#endif
							}
						}
					}
				}
				else
				{
					// No route back by public transport.
					// Either walk or mark as no route.
					if(walking_time <= tolerance)
					{
						return_on_foot = true;
					}
					else
					{
#ifdef MULTI_THREAD
						mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
						assert(mutex_error == 0);
#endif
						ret_halt->add_pax_no_route(units_this_step);
#ifdef MULTI_THREAD
						mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
						assert(mutex_error == 0);
#endif
					}
				}
			}
		
			if(return_in_private_car)
			{
#ifdef MULTI_THREAD
				mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
				assert(mutex_error == 0);
#endif
				if(car_minutes < UINT32_MAX_VALUE)
				{
					// Do not check tolerance, as they must come back!
					if(wtyp == goods_manager_t::passengers)
					{
						if(destination_town)
						{
							destination_town->set_private_car_trip(units_this_step, city);
						}
						else
						{
							// Industry, attraction or local
							city->set_private_car_trip(units_this_step, NULL);
						}
					}
					else
					{
						// Mail
						if(destination_town)
						{
							destination_town->add_transported_mail(units_this_step);
						}
						else if(city)
						{
							city->add_transported_mail(units_this_step);
						}
					}
#ifdef DESTINATION_CITYCARS
					city->generate_private_cars(current_destination.location, car_minutes, origin_pos.get_2d(), units_this_step);
#endif
					if(current_destination.type == factory && (trip == commuting_trip || trip == mail_trip))
					{
						current_destination.building->get_fabrik()->book_stat(units_this_step, (wtyp == goods_manager_t::passengers ? FAB_PAX_DEPARTED : FAB_MAIL_DEPARTED));
					}
				}
				else
				{
					if(ret_halt.is_bound())
					{
						ret_halt->add_pax_no_route(units_this_step);
					}
					if(city)
					{
						city->merke_passagier_ziel(origin_pos.get_2d(), COL_DARK_ORANGE);
					}
				}
#ifdef MULTI_THREAD
				mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
				assert(mutex_error == 0);
#endif
			}
return_on_foot:
			if(return_on_foot)
			{
#ifdef MULTI_THREAD
				mutex_error = pthread_mutex_lock(&karte_t::step_passengers_and_mail_mutex);
				assert(mutex_error == 0);
#endif
				if(wtyp == goods_manager_t::passengers)
				{
					if (settings.get_random_pedestrians())
					{
						koord3d destination_pos_3d;
						destination_pos_3d.x = destination_pos.x;
						destination_pos_3d.y = destination_pos.y;
						destination_pos_3d.z = lookup_hgt(destination_pos);
						pedestrian_t::generate_pedestrians_at(destination_pos_3d, units_this_step, get_seconds_to_ticks(walking_time * 6));
					}
					if(destination_town)
					{
						destination_town->add_walking_passengers(units_this_step);
					}
					else if(city)
					{
						// Local, attraction or industry.
						city->merke_passagier_ziel(origin_pos.get_2d(), COL_DARK_YELLOW);
						city->add_walking_passengers(units_this_step);
					}
				}
				else
				{
					// Mail
					if(destination_town)
					{
						destination_town->add_transported_mail(units_this_step);
					}
					else if(city)
					{
						city->add_transported_mail(units_this_step);
					}
				}
				if(current_destination.type == factory && (trip == commuting_trip || trip == mail_trip))
				{
					current_destination.building->get_fabrik()->book_stat(units_this_step, (wtyp==goods_manager_t::passengers ? FAB_PAX_DEPARTED : FAB_MAIL_DEPARTED));
				}
#ifdef MULTI_THREAD
				mutex_error = pthread_mutex_unlock(&karte_t::step_passengers_and_mail_mutex);
				assert(mutex_error == 0);
#endif
			}

		} // Set return trip
	} // Onward journeys (for loop)
	return (sint32)units_this_step;
}

karte_t::destination karte_t::find_destination(trip_type trip, uint8 g_class)
{
	destination current_destination;
	current_destination.type = karte_t::invalid;
	gebaeude_t* gb;

	switch(trip)
	{
	case commuting_trip: 
		gb = pick_any_weighted(commuter_targets[g_class]);
		break;

	case visiting_trip:
		gb = pick_any_weighted(visitor_targets[g_class]);
		break;

	default:
	case mail_trip:
		gb = pick_any_weighted(mail_origins_and_targets);
	};
	if(!gb)
	{
		// Might happen if the relevant collection object is empty.		
		current_destination.location = koord::invalid;
		return current_destination;
	}

	current_destination.location = gb->get_pos().get_2d();
	current_destination.building = gb;

	// Add the correct object type.
	fabrik_t* const fab = gb->get_fabrik();
	stadt_t* const city = gb->get_stadt();
	if(fab)
	{
		current_destination.type = karte_t::factory;
	}
	else if(city)
	{
		current_destination.type = karte_t::town;
	}
	else // Attraction (out of town)
	{
		current_destination.type = karte_t::attraction;
	}

	return current_destination;
}


// recalculates world statistics for older versions
void karte_t::restore_history()
{
	last_month_bev = -1;
	for(  int m=12-1;  m>0;  m--  ) {
		// now step all towns
		sint64 bev=0;
		sint64 total_pas = 1, trans_pas = 0;
		sint64 total_mail = 1, trans_mail = 0;
		sint64 total_goods = 1, supplied_goods = 0;
		FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
			bev            += i->get_finance_history_month(m, HIST_CITICENS);
			trans_pas      += i->get_finance_history_month(m, HIST_PAS_TRANSPORTED);
			trans_pas      += i->get_finance_history_month(m, HIST_PAS_WALKED);
			trans_pas      += i->get_finance_history_month(m, HIST_CITYCARS);
			total_pas      += i->get_finance_history_month(m, HIST_PAS_GENERATED);
			trans_mail     += i->get_finance_history_month(m, HIST_MAIL_TRANSPORTED);
			total_mail     += i->get_finance_history_month(m, HIST_MAIL_GENERATED);
			supplied_goods += i->get_finance_history_month(m, HIST_GOODS_RECIEVED);
			total_goods    += i->get_finance_history_month(m, HIST_GOODS_NEEDED);
		}

		// the inhabitants stuff
		if(last_month_bev == -1) {
			last_month_bev = bev;
		}
		finance_history_month[m][WORLD_GROWTH] = bev-last_month_bev;
		finance_history_month[m][WORLD_CITICENS] = bev;
		last_month_bev = bev;

		// transportation ratio and total number
		finance_history_month[m][WORLD_PAS_RATIO] = (10000*trans_pas)/total_pas;
		finance_history_month[m][WORLD_PAS_GENERATED] = total_pas-1;
		finance_history_month[m][WORLD_MAIL_RATIO] = (10000*trans_mail)/total_mail;
		finance_history_month[m][WORLD_MAIL_GENERATED] = total_mail-1;
		finance_history_month[m][WORLD_GOODS_RATIO] = (10000*supplied_goods)/total_goods;
	}

	// update total transported, including passenger and mail
	for(  int m=min(MAX_WORLD_HISTORY_MONTHS,MAX_PLAYER_HISTORY_MONTHS)-1;  m>0;  m--  ) {
		sint64 transported = 0;
		for(  uint i=0;  i<MAX_PLAYER_COUNT;  i++ ) {
			if(  players[i]!=NULL  ) {
				transported += players[i]->get_finance()->get_history_veh_month( TT_ALL, m, ATV_TRANSPORTED );
			}
		}
		finance_history_month[m][WORLD_TRANSPORTED_GOODS] = transported;
	}

	sint64 bev_last_year = -1;
	for(  int y=min(MAX_WORLD_HISTORY_YEARS,MAX_CITY_HISTORY_YEARS)-1;  y>0;  y--  ) {
		// now step all towns (to generate passengers)
		sint64 bev=0;
		sint64 total_pas_year = 1, trans_pas_year = 0;
		sint64 total_mail_year = 1, trans_mail_year = 0;
		sint64 total_goods_year = 1, supplied_goods_year = 0;
		FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
			bev                 += i->get_finance_history_year(y, HIST_CITICENS);
			trans_pas_year      += i->get_finance_history_year(y, HIST_PAS_TRANSPORTED);
			trans_pas_year      += i->get_finance_history_year(y, HIST_PAS_WALKED);
			trans_pas_year      += i->get_finance_history_year(y, HIST_CITYCARS);
			total_pas_year      += i->get_finance_history_year(y, HIST_PAS_GENERATED);
			trans_mail_year     += i->get_finance_history_year(y, HIST_MAIL_TRANSPORTED);
			total_mail_year     += i->get_finance_history_year(y, HIST_MAIL_GENERATED);
			supplied_goods_year += i->get_finance_history_year(y, HIST_GOODS_RECIEVED);
			total_goods_year    += i->get_finance_history_year(y, HIST_GOODS_NEEDED);
		}

		// the inhabitants stuff
		if(bev_last_year == -1) {
			bev_last_year = bev;
		}
		finance_history_year[y][WORLD_GROWTH] = bev-bev_last_year;
		finance_history_year[y][WORLD_CITICENS] = bev;
		bev_last_year = bev;

		// transportation ratio and total number
		finance_history_year[y][WORLD_PAS_RATIO] = (10000*trans_pas_year)/total_pas_year;
		finance_history_year[y][WORLD_PAS_GENERATED] = total_pas_year-1;
		finance_history_year[y][WORLD_MAIL_RATIO] = (10000*trans_mail_year)/total_mail_year;
		finance_history_year[y][WORLD_MAIL_GENERATED] = total_mail_year-1;
		finance_history_year[y][WORLD_GOODS_RATIO] = (10000*supplied_goods_year)/total_goods_year;
	}

	for(  int y=min(MAX_WORLD_HISTORY_YEARS,MAX_CITY_HISTORY_YEARS)-1;  y>0;  y--  ) {
		sint64 transported_year = 0;
		for(  uint i=0;  i<MAX_PLAYER_COUNT;  i++ ) {
			if(  players[i]  ) {
				transported_year += players[i]->get_finance()->get_history_veh_year( TT_ALL, y, ATV_TRANSPORTED );
			}
		}
		finance_history_year[y][WORLD_TRANSPORTED_GOODS] = transported_year;
	}
	// fix current month/year
	update_history();
}


void karte_t::update_history()
{
	finance_history_year[0][WORLD_CONVOIS] = finance_history_month[0][WORLD_CONVOIS] = convoi_array.get_count();
	finance_history_year[0][WORLD_FACTORIES] = finance_history_month[0][WORLD_FACTORIES] = fab_list.get_count();

	// now step all towns (to generate passengers)
	sint64 bev = 0;
	sint64 jobs = 0;
	sint64 visitor_demand = 0;
	sint64 total_pas = 1, trans_pas = 0;
	sint64 total_mail = 1, trans_mail = 0;
	sint64 total_goods = 1, supplied_goods = 0;
	sint64 total_pas_year = 1, trans_pas_year = 0;
	sint64 total_mail_year = 1, trans_mail_year = 0;
	sint64 total_goods_year = 1, supplied_goods_year = 0;
	FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
		bev							+= i->get_finance_history_month(0, HIST_CITICENS);
		jobs						+= i->get_finance_history_month(0, HIST_JOBS);
		visitor_demand				+= i->get_finance_history_month(0, HIST_VISITOR_DEMAND);
		trans_pas					+= i->get_finance_history_month(0, HIST_PAS_TRANSPORTED);
		trans_pas					+= i->get_finance_history_month(0, HIST_PAS_WALKED);
		trans_pas					+= i->get_finance_history_month(0, HIST_CITYCARS);
		total_pas					+= i->get_finance_history_month(0, HIST_PAS_GENERATED);
		trans_mail					+= i->get_finance_history_month(0, HIST_MAIL_TRANSPORTED);
		total_mail					+= i->get_finance_history_month(0, HIST_MAIL_GENERATED);
		supplied_goods				+= i->get_finance_history_month(0, HIST_GOODS_RECIEVED);
		total_goods					+= i->get_finance_history_month(0, HIST_GOODS_NEEDED);
		trans_pas_year				+= i->get_finance_history_year( 0, HIST_PAS_TRANSPORTED);
		trans_pas_year				+= i->get_finance_history_year( 0, HIST_PAS_WALKED);
		trans_pas_year				+= i->get_finance_history_year( 0, HIST_CITYCARS);
		total_pas_year				+= i->get_finance_history_year( 0, HIST_PAS_GENERATED);
		trans_mail_year				+= i->get_finance_history_year( 0, HIST_MAIL_TRANSPORTED);
		total_mail_year				+= i->get_finance_history_year( 0, HIST_MAIL_GENERATED);
		supplied_goods_year			+= i->get_finance_history_year( 0, HIST_GOODS_RECIEVED);
		total_goods_year			+= i->get_finance_history_year( 0, HIST_GOODS_NEEDED);
	}

	finance_history_month[0][WORLD_GROWTH] = bev - last_month_bev;
	finance_history_year[0][WORLD_GROWTH] = bev - (finance_history_year[1][WORLD_CITICENS]==0 ? finance_history_month[0][WORLD_CITICENS] : finance_history_year[1][WORLD_CITICENS]);

	// the inhabitants stuff
	finance_history_year[0][WORLD_TOWNS] = finance_history_month[0][WORLD_TOWNS] = stadt.get_count();
	finance_history_year[0][WORLD_CITICENS] = finance_history_month[0][WORLD_CITICENS] = bev;
	finance_history_year[0][WORLD_JOBS] = finance_history_month[0][WORLD_JOBS] = jobs;
	finance_history_year[0][WORLD_VISITOR_DEMAND] = finance_history_month[0][WORLD_VISITOR_DEMAND] = visitor_demand;
	finance_history_month[0][WORLD_GROWTH] = bev - last_month_bev;
	finance_history_year[0][WORLD_GROWTH] = bev - (finance_history_year[1][WORLD_CITICENS] == 0 ? finance_history_month[0][WORLD_CITICENS] : finance_history_year[1][WORLD_CITICENS]);

	// transportation ratio and total number
	finance_history_month[0][WORLD_PAS_RATIO] = (10000*trans_pas)/total_pas;
	finance_history_month[0][WORLD_PAS_GENERATED] = total_pas-1;
	finance_history_month[0][WORLD_MAIL_RATIO] = (10000*trans_mail)/total_mail;
	finance_history_month[0][WORLD_MAIL_GENERATED] = total_mail-1;
	finance_history_month[0][WORLD_GOODS_RATIO] = (10000*supplied_goods)/total_goods;

	finance_history_year[0][WORLD_PAS_RATIO] = (10000*trans_pas_year)/total_pas_year;
	finance_history_year[0][WORLD_PAS_GENERATED] = total_pas_year-1;
	finance_history_year[0][WORLD_MAIL_RATIO] = (10000*trans_mail_year)/total_mail_year;
	finance_history_year[0][WORLD_MAIL_GENERATED] = total_mail_year-1;
	finance_history_year[0][WORLD_GOODS_RATIO] = (10000*supplied_goods_year)/total_goods_year;

	// update total transported, including passenger and mail
	sint64 transported = 0;
	sint64 transported_year = 0;
	for(  uint i=0;  i<MAX_PLAYER_COUNT;  i++ ) {
		if(  players[i]!=NULL  ) {
			transported += players[i]->get_finance()->get_history_veh_month( TT_ALL, 0, ATV_TRANSPORTED_GOOD );
			transported_year += players[i]->get_finance()->get_history_veh_year( TT_ALL, 0, ATV_TRANSPORTED_GOOD );
		}
	}
	finance_history_month[0][WORLD_TRANSPORTED_GOODS] = transported;
	finance_history_year[0][WORLD_TRANSPORTED_GOODS] = transported_year;

	// Find the global average car ownership.
	// TODO: Consider whether we need separate graphs for each class of passenger here.

	sint64 average_car_ownership_percent = 0;

	for (uint8 i = 0; i < goods_manager_t::passengers->get_number_of_classes(); i++)
	{
		average_car_ownership_percent += get_private_car_ownership(get_timeline_year_month(), i);
	}
	average_car_ownership_percent /= goods_manager_t::passengers->get_number_of_classes();

	finance_history_month[0][WORLD_CAR_OWNERSHIP] = average_car_ownership_percent;
	

	// Average the annual figure
	sint64 car_ownership_sum = 0;
	for(uint8 months = 0; months < MAX_WORLD_HISTORY_MONTHS; months ++)
	{
		car_ownership_sum += finance_history_month[months][WORLD_CAR_OWNERSHIP];
	}
	finance_history_year[0][WORLD_CAR_OWNERSHIP] = car_ownership_sum / MAX_WORLD_HISTORY_MONTHS;
}


static sint8 median( sint8 a, sint8 b, sint8 c )
{
#if 0
	if(  a==b  ||  a==c  ) {
		return a;
	}
	else if(  b==c  ) {
		return b;
	}
	else {
		// noting matches
//		return (3*128+1 + a+b+c)/3-128;
		return -128;
	}
#elif 0
	if(  a<=b  ) {
		return b<=c ? b : max(a,c);
	}
	else {
		return b>c ? b : min(a,c);
	}
#else
		return (6*128+3 + a+a+b+b+c+c)/6-128;
#endif
}


uint8 karte_t::recalc_natural_slope( const koord k, sint8 &new_height ) const
{
	grund_t *gr = lookup_kartenboden(k);
	if(!gr) {
		return slope_t::flat;
	}
	else {
		const sint8 max_hdiff = ground_desc_t::double_grounds ? 2 : 1;

		sint8 corner_height[4];

		// get neighbour corner heights
		sint8 neighbour_height[8][4];
		get_neighbour_heights( k, neighbour_height );

		//check whether neighbours are foundations
		bool neighbour_fundament[8];
		for(  int i = 0;  i < 8;  i++  ) {
			grund_t *gr2 = lookup_kartenboden( k + koord::neighbours[i] );
			neighbour_fundament[i] = (gr2  &&  gr2->get_typ() == grund_t::fundament);
		}

		for(  uint8 i = 0;  i < 4;  i++  ) { // 0 = sw, 1 = se etc.
			// corner_sw (i=0): tests vs neighbour 1:w (corner 2 j=1),2:sw (corner 3) and 3:s (corner 4)
			// corner_se (i=1): tests vs neighbour 3:s (corner 3 j=2),4:se (corner 4) and 5:e (corner 1)
			// corner_ne (i=2): tests vs neighbour 5:e (corner 4 j=3),6:ne (corner 1) and 7:n (corner 2)
			// corner_nw (i=3): tests vs neighbour 7:n (corner 1 j=0),0:nw (corner 2) and 1:w (corner 3)

			sint16 median_height = 0;
			uint8 natural_corners = 0;
			for(  int j = 1;  j < 4;  j++  ) {
				if(  !neighbour_fundament[(i * 2 + j) & 7]  ) {
					natural_corners++;
					median_height += neighbour_height[(i * 2 + j) & 7][(i + j) & 3];
				}
			}
			switch(  natural_corners  ) {
				case 1: {
					corner_height[i] = (sint8)median_height;
					break;
				}
				case 2: {
					corner_height[i] = median_height >> 1;
					break;
				}
				default: {
					// take the average of all 3 corners (if no natural corners just use the artificial ones anyway)
					corner_height[i] = median( neighbour_height[(i * 2 + 1) & 7][(i + 1) & 3], neighbour_height[(i * 2 + 2) & 7][(i + 2) & 3], neighbour_height[(i * 2 + 3) & 7][(i + 3) & 3] );
					break;
				}
			}
		}

		// new height of that tile ...
		sint8 min_height = min( min( corner_height[0], corner_height[1] ), min( corner_height[2], corner_height[3] ) );
		sint8 max_height = max( max( corner_height[0], corner_height[1] ), max( corner_height[2], corner_height[3] ) );
		/* check for an artificial slope on a steep sidewall */
		bool not_ok = abs( max_height - min_height ) > max_hdiff  ||  min_height == -128;

		sint8 old_height = gr->get_hoehe();
		new_height = min_height;

		// now we must make clear, that there is no ground above/below the slope
		if(  old_height!=new_height  ) {
			not_ok |= lookup(koord3d(k,new_height))!=NULL;
			if(  old_height > new_height  ) {
				not_ok |= lookup(koord3d(k,old_height-1))!=NULL;
			}
			if(  old_height < new_height  ) {
				not_ok |= lookup(koord3d(k,old_height+1))!=NULL;
			}
		}

		if(  not_ok  ) {
			/* difference too high or ground above/below
			 * we just keep it as it was ...
			 */
			new_height = old_height;
			return gr->get_grund_hang();
		}

		const sint16 d1 = min( corner_height[0] - new_height, max_hdiff );
		const sint16 d2 = min( corner_height[1] - new_height, max_hdiff );
		const sint16 d3 = min( corner_height[2] - new_height, max_hdiff );
		const sint16 d4 = min( corner_height[3] - new_height, max_hdiff );
		return d4 * 27 + d3 * 9 + d2 * 3 + d1;
	}
	return 0;
}


uint8 karte_t::calc_natural_slope( const koord k ) const
{
	if(is_within_grid_limits(k.x, k.y)) {

		const sint8 * p = &grid_hgts[k.x + k.y*(sint32)(get_size().x+1)];

		const int h1 = *p;
		const int h2 = *(p+1);
		const int h3 = *(p+get_size().x+2);
		const int h4 = *(p+get_size().x+1);

		const int mini = min(min(h1,h2), min(h3,h4));

		const int d1=h1-mini;
		const int d2=h2-mini;
		const int d3=h3-mini;
		const int d4=h4-mini;

		return d1 * 27 + d2 * 9 + d3 * 3 + d4;
	}
	return 0;
}


bool karte_t::is_water(koord k, koord dim) const
{
	koord k_check;
	for(  k_check.x = k.x;  k_check.x < k.x + dim.x;  k_check.x++  ) {
		for(  k_check.y = k.y;  k_check.y < k.y + dim.y;  k_check.y++  ) {
			if(  !is_within_grid_limits( k_check + koord(1, 1) )  ||  max_hgt(k_check) > get_water_hgt(k_check)  ) {
				return false;
			}
		}
	}
	return true;
}


bool karte_t::square_is_free(koord k, sint16 w, sint16 h, int *last_y, climate_bits cl) const
{
	if(k.x < 0  ||  k.y < 0  ||  k.x+w > get_size().x || k.y+h > get_size().y) {
		return false;
	}

	grund_t *gr = lookup_kartenboden(k);
	const sint16 platz_h = gr->get_grund_hang() ? max_hgt(k) : gr->get_hoehe();	// remember the max height of the first tile

	koord k_check;
	for(k_check.y=k.y+h-1; k_check.y>=k.y; k_check.y--) {
		for(k_check.x=k.x; k_check.x<k.x+w; k_check.x++) {
			const grund_t *gr = lookup_kartenboden(k_check);

			// we can built, if: max height all the same, everything removable and no buildings there
			slope_t::type slope = gr->get_grund_hang();
			sint8 max_height = gr->get_hoehe() + slope_t::max_diff(slope);
			climate test_climate = get_climate(k_check);
			if(  cl & (1 << water_climate)  &&  test_climate != water_climate  ) {
				bool neighbour_water = false;
				for(int i=0; i<8  &&  !neighbour_water; i++) {
					if(  is_within_limits(k_check + koord::neighbours[i])  &&  get_climate( k_check + koord::neighbours[i] ) == water_climate  ) {
						neighbour_water = true;
					}
				}
				if(  neighbour_water  ) {
					test_climate = water_climate;
				}
			}
			if(  platz_h != max_height  ||  !gr->ist_natur()  ||  gr->kann_alle_obj_entfernen(NULL) != NULL  ||
			     (cl & (1 << test_climate)) == 0  ||  ( slope && (lookup( gr->get_pos()+koord3d(0,0,1) ) ||
			     (slope_t::max_diff(slope)==2 && lookup( gr->get_pos()+koord3d(0,0,2) )) ))  ) {
				if(  last_y  ) {
					*last_y = k_check.y;
				}
				return false;
			}
		}
	}
	return true;
}


slist_tpl<koord> *karte_t::find_squares(sint16 w, sint16 h, climate_bits cl, sint16 old_x, sint16 old_y) const
{
	slist_tpl<koord> * list = new slist_tpl<koord>();
	koord start;
	int last_y;

DBG_DEBUG("karte_t::finde_plaetze()","for size (%i,%i) in map (%i,%i)",w,h,get_size().x,get_size().y );
	for(start.x=0; start.x<get_size().x-w; start.x++) {
		for(start.y=start.x<old_x?old_y:0; start.y<get_size().y-h; start.y++) {
			if(square_is_free(start, w, h, &last_y, cl)) {
				list->insert(start);
			}
			else {
				// Optimiert fuer groessere Felder, hehe!
				// Die Idee: wenn bei 2x2 die untere Reihe nicht geht, koennen
				// wir gleich 2 tiefer weitermachen! V. Meyer
				start.y = last_y;
			}
		}
	}
	return list;
}


/**
 * Play a sound, but only if near enough.
 * Sounds are muted by distance and clipped completely if too far away.
 *
 * @author Hj. Malthaner
 */
bool karte_t::play_sound_area_clipped(koord const k, uint16 const idx) const
{
	if(is_sound && viewport && display_get_width() > 0 && get_tile_raster_width() > 0) {
		int dist = koord_distance(k, viewport->get_world_position());
		bool play = false;

		if(dist < 96) {
			int xw = (6*display_get_width())/get_tile_raster_width();
			int yw = (4*display_get_height())/get_tile_raster_width();

			dist = max(dist - 8, 0);

			uint8 const volume = (uint8)(255U * (xw + yw) / (xw + yw + 32 * dist));
			if (volume > 8) {
				sound_play(idx, volume);
				play = true;
			}
		}
		return play;
	}
	return false;
}


void karte_t::save(const char *filename, loadsave_t::mode_t savemode, const char *version_str, const char *ex_version_str, const char* ex_revision_str, bool silent )
{
DBG_MESSAGE("karte_t::save()", "saving game to '%s'", filename);
	loadsave_t  file;
	bool save_temp = strstart( filename, "save/" );
	const char *savename = save_temp ? "save/_temp.sve" : filename;

	display_show_load_pointer( true );
	if(env_t::networkmode && !env_t::server && savemode == loadsave_t::bzip2)
	{
		// Make local saving/loading faster in network mode.
		savemode = loadsave_t::zipped;
	}
	if(!file.wr_open( savename, savemode, env_t::objfilename.c_str(), version_str, ex_version_str, ex_revision_str )) {
		create_win(new news_img("Kann Spielstand\nnicht speichern.\n"), w_info, magic_none);
		dbg->error("karte_t::save()","cannot open file for writing! check permissions!");
	}
	else {
		save(&file,silent);
		const char *success = file.close();
		if(success) {
			static char err_str[512];
			sprintf( err_str, translator::translate("Error during saving:\n%s"), success );
			create_win( new news_img(err_str), w_time_delete, magic_none);
		}
		else {
			if(  save_temp  ) {
				dr_rename( savename, filename );
			}
			if(!silent) {
				create_win( new news_img("Spielstand wurde\ngespeichert!\n"), w_time_delete, magic_none);
				// update the filename, if no autosave
				settings.set_filename(filename);
			}
		}
		reset_interaction();
	}
	display_show_load_pointer( false );
}


void karte_t::save(loadsave_t *file, bool silent)
{
	bool needs_redraw = false;

	loadingscreen_t *ls = NULL;
DBG_MESSAGE("karte_t::save(loadsave_t *file)", "start");
	if(!silent) {
		ls = new loadingscreen_t( translator::translate("Saving map ..."), get_size().y );
	}
#ifdef MULTI_THREAD
	stop_path_explorer(); 
#endif
	// rotate the map until it can be saved completely
	for( int i=0;  i<4  &&  nosave_warning;  i++  ) {
		rotate90();
		needs_redraw = true;
	}
	// seems not successful
	if(nosave_warning) {
		// but then we try to rotate until only warnings => some buildings may be broken, but factories should be fine
		for( int i=0;  i<4  &&  nosave;  i++  ) {
			rotate90();
			needs_redraw = true;
		}
		if(  nosave  ) {
			dbg->error( "karte_t::save()","Map cannot be saved in any rotation!" );
			create_win( new news_img("Map may be not saveable in any rotation!"), w_info, magic_none);
			// still broken, but we try anyway to save it ...
		}
	}
	// only broken buildings => just warn
	if(nosave_warning) {
		dbg->error( "karte_t::save()","Some buildings may be broken by saving!" );
	}

	/* If the current tool is a two_click_werkzeug, call cleanup() in order to delete dummy grounds (tunnel + monorail preview)
	 * THIS MUST NOT BE DONE IN NETWORK MODE!
	 */
	for(  uint8 sp_nr=0;  sp_nr<MAX_PLAYER_COUNT;  sp_nr++  ) {
		if(  two_click_tool_t* tool = dynamic_cast<two_click_tool_t*>(selected_tool[sp_nr]) ) {
			tool->cleanup();
		}
	}

	file->set_buffered(true);

	// do not set value for empty player
	uint8 old_players[MAX_PLAYER_COUNT];
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		old_players[i] = settings.get_player_type(i);
		if(  players[i]==NULL  ) {
			settings.set_player_type(i, player_t::EMPTY);
		}
	}
	settings.rdwr(file);
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		settings.set_player_type(i, old_players[i]);
	}

	if(file->get_extended_version() <= 1)
	{
		uint32 old_ticks = (uint32)ticks;
		file->rdwr_long(old_ticks);
		ticks = old_ticks;
	}
	else
	{
		file->rdwr_longlong(ticks);
	}
	file->rdwr_long(last_month);
	file->rdwr_long(last_year);

	// rdwr cityrules (and associated settings) for networkgames
	if(file->get_version()>102002 && (file->get_extended_version() == 0 || file->get_extended_version() >= 9))
	{
		bool do_rdwr = env_t::networkmode;
		file->rdwr_bool(do_rdwr);
		if (do_rdwr) 
		{
			if(file->get_extended_version() >= 9)
			{
				stadt_t::cityrules_rdwr(file);
				privatecar_rdwr(file);
			}
			stadt_t::electricity_consumption_rdwr(file);
			if(file->get_extended_version() < 13 && file->get_extended_revision() < 24 && file->get_version()>102003 && (file->get_extended_version() == 0 || file->get_extended_version() >= 9)) 
			{
				vehicle_builder_t::rdwr_speedbonus(file);
			}
		}
	}

	FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
		i->rdwr(file);
		if(silent) {
			INT_CHECK("saving");
		}
	}
DBG_MESSAGE("karte_t::save(loadsave_t *file)", "saved cities ok");

	for(int j=0; j<get_size().y; j++) {
		for(int i=0; i<get_size().x; i++) {
			plan[i+j*cached_grid_size.x].rdwr(file, koord(i,j) );
		}
		if(silent) {
			INT_CHECK("saving");
		}
		else {
			ls->set_progress(j);
		}
	}
DBG_MESSAGE("karte_t::save(loadsave_t *file)", "saved tiles");

	if(  file->get_version()<=102001  ) {
		// not needed any more
		for(int j=0; j<(get_size().y+1)*(sint32)(get_size().x+1); j++) {
			file->rdwr_byte(grid_hgts[j]);
		}
	DBG_MESSAGE("karte_t::save(loadsave_t *file)", "saved hgt");
	}

	sint32 fabs = fab_list.get_count();
	file->rdwr_long(fabs);
	FOR(vector_tpl<fabrik_t*>, const f, fab_list) {
		f->rdwr(file);
		if(silent) {
			INT_CHECK("saving");
		}
	}
DBG_MESSAGE("karte_t::save(loadsave_t *file)", "saved fabs");

	sint32 haltcount=haltestelle_t::get_alle_haltestellen().get_count();
	file->rdwr_long(haltcount);
	FOR(vector_tpl<halthandle_t>, const s, haltestelle_t::get_alle_haltestellen()) {
		s->rdwr(file);
	}
DBG_MESSAGE("karte_t::save(loadsave_t *file)", "saved stops");

	// save number of convois
	if(  file->get_version()>=101000  ) {
		uint16 i=convoi_array.get_count();
		file->rdwr_short(i);
	}
	FOR(vector_tpl<convoihandle_t>, const cnv, convoi_array) {
		// one MUST NOT call INT_CHECK here or else the convoi will be broken during reloading!
		cnv->rdwr(file);
	}
	if(  file->get_version()<101000  ) {
		file->wr_obj_id("Ende Convois");
	}
	if(silent) {
		INT_CHECK("saving");
	}
DBG_MESSAGE("karte_t::save(loadsave_t *file)", "saved %i convois",convoi_array.get_count());

	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
// **** REMOVE IF SOON! *********
		if(file->get_version()<101000) {
			if(  i<8  ) {
				if(  players[i]  ) {
					players[i]->rdwr(file);
				}
				else {
					// simulate old ones ...
					player_t *player = new player_t( this, i );
					player->rdwr(file);
					delete player;
				}
			}
		}
		else {
			if(  players[i]  ) {
				players[i]->rdwr(file);
			}
		}
	}
DBG_MESSAGE("karte_t::save(loadsave_t *file)", "saved players");

	// saving messages
	if(  file->get_version()>=102005  ) {
		msg->rdwr(file);
	}
DBG_MESSAGE("karte_t::save(loadsave_t *file)", "saved messages");

	// centered on what?
	sint32 dummy = viewport->get_world_position().x;
	file->rdwr_long(dummy);
	dummy = viewport->get_world_position().y;
	file->rdwr_long(dummy);

	if(file->get_version() >= 99018)
	{
		// Most recent Standard version is 99018
		
		for (int year = 0; year < /*MAX_WORLD_HISTORY_YEARS*/ 12; year++)
		{
			for (int cost_type = 0; cost_type < MAX_WORLD_COST; cost_type++)
			{
				if(file->get_extended_version() < 12 && (cost_type == WORLD_JOBS || cost_type == WORLD_VISITOR_DEMAND || cost_type == WORLD_CAR_OWNERSHIP))
				{
					finance_history_year[year][cost_type] = 0;
				}
				else
				{
					file->rdwr_longlong(finance_history_year[year][cost_type]);
				}
			}
		}
		for (int month = 0; month < /*MAX_WORLD_HISTORY_MONTHS*/ 12; month++)
		{
			for (int cost_type = 0; cost_type < MAX_WORLD_COST; cost_type++)
			{
				if(file->get_extended_version() < 12 && (cost_type == WORLD_JOBS || cost_type == WORLD_VISITOR_DEMAND || cost_type == WORLD_CAR_OWNERSHIP))
				{
					finance_history_month[month][cost_type] = 0;
				}
				else
				{
					file->rdwr_longlong(finance_history_month[month][cost_type]);
				}
			}
		}
	}

	// finally a possible scenario
	scenario->rdwr( file );

	if(file->get_extended_version() >= 2)
	{
		file->rdwr_short(base_pathing_counter);
	}
	if(file->get_extended_version() >= 7 && file->get_extended_version() < 9 && file->get_version() < 110006)
	{
		double old_proportion = (double)industry_density_proportion / 10000.0;
		file->rdwr_double(old_proportion);
		industry_density_proportion = old_proportion * 10000.0;
	}
	else if(file->get_extended_version() >= 9 && file->get_version() >= 110006 && file->get_extended_version() < 11)
	{
		// Versions before 10.16 used an excessively low (and therefore inaccurate) integer for the industry density proportion. 
		// Detect this by checking whether the highest bit is set (it will not be naturally, so will only be set if this is 
		// 10.16 or higher, and not 11.0 and later, where we can assume that the numbers are correct and can be dealt with simply).
		uint32 idp = industry_density_proportion;

		idp |= 0x8000;
		file->rdwr_long(idp);
	}
	else if(file->get_extended_version() >= 11)
	{
		file->rdwr_long(industry_density_proportion);
	}

	if(file->get_extended_version() >=9 && file->get_version() >= 110000)
	{
		if(file->get_extended_version() < 11)
		{
			// Was next_private_car_update_month
			uint8 dummy;
			file->rdwr_byte(dummy);
		}
		
		// Existing values now saved in order to prevent network desyncs
		file->rdwr_long(citycar_speed_average);
		file->rdwr_bool(recheck_road_connexions);
		if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
		{
			file->rdwr_long(generic_road_time_per_tile_city);
			file->rdwr_long(generic_road_time_per_tile_intercity);
		}
		else
		{
			uint16 tmp = generic_road_time_per_tile_city < UINT32_MAX_VALUE ? (uint16)generic_road_time_per_tile_city : 65535;
			file->rdwr_short(tmp);
			if (tmp == 65535)
			{
				generic_road_time_per_tile_city = UINT32_MAX_VALUE;
			}
			else
			{
				generic_road_time_per_tile_city = (uint32)tmp;
			}

			tmp = (uint16)generic_road_time_per_tile_intercity;
			file->rdwr_short(tmp);
			generic_road_time_per_tile_intercity = (uint32)tmp;
		}
		
		file->rdwr_long(max_road_check_depth);
		if(file->get_extended_version() < 10)
		{
			double old_density = actual_industry_density / 100.0;
			file->rdwr_double(old_density);
			actual_industry_density = old_density * 100;
		}
		else
		{
			file->rdwr_long(actual_industry_density);
		}
	}

	if(file->get_extended_version() >= 12)
	{
#ifdef MULTI_THREAD
		pthread_mutex_lock(&step_passengers_and_mail_mutex);
#endif
		file->rdwr_long(next_step_passenger);
		file->rdwr_long(next_step_mail);

#ifdef MULTI_THREAD
		pthread_mutex_unlock(&step_passengers_and_mail_mutex);
#endif

		if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 13)
		{
			if (env_t::networkmode)
			{
				if (env_t::server)
				{
					sint32 po = env_t::num_threads - 1;
					file->rdwr_long(po);
					parallel_operations = 0;
				}
				else
				{
					file->rdwr_long(parallel_operations);
				}
			}
			else
			{
				sint32 dummy = -1;
				file->rdwr_long(dummy);
				parallel_operations = -1;
			}
		}
	}

	if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 15)
	{
		uint32 count;
		sint64 ready;
		ware_t ware;
#ifdef MULTI_THREAD
		count = 0;
		for (sint32 i = 0; i < get_parallel_operations(); i++)
		{
			count += transferring_cargoes[i].get_count();
		}
#else
		count = transferring_cargoes[0].get_count();
#endif

		file->rdwr_long(count);

		sint32 po;
#ifdef MULTI_THREAD
		po = get_parallel_operations();
#else
		po = 1;
#endif

		for (sint32 i = 0; i < po; i++)
		{
			for (uint32 j = 0; j < transferring_cargoes[i].get_count(); j++)
			{
				ready = transferring_cargoes[i][j].ready_time;
				ware = transferring_cargoes[i][j].ware;

				file->rdwr_longlong(ready);
				ware.rdwr(file);
			}
		}
	}

	if(  file->get_version() >= 112008  ) {
		xml_tag_t t( file, "motd_t" );

		chdir( env_t::user_dir );
		// maybe show message about server
DBG_MESSAGE("karte_t::save(loadsave_t *file)", "motd filename %s", env_t::server_motd_filename.c_str() );
		if(  FILE *fmotd = fopen( env_t::server_motd_filename.c_str(), "r" )  ) {
			struct stat st;
			stat( env_t::server_motd_filename.c_str(), &st );
			sint32 len = min( 32760, st.st_size+1 );
			char *motd = (char *)malloc( len );
			fread( motd, len-1, 1, fmotd );
			fclose( fmotd );
			motd[len-1] = 0;
			file->rdwr_str( motd, len );
			free( motd );
		}
		else {
			// no message
			plainstring motd("");
			file->rdwr_str( motd );
		}
	}

	// MUST be at the end of the load/save routine.
	// save all open windows (upon request)
	file->rdwr_byte( active_player_nr );
	rdwr_all_win(file);

	file->set_buffered(false);

	if(needs_redraw) {
		update_map();
	}
	if(!silent) {
		delete ls;
	}
}


// store missing obj during load and their severity
void karte_t::add_missing_paks( const char *name, missing_level_t level )
{
	if(  missing_pak_names.get( name )==NOT_MISSING  ) {
		missing_pak_names.put( strdup(name), level );
	}
}


// LOAD, not save
// just the preliminaries, opens the file, checks the versions ...
bool karte_t::load(const char *filename)
{
	cbuffer_t name;
	bool ok = false;
	bool restore_player_nr = false;
	bool server_reload_pwd_hashes = false;
	mute_sound(true);
	display_show_load_pointer(true);
	loadsave_t file;
	cities_awaiting_private_car_route_check.clear();
	time_interval_signals_to_check.clear();

	// clear hash table with missing paks (may cause some small memory loss though)
	missing_pak_names.clear();

	DBG_MESSAGE("karte_t::load", "loading game from '%s'", filename);

	// reloading same game? Remember pos
	const koord oldpos = settings.get_filename()[0]>0  &&  strncmp(filename,settings.get_filename(),strlen(settings.get_filename()))==0 ? viewport->get_world_position() : koord::invalid;

	if(  strstart(filename, "net:")  ) {
		// probably finish network mode?
		if(  env_t::networkmode  ) {
			network_core_shutdown();
		}
		chdir( env_t::user_dir );
		const char *err = network_connect(filename+4, this);
		if(err) {
			create_win( new news_img(err), w_info, magic_none );
			display_show_load_pointer(false);
			step_mode = NORMAL;
			return false;
		}
		else {
			env_t::networkmode = true;
			name.printf( "client%i-network.sve", network_get_client_id() );
			restore_player_nr = strcmp( last_network_game.c_str(), filename )==0;
			if(  !restore_player_nr  ) {
				last_network_game = filename;
			}
		}
	}
	else {
		// probably finish network mode first?
		if(  env_t::networkmode  ) {
			if (  env_t::server  ) {
				char fn[256];
				sprintf( fn, "server%d-network.sve", env_t::server );
				if(  strcmp(filename, fn) != 0  ) {
					// stay in networkmode, but disconnect clients
					dbg->warning("karte_t::load","disconnecting all clients");
					network_reset_server();
				}
				else {
					// read password hashes from separate file
					// as they are not in the savegame to avoid sending them over network
					server_reload_pwd_hashes = true;
				}
			}
			else {
				// check, if reload during sync
				char fn[256];
				sprintf( fn, "client%i-network.sve", network_get_client_id() );
				if(  strcmp(filename,fn)!=0  ) {
					// no sync => finish network mode
					dbg->warning("karte_t::load","finished network mode");
					network_disconnect();
					finish_loop = false; // do not trigger intro screen
					// closing the socket will tell the server, I am away too
				}
			}
		}
		name.append(filename);
	}

	if(!file.rd_open(name)) {

		if(  (sint32)file.get_version()==-1  ||  file.get_version()>loadsave_t::int_version(SAVEGAME_VER_NR, NULL, NULL).version  ) {
			dbg->warning("karte_t::load()", translator::translate("WRONGSAVE") );
			create_win( new news_img("WRONGSAVE"), w_info, magic_none );
		}
		else {
			dbg->warning("karte_t::load()", translator::translate("Kann Spielstand\nnicht laden.\n") );
			create_win(new news_img("Kann Spielstand\nnicht laden.\n"), w_info, magic_none);
		}
	}
	else if(file.get_version() < 84006) {
		// too old
		dbg->warning("karte_t::load()", translator::translate("WRONGSAVE") );
		create_win(new news_img("WRONGSAVE"), w_info, magic_none);
	}
	else {
DBG_MESSAGE("karte_t::load()","Savegame version is %d", file.get_version());

		load(&file);

		if(  env_t::networkmode  ) {
			clear_command_queue();
		}

		if(  env_t::server  ) {
			step_mode = FIX_RATIO;
			if(  env_t::server  ) {
				// meaningless to use a locked map; there are passwords now
				settings.set_allow_player_change(true);
				// language of map becomes server language
				settings.set_name_language_iso(translator::get_lang()->iso_base);
			}

			if(  server_reload_pwd_hashes  ) {
				char fn[256];
				sprintf( fn, "server%d-pwdhash.sve", env_t::server );
				loadsave_t pwdfile;
				if(  pwdfile.rd_open(fn)  ) {
					rdwr_player_password_hashes( &pwdfile );
					// correct locking info
					nwc_auth_player_t::init_player_lock_server(this);
					pwdfile.close();
				}
			}
		}
		else if(  env_t::networkmode  ) {
			step_mode = PAUSE_FLAG|FIX_RATIO;
			switch_active_player( last_active_player_nr, true );
			if(  is_within_limits(oldpos)  ) {
				// go to position when last disconnected
				viewport->change_world_position( oldpos );
			}
		}
		else {
			step_mode = NORMAL;
		}

		ok = true;
		file.close();

		if(  !scenario->rdwr_ok()  ) {
			// error during loading of savegame of scenario
			const char* err = scenario->get_error_text();
			if (err == NULL) {
				err = "Loading scenario failed.";
			}
			create_win( new news_img( err ), w_info, magic_none);
			delete scenario;
			scenario = new scenario_t(this);
		}
		else if(  !env_t::networkmode  ||  !env_t::restore_UI  ) {
			// warning message about missing paks
			if(  !missing_pak_names.empty()  ) {

				cbuffer_t msg;
				msg.append("<title>");
				msg.append(translator::translate("Missing pakfiles"));
				msg.append("</title>\n");

				cbuffer_t error_paks;
				cbuffer_t warning_paks;

				cbuffer_t paklog;
				paklog.append( "\n" );
				FOR(stringhashtable_tpl<missing_level_t>, const& i, missing_pak_names) {
					if (i.value <= MISSING_ERROR) {
						error_paks.append(translator::translate(i.key));
						error_paks.append("<br>\n");
						paklog.append( i.key );
						paklog.append("\n" );
					}
					else {
						warning_paks.append(translator::translate(i.key));
						warning_paks.append("<br>\n");
					}
				}

				if(  error_paks.len()>0  ) {
					msg.append("<h1>");
					msg.append(translator::translate("Pak which may cause severe errors:"));
					msg.append("</h1><br>\n");
					msg.append("<br>\n");
					msg.append( error_paks );
					msg.append("<br>\n");
					dbg->warning( "The following paks are missing and may cause errors", paklog );
				}

				if(  warning_paks.len()>0  ) {
					msg.append("<h1>");
					msg.append(translator::translate("Pak which may cause visual errors:"));
					msg.append("</h1><br>\n");
					msg.append("<br>\n");
					msg.append( warning_paks );
					msg.append("<br>\n");
				}

				help_frame_t *win = new help_frame_t();
				win->set_text( msg );
				create_win(win, w_info, magic_pakset_info_t);
			}
			// do not notify if we restore everything
			create_win( new news_img("Spielstand wurde\ngeladen!\n"), w_time_delete, magic_none);
		}
		set_dirty();

		reset_timer();
		recalc_average_speed();
		mute_sound(false);

		tool_t::update_toolbars();
		set_tool( tool_t::general_tool[TOOL_QUERY], get_active_player() );
	}

	settings.set_filename(filename);
	display_show_load_pointer(false);

	calc_generic_road_time_per_tile_city();
	calc_generic_road_time_per_tile_intercity();
	calc_max_road_check_depth();

	return ok;
}

#ifdef MULTI_THREAD
static pthread_mutex_t height_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

void karte_t::plans_finish_rd( sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max )
{
	sint8 min_h = min_height, max_h = max_height;
	for(  int y = y_min;  y < y_max;  y++  ) {
		for(  int x = x_min; x < x_max;  x++  ) {
			const planquadrat_t *plan = access_nocheck(x,y);
			const int boden_count = plan->get_boden_count();
			for(  int schicht = 0;  schicht < boden_count;  schicht++  ) {
				grund_t *gr = plan->get_boden_bei(schicht);
				if(  min_h > gr->get_hoehe()  ) {
					min_h = gr->get_hoehe();
				}
				else if(  max_h < gr->get_hoehe()  ) {
					max_h = gr->get_hoehe();
				}
				for(  int n = 0;  n < gr->get_top();  n++  ) {
					obj_t *obj = gr->obj_bei(n);
					if(obj) {
						obj->finish_rd();
					}
				}
				if(  load_version.version<=111000  &&  gr->ist_natur()  ) {
					gr->sort_trees();
				}
				gr->calc_image();
			}
		}
	}
	// update heights
#ifdef MULTI_THREAD
	pthread_mutex_lock( &height_mutex );
	if(  min_height > min_h  ) {
		min_height = min_h;
	}
	if(  max_height < max_h  ) {
		max_height = max_h;
	}
	pthread_mutex_unlock( &height_mutex );
#else
	min_height = min_h;
	max_height = max_h;
#endif
}


void karte_t::load(loadsave_t *file)
{
	char buf[80];

	intr_disable();
	dbg->message("karte_t::load()", "Prepare for loading" );
	dbg->message("karte_t::load()", "Time is now: %i", dr_time()); 
	for (uint8 sp_nr = 0; sp_nr < MAX_PLAYER_COUNT; sp_nr++) {
		if (two_click_tool_t* tool = dynamic_cast<two_click_tool_t*>(selected_tool[sp_nr])) {
			tool->cleanup();
		}
	}

	destroy_all_win(true);

	clear_random_mode(~LOAD_RANDOM);
	set_random_mode(LOAD_RANDOM);
	destroy();

	loadingscreen_t ls(translator::translate("Loading map ..."), 1, true, true );

	clear_random_mode(~LOAD_RANDOM);
	set_random_mode(LOAD_RANDOM);

	// Added by : Knightly
	path_explorer_t::initialise(this);

#ifdef MULTI_THREAD
	// destroy() destroys the threads, so this must be here.
	init_threads();
	first_step = 1;
#endif

	tile_counter = 0;

	simloops = 60;

	// zum laden vorbereiten -> tablele loeschen
	powernet_t::new_world();
	pumpe_t::new_world();
	senke_t::new_world();

	const uint16 old_scale_factor = get_settings().get_meters_per_tile();
	file->set_buffered(true);

	// jetzt geht das laden los
	dbg->warning("karte_t::load", "Fileversion: %d", file->get_version());
	// makes a copy:
	settings = env_t::default_settings;
	settings.rdwr(file);

	// We may wish to override the settings saved in the file.
	// But not if we are a network client.
	if (  !env_t::networkmode || env_t::server  ) {
		bool read_progdir_simuconf = env_t::default_settings.get_progdir_overrides_savegame_settings();
		bool read_pak_simuconf = env_t::default_settings.get_pak_overrides_savegame_settings();
		bool read_userdir_simuconf = env_t::default_settings.get_userdir_overrides_savegame_settings();
		tabfile_t simuconf;
		sint16 idummy;
		string dummy;

		if (read_progdir_simuconf) {
			chdir( env_t::program_dir );
			if(simuconf.open("config/simuconf.tab")) {
				printf("parse_simuconf() in program dir (%s) for override of save file: ", "config/simuconf.tab");
				settings.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
				simuconf.close();
			}
			chdir( env_t::user_dir );
		}
		if (read_pak_simuconf) {
			chdir( env_t::program_dir );
			std::string pak_simuconf = env_t::objfilename + "config/simuconf.tab";
			if(simuconf.open(pak_simuconf.c_str())) {
				printf("parse_simuconf() in pak dir (%s) for override of save file: ", pak_simuconf.c_str() );
				settings.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
				simuconf.close();
			}
			chdir( env_t::user_dir );
		}
		if (read_userdir_simuconf) {
			chdir( env_t::user_dir );
			std::string userdir_simuconf = "simuconf.tab";
			if(simuconf.open("simuconf.tab")) {
				printf("parse_simuconf() in user dir (%s) for override of save file: ", userdir_simuconf.c_str() );
				settings.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
				simuconf.close();
			}
		}
	}

	loaded_rotation = settings.get_rotation();


	// some functions (finish_rd) need to know what version was loaded
	load_version.version = file->get_version();
	load_version.extended_version = file->get_extended_version();
	load_version.extended_revision = file->get_extended_revision();

	if(  env_t::networkmode  ) {
		// clear the checklist history
		for(  int i=0;  i<LAST_CHECKLISTS_COUNT;  ++i  ) {
			last_checklists[i] = checklist_t();
		}
		for(  int i = 0;  i < CHK_RANDS  ;  i++  ) {
			rands[i] = 0;
		}
	}


#ifndef DEBUG_SIMRAND_CALLS
	if(  env_t::networkmode  ) {
		// to have games synchronized, transfer random counter too
		setsimrand(settings.get_random_counter(), 0xFFFFFFFFu );
//		setsimrand(0, 0xFFFFFFFFu );
		translator::init_custom_names(settings.get_name_language_id());
	}
#endif

	if(  !env_t::networkmode  ||  (env_t::server  &&  socket_list_t::get_playing_clients()==0)  ) {
		if (settings.get_allow_player_change() && env_t::default_settings.get_use_timeline() < 2) {
			// not locked => eventually switch off timeline settings, if explicitly stated
			settings.set_use_timeline(env_t::default_settings.get_use_timeline());
			DBG_DEBUG("karte_t::load", "timeline: reset to %i", env_t::default_settings.get_use_timeline() );
		}
	}
	if (settings.get_beginner_mode()) {
		goods_manager_t::set_multiplier(settings.get_beginner_price_factor(), settings.get_meters_per_tile());
	}
	else {
		goods_manager_t::set_multiplier( 1000, settings.get_meters_per_tile() );
	}

	if(old_scale_factor != get_settings().get_meters_per_tile())
	{
		set_scale();
	}

	groundwater = (sint8)(settings.get_groundwater());
	min_height = max_height = groundwater;
	DBG_DEBUG("karte_t::load()","groundwater %i",groundwater);

	if (file->get_version() < 112007) {
		// r7930 fixed a bug in init_height_to_climate
		// recover old behavior to not mix up climate when loading old savegames
		groundwater = settings.get_climate_borders()[0];
		init_height_to_climate();
		groundwater = settings.get_groundwater();
	}
	else {
		init_height_to_climate();
	}

	// just an initialisation for the loading
	season = (2+last_month/3)&3; // summer always zero
	snowline = settings.get_winter_snowline() + groundwater;

	DBG_DEBUG("karte_t::load", "settings loaded (size %i,%i) timeline=%i beginner=%i", settings.get_size_x(), settings.get_size_y(), settings.get_use_timeline(), settings.get_beginner_mode());

	// wird gecached, um den Pointerzugriff zu sparen, da
	// die size _sehr_ oft referenziert wird
	cached_grid_size.x = settings.get_size_x();
	cached_grid_size.y = settings.get_size_y();
	cached_size_max = max(cached_grid_size.x,cached_grid_size.y);
	cached_size.x = cached_grid_size.x-1;
	cached_size.y = cached_grid_size.y-1;
	viewport->set_x_off(0);
	viewport->set_y_off(0);

	// Reliefkarte an neue welt anpassen
	reliefkarte_t::get_karte()->init();

	ls.set_max( get_size().y*2+256 );
	init_tiles();


	// reinit pointer with new pointer object and old values
	zeiger = new zeiger_t(koord3d::invalid, NULL );

	hausbauer_t::new_world();
	factory_builder_t::new_world();

	DBG_DEBUG("karte_t::load", "init felder ok");

	if(file->get_extended_version() <= 1)
	{
		uint32 old_ticks = (uint32)ticks;
		file->rdwr_long(old_ticks);
		ticks = (sint64)old_ticks;
	}
	else
	{
		file->rdwr_longlong(ticks);
	}
	file->rdwr_long(last_month);
	file->rdwr_long(last_year);
	if(file->get_version()<86006) {
		last_year += env_t::default_settings.get_starting_year();
	}
	// old game might have wrong month
	last_month %= 12;
	// set the current month count
	set_ticks_per_world_month_shift(settings.get_bits_per_month());
	current_month = last_month + (last_year*12);
	season = (2+last_month/3)&3; // summer always zero
	next_month_ticks = ( (ticks >> karte_t::ticks_per_world_month_shift) + 1 ) << karte_t::ticks_per_world_month_shift;
	last_step_ticks = ticks;
	network_frame_count = 0;
	sync_steps = 0;
	steps = 0;
	sync_steps_barrier = sync_steps;
	step_mode = PAUSE_FLAG;

DBG_MESSAGE("karte_t::load()","savegame loading at tick count %i",ticks);
	recalc_average_speed();	// resets timeline
	// recalc_average_speed may have opened message windows
	destroy_all_win(true);

DBG_MESSAGE("karte_t::load()", "init player");
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(  file->get_version()>=101000  ) {
			// since we have different kind of AIs
			delete players[i];
			players[i] = NULL;
			init_new_player(i, settings.player_type[i]);
		}
		else if(i<8) {
			// get the old player ...
			if(  players[i]==NULL  ) {
				init_new_player( i, (i==3) ? player_t::AI_PASSENGER : player_t::AI_GOODS );
			}
			settings.player_type[i] = players[i]->get_ai_id();
		}
	}
	// so far, player 1 will be active (may change in future)
	active_player = players[0];
	active_player_nr = 0;

	// rdwr cityrules for networkgames
	if(file->get_version() > 102002 && (file->get_extended_version() == 0 || file->get_extended_version() >= 9)) {
		bool do_rdwr = env_t::networkmode;
		file->rdwr_bool(do_rdwr);
		if(do_rdwr) 
		{
			// This stuff should not be in a saved game.  Unfortunately, due to the vagaries
			// of the poorly-designed network interface, it is.  Because it is, we need to override
			// it on demand.
			bool pak_overrides = env_t::default_settings.get_pak_overrides_savegame_settings();

			// First cityrules
			stadt_t::cityrules_rdwr(file);
			if (  !env_t::networkmode || env_t::server  ) {
				if (pak_overrides) {
					chdir( env_t::program_dir );
					printf("stadt_t::cityrules_init in pak dir (%s) for override of save file: ", env_t::objfilename.c_str() );
					stadt_t::cityrules_init( env_t::objfilename );
					chdir( env_t::user_dir );
				}
			}

			// Next privatecar and electricity
			if(file->get_extended_version() >= 9)
			{
				privatecar_rdwr(file);
				stadt_t::electricity_consumption_rdwr(file);
				if(!env_t::networkmode || env_t::server) 
				{
					if(pak_overrides) 
					{
						chdir(env_t::program_dir);
						printf("stadt_t::privatecar_init in pak dir (%s) for override of save file: ", env_t::objfilename.c_str());
						privatecar_init(env_t::objfilename);
						printf("stadt_t::electricity_consumption_init in pak dir (%s) for override of save file: ", env_t::objfilename.c_str());
						stadt_t::electricity_consumption_init(env_t::objfilename);
						chdir(env_t::user_dir);
					}
				}
			}

			// Finally speedbonus
			if(file->get_extended_version() < 13 && file->get_extended_revision() < 24 && file->get_version()>102003 && (file->get_extended_version() == 0 || file->get_extended_version() >= 9)) 
			{
				// Retained for save game compatibility with older games saved with versions that still had the speed bonus.
				vehicle_builder_t::rdwr_speedbonus(file);
			}
		}
	}
	DBG_DEBUG("karte_t::load", "init %i cities", settings.get_city_count());
	stadt.clear();
	stadt.resize(settings.get_city_count());
	for (int i = 0; i < settings.get_city_count(); ++i) {
		stadt_t *s = new stadt_t(file);
		const sint32 population = s->get_einwohner();
		stadt.append(s, population > 0 ? population : 1); // This has to be at least 1, or else the weighted vector will not add it. TODO: Remove this check once the population checking method is improved.
	}

	DBG_MESSAGE("karte_t::load()","loading blocks");
	old_blockmanager_t::rdwr(this, file);

	DBG_MESSAGE("karte_t::load()","loading tiles");
	for (int y = 0; y < get_size().y; y++) {
		for (int x = 0; x < get_size().x; x++) {
			plan[x+y*cached_grid_size.x].rdwr(file, koord(x,y) );
		}
		if(file->is_eof()) {
			dbg->fatal("karte_t::load()","Savegame file mangled (too short)!");
		}
		ls.set_progress( y/2 );
	}

	if(file->get_version()<99005) {
		DBG_MESSAGE("karte_t::load()","loading grid for older versions");
		for (int y = 0; y <= get_size().y; y++) {
			for (int x = 0; x <= get_size().x; x++) {
				sint32 hgt;
				file->rdwr_long(hgt);
				// old height step was 16!
				set_grid_hgt(x, y, hgt/16 );
			}
		}
	}
	else if(  file->get_version()<=102001  )  {
		// hgt now bytes
		DBG_MESSAGE("karte_t::load()","loading grid for older versions");
		for( sint32 i=0;  i<(get_size().y+1)*(sint32)(get_size().x+1);  i++  ) {
			file->rdwr_byte(grid_hgts[i]);
		}
	}

	if(file->get_version()<88009) {
		DBG_MESSAGE("karte_t::load()","loading slopes from older version");
		// Hajo: load slopes for older versions
		// now part of the grund_t structure
		for (int y = 0; y < get_size().y; y++) {
			for (int x = 0; x < get_size().x; x++) {
				sint8 slope;
				file->rdwr_byte(slope);
				// convert slopes from old single height saved game
				slope = (scorner_sw(slope) + scorner_se(slope) * 3 + scorner_ne(slope) * 9 + scorner_nw(slope) * 27) * env_t::pak_height_conversion_factor;
				access_nocheck(x, y)->get_kartenboden()->set_grund_hang(slope);
			}
		}
	}

	if(file->get_version()<=88000) {
		// because from 88.01.4 on the foundations are handled differently
		for (int y = 0; y < get_size().y; y++) {
			for (int x = 0; x < get_size().x; x++) {
				koord k(x,y);
				grund_t *gr = access_nocheck(x, y)->get_kartenboden();
				if(  gr->get_typ()==grund_t::fundament  ) {
					gr->set_hoehe( max_hgt_nocheck(k) );
					gr->set_grund_hang( slope_t::flat );
					// transfer object to on new grund
					for(  int i=0;  i<gr->get_top();  i++  ) {
						gr->obj_bei(i)->set_pos( gr->get_pos() );
					}
				}
			}
		}
	}

	if(  file->get_version() < 112007  ) {
		// set climates
		for(  sint16 y = 0;  y < get_size().y;  y++  ) {
			for(  sint16 x = 0;  x < get_size().x;  x++  ) {
				calc_climate( koord( x, y ), false );
			}
		}
	}

	// Reliefkarte an neue welt anpassen
	DBG_MESSAGE("karte_t::load()", "init relief");
	win_set_world( this );
	reliefkarte_t::get_karte()->init();

	sint32 fabs;
	file->rdwr_long(fabs);
	DBG_MESSAGE("karte_t::load()", "prepare for %i factories", fabs);

	for(sint32 i = 0; i < fabs; i++) {
		// list in gleicher rownfolge wie vor dem speichern wieder aufbauen
		fabrik_t *fab = new fabrik_t(file);
		if(fab->get_desc()) {
			fab_list.append(fab);
		}
		else {
			dbg->error("karte_t::load()","Unknown factory skipped!");
			delete fab;
		}
		if(i&7) {
			ls.set_progress( get_size().y/2+(128*i)/fabs );
		}
	}

	// load linemanagement status (and lines)
	// @author hsiegeln
	if (file->get_version() > 82003  &&  file->get_version()<88003) {
		DBG_MESSAGE("karte_t::load()", "load linemanagement");
		get_player(0)->simlinemgmt.rdwr(file, get_player(0));
	}
	// end load linemanagement

	DBG_MESSAGE("karte_t::load()", "load stops");
	// now load the stops
	// (the players will be load later and overwrite some values,
	//  like the total number of stops build (for the numbered station feature)
	haltestelle_t::start_load_game();
	if(file->get_version()>=99008) {
		sint32 halt_count;
		file->rdwr_long(halt_count);
		DBG_MESSAGE("karte_t::load()","%d halts loaded",halt_count);
		for(int i=0; i<halt_count; i++) {
			halthandle_t halt = haltestelle_t::create( file );
			if(!halt->existiert_in_welt()) {
				dbg->warning("karte_t::load()", "could not restore stop near %i,%i", halt->get_init_pos().x, halt->get_init_pos().y );
			}
			ls.set_progress( get_size().y/2+128+(get_size().y*i)/(2*halt_count) );
		}
		DBG_MESSAGE("karte_t::load()","%d halts loaded",halt_count);
	}

	DBG_MESSAGE("karte_t::load()", "load convois");
	uint16 convoi_nr = 65535;
	uint16 max_convoi = 65535;
	if(  file->get_version()>=101000  ) {
		file->rdwr_short(convoi_nr);
		max_convoi = convoi_nr;
	}
	while(  convoi_nr-->0  ) {

		if(  file->get_version()<101000  ) {
			file->rd_obj_id(buf, 79);
			if (strcmp(buf, "Ende Convois") == 0) {
				break;
			}
		}
		convoi_t *cnv = new convoi_t(file);
		convoi_array.append(cnv->self);

		if(cnv->in_depot()) {
			grund_t * gr = lookup(cnv->get_pos());
			depot_t *dep = gr ? gr->get_depot() : 0;
			if(dep) {
				//cnv->enter_depot(dep);
				dep->convoi_arrived(cnv->self, false);
			}
			else {
				dbg->error("karte_t::load()", "no depot for convoi, blocks may now be wrongly reserved!");
				cnv->destroy();
			}
		}
		else {
			sync.add( cnv );
		}
		if(  (convoi_array.get_count()&7) == 0  ) {
			ls.set_progress( get_size().y+(get_size().y*convoi_array.get_count())/(2*max_convoi)+128 );
		}
	}
DBG_MESSAGE("karte_t::load()", "%d convois/trains loaded", convoi_array.get_count());

	// now the player can be loaded
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(  players[i]  ) {
			players[i]->rdwr(file);
			settings.player_active[i] = players[i]->is_active();
		}
		else {
			settings.player_active[i] = false;
		}
		ls.set_progress( (get_size().y*3)/2+128+8*i );
	}
DBG_MESSAGE("karte_t::load()", "players loaded");

	// loading messages
	if(  file->get_version()>=102005  ) {
		msg->rdwr(file);
	}
	else if(  !env_t::networkmode  ) {
		msg->clear();
	}
DBG_MESSAGE("karte_t::load()", "messages loaded");

	// nachdem die welt jetzt geladen ist koennen die Blockstrecken neu
	// angelegt werden
	old_blockmanager_t::finish_rd(this);
	DBG_MESSAGE("karte_t::load()", "blocks loaded");

	sint32 mi,mj;
	file->rdwr_long(mi);
	file->rdwr_long(mj);
	DBG_MESSAGE("karte_t::load()", "Setting view to %d,%d", mi,mj);
	viewport->change_world_position( koord3d(mi,mj,0) );

	// right season for recalculations
	recalc_season_snowline(false);

DBG_MESSAGE("karte_t::load()", "%d ways loaded",weg_t::get_alle_wege().get_count());

	ls.set_progress( (get_size().y*3)/2+256 );

	world_xy_loop(&karte_t::plans_finish_rd, SYNCX_FLAG);

	if(  file->get_version() < 112007  ) {
		// set transitions - has to be done after plans_finish_rd
		world_xy_loop(&karte_t::recalc_transitions_loop, 0);
	}

	ls.set_progress( (get_size().y*3)/2+256+get_size().y/8 );

DBG_MESSAGE("karte_t::load()", "laden_abschliesen for tiles finished" );

	// must finish loading cities first before cleaning up factories
	weighted_vector_tpl<stadt_t*> new_weighted_stadt(stadt.get_count() + 1);
	FOR(weighted_vector_tpl<stadt_t*>, const s, stadt) {
		s->finish_rd();
		new_weighted_stadt.append(s, s->get_einwohner());
		INT_CHECK("simworld 1278");
	}
	swap(stadt, new_weighted_stadt);
	DBG_MESSAGE("karte_t::load()", "cities initialized");

	ls.set_progress( (get_size().y*3)/2+256+get_size().y/4 );

	DBG_MESSAGE("karte_t::load()", "clean up factories");
	FOR(vector_tpl<fabrik_t*>, const f, fab_list) {
		f->finish_rd();
	}

DBG_MESSAGE("karte_t::load()", "%d factories loaded", fab_list.get_count());

	ls.set_progress( (get_size().y*3)/2+256+get_size().y/3 );

	// resolve dummy stops into real stops first ...
	FOR(vector_tpl<halthandle_t>, const i, haltestelle_t::get_alle_haltestellen()) {
		if (i->get_owner() && i->existiert_in_welt()) {
			i->finish_rd(file->get_extended_version() < 10);
		}
	}

	// ... before removing dummy stops
	for(  vector_tpl<halthandle_t>::const_iterator i=haltestelle_t::get_alle_haltestellen().begin(); i!=haltestelle_t::get_alle_haltestellen().end();  ) {
		halthandle_t const h = *i;
		if (!h->get_owner() || !h->existiert_in_welt()) {
			// this stop was only needed for loading goods ...
			haltestelle_t::destroy(h);	// remove from list
		}
		else {
				++i;
		}
	}

	ls.set_progress( (get_size().y*3)/2+256+(get_size().y*3)/8 );

	// adding lines and other stuff for convois
	for(unsigned i=0;  i<convoi_array.get_count();  i++ ) {
		convoihandle_t cnv = convoi_array[i];
		cnv->finish_rd();
		// was deleted during loading => use same position again
		if(!cnv.is_bound()) {
			i--;
		}
	}
	haltestelle_t::end_load_game();

	// register all line stops and change line types, if needed
	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		if(  players[i]  ) {
			players[i]->load_finished();
		}
	}


#if 0
	// reroute goods for benchmarking
	dt = dr_time();
	FOR(vector_tpl<halthandle_t>, const i, haltestelle_t::get_alle_haltestellen()) {
		sint16 dummy = 0x7FFF;
		i->reroute_goods(dummy);
	}
	DBG_MESSAGE("reroute_goods()","for all haltstellen_t took %ld ms", dr_time()-dt );
#endif

	// load history/create world history
	if(file->get_version()<99018) {
		restore_history();
	}
	else 
	{
		for(int year = 0; year < MAX_WORLD_HISTORY_YEARS; year++) 
		{
			for(int cost_type = 0; cost_type < MAX_WORLD_COST; cost_type++) 
			{
				if(file->get_extended_version() < 12 && (cost_type == WORLD_JOBS || cost_type == WORLD_VISITOR_DEMAND || cost_type == WORLD_CAR_OWNERSHIP))
				{
					finance_history_year[year][cost_type] = 0;
				}
				else
				{
					file->rdwr_longlong(finance_history_year[year][cost_type]);
				}
			}
		}
		for(int month = 0; month < MAX_WORLD_HISTORY_MONTHS; month++) 
		{
			for(int cost_type = 0; cost_type < MAX_WORLD_COST; cost_type++) 
			{
				if(file->get_extended_version() < 12 && (cost_type == WORLD_JOBS || cost_type == WORLD_VISITOR_DEMAND || cost_type == WORLD_CAR_OWNERSHIP))
				{
					finance_history_year[month][cost_type] = 0;
				}
				else
				{
					file->rdwr_longlong(finance_history_month[month][cost_type]);
				}
			}
		}
		last_month_bev = finance_history_month[1][WORLD_CITICENS];
	}

	// finally: do we run a scenario?
	if(file->get_version()>=99018) {
		scenario->rdwr(file);
	}

	// restore locked state
	// network game this will be done in nwc_sync_t::do_command
	if(  !env_t::networkmode  ) {
		for(  uint8 i=0;  i<PLAYER_UNOWNED;  i++  ) {
			if(  players[i]  ) {
				players[i]->check_unlock( player_password_hash[i] );
			}
		}
	}

	// initialize lock info for local server player
	// if call from sync command, lock info will be corrected there
	if(  env_t::server) {
		nwc_auth_player_t::init_player_lock_server(this);
	}

	if(file->get_extended_version() >= 2)
	{
		file->rdwr_short(base_pathing_counter);
	}
	
	if((file->get_extended_version() >= 7 && file->get_extended_version() < 9 && file->get_version() < 110006))
	{
		double old_proportion = industry_density_proportion / 10000.0;
		file->rdwr_double(old_proportion);
		industry_density_proportion = old_proportion * 10000.0;
	}
	else if(file->get_extended_version() >= 9 && file->get_version() >= 110006)
	{
		if(file->get_extended_version() >= 11)
		{
			file->rdwr_long(industry_density_proportion);
		}
		else
		{
			uint32 idp = 0;
			file->rdwr_long(idp);
			idp = (idp & 0x8000) != 0 ? idp & 0x7FFF : idp * 150;
			industry_density_proportion = idp;
		}
	}
	else if(file->is_loading())
	{
		// Reconstruct the actual industry density.
		// @author: jamespetts			
		// Loading a game - must set this to zero here and recalculate.
		actual_industry_density = 0;
		uint32 weight;
		FOR(vector_tpl<fabrik_t*>, factory, fab_list)
		{
			const factory_desc_t* factory_type = factory->get_desc();
			if(!factory_type->is_electricity_producer())
			{
				// Power stations are excluded from the target weight:
				// a different system is used for them.
				weight = max(factory_type->get_distribution_weight(), 1); // To prevent divisions by zero
				actual_industry_density += (100 / weight);
			}
		}
		industry_density_proportion = ((sint64)actual_industry_density * 10000ll) / finance_history_month[0][WORLD_CITICENS];
	}

	if(file->get_extended_version() >=9 && file->get_version() >= 110000)
	{
		if(file->get_extended_version() < 11)
		{
			// Was next_private_car_update_month
			uint8 dummy;
			file->rdwr_byte(dummy);
		}
		
		// Existing values now saved in order to prevent network desyncs
		file->rdwr_long(citycar_speed_average);
		file->rdwr_bool(recheck_road_connexions);
		if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 14)
		{
			file->rdwr_long(generic_road_time_per_tile_city);
			file->rdwr_long(generic_road_time_per_tile_intercity);
		}
		else
		{
			uint16 tmp = generic_road_time_per_tile_city < UINT32_MAX_VALUE ? (uint16)generic_road_time_per_tile_city : 65535;
			file->rdwr_short(tmp);
			if (tmp == 65535)
			{
				generic_road_time_per_tile_city = UINT32_MAX_VALUE;
			}
			else
			{
				generic_road_time_per_tile_city = (uint32)tmp;
			}

			tmp = generic_road_time_per_tile_intercity < UINT32_MAX_VALUE ? (uint16)generic_road_time_per_tile_intercity : 65535;
			file->rdwr_short(tmp);
			if (tmp == 65535)
			{
				generic_road_time_per_tile_intercity = UINT32_MAX_VALUE;
			}
			else
			{
				generic_road_time_per_tile_intercity = (uint32)tmp;
			}
		}
		file->rdwr_long(max_road_check_depth);
		if(file->get_extended_version() < 10)
		{
			double old_density = actual_industry_density / 100.0;
			file->rdwr_double(old_density);
			actual_industry_density = old_density * 100.0;
		}
		else
		{
			file->rdwr_long(actual_industry_density);
		}
		if(fab_list.empty() && file->get_version() < 111100)
		{
			// Correct some older saved games where the actual industry density was over-stated.
			actual_industry_density = 0;
		}
	}

	if(file->get_extended_version() >= 12)
	{
#ifdef MULTI_THREAD
		pthread_mutex_lock(&step_passengers_and_mail_mutex);
#endif
		file->rdwr_long(next_step_passenger);
		file->rdwr_long(next_step_mail);

#ifdef MULTI_THREAD
		pthread_mutex_unlock(&step_passengers_and_mail_mutex);
#endif

		if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 13)
		{
			if (env_t::networkmode)
			{
				if (env_t::server)
				{
					sint32 po = env_t::num_threads - 1;
					file->rdwr_long(po);
					parallel_operations = 0;
				}
				else
				{
					file->rdwr_long(parallel_operations);
				}
			}
			else
			{
				sint32 dummy;
				file->rdwr_long(dummy);
				parallel_operations = -1;
			}
		}
	}

#ifdef MULTI_THREAD
	destroy_threads();
	init_threads();
#else
	delete[] transferring_cargoes;
	transferring_cargoes = new vector_tpl<transferring_cargo_t>[1];
#endif

	if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 15)
	{
		uint32 count = 0;
		sint64 ready;
		ware_t ware;

		file->rdwr_long(count);

		for (uint32 i = 0; i < count; i++)
		{
			file->rdwr_longlong(ready);
			ware.rdwr(file);

			transferring_cargo_t tc;
			tc.ready_time = ready;
			tc.ware = ware;
			// On re-loading, there is no need to distribute the
			// cargoes about different members of this array.
			transferring_cargoes[0].append(tc);
			fabrik_t* fab = fabrik_t::get_fab(tc.ware.get_zielpos());
			if (fab)
			{
				fab->update_transit(tc.ware, true);
			}
		}
	}

	// show message about server
	if(  file->get_version() >= 112008  ) {
		xml_tag_t t( file, "motd_t" );
		char msg[32766];
		file->rdwr_str( msg, 32766 );
		if(  *msg  &&  !env_t::server  ) {
			// if not empty ...
			help_frame_t *win = new help_frame_t();
			win->set_text( msg );
			create_win(win, w_info, magic_motd);
		}
	}

	vehicle_t::sound_ticks = 0;

	// MUST be at the end of the load/save routine.
	if(  file->get_version()>=102004  ) {
		if(  env_t::restore_UI  ) {
			file->rdwr_byte( active_player_nr );
			active_player = players[active_player_nr];
			/* restore all open windows
			 * otherwise it will be ignored
			 * which is save, since it is the end of file
			 */
			rdwr_all_win( file );
		}
	}

	// Check attractions' road connexions
	FOR(weighted_vector_tpl<gebaeude_t*>, const &i, world_attractions)
	{
		i->check_road_tiles(false);
	}

	// Added by : Knightly
	path_explorer_t::full_instant_refresh();

	file->set_buffered(false);
	clear_random_mode(LOAD_RANDOM);

	// loading finished, reset savegame version to current
	load_version = loadsave_t::int_version( env_t::savegame_version_str, NULL, NULL );

	FOR(slist_tpl<depot_t *>, const dep, depot_t::get_depot_list())
	{
		// This must be done here, as the cities have not been initialised on loading.
		dep->add_to_world_list();
	}

	pedestrian_t::check_timeline_pedestrians();

	dbg->warning("karte_t::load()","loaded savegame from %i/%i, next month=%i, ticks=%i (per month=1<<%i)",last_month,last_year,next_month_ticks,ticks,karte_t::ticks_per_world_month_shift);
}


// recalcs all ground tiles on the map
void karte_t::update_map_intern(sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max)
{
	if(  (loaded_rotation + settings.get_rotation()) & 1  ) {  // 1 || 3  // ~14% faster loop blocking rotations 1 and 3
		const int LOOP_BLOCK = 128;
		for(  int xx = x_min;  xx < x_max;  xx += LOOP_BLOCK  ) {
			for(  int yy = y_min;  yy < y_max;  yy += LOOP_BLOCK  ) {
				for(  int y = yy;  y < min(yy + LOOP_BLOCK, y_max);  y++  ) {
					for(  int x = xx;  x < min(xx + LOOP_BLOCK, x_max);  x++  ) {
						const int nr = y * cached_grid_size.x + x;
						for(  uint i = 0;  i < plan[nr].get_boden_count();  i++  ) {
							plan[nr].get_boden_bei(i)->calc_image();
						}
					}
				}
			}
		}
	}
	else {
		for(  int y = y_min;  y < y_max;  y++  ) {
			for(  int x = x_min;  x < x_max;  x++  ) {
				const int nr = y * cached_grid_size.x + x;
				for(  uint i = 0;  i < plan[nr].get_boden_count();  i++  ) {
					plan[nr].get_boden_bei(i)->calc_image();
				}
			}
		}
	}
}


// recalcs all ground tiles on the map
void karte_t::update_map()
{
	DBG_MESSAGE( "karte_t::update_map()", "" );
	world_xy_loop(&karte_t::update_map_intern, SYNCX_FLAG);
	set_dirty();
}

/**
 * return an index to a halt
 * optionally limit to that owned by player player
 * by default create a new halt if none found
 * -- create_halt==true is used during loading of *old* saved games
 */
halthandle_t karte_t::get_halt_koord_index(koord k, player_t *player, bool create_halt)
{
	halthandle_t my_halt;

	// already there?
	// check through all the grounds
	const planquadrat_t* plan = access(k);
	if (plan)
	{
		//for(  uint8 i=0;  i < plan->get_boden_count();  i++  ) {
		//	halthandle_t my_halt = plan->get_boden_bei(i)->get_halt();
		//	if(  my_halt.is_bound()  ) {
		//		// Stop at first halt found (always prefer ground level)
		//		return my_halt;
		//	}
		//}
		// for compatibility with old code (see above) we do not pass player to get_halt():
		halthandle_t my_halt = plan->get_halt(NULL);
	}
	if( create_halt && !my_halt.is_bound() ) {
		// No halts found => create one
		my_halt = haltestelle_t::create( k, NULL );
	}
	return my_halt;
}

void karte_t::calc_climate(koord k, bool recalc)
{
	planquadrat_t *pl = access(k);
	if(  !pl  ) {
		return;
	}

	grund_t *gr = pl->get_kartenboden();
	if(  gr  ) {
		if(  !gr->is_water()  ) {
			bool beach = false;
			if(  gr->get_pos().z == groundwater  ) {
				for(  int i = 0;  i < 8 && !beach;  i++  ) {
					grund_t *gr2 = lookup_kartenboden( k + koord::neighbours[i] );
					if(  gr2 && gr2->is_water()  ) {
						beach = true;
					}
				}
			}
			pl->set_climate( beach ? desert_climate : get_climate_at_height( max( gr->get_pos().z, groundwater + 1 ) ) );
		}
		else {
			pl->set_climate( water_climate );
		}
		pl->set_climate_transition_flag(false);
		pl->set_climate_corners(0);
	}

	if(  recalc  ) {
		recalc_transitions(k);
		for(  int i = 0;  i < 8;  i++  ) {
			recalc_transitions( k + koord::neighbours[i] );
		}
	}
}


// fills array with neighbour heights
void karte_t::get_neighbour_heights(const koord k, sint8 neighbour_height[8][4]) const
{
	for(  int i = 0;  i < 8;  i++  ) { // 0 = nw, 1 = w etc.
		planquadrat_t *pl2 = access( k + koord::neighbours[i] );
		if(  pl2  ) {
			grund_t *gr2 = pl2->get_kartenboden();
			slope_t::type slope_corner = gr2->get_grund_hang();
			for(  int j = 0;  j < 4;  j++  ) {
				neighbour_height[i][j] = gr2->get_hoehe() + slope_corner % 3;
				slope_corner /= 3;
			}
		}
		else {
			switch(i) {
				case 0: // nw
					neighbour_height[i][0] = groundwater;
					neighbour_height[i][1] = max( lookup_hgt( k+koord(0,0) ), get_water_hgt( k ) );
					neighbour_height[i][2] = groundwater;
					neighbour_height[i][3] = groundwater;
				break;
				case 1: // w
					neighbour_height[i][0] = groundwater;
					neighbour_height[i][1] = max( lookup_hgt( k+koord(0,1) ), get_water_hgt( k ) );
					neighbour_height[i][2] = max( lookup_hgt( k+koord(0,0) ), get_water_hgt( k ) );
					neighbour_height[i][3] = groundwater;
				break;
				case 2: // sw
					neighbour_height[i][0] = groundwater;
					neighbour_height[i][1] = groundwater;
					neighbour_height[i][2] = max( lookup_hgt( k+koord(0,1) ), get_water_hgt( k ) );
					neighbour_height[i][3] = groundwater;
				break;
				case 3: // s
					neighbour_height[i][0] = groundwater;
					neighbour_height[i][1] = groundwater;
					neighbour_height[i][2] = max( lookup_hgt( k+koord(1,1) ), get_water_hgt( k ) );
					neighbour_height[i][3] = max( lookup_hgt( k+koord(0,1) ), get_water_hgt( k ) );
				break;
				case 4: // se
					neighbour_height[i][0] = groundwater;
					neighbour_height[i][1] = groundwater;
					neighbour_height[i][2] = groundwater;
					neighbour_height[i][3] = max( lookup_hgt( k+koord(1,1) ), get_water_hgt( k ) );
				break;
				case 5: // e
					neighbour_height[i][0] = max( lookup_hgt( k+koord(1,1) ), get_water_hgt( k ) );
					neighbour_height[i][1] = groundwater;
					neighbour_height[i][2] = groundwater;
					neighbour_height[i][3] = max( lookup_hgt( k+koord(1,0) ), get_water_hgt( k ) );
				break;
				case 6: // ne
					neighbour_height[i][0] = max( lookup_hgt( k+koord(1,0) ), get_water_hgt( k ) );
					neighbour_height[i][1] = groundwater;
					neighbour_height[i][2] = groundwater;
					neighbour_height[i][3] = groundwater;
				break;
				case 7: // n
					neighbour_height[i][0] = max( lookup_hgt( k+koord(0,0) ), get_water_hgt( k ) );
					neighbour_height[i][1] = max( lookup_hgt( k+koord(1,0) ), get_water_hgt( k ) );
					neighbour_height[i][2] = groundwater;
					neighbour_height[i][3] = groundwater;
				break;
			}

			/*neighbour_height[i][0] = groundwater;
			neighbour_height[i][1] = groundwater;
			neighbour_height[i][2] = groundwater;
			neighbour_height[i][3] = groundwater;*/
		}
	}
}


void karte_t::rotate_transitions(koord k)
{
	planquadrat_t *pl = access(k);
	if(  !pl  ) {
		return;
	}

	uint8 climate_corners = pl->get_climate_corners();
	if(  climate_corners != 0  ) {
		climate_corners = (climate_corners >> 1) | ((climate_corners & 1) << 3);
		pl->set_climate_corners( climate_corners );
	}
}


void karte_t::recalc_transitions_loop( sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max )
{
	for(  int y = y_min;  y < y_max;  y++  ) {
		for(  int x = x_min; x < x_max;  x++  ) {
			recalc_transitions( koord( x, y ) );
		}
	}
}


void karte_t::recalc_transitions(koord k)
{
	planquadrat_t *pl = access(k);
	if(  !pl  ) {
		return;
	}

	grund_t *gr = pl->get_kartenboden();
	if(  !gr->is_water()  ) {
		// get neighbour corner heights
		sint8 neighbour_height[8][4];
		get_neighbour_heights( k, neighbour_height );

		// look up neighbouring climates
		climate neighbour_climate[8];
		for(  int i = 0;  i < 8;  i++  ) { // 0 = nw, 1 = w etc.
			koord k_neighbour = k + koord::neighbours[i];
			if(  !is_within_limits(k_neighbour)  ) {
				k_neighbour = get_closest_coordinate(k_neighbour);
			}
			neighbour_climate[i] = get_climate( k_neighbour );
		}

		uint8 climate_corners = 0;
		climate climate0 = get_climate(k);

		slope_t::type slope_corner = gr->get_grund_hang();
		for(  uint8 i = 0;  i < 4;  i++  ) { // 0 = sw, 1 = se etc.
			// corner_sw (i=0): tests vs neighbour 1:w (corner 2 j=1),2:sw (corner 3) and 3:s (corner 4)
			// corner_se (i=1): tests vs neighbour 3:s (corner 3 j=2),4:se (corner 4) and 5:e (corner 1)
			// corner_ne (i=2): tests vs neighbour 5:e (corner 4 j=3),6:ne (corner 1) and 7:n (corner 2)
			// corner_nw (i=3): tests vs neighbour 7:n (corner 1 j=0),0:nw (corner 2) and 1:w (corner 3)
			sint8 corner_height = gr->get_hoehe() + slope_corner % 3;

			climate transition_climate = water_climate;
			climate min_climate = arctic_climate;

			for(  int j = 1;  j < 4;  j++  ) {
				if(  corner_height == neighbour_height[(i * 2 + j) & 7][(i + j) & 3]) {
					climate climatej = neighbour_climate[(i * 2 + j) & 7];
					climatej > transition_climate ? transition_climate = climatej : 0;
					climatej < min_climate ? min_climate = climatej : 0;
				}
			}

			if(  min_climate == water_climate  ||  transition_climate > climate0  ) {
				climate_corners |= 1 << i;
			}
			slope_corner /= 3;
		}
		pl->set_climate_transition_flag( climate_corners != 0 );
		pl->set_climate_corners( climate_corners );
	}
	gr->calc_image();
}


void karte_t::create_grounds_loop( sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max )
{
	for(  int y = y_min;  y < y_max;  y++  ) {
		for(  int x = x_min; x < x_max;  x++  ) {
			koord k(x,y);
			access_nocheck(k)->kartenboden_setzen( new boden_t( koord3d( x, y, max(min_hgt_nocheck(k),get_water_hgt_nocheck(k)) ), 0 ) );
		}
	}
}


uint8 karte_t::sp2num(player_t *player)
{
	if(  player==NULL  ) {
		return PLAYER_UNOWNED;
	}
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(players[i] == player) {
			return i;
		}
	}
	dbg->fatal( "karte_t::sp2num()", "called with an invalid player!" );
}


void karte_t::load_heightfield(settings_t* const sets)
{
	sint16 w, h;
	sint8 *h_field;
	height_map_loader_t hml(sets);
	if(hml.get_height_data_from_file(sets->heightfield.c_str(), (sint8)(sets->get_groundwater()), h_field, w, h, false )) {
		sets->set_groesse(w,h);
		// create map
		init(sets,h_field);
		delete [] h_field;
	}
	else {
		dbg->error("karte_t::load_heightfield()","Cant open file '%s'", sets->heightfield.c_str());
		create_win( new news_img("\nCan't open heightfield file.\n"), w_info, magic_none );
	}
}


void karte_t::mark_area( const koord3d pos, const koord size, const bool mark ) const
{
	for( sint16 y=pos.y;  y<pos.y+size.y;  y++  ) {
		for( sint16 x=pos.x;  x<pos.x+size.x;  x++  ) {
			grund_t *gr = lookup( koord3d(x,y,pos.z));
			if (!gr) {
				gr = lookup_kartenboden( x,y );
			}
			if(gr) {
				if(mark) {
					gr->set_flag(grund_t::marked);
				}
				else {
					gr->clear_flag(grund_t::marked);
				}
				gr->set_flag(grund_t::dirty);
			}
		}
	}
}


void karte_t::reset_timer()
{
	// Reset timers
	long last_tick_sync = dr_time();
	mouse_rest_time = last_tick_sync;
	sound_wait_time = AMBIENT_SOUND_INTERVALL;
	intr_set_last_time(last_tick_sync);

	if(  env_t::networkmode  &&  (step_mode&PAUSE_FLAG)==0  ) {
		step_mode = FIX_RATIO;
	}

	last_step_time = last_interaction = last_tick_sync;
	last_step_ticks = ticks;

	// reinit simloop counter
	for(  int i=0;  i<32;  i++  ) {
		last_step_nr[i] = steps;
	}

	if(  step_mode&PAUSE_FLAG  ) {
		intr_disable();
	}
	else if(step_mode==FAST_FORWARD) {
		next_step_time = last_tick_sync+1;
		idle_time = 0;
		set_frame_time( 100 );
		time_multiplier = 16;
		intr_enable();
	}
	else if(step_mode==FIX_RATIO) {
		last_frame_idx = 0;
		fix_ratio_frame_time = 1000 / clamp(settings.get_frames_per_second(), 5, 100);
		next_step_time = last_tick_sync + fix_ratio_frame_time;
		set_frame_time( fix_ratio_frame_time );
		intr_disable();
		// other stuff needed to synchronize
		tile_counter = 0;
		pending_season_change = 1;
		pending_snowline_change = 1;
	}
	else {
		// make timer loop invalid
		for( int i=0;  i<32;  i++ ) {
			last_frame_ms[i] = 0x7FFFFFFFu;
			last_step_nr[i] = 0xFFFFFFFFu;
		}
		last_frame_idx = 0;
		simloops = 60;

		set_frame_time( 1000/env_t::fps );
		next_step_time = last_tick_sync+(3200/get_time_multiplier() );
		intr_enable();
	}
	DBG_MESSAGE("karte_t::reset_timer()","called, mode=$%X", step_mode);
}


void karte_t::reset_interaction()
{
	last_interaction = dr_time();
}


void karte_t::set_map_counter(uint32 new_map_counter)
{
	map_counter = new_map_counter;
	if(  env_t::server  ) {
		nwc_ready_t::append_map_counter(map_counter);
	}
}


uint32 karte_t::generate_new_map_counter() const
{
	return (uint32)dr_time();
}


// jump one year ahead
// (not updating history!)
void karte_t::step_year()
{
	DBG_MESSAGE("karte_t::step_year()","called");
//	ticks += 12*karte_t::ticks_per_world_month;
//	next_month_ticks += 12*karte_t::ticks_per_world_month;
	current_month += 12;
	last_year ++;
	reset_timer();
	recalc_average_speed();
}


// jump one or more months ahead
// (updating history!)
void karte_t::step_month( sint16 months )
{
	while(  months-->0  ) {
		new_month();
	}
	reset_timer();
}


void karte_t::change_time_multiplier(sint32 delta)
{
	time_multiplier += delta;
	if(time_multiplier<=0) {
		time_multiplier = 1;
	}
	if(step_mode!=NORMAL) {
		step_mode = NORMAL;
		reset_timer();
	}
}


void karte_t::set_pause(bool p)
{
	bool pause = step_mode&PAUSE_FLAG;
	if(p!=pause) {
		step_mode ^= PAUSE_FLAG;
		if(p) {
			intr_disable();
		}
		else {
			reset_timer();
		}
	}
}


void karte_t::set_fast_forward(bool ff)
{
	if(  !env_t::networkmode  ) {
		if(  ff  ) {
			if(  step_mode==NORMAL  ) {
				step_mode = FAST_FORWARD;
				reset_timer();
			}
		}
		else {
			if(  step_mode==FAST_FORWARD  ) {
				step_mode = NORMAL;
				reset_timer();
			}
		}
	}
}


koord karte_t::get_closest_coordinate(koord outside_pos)
{
	outside_pos.clip_min(koord(0,0));
	outside_pos.clip_max(koord(get_size().x-1,get_size().y-1));

	return outside_pos;
}


/* creates a new player with this type */
const char *karte_t::init_new_player(uint8 new_player_in, uint8 type)
{
	if(  new_player_in>=PLAYER_UNOWNED  ||  get_player(new_player_in)!=NULL  ) {
		return "Id invalid/already in use!";
	}
	switch( type ) {
		case player_t::EMPTY: break;
		case player_t::HUMAN: players[new_player_in] = new player_t(this,new_player_in); break;
		case player_t::AI_GOODS: players[new_player_in] = new ai_goods_t(this,new_player_in); break;
		case player_t::AI_PASSENGER: players[new_player_in] = new ai_passenger_t(this,new_player_in); break;
		default: return "Unknow AI type!";
	}
	settings.set_player_type(new_player_in, type);
	return NULL;
}


void karte_t::remove_player(uint8 player_nr)
{
	if ( player_nr!=1  &&  player_nr<PLAYER_UNOWNED  &&  players[player_nr]!=NULL) {
		players[player_nr]->ai_bankrupt();
		delete players[player_nr];
		players[player_nr] = 0;
		nwc_chg_player_t::company_removed(player_nr);
		// if default human, create new instace of it (to avoid crashes)
		if(  player_nr == 0  ) {
			players[0] = new player_t( this, 0 );
		}

		// Reset all access rights
		for(sint32 i = 0; i < MAX_PLAYER_COUNT; i++)
		{
			if(players[i] != NULL && i != player_nr)
			{
				players[i]->set_allow_access_to(player_nr, i == 1); // Public player (no. 1) allows access by default, others do not allow by default.
			}
		}

		// if currently still active => reset to default human
		if(  player_nr == active_player_nr  ) {
			active_player_nr = 0;
			active_player = players[0];
			if(  !env_t::server  ) {
				create_win( display_get_width()/2-128, 40, new news_img("Bankrott:\n\nDu bist bankrott.\n"), w_info, magic_none);
			}
		}
	}
}


/* goes to next active player */
void karte_t::switch_active_player(uint8 new_player, bool silent)
{
	for(  uint8 i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  players[(i+new_player)%MAX_PLAYER_COUNT] != NULL  ) {
			new_player = (i+new_player)%MAX_PLAYER_COUNT;
			break;
		}
	}
	koord3d old_zeiger_pos = zeiger->get_pos();

	// no cheating allowed?
	if (!settings.get_allow_player_change() && players[1]->is_locked()) {
		active_player_nr = 0;
		active_player = players[0];
		if(new_player!=0) {
			create_win( new news_img("On this map, you are not\nallowed to change player!\n"), w_time_delete, magic_none);
		}
	}
	else {
		zeiger->change_pos( koord3d::invalid ); // unmark area
		// exit active tool to remove pointers (for two_click_tool_t's, stop mover, factory linker)
		if(selected_tool[active_player_nr]) {
			selected_tool[active_player_nr]->exit(active_player);
		}
		active_player_nr = new_player;
		active_player = players[new_player];
		if(  !silent  ) {
			// tell the player
			cbuffer_t buf;
			buf.printf( translator::translate("Now active as %s.\n"), get_active_player()->get_name() );
			msg->add_message(buf, koord::invalid, message_t::ai | message_t::local_flag, PLAYER_FLAG|get_active_player()->get_player_nr(), IMG_EMPTY);
		}

		// update menu entries
		tool_t::update_toolbars();
		set_dirty();
	}

	// update pointer image / area
	selected_tool[active_player_nr]->init_cursor(zeiger);
	// set position / mark area
	zeiger->change_pos( old_zeiger_pos );
}


void karte_t::stop(bool exit_game)
{
	finish_loop = true;
	env_t::quit_simutrans = exit_game;
}


void karte_t::network_game_set_pause(bool pause_, uint32 syncsteps_)
{
	if (env_t::networkmode) {
		time_multiplier = 16;// reset to normal speed
		sync_steps = syncsteps_;
		sync_steps_barrier = sync_steps;
		steps = sync_steps / settings.get_frames_per_step();
		network_frame_count = sync_steps % settings.get_frames_per_step();
		dbg->warning("karte_t::network_game_set_pause", "steps=%d sync_steps=%d pause=%d", steps, sync_steps, pause_);
		if (pause_) {
			if (!env_t::server) {
				reset_timer();
				step_mode = PAUSE_FLAG|FIX_RATIO;
			}
			else {
				// TODO
			}
		}
		else {
			step_mode = FIX_RATIO;
			reset_timer();
			if(  !env_t::server  ) {
				// allow server to run ahead the specified number of frames, plus an extra 50%. Better to catch up than be ahead.
				next_step_time = dr_time() + (settings.get_server_frames_ahead() + (uint32)env_t::additional_client_frames_behind) * fix_ratio_frame_time * 3 / 2;
			}
		}
	}
	else {
		set_pause(pause_);
	}
}

const char* karte_t::call_work(tool_t *tool, player_t *player, koord3d pos, bool &suspended)
{
	const char *err = NULL;
	if (!env_t::networkmode || tool->is_work_network_save() || tool->is_work_here_network_save(player, pos)) {
		// do the work
		tool->flags |= tool_t::WFL_LOCAL;
		// check allowance by scenario
		if ((tool->flags & tool_t::WFL_NO_CHK) == 0 && get_scenario()->is_scripted()) {
			if (!get_scenario()->is_tool_allowed(player, tool->get_id(), tool->get_waytype())) {
				err = "";
			}
			else {
				err = get_scenario()->is_work_allowed_here(player, tool->get_id(), tool->get_waytype(), pos);
			}
		}
		if (err == NULL) {
			err = tool->work(player, pos);
		}
		suspended = false;
	}
	else {
		// queue tool for network
		nwc_tool_t *nwc = new nwc_tool_t(player, tool, pos, get_steps(), get_map_counter(), false);
		network_send_server(nwc);
		suspended = true;
		// reset tool
		tool->init(player);
	}
	return err;
}


static slist_tpl<network_world_command_t*> command_queue;

void karte_t::command_queue_append(network_world_command_t* nwc) const
{
	slist_tpl<network_world_command_t*>::iterator i = command_queue.begin();
	slist_tpl<network_world_command_t*>::iterator end = command_queue.end();
	while(i != end  &&  network_world_command_t::cmp(*i, nwc)) {
		++i;
	}
	command_queue.insert(i, nwc);
}


void karte_t::clear_command_queue() const
{
	while (!command_queue.empty()) {
		delete command_queue.remove_first();
	}
}


static void encode_URI(cbuffer_t& buf, char const* const text)
{
	for (char const* i = text; *i != '\0'; ++i) {
		char const c = *i;
		if (('A' <= c && c <= 'Z') ||
				('a' <= c && c <= 'z') ||
				('0' <= c && c <= '9') ||
				c == '-' || c == '.' || c == '_' || c == '~') {
			char const two[] = { c, '\0' };
			buf.append(two);
		} else {
			buf.printf("%%%02X", (unsigned char)c);
		}
	}
}


void karte_t::process_network_commands(sint32 *ms_difference)
{
	// did we receive a new command?
	uint32 ms = dr_time();
	sint32 time_to_next_step = (sint32)next_step_time - (sint32)ms;
	network_command_t *nwc = network_check_activity( this, time_to_next_step > 0 ? min( time_to_next_step, 5) : 0 );
	if(  nwc==NULL  &&  !network_check_server_connection()  ) {
		dbg->warning("karte_t::process_network_commands", "lost connection to server");
		network_disconnect();
		return;
	}

	// process the received command
	while(  nwc  ) {
		// check timing
		uint16 const nwcid = nwc->get_id();
		if(  nwcid == NWC_CHECK  ||  nwcid == NWC_STEP  ) {
			// pull out server sync step
			const uint32 server_sync_step = nwcid == NWC_CHECK ? dynamic_cast<nwc_check_t *>(nwc)->server_sync_step : dynamic_cast<nwc_step_t *>(nwc)->get_sync_step();

			// are we on time?
			*ms_difference = 0;
			const uint32 timems = dr_time();
			const sint32 time_to_next = (sint32)next_step_time - (sint32)timems; // +'ve - still waiting for next,  -'ve - lagging
			const sint64 frame_timediff = ((sint64)server_sync_step - sync_steps - settings.get_server_frames_ahead() - env_t::additional_client_frames_behind) * fix_ratio_frame_time; // +'ve - server is ahead,  -'ve - client is ahead
			const sint64 timediff = time_to_next + frame_timediff;
			dbg->warning("NWC_CHECK", "time difference to server %lli", frame_timediff );

			if(  frame_timediff < (0 - (sint64)settings.get_server_frames_ahead() - (sint64)env_t::additional_client_frames_behind) * (sint64)fix_ratio_frame_time / 2  ) {
				// running way ahead - more than half margin, simply set next_step_time ahead to where it should be
				next_step_time = (sint64)timems - frame_timediff;
			}
			else if(  frame_timediff < 0  ) {
				// running ahead
				if(  time_to_next > -frame_timediff  ) {
					// already waiting longer than how far we're ahead, so set wait time shorter to the time ahead.
					next_step_time = (sint64)timems - frame_timediff;
			}
			else if(  nwcid == NWC_CHECK  ) {
					// gentle slowing down
					*ms_difference = timediff;
				}
			}
			else if(  frame_timediff > 0  ) {
				// running behind
				if(  time_to_next > (sint32)fix_ratio_frame_time / 4  ) {
					// behind but we're still waiting for the next step time - get going.
					next_step_time = timems;
					*ms_difference = frame_timediff;
				}
				else if(  nwcid == NWC_CHECK  ) {
					// gentle catching up
					*ms_difference = timediff;
				}
			}

			if(  sync_steps_barrier < server_sync_step  ) {
				sync_steps_barrier = server_sync_step;
			}
		}

		// check random number generator states
		if(  env_t::server  &&  nwcid  ==  NWC_TOOL  ) {
			nwc_tool_t *nwt = dynamic_cast<nwc_tool_t *>(nwc);
			if(  nwt->is_from_initiator()  ) {
				if(  nwt->last_sync_step>sync_steps  ) {
					dbg->warning("karte_t::process_network_commands", "client was too fast (skipping command)" );
					delete nwc;
					nwc = NULL;
				}
				// out of sync => drop client (but we can only compare if nwt->last_sync_step is not too old)
				else if(  is_checklist_available(nwt->last_sync_step)  &&  LCHKLST(nwt->last_sync_step)!=nwt->last_checklist  ) {
					// lost synchronisation -> server kicks client out actively
					char buf[256];
					const int offset = LCHKLST(nwt->last_sync_step).print(buf, "server");
					nwt->last_checklist.print(buf + offset, "initiator");
					dbg->warning("karte_t::process_network_commands", "kicking client due to checklist mismatch : sync_step=%u %s", nwt->last_sync_step, buf);
					socket_list_t::remove_client( nwc->get_sender() );
					delete nwc;
					nwc = NULL;
				}
			}
		}

		// execute command, append to command queue if necessary
		if(nwc  &&  nwc->execute(this)) {
			// network_world_command_t's will be appended to command queue in execute
			// all others have to be deleted here
			delete nwc;

		}
		// fetch the next command
		nwc = network_get_received_command();
	}
	uint32 next_command_step = get_next_command_step();

	// send data
	ms = dr_time();
	network_process_send_queues( next_step_time>ms ? min( next_step_time-ms, 5) : 0 );

	// process enqueued network world commands
	while(  !command_queue.empty()  &&  (next_command_step<=sync_steps/*  ||  step_mode&PAUSE_FLAG*/)  ) {
		network_world_command_t *nwc = command_queue.remove_first();
		if (nwc) {
			do_network_world_command(nwc);
			delete nwc;
		}
		next_command_step = get_next_command_step();
	}
}

void karte_t::do_network_world_command(network_world_command_t *nwc)
{
	// want to execute something in the past?
	if (nwc->get_sync_step() < sync_steps) {
		if (!nwc->ignore_old_events()) {
			dbg->warning("karte_t:::do_network_world_command", "wanted to do_command(%d) in the past", nwc->get_id());
			network_disconnect();
		}
	}
	// check map counter
	else if (nwc->get_map_counter() != map_counter) {
		dbg->warning("karte_t:::do_network_world_command", "wanted to do_command(%d) from another world", nwc->get_id());
	}
	// check random counter?
	else if(  nwc->get_id()==NWC_CHECK  ) {
		nwc_check_t* nwcheck = (nwc_check_t*)nwc;
		// this was the random number at the previous sync step on the server
		const checklist_t &server_checklist = nwcheck->server_checklist;
		const uint32 server_sync_step = nwcheck->server_sync_step;
		const checklist_t client_checklist = LCHKLST(server_sync_step);
		char buf[2048];
		const int offset = server_checklist.print(buf, "server");
		assert(offset < 2048);
		const int offset2 = offset + client_checklist.print(buf + offset, "client");
		assert(offset2 < 2048);
		dbg->warning("karte_t:::do_network_world_command", "sync_step=%u  %s", server_sync_step, buf);
		if(client_checklist != server_checklist) {
			dbg->warning("karte_t:::do_network_world_command", "disconnecting due to checklist mismatch" );
			network_disconnect();
		}
	}
	else {
		if(  nwc->get_id()==NWC_TOOL  ) {
			nwc_tool_t *nwt = dynamic_cast<nwc_tool_t *>(nwc);
			if(  is_checklist_available(nwt->last_sync_step)  &&  LCHKLST(nwt->last_sync_step)!=nwt->last_checklist  ) {
				// lost synchronisation ...
				char buf[2048];
				const int offset = nwt->last_checklist.print(buf, "server");
				assert(offset < 2048);
				const int offset2 = offset + LCHKLST(nwt->last_sync_step).print(buf + offset, "executor");
				assert(offset2 < 2048);
				dbg->warning("karte_t:::do_network_world_command", "skipping command due to checklist mismatch : sync_step=%u %s", nwt->last_sync_step, buf);
				if(  !env_t::server  ) {
					network_disconnect();
				}
			}
		}
		nwc->do_command(this);
	}
}

uint32 karte_t::get_next_command_step()
{
	// when execute next command?
	if(  !command_queue.empty()  ) {
		return command_queue.front()->get_sync_step();
	}
	else {
		return 0xFFFFFFFFu;
	}
}

sint16 karte_t::get_sound_id(grund_t *gr)
{
	if(  gr->ist_natur()  ||  gr->is_water()  ) {
		sint16 id = NO_SOUND;
		if(  gr->get_pos().z >= get_snowline()  ) {
			id = sound_desc_t::climate_sounds[ arctic_climate ];
		}
		else {
			id = sound_desc_t::climate_sounds[get_climate( zeiger->get_pos().get_2d() )];
		}
		if (id != NO_SOUND) {
			return id;
		}
		// try, if there is another sound ready
		if(  zeiger->get_pos().z==groundwater  &&  !gr->is_water()  ) {
			return sound_desc_t::beach_sound;
		}
		else if(  gr->get_top()>0  &&  gr->obj_bei(0)->get_typ()==obj_t::baum  ) {
			return sound_desc_t::forest_sound;
		}
	}
	return NO_SOUND;
}


bool karte_t::interactive(uint32 quit_month)
{

	finish_loop = false;
	sync_steps = 0;
	sync_steps_barrier = sync_steps;

	network_frame_count = 0;
	vector_tpl<uint16>hashes_ok;	// bit set: this client can do something with this player

	if(  !scenario->rdwr_ok()  ) {
		// error during loading of savegame of scenario
		create_win( new news_img( scenario->get_error_text() ), w_info, magic_none);
		scenario->stop();
	}
	// only needed for network
	if(  env_t::networkmode  ) {
		// clear the checklist history
		for(  int i=0;  i<LAST_CHECKLISTS_COUNT;  ++i  ) {
			last_checklists[i] = checklist_t();
		}
	}
	sint32 ms_difference = 0;
	reset_timer();
	DBG_DEBUG4("karte_t::interactive", "welcome in this routine");

	if(  env_t::server  ) {
		step_mode |= FIX_RATIO;

		reset_timer();
		// Announce server startup to the listing server
		if(  env_t::server_announce  ) {
			announce_server( 0 );
		}
	}

	DBG_DEBUG4("karte_t::interactive", "start the loop");
	do {
		// check for too much time eaten by frame updates ...
		if(  step_mode==NORMAL  ) {
			DBG_DEBUG4("karte_t::interactive", "decide to play a sound");
			last_interaction = dr_time();
			if(  mouse_rest_time+sound_wait_time < last_interaction  ) {
				// we play an ambient sound, if enabled
				grund_t *gr = lookup(zeiger->get_pos());
				if(  gr  ) {
					sint16 id = get_sound_id(gr);
					if(  id!=NO_SOUND  ) {
						sound_play(id);
					}
				}
				sound_wait_time *= 2;
			}
			DBG_DEBUG4("karte_t::interactive", "end of sound");
		}

		// check events queued since our last iteration
		eventmanager->check_events();

		if (env_t::quit_simutrans){
			break;
		}
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
		station_check("karte_t::interactive after win_poll_event", this);
#endif
#endif

		if(  env_t::networkmode  ) {
			process_network_commands(&ms_difference);

		}
		else {
			// we wait here for maximum 9ms
			// average is 5 ms, so we usually
			// are quite responsive
			DBG_DEBUG4("karte_t::interactive", "can I get some sleep?");
			INT_CHECK( "karte_t::interactive()" );
			const sint32 wait_time = (sint32)(next_step_time-dr_time());
			if(wait_time>0) {
				if(wait_time<10  ) {
					dr_sleep( wait_time );
				}
				else {
					dr_sleep( 9 );
				}
				INT_CHECK( "karte_t::interactive()" );
			}
			DBG_DEBUG4("karte_t::interactive", "end of sleep");
		}

		// time for the next step?
		uint32 time = dr_time(); // - (env_t::server ? 0 : 5000);
		if(  next_step_time<=time  ) {
			if (step_mode&PAUSE_FLAG ||
				(env_t::networkmode && !env_t::server  &&  sync_steps >= sync_steps_barrier)) {
				// only update display
				sync_step( 0, false, true );
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
				station_check("karte_t::interactive PAUSE after sync_step", this);
#endif
#endif
				idle_time = 100;
			}
			else {
				if(  step_mode==FAST_FORWARD  ) {
					sync_step( 100, true, false );
					set_random_mode( STEP_RANDOM );
					step();
					clear_random_mode( STEP_RANDOM );
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
					station_check("karte_t::interactive FAST_FORWARD after step", this);
#endif
#endif
				}
				else if(  step_mode==FIX_RATIO  ) {
					if(  env_t::server  ) {
						next_step_time += fix_ratio_frame_time;
					}
					else {
						const sint32 lag_time = (sint32)time - (sint32)next_step_time;
						if(  lag_time > 0  ) {
							ms_difference += lag_time;
							next_step_time = time;
						}

						const sint32 nst_diff = clamp( ms_difference, -fix_ratio_frame_time * 2, fix_ratio_frame_time * 8 ) / 10; // allows timerate between 83% and 500% of normal
						next_step_time += fix_ratio_frame_time - nst_diff;
						ms_difference -= nst_diff;
					}

					sync_step( (fix_ratio_frame_time*time_multiplier)/16, true, true );
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
					station_check("karte_t::interactive FIX_RATIO after sync_step", this);
#endif
#endif
					if (++network_frame_count == settings.get_frames_per_step()) {
						// ever Nth frame (default: every 4th - can be set in simuconf.tab)
						set_random_mode( STEP_RANDOM );
						step();
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
					station_check("karte_t::interactive FIX_RATIO after step", this);
#endif
#endif
						clear_random_mode( STEP_RANDOM );
						network_frame_count = 0;
					}
					sync_steps = steps * settings.get_frames_per_step() + network_frame_count;
					LCHKLST(sync_steps) = checklist_t(sync_steps, (uint32)steps, network_frame_count, get_random_seed(), halthandle_t::get_next_check(), linehandle_t::get_next_check(), convoihandle_t::get_next_check(),
						rands
					);

#ifdef DEBUG_SIMRAND_CALLS
					char buf[2048];
					const int offset = LCHKLST(sync_steps).print(buf, "chklist");
					assert(offset<2048);
					dbg->warning("karte_t::interactive", "sync_step=%u  %s", sync_steps, buf);
#endif

					// some server side tasks
					if(  env_t::networkmode  &&  env_t::server  ) {
						// broadcast sync info regularly and when lagged
						const sint64 timelag = (sint32)dr_time() - (sint32)next_step_time;
						if(  (network_frame_count == 0  &&  timelag > fix_ratio_frame_time * settings.get_server_frames_ahead() / 2)  ||  (sync_steps % env_t::server_sync_steps_between_checks) == 0  ) {
							if(  timelag > fix_ratio_frame_time * settings.get_frames_per_step()  ) {
								// log when server is lagged more than one step
								dbg->warning("karte_t::interactive", "server lagging by %lli", timelag );
							}

							nwc_check_t* nwc = new nwc_check_t(sync_steps + 1, map_counter, LCHKLST(sync_steps), sync_steps);
							network_send_all(nwc, true);
						}
						else {
							// broadcast sync_step
							nwc_step_t* nwcstep = new nwc_step_t(sync_steps, map_counter);
							network_send_all(nwcstep, true);
						}
					}
#if DEBUG>4
					if(  env_t::networkmode  &&  (sync_steps & 7)==0  &&  env_t::verbose_debug>4  ) {
						dbg->message("karte_t::interactive", "time=%lu sync=%d"/*  rand=%d"*/, dr_time(), sync_steps/*, LRAND(sync_steps)*/);
					}
#endif

					// no clients -> pause game
					if (  env_t::networkmode  &&  env_t::pause_server_no_clients  &&  socket_list_t::get_playing_clients() == 0  &&  !nwc_join_t::is_pending()  ) {
						set_pause(true);
					}
				}
				else { // Normal step mode
					INT_CHECK( "karte_t::interactive()" );
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
					station_check("karte_t::interactive else after INT_CHECK 1", this);
#endif
#endif
					set_random_mode( STEP_RANDOM );
					step();
#ifdef DEBUG_SIMRAND_CALLS
#ifdef STATION_CHECK
					station_check("karte_t::interactive else after step", this);
#endif
#endif
					clear_random_mode( STEP_RANDOM );
					idle_time = ((idle_time*7) + next_step_time - dr_time())/8;
					INT_CHECK( "karte_t::interactive()" );
				}
			}
		}

		// Interval-based server announcements
		if (  env_t::server  &&  env_t::server_announce  &&  env_t::server_announce_interval > 0  &&
			dr_time() >= server_last_announce_time + (uint32)env_t::server_announce_interval * 1000  ) {
			announce_server( 1 );
		}

		DBG_DEBUG4("karte_t::interactive", "point of loop return");
	} while(!finish_loop  &&  get_current_month()<quit_month);

	if(  env_t::quit_simutrans  &&  env_t::reload_and_save_on_quit  ) {
		// construct from pak name an autosave if requested
		std::string pak_name( "autosave-" );
		pak_name.append( env_t::objfilename );
		pak_name.erase( pak_name.length()-1 );
		pak_name.append( ".sve" );
		save( pak_name.c_str(), loadsave_t::autosave_mode, SERVER_SAVEGAME_VER_NR, EXTENDED_VER_NR, EXTENDED_REVISION_NR, false );
	}

	if(  get_current_month() >= quit_month  ) {
		env_t::quit_simutrans = true;
	}

	// On quit announce server as being offline
	if(  env_t::server  &&  env_t::server_announce  ) {
		announce_server( 2 );
	}

	intr_enable();
	display_show_pointer(true);
	return finish_loop;
#undef LRAND
}


// Announce server to central listing server
// Status is one of:
// 0 - startup
// 1 - interval
// 2 - shutdown
void karte_t::announce_server(int status)
{
	DBG_DEBUG( "announce_server()", "status: %i",  status );
	// Announce game info to server, format is:
	// st=on&dns=server.com&port=13353&rev=1234&pak=pak128&name=some+name&time=3,1923&size=256,256&active=[0-16]&locked=[0-16]&clients=[0-16]&towns=15&citizens=3245&factories=33&convoys=56&stops=17
	// (This is the data part of an HTTP POST)
	if(  env_t::server_announce  ) {
		cbuffer_t buf;
		// Always send dns and port as these are used as the unique identifier for the server
		buf.append( "&dns=" );
		encode_URI( buf, env_t::server_dns.c_str() );
		buf.printf( "&port=%u", env_t::server );
		// Always send announce interval to allow listing server to predict next announce
		buf.printf( "&aiv=%u", env_t::server_announce_interval );
		// Always send status, either online or offline
		if (  status == 0  ||  status == 1  ) {
			buf.append( "&st=1" );
#ifndef REVISION
#	define REVISION 0
#endif
			// Simple revision used for matching (integer)
			//buf.printf( "&rev=%d", atol( QUOTEME(REVISION) ) );
			buf.printf("&rev=%d", strtol(QUOTEME(REVISION), NULL, 16));
			// Complex version string used for display
			//buf.printf( "&ver=Simutrans %s (#%s) built %s", QUOTEME(VERSION_NUMBER), QUOTEME(REVISION), QUOTEME(VERSION_DATE) );
			// For some reason not yet clear, the above does not work. Attempt a simpler version for now, with only the revision.
			buf.printf("&ver=%x", strtol(QUOTEME(REVISION), NULL, 16));
			// Pakset version
			buf.append( "&pak=" );
			// Announce pak set, ideally get this from the copyright field of ground.Outside.pak
			char const* const copyright = ground_desc_t::outside->get_copyright();
			if (copyright && STRICMP("none", copyright) != 0) {
				// construct from outside object copyright string
				encode_URI( buf, copyright );
			}
			else {
				// construct from pak name
				std::string pak_name = env_t::objfilename;
				pak_name.erase( pak_name.length() - 1 );
				encode_URI( buf, pak_name.c_str() );
			}
			// TODO - change this to be the start date of the current map
			buf.printf( "&start=%u,%u", settings.get_starting_month() + 1, settings.get_starting_year() );
			// Add server name for listing
			buf.append( "&name=" );
			encode_URI( buf, env_t::server_name.c_str() );
			// Add server comments for listing
			buf.append( "&comments=" );
			encode_URI( buf, env_t::server_comments.c_str() );
			// Add server maintainer email for listing
			buf.append( "&email=" );
			encode_URI( buf, env_t::server_email.c_str() );
			// Add server pakset URL for listing
			buf.append( "&pakurl=" );
			encode_URI( buf, env_t::server_pakurl.c_str() );
			// Add server info URL for listing
			buf.append( "&infurl=" );
			encode_URI( buf, env_t::server_infurl.c_str() );

			// Now add the game data part
			uint8 active = 0, locked = 0;
			for(  uint8 i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
				if(  players[i]  &&  players[i]->get_ai_id()!=player_t::EMPTY  ) {
					active ++;
					if(  players[i]->is_locked()  ) {
						locked ++;
					}
				}
			}
			buf.printf( "&time=%u,%u",   (get_current_month() % 12) + 1, get_current_month() / 12 );
			buf.printf( "&size=%u,%u",   get_size().x, get_size().y );
			buf.printf( "&active=%u",    active );
			buf.printf( "&locked=%u",    locked );
			buf.printf( "&clients=%u",   socket_list_t::get_playing_clients() );
			buf.printf( "&towns=%u",     stadt.get_count() );
			buf.printf( "&citizens=%u",  stadt.get_sum_weight() );
			buf.printf( "&factories=%u", fab_list.get_count() );
			buf.printf( "&convoys=%u",   convoys().get_count());
			buf.printf( "&stops=%u",     haltestelle_t::get_alle_haltestellen().get_count() );
		}
		else {
			buf.append( "&st=0" );
		}

		network_http_post( ANNOUNCE_SERVER, ANNOUNCE_URL, buf, NULL );

		// Record time of this announce
		server_last_announce_time = dr_time();
	}
}


void karte_t::network_disconnect()
{
	// force disconnect
	dbg->warning("karte_t::network_disconnect()", "Lost synchronisation with server. Random flags: %d", get_random_mode());
	network_core_shutdown();
	destroy_all_win(true);

	clear_random_mode( INTERACTIVE_RANDOM );
	step_mode = NORMAL;
	reset_timer();
	clear_command_queue();
	create_win( display_get_width()/2-128, 40, new news_img("Lost synchronisation\nwith server."), w_info, magic_none);
	ticker::add_msg( translator::translate("Lost synchronisation\nwith server."), koord::invalid, COL_BLACK );
	last_active_player_nr = active_player_nr;

	stop(false);

#ifdef DEBUG_SIMRAND_CALLS
	print_randoms = false;
	printf("Lost synchronisation\nwith server.\n");
	ITERATE(karte_t::random_callers, n)
	{
		get_message()->add_message(random_callers.get_element(n), koord::invalid, message_t::ai);
	}
#endif
}

void karte_t::set_citycar_speed_average()
{
	if(private_car_t::table.empty())
	{
		// No city cars - use default speed.
		citycar_speed_average = 50;
		return;
	}
	sint32 vehicle_speed_sum = 0;
	sint32 count = 0;
	FOR(stringhashtable_tpl<const citycar_desc_t *>, const& iter, private_car_t::table)
	{
		// Take into account the *distribution_weight* of vehicles, too: fewer people have sports cars than Minis. 
		vehicle_speed_sum += (speed_to_kmh(iter.value->get_topspeed())) * iter.value->get_distribution_weight();
		count += iter.value->get_distribution_weight();
	}
	citycar_speed_average = vehicle_speed_sum / count;
}

void karte_t::calc_generic_road_time_per_tile_intercity()
{
	// This method is used only when private car connexion
	// checking is turned off.
	
	// Adapted from the method used to build city roads in the first place, written by Hajo.
	const way_desc_t* desc = settings.get_intercity_road_type(get_timeline_year_month());
	if(desc == NULL) 
	{
		// Hajo: try some default (might happen with timeline ... )
		desc = way_builder_t::weg_search(road_wt, get_timeline_year_month(), 5, get_timeline_year_month(),type_flat, 25000000);
	}
	generic_road_time_per_tile_intercity = (uint16)calc_generic_road_time_per_tile(desc);
}

sint32 karte_t::calc_generic_road_time_per_tile(const way_desc_t* desc)
{
	sint32 speed_average = citycar_speed_average;
	if(desc)
	{
		const sint32 road_speed_limit = desc->get_topspeed();
		if (speed_average > road_speed_limit)
		{
			speed_average = road_speed_limit;
		}
	}
	else if(city_road)
	{
		const sint32 road_speed_limit = city_road->get_topspeed();
		if (speed_average > road_speed_limit)
		{
			speed_average = road_speed_limit;
		}
	}

	// Reduce by 1/3 to reflect the fact that vehicles will not always
	// be able to maintain maximum speed even in uncongested environs,
	// and the fact that we are converting route distances to straight
	// line distances.
	speed_average *= 2;
	speed_average /= 3; 

	if(speed_average == 0)
	{
		speed_average = 1;
	}
	
	return ((600 / speed_average) * settings.get_meters_per_tile()) / 100;
}

void karte_t::calc_max_road_check_depth()
{
	sint32 max_road_speed = 0;
	stringhashtable_tpl <way_desc_t *> * ways = way_builder_t::get_all_ways();

	if(ways != NULL)
	{
		FOR(stringhashtable_tpl <way_desc_t *>, const& iter, *ways)
		{
			if(iter.value->get_wtyp() != road_wt || iter.value->get_intro_year_month() > current_month || iter.value->get_retire_year_month() > current_month)
			{
				continue;
			}
			if(iter.value->get_topspeed() > max_road_speed)
			{
				max_road_speed = iter.value->get_topspeed();
			}
		}
		if(max_road_speed == 0)
		{
			max_road_speed = citycar_speed_average;
		}
	}
	else
	{
		max_road_speed = citycar_speed_average;
	}

	// unit of max_road_check_depth: (min/10 * 100) / (m/tile * 6) * km/h  --> tile * 1000 / 36
	max_road_check_depth = ((uint32)settings.get_range_visiting_tolerance() * 100) / (settings.get_meters_per_tile() * 6) * min(citycar_speed_average, max_road_speed);
}

static bool sort_ware_by_name(const goods_desc_t* a, const goods_desc_t* b)
{
	int diff = strcmp(translator::translate(a->get_name()), translator::translate(b->get_name()));
	return diff < 0;
}


// Returns a list of goods produced by factories that exist in current game
const vector_tpl<const goods_desc_t*> &karte_t::get_goods_list()
{
	if (goods_in_game.empty()) {
		// Goods list needs to be rebuilt

		// Reset last vehicle filter, in case goods list has changed
		gui_convoy_assembler_t::selected_filter = VEHICLE_FILTER_RELEVANT;

		FOR(vector_tpl<fabrik_t*>, const factory, get_fab_list()) {
			slist_tpl<goods_desc_t const*>* const produced_goods = factory->get_produced_goods();
			FOR(slist_tpl<goods_desc_t const*>, const good, *produced_goods) {
				goods_in_game.insert_unique_ordered(good, sort_ware_by_name);
			}
			delete produced_goods;
		}

		goods_in_game.insert_at(0, goods_manager_t::passengers);
		goods_in_game.insert_at(1, goods_manager_t::mail);
	}

	return goods_in_game;
}

player_t *karte_t::get_public_player(grund_t const *const gr) const
{
	return get_public_player();
}

player_t *karte_t::get_public_player() const
{
	return get_player(1);
}

void karte_t::add_building_to_world_list(gebaeude_t *gb, bool ordered)
{
	assert(gb);
	gb->set_in_world_list(true);
	if(gb != gb->get_first_tile())
	{
		return;
	}
	const building_desc_t *building = gb->get_tile()->get_desc();

	if (building->get_mail_demand_and_production_capacity() == 0 && building->get_population_and_visitor_demand_capacity() == 0 && building->get_employment_capacity() == 0)
	{
		// This building is no longer capable of dealing with passengers/mail: do not add it.
		gb->set_adjusted_jobs(0);
		gb->set_adjusted_mail_demand(0);
		gb->set_adjusted_visitor_demand(0);
		return;
	}

	if(gb->get_adjusted_population() > 0)
	{
		if(ordered)
		{
			passenger_origins.insert_ordered(gb, gb->get_adjusted_population(), stadt_t::compare_gebaeude_pos);
		}
		else
		{
			passenger_origins.append(gb, gb->get_adjusted_population());
		}
		passenger_step_interval = calc_adjusted_step_interval(passenger_origins.get_sum_weight(), get_settings().get_passenger_trips_per_month_hundredths());
	}	

	const uint8 number_of_classes = goods_manager_t::passengers->get_number_of_classes();

	if(ordered)
	{
		
		if (building->get_class_proportions_sum() > 0)
		{
			for (uint8 i = 0; i < number_of_classes; i++)
			{
				visitor_targets[i].insert_ordered(gb, gb->get_adjusted_visitor_demand() * building->get_class_proportion(i) / building->get_class_proportions_sum(), stadt_t::compare_gebaeude_pos);
			}
		}
		else
		{
			for (uint8 i = 0; i < number_of_classes; i++)
			{
				visitor_targets[i].insert_ordered(gb, gb->get_adjusted_visitor_demand() / number_of_classes, stadt_t::compare_gebaeude_pos);
			}
		}

		if (building->get_class_proportions_sum_jobs() > 0)
		{
			for (uint8 i = 0; i < number_of_classes; i++)
			{
				commuter_targets[i].insert_ordered(gb, gb->get_adjusted_jobs() * building->get_class_proportion_jobs(i) / building->get_class_proportions_sum_jobs(), stadt_t::compare_gebaeude_pos);
			}
		}
		else
		{
			for (uint8 i = 0; i < number_of_classes; i++)
			{
				commuter_targets[i].insert_ordered(gb, gb->get_adjusted_jobs() / number_of_classes, stadt_t::compare_gebaeude_pos);
			}
		}		
	}
	else
	{
		if (building->get_class_proportions_sum() > 0)
		{
			for (uint8 i = 0; i < number_of_classes; i++)
			{
				visitor_targets[i].append(gb, gb->get_adjusted_visitor_demand() * building->get_class_proportion(i) / building->get_class_proportions_sum());
			}
		}
		else
		{
			for (uint8 i = 0; i < number_of_classes; i++)
			{
				visitor_targets[i].append(gb, gb->get_adjusted_visitor_demand() / number_of_classes);
			}
		}

		if (building->get_class_proportions_sum_jobs() > 0)
		{
			for (uint8 i = 0; i < number_of_classes; i++)
			{
				commuter_targets[i].append(gb, gb->get_adjusted_jobs() * building->get_class_proportion_jobs(i) / building->get_class_proportions_sum_jobs());
			}
		}
		else
		{
			for (uint8 i = 0; i < number_of_classes; i++)
			{
				commuter_targets[i].append(gb, gb->get_adjusted_jobs() / number_of_classes);
			}
		}		
	}

	if(gb->get_adjusted_mail_demand() > 0)
	{
		if(ordered)
		{
			mail_origins_and_targets.insert_ordered(gb, gb->get_adjusted_mail_demand(), stadt_t::compare_gebaeude_pos);
		}
		else
		{		
			mail_origins_and_targets.append(gb, gb->get_adjusted_mail_demand());
		}
		mail_step_interval = calc_adjusted_step_interval(mail_origins_and_targets.get_sum_weight(), get_settings().get_mail_packets_per_month_hundredths());
	}
}

void karte_t::remove_building_from_world_list(gebaeude_t *gb)
{
	if (!gb || !gb->get_is_in_world_list())
	{
		return;
	}

	// We do not need to specify the type here, as we can try removing from all lists.
	passenger_origins.remove_all(gb);
	for (uint8 i = 0; i < goods_manager_t::passengers->get_number_of_classes(); i++)
	{
		commuter_targets[i].remove_all(gb);
		visitor_targets[i].remove_all(gb);
	}
	mail_origins_and_targets.remove_all(gb);

	passenger_step_interval = calc_adjusted_step_interval(passenger_origins.get_sum_weight(), get_settings().get_passenger_trips_per_month_hundredths());
	mail_step_interval = calc_adjusted_step_interval(mail_origins_and_targets.get_sum_weight(), get_settings().get_mail_packets_per_month_hundredths());

	gb->set_in_world_list(false);
}

void karte_t::update_weight_of_building_in_world_list(gebaeude_t *gb)
{
	gb = gb->access_first_tile();
	if(!gb || (gb->get_is_factory() && gb->get_fabrik() == NULL))
	{
		// The tile will be set to "is_factory" but the factory pointer will be NULL when
		// this is called from a field of a factory that is closing down.
		return;
	}

	if(passenger_origins.is_contained(gb))
	{
		passenger_origins.update_at(passenger_origins.index_of(gb), gb->get_adjusted_population());
		passenger_step_interval = calc_adjusted_step_interval(passenger_origins.get_sum_weight(), get_settings().get_passenger_trips_per_month_hundredths());
	}

	for (uint8 i = 0; i < goods_manager_t::passengers->get_number_of_classes(); i++)
	{
		if (commuter_targets[i].is_contained(gb))
		{
			commuter_targets[i].update_at(commuter_targets[i].index_of(gb), (gb->get_tile()->get_desc()->get_class_proportions_sum_jobs() > 0 ? (gb->get_adjusted_jobs() * gb->get_tile()->get_desc()->get_class_proportion_jobs(i)) / gb->get_tile()->get_desc()->get_class_proportions_sum_jobs() : gb->get_adjusted_jobs()));
		}

		if (visitor_targets[i].is_contained(gb))
		{
			visitor_targets[i].update_at(visitor_targets[i].index_of(gb), (gb->get_tile()->get_desc()->get_class_proportions_sum() > 0 ? (gb->get_adjusted_visitor_demand() * gb->get_tile()->get_desc()->get_class_proportion(i)) / gb->get_tile()->get_desc()->get_class_proportions_sum() : gb->get_adjusted_visitor_demand()));
		}
	}
	if(mail_origins_and_targets.is_contained(gb))
	{
		mail_origins_and_targets.update_at(mail_origins_and_targets.index_of(gb), gb->get_adjusted_mail_demand());
		mail_step_interval = calc_adjusted_step_interval(mail_origins_and_targets.get_sum_weight(), get_settings().get_mail_packets_per_month_hundredths());
	}
}

void karte_t::remove_all_building_references_to_city(stadt_t* city)
{
	FOR(weighted_vector_tpl <gebaeude_t *>, building, passenger_origins)
	{
		if(building->get_stadt() == city)
		{
			building->set_stadt(NULL);
		}
	}

	FOR(weighted_vector_tpl <gebaeude_t *>, building, mail_origins_and_targets)
	{
		if(building->get_stadt() == city)
		{
			building->set_stadt(NULL);
		}
	}

	for (uint8 i = 0; i < goods_manager_t::passengers->get_number_of_classes(); i++)
	{
		FOR(weighted_vector_tpl <gebaeude_t *>, building, commuter_targets[i])
		{
			if (building->get_stadt() == city)
			{
				building->set_stadt(NULL);
			}
		}

		FOR(weighted_vector_tpl <gebaeude_t *>, building, visitor_targets[i])
		{
			if (building->get_stadt() == city)
			{
				building->set_stadt(NULL);
			}
		}
	}	
}

vector_tpl<car_ownership_record_t> *karte_t::car_ownership;

sint16 karte_t::get_private_car_ownership(sint32 monthyear, uint8 g_class) const
{
	const uint8 number_of_passenger_classes = goods_manager_t::passengers->get_number_of_classes();
	if(monthyear == 0 || !car_ownership)
	{
		if (number_of_passenger_classes > 1 && g_class == number_of_passenger_classes - 1)
		{
			// By default, the highest class of passenger always has access to a private car where there are multiple classes of passengers
			// defined.
			return 100;
		}
		else
		{
			return default_car_ownership_percent;
		}
	}

	// Check for data
	if(car_ownership[g_class].get_count())
	{
		uint i=0;
		while(i < car_ownership[g_class].get_count() && monthyear >= car_ownership[g_class][i].year)
		{
			i++;
		}
		if(i == car_ownership[g_class].get_count())
		{
			return car_ownership[g_class][i-1].ownership_percent;
		}
		else if(i == 0) 
		{
			return car_ownership[g_class][0].ownership_percent;
		}
		else 
		{
			// Interpolate linear
			const sint32 delta_ownership_percent = car_ownership[g_class][i].ownership_percent - car_ownership[g_class][i-1].ownership_percent;
			const sint64 delta_years = car_ownership[g_class][i].year - car_ownership[g_class][i-1].year;
			return ((delta_ownership_percent * (monthyear-car_ownership[g_class][i-1].year)) / delta_years ) + car_ownership[g_class][i-1].ownership_percent;
		}
	}
	else
	{
		if (number_of_passenger_classes > 1 && g_class == number_of_passenger_classes - 1)
		{
			// By default, the highest class of passenger always has access to a private car where there are multiple classes of passengers
			// defined.
			return 100;
		}
		else
		{
			return default_car_ownership_percent;
		}
	}
}

void karte_t::privatecar_init(const std::string &objfilename)
{
	tabfile_t ownership_file;
	// first take user data, then user global data
	if(!ownership_file.open((objfilename+"config/privatecar.tab").c_str()))
	{
		dbg->message("stadt_t::privatecar_init()", "Error opening config/privatecar.tab.\nWill use default values." );
		return;
	}

	const uint8 number_of_passenger_classes = goods_manager_t::passengers->get_number_of_classes();

	tabfileobj_t contents;
	ownership_file.read(contents);
	car_ownership = new vector_tpl<car_ownership_record_t>[number_of_passenger_classes];

	/* init the values from line with the form year, proportion, year, proportion
	 * must be increasing order!
	 */
	bool wrote_any_data = false;
	for (uint8 cl = 0; cl < number_of_passenger_classes; cl++)
	{
		char buf[40];
		sprintf(buf, "car_ownership[%i]", cl);
		int *tracks = contents.get_ints(buf);
		if ((tracks[0] & 1) == 1)
		{
			dbg->message("stadt_t::privatecar_init()", "Ill formed line in config/privatecar.tab.\nWill use default value. Format is year,ownership percentage[ year,ownership percentage]!");
			car_ownership[cl].clear();
			delete[] tracks;
			return;
		}
		car_ownership[cl].resize(tracks[0] / 2);
		for (int i = 1; i < tracks[0]; i += 2)
		{
			car_ownership_record_t c(tracks[i], tracks[i + 1]);
			car_ownership[cl].append(c);
			wrote_any_data = true;
		}
		delete[] tracks;
	}

	if (wrote_any_data)
	{
		return;
	}

	for (uint8 cl = 0; cl < number_of_passenger_classes; cl++)
	{
		if (number_of_passenger_classes == 1 || cl < number_of_passenger_classes - 1)
		{
			int *tracks = contents.get_ints("car_ownership");
			if ((tracks[0] & 1) == 1)
			{
				dbg->message("stadt_t::privatecar_init()", "Ill formed line in config/privatecar.tab.\nWill use default value. Format is year,ownership percentage[ year,ownership percentage]!");
				car_ownership[cl].clear();
				delete[] tracks;
				return;
			}
			car_ownership[cl].resize(tracks[0] / 2);
			for (int i = 1; i < tracks[0]; i += 2)
			{
				car_ownership_record_t c(tracks[i], tracks[i + 1]);
				car_ownership[cl].append(c);
			}
			delete[] tracks;
		}
		else
		{
			// In the case of an old style privatecar.tab,
			// assume that the highest class (where there are
			// multiple passenger classes) always has access
			// to a private car at all times in history.
			car_ownership_record_t c(0, 100);
			car_ownership[cl].append(c);
			c.year = 5000;
			c.ownership_percent = 100;
			car_ownership[cl].append(c);
		}
	}
}

/**
* Reads/writes private car ownership data from/to a savegame
* called from karte_t::save and karte_t::load
* only written for networkgames
* @author jamespetts
*/
void karte_t::privatecar_rdwr(loadsave_t *file)
{
	if(file->get_extended_version() < 9)
	{
		 return;
	}
	
	uint8 number_of_passenger_classes = file->get_extended_version() >= 13 || file->get_extended_revision() >= 24 ? goods_manager_t::passengers->get_number_of_classes() : 1;

	if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 24)
	{
		file->rdwr_byte(number_of_passenger_classes);
	}

	if(!car_ownership)
	{
		car_ownership = new vector_tpl<car_ownership_record_t>[number_of_passenger_classes];
	}

	for (uint8 cl = 0; cl < number_of_passenger_classes; cl++)
	{
		if (file->is_saving())
		{
			uint32 count = car_ownership[cl].get_count();
			file->rdwr_long(count);
			ITERATE(car_ownership[cl], i)
			{
				file->rdwr_longlong(car_ownership[cl].get_element(i).year);
				file->rdwr_short(car_ownership[cl].get_element(i).ownership_percent);
			}
		}

		else
		{		
			uint32 counter;
			file->rdwr_long(counter);
			sint64 year = 0;
			uint16 ownership_percent = 0;
			if(counter > 0)
			{
				car_ownership[cl].clear();
			}

			for (uint32 c = 0; c < counter; c++)
			{
				file->rdwr_longlong(year);
				file->rdwr_short(ownership_percent);
				if (cl < goods_manager_t::passengers->get_number_of_classes())
				{
					car_ownership_record_t cow(year / 12, ownership_percent);
					car_ownership[cl].append(cow);
				}
			}
		}
	}

	// The below is probably redundant, as the remaining classes will be filled in by whatever is in privatecar.tab
	/*
	if (file->is_loading() && number_of_passenger_classes < goods_manager_t::passengers->get_number_of_classes())
	{
		// Must fill in additional classes with data from the classes that we have when loading older games.
		for (uint8 cl = number_of_passenger_classes; cl < goods_manager_t::passengers->get_number_of_classes(); cl++)
		{
			car_ownership[cl].clear();
			if (cl < number_of_passenger_classes - 1)
			{
				FOR(vector_tpl<car_ownership_record_t>, car_own, car_ownership[number_of_passenger_classes - 1])
				{
					car_ownership[cl].append(car_own);
				}
			}
			else
			{
				// In the case of an old style privatecar.tab,
				// assume that the highest class (where there are
				// multiple passenger classes) always has access
				// to a private car at all times in history.
				car_ownership_record_t c(0, 100);
				car_ownership[cl].append(c);
				c.year = 5000;
				c.ownership_percent = 100;
				car_ownership[cl].append(c);
			}
		}
	}*/
}

sint64 karte_t::get_land_value (koord3d k)
{
	// TODO: Have this based on a much more sophisticated
	// formula derived from local desirability, based on 
	// transport success rates. 

	// NOTE: settings.cst_buy_land is a *negative* number.
	sint64 cost = settings.cst_buy_land;
	const stadt_t* city = get_city(k.get_2d());
	const grund_t* gr = lookup_kartenboden(k.get_2d());
	if(city)
	{
		if(city->get_city_population() >= settings.get_city_threshold_size())
		{
			cost *= 4;
		}
		else if(city->get_city_population() >= settings.get_capital_threshold_size())
		{
			cost *= 6;
		}
		else
		{
			cost *= 3;
		}
	}
	else
	{
		if(k.z > get_groundwater() + 10)
		{
			// Mountainous areas are cheaper
			cost *= 70;
			cost /= 100;
		}
	}

	if(lookup_hgt(k.get_2d()) != k.z && gr)
	{
		// Elevated or underground way being built.
		// Check for building and pay wayleaves if necessary.
		const gebaeude_t* gb = obj_cast<gebaeude_t>(gr->first_obj());
		if(gb)
		{
			cost -= (gb->get_tile()->get_desc()->get_level() * settings.cst_buy_land) / 5;
		}
		// Building other than on the surface of the land is cheaper in any event.
		cost /= 2;
	}

	if(gr && gr->is_water())
	{
		// Water is cheaper than land.
		cost /= 4;
	}

	return cost;
}

double karte_t::get_forge_cost(waytype_t waytype, koord3d position)
{
	sint64 forge_cost = get_settings().get_forge_cost(waytype);
	const koord3d pos = position;

	for (int n = 0; n < 8; n++)
	{
		const koord kn = pos.get_2d().neighbours[n] + pos.get_2d();
		if (!is_within_grid_limits(kn))
		{
			continue;
		}
		const koord3d kn3d(kn, lookup_hgt(kn));
		grund_t* to = lookup(kn3d);
		const grund_t* gr_this_tile = lookup_kartenboden(pos.get_2d());
		const grund_t* gr_neighbour = lookup_kartenboden(kn);
		if (gr_this_tile && gr_this_tile->get_weg(waytype))
		{
			// There exists a way of the same waytype on this tile - no forge costs.
			forge_cost = 0;
			break;
		}
		else if (gr_neighbour && gr_neighbour->get_weg(waytype))
		{
			// This is a parallel way of the same type - reduce the forge cost.
			forge_cost *= get_settings().get_parallel_ways_forge_cost_percentage(waytype);
			forge_cost /= 100ll;
			break;
		}

	}
	return forge_cost;
}

bool karte_t::is_forge_cost_reduced(waytype_t waytype, koord3d position)
{
	sint64 forge_cost = get_settings().get_forge_cost(waytype);
	const koord3d pos = position;
	bool is_cost_reduced = false;

	for (int n = 0; n < 8; n++)
	{
		const koord kn = pos.get_2d().neighbours[n] + pos.get_2d();
		if (!is_within_grid_limits(kn))
		{
			continue;
		}
		const koord3d kn3d(kn, lookup_hgt(kn));
		grund_t* to = lookup(kn3d);

		const grund_t* gr_neighbour = lookup_kartenboden(kn);
		if (gr_neighbour && gr_neighbour->get_weg(waytype))
		{
			// This is a parallel way of the same type - reduce the forge cost.
			is_cost_reduced = true;
			break;
		}
	}
	return is_cost_reduced;
}

sint64 karte_t::calc_monthly_job_demand() const
{
	sint64 value = (get_finance_history_month(0, karte_t::WORLD_CITICENS) * get_settings().get_commuting_trip_chance_percent()) / get_settings().get_passenger_trips_per_month_hundredths();
	return value;
}
