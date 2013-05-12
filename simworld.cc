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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "simcity.h"
#include "simcolor.h"
#include "simconvoi.h"
#include "simdebug.h"
#include "simdepot.h"
#include "simfab.h"
#include "simgraph.h"
#include "simhalt.h"
#include "simimg.h"
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
#include "simtools.h"
#include "simunits.h"
#include "simversion.h"
#include "simview.h"
#include "simwerkz.h"
#include "simwin.h"
#include "simworld.h"


#include "tpl/vector_tpl.h"
#include "tpl/binary_heap_tpl.h"

#include "boden/boden.h"
#include "boden/wasser.h"

#include "old_blockmanager.h"
#include "vehicle/simvehikel.h"
#include "vehicle/simverkehr.h"
#include "vehicle/movingobj.h"
#include "boden/wege/schiene.h"

#include "dings/zeiger.h"
#include "dings/baum.h"
#include "dings/signal.h"
#include "dings/roadsign.h"
#include "dings/wayobj.h"
#include "dings/groundobj.h"
#include "dings/gebaeude.h"
#include "dings/leitung2.h"

#include "gui/password_frame.h"
#include "gui/messagebox.h"
#include "gui/help_frame.h"
#include "gui/karte.h"
#include "gui/player_frame_t.h"

#include "dataobj/network.h"
#include "dataobj/network_file_transfer.h"
#include "dataobj/network_socket_list.h"
#include "dataobj/network_cmd_ingame.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "dataobj/scenario.h"
#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
#include "dataobj/powernet.h"

#include "utils/cbuffer_t.h"
#include "utils/simstring.h"
#include "utils/memory_rw.h"

#include "bauer/brueckenbauer.h"
#include "bauer/tunnelbauer.h"
#include "bauer/fabrikbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"
#include "bauer/vehikelbauer.h"

#include "besch/grund_besch.h"

#include "player/simplay.h"
#include "player/finance.h"
#include "player/ai_passenger.h"
#include "player/ai_goods.h"



// advance 201 ms per sync_step in fast forward mode
#define MAGIC_STEP (201)

// frame per second for fast forward
#define FF_PPS (10)


static bool is_dragging = false;
static uint32 last_clients = -1;
static uint8 last_active_player_nr = 0;
static std::string last_network_game;

stringhashtable_tpl<karte_t::missing_level_t>missing_pak_names;

#if MULTI_THREAD>1
// enable barriers by this
#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <semaphore.h>

// to start a thread
typedef struct{
	karte_t *welt;
	int thread_num;
	sint16 x_step;
	sint16 y_min;
	sint16 y_max;
	sem_t* wait_for_previous;
	sem_t* signal_to_next;
	xy_loop_func function;
} world_thread_param_t;


void *karte_t::world_xy_loop_thread(void *ptr)
{
	world_thread_param_t *param = reinterpret_cast<world_thread_param_t *>(ptr);
	sint16 x_min = 0;
	sint16 x_max = param->x_step;

	while(  x_min < param->welt->cached_grid_size.x  ) {
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
		x_max = min(x_max + param->x_step, param->welt->cached_grid_size.x);
	}
	return NULL;
}
#endif


void karte_t::world_xy_loop(xy_loop_func function, bool sync_x_steps)
{
#if MULTI_THREAD>1
	set_random_mode( INTERACTIVE_RANDOM ); // do not allow simrand() here!

	pthread_t thread[MULTI_THREAD];
	pthread_attr_t attr;
	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	// semaphores to synchronize progress in x direction
	sem_t sems[MULTI_THREAD-1];

	// now the parameters
	world_thread_param_t ka[MULTI_THREAD];

	for(  int t = 0;  t < MULTI_THREAD;  t++  ) {
		if(  sync_x_steps  &&  t < MULTI_THREAD - 1  ) {
			sem_init(&sems[t], 0, 0);
		}

   		ka[t].welt = this;
   		ka[t].thread_num = t;
		ka[t].x_step = min(64, cached_grid_size.x/MULTI_THREAD);
		ka[t].y_min = (t*cached_grid_size.y)/MULTI_THREAD;
		ka[t].y_max = ((t+1)*cached_grid_size.y)/MULTI_THREAD;
		ka[t].function = function;

		ka[t].wait_for_previous = sync_x_steps  &&  t > 0 ? &sems[t-1] : NULL;
		ka[t].signal_to_next    = sync_x_steps  &&  t < MULTI_THREAD - 1 ? &sems[t] : NULL;

		if(  t < MULTI_THREAD - 1  ) {
			if(  pthread_create(&thread[t], &attr, world_xy_loop_thread, &ka[t])  ) {
				// here some more sophisticated error handling would be fine ...
			}
		}
	}
	pthread_attr_destroy(&attr);

	// the last rows we do ourselves...
	world_xy_loop_thread(&ka[MULTI_THREAD-1]);

	// return from thread
	for(  int t = 0;  t < MULTI_THREAD - 1;  t++  ) {
		void *status;
		pthread_join(thread[t], &status);

		if(  sync_x_steps  ) {
			sem_destroy(&sems[t]);
		}
	}

	clear_random_mode( INTERACTIVE_RANDOM ); // do not allow simrand() here!

#else
	// slow serial way of display
	(this->*function)( 0, cached_grid_size.x, 0, cached_grid_size.y );
#endif
}


void checklist_t::rdwr(memory_rw_t *buffer)
{
	buffer->rdwr_long(random_seed);
	buffer->rdwr_short(halt_entry);
	buffer->rdwr_short(line_entry);
	buffer->rdwr_short(convoy_entry);
}


int checklist_t::print(char *buffer, const char *entity) const
{
	return sprintf(buffer, "%s=[rand=%u halt=%u line=%u cnvy=%u] ", entity, random_seed, halt_entry, line_entry, convoy_entry);
}


// changes the snowline height (for the seasons)
bool karte_t::recalc_snowline()
{
	static int mfactor[12] = { 99, 95, 80, 50, 25, 10, 0, 5, 20, 35, 65, 85 };
	static uint8 month_to_season[12] = { 2, 2, 2, 3, 3, 0, 0, 0, 0, 1, 1, 2 };

	// calculate snowline with day precision
	// use linear interpolation
	const long ticks_this_month = get_zeit_ms() & (karte_t::ticks_per_world_month-1);
	const long faktor = mfactor[last_month] + (  ( (mfactor[(last_month+1)%12]-mfactor[last_month])*(ticks_this_month>>12) ) >> (karte_t::ticks_per_world_month_shift-12) );

	// just remember them
	const sint16 old_snowline = snowline;
	const sint16 old_season = season;

	// and calculate new values
	season=month_to_season[last_month];   //  (2+last_month/3)&3; // summer always zero
	int const winterline = settings.get_winter_snowline();
	int const summerline = settings.get_climate_borders()[arctic_climate] + 1;
	snowline = summerline - (sint16)(((summerline-winterline)*faktor)/100);
	snowline = snowline + grundwasser;

	// changed => we update all tiles ...
	return (old_snowline!=snowline  ||  old_season!=season);
}


// read height data from bmp or ppm files
bool karte_t::get_height_data_from_file( const char *filename, sint8 grundwasser, sint8 *&hfield, sint16 &ww, sint16 &hh, bool update_only_values )
{
	if (FILE* const file = fopen(filename, "rb")) {
		char id[3];
		// parsing the header of this mixed file format is nottrivial ...
		id[0] = fgetc(file);
		id[1] = fgetc(file);
		id[2] = 0;
		if(strcmp(id, "P6")) {
			if(strcmp(id, "BM")) {
				fclose(file);
				dbg->error("karte_t::load_heightfield()","Heightfield has wrong image type %s instead P6/BM", id);
				return false;
			}
			// bitmap format
			fseek( file, 10, SEEK_SET );
			uint32 data_offset;
			sint32 w, h, format, table;
			sint16 bit_depth;
#ifdef SIM_BIG_ENDIAN
			uint32 l;
			uint16 s;
			fread( &l, 4, 1, file );
			data_offset = endian(l);
			fseek( file, 18, SEEK_SET );
			fread( &l, 4, 1, file );
			w = endian(l);
			fread( &l, 4, 1, file );
			h = endian(l);
			fseek( file, 28, SEEK_SET );
			fread( &s, 2, 1, file );
			bit_depth = endian(s);
			fread( &l, 4, 1, file );
			format = endian(l);
			fseek( file, 46, SEEK_SET );
			fread( &l, 4, 1, file );
			table = endian(l);
#else
			fread( &data_offset, 4, 1, file );
			fseek( file, 18, SEEK_SET );
			fread( &w, 4, 1, file );
			fread( &h, 4, 1, file );
			fseek( file, 28, SEEK_SET );
			fread( &bit_depth, 2, 1, file );
			fread( &format, 4, 1, file );
			fseek( file, 46, SEEK_SET );
			fread( &table, 4, 1, file );
#endif
			if((bit_depth!=8  &&  bit_depth!=24)  ||  format>1) {
				if(!update_only_values) {
					dbg->fatal("karte_t::get_height_data_from_file()","Can only use 8Bit (RLE or normal) or 24 bit bitmaps!");
				}
				fclose( file );
				return false;
			}

			// skip parsing body
			if(update_only_values) {
				ww = w;
				hh = abs(h);
				return true;
			}

			// now read the data and convert them on the fly
			hfield = new sint8[w*h];
			memset( hfield, grundwasser, w*h );
			if(bit_depth==8) {
				// convert color tables to height levels
				if(table==0) {
					table = 256;
				}
				sint8 h_table[256];
				fseek( file, 54, SEEK_SET );
				for( int i=0;  i<table;  i++  ) {
					int B = fgetc(file);
					int G = fgetc(file);
					int R = fgetc(file);
					fgetc(file);	// dummy
					h_table[i] = (((R*2+G*3+B)/4 - 224) & 0xFFF0)/16;
				}
				// now read the data
				fseek( file, data_offset, SEEK_SET );
				if(format==0) {
					// uncompressed (usually mirrored, if h>0)
					bool mirror = (h<0);
					h = abs(h);
					for(  sint32 y=0;  y<h;  y++  ) {
						sint32 offset = mirror ? y*w : (h-y-1)*w;
						for(  sint32 x=0;  x<w;  x++  ) {
							hfield[x+offset] = h_table[fgetc(file)];
						}
						// skip line offset
						if(w&1) {
							fgetc(file);
						}
					}
				}
				else {
					// compressed RLE (reverse y, since mirrored)
					sint32 x=0, y=h-1;
					while (!feof(file)) {
						uint8 Count= fgetc(file);
						uint8 ColorIndex = fgetc(file);

						if (Count > 0) {
							for( sint32 k = 0;  k < Count;  k++, x++  ) {
								hfield[x+(y*w)] = h_table[ColorIndex];
							}
						} else if (Count == 0) {
							sint32 Flag = ColorIndex;
							if (Flag == 0) {
								// goto next line
								x = 0;
								y--;
							}
							else if (Flag == 1) {
								// end of bitmap
								break;
							}
							else if (Flag == 2) {
								// skip with cursor
								x += (uint8)fgetc(file);
								y -= (uint8)fgetc(file);
							}
							else {
								// uncompressed run
								Count = Flag;
								for( sint32 k = 0;  k < Count;  k++, x++  ) {
									hfield[x+y*w] = h_table[(uint8)fgetc(file)];
								}
								if (ftell(file) & 1) {	// alway even offset in file
									fseek(file, 1, SEEK_CUR);
								}
							}
						}
					}
				}
			}
			else {
				// uncompressed 24 bits
				bool mirror = (h<0);
				h = abs(h);
				for(  sint32 y=0;  y<h;  y++  ) {
					sint32 offset = mirror ? y*w : (h-y-1)*w;
					for(  sint32 x=0;  x<w;  x++  ) {
						int B = fgetc(file);
						int G = fgetc(file);
						int R = fgetc(file);
						hfield[x+offset] = (((R*2+G*3+B)/4 - 224) & 0xFFF0)/16;
					}
					fseek( file, (4-((w*3)&3))&3, SEEK_CUR );	// skip superfluos bytes at the end of each scanline
				}
			}
			// success ...
			fclose(file);
			ww = w;
			hh = h;
			return true;
		}
		else {
			// ppm format
			char buf[255];
			char *c = id+2;
			sint32 param[3]={0,0,0};
			for(int index=0;  index<3;  ) {
				// the format is "P6[whitespace]width[whitespace]height[[whitespace bitdepth]]newline]
				// however, Photoshop is the first program, that uses space for the first whitespace ...
				// so we cater for Photoshop too
				while(*c  &&  *c<=32) {
					c++;
				}
				// usually, after P6 there comes a comment with the maker
				// but comments can be anywhere
				if(*c==0) {
					read_line(buf, sizeof(buf), file);
					c = buf;
					continue;
				}
				param[index++] = atoi(c);
				while(*c>='0'  &&  *c<='9') {
					c++;
				}
			}
			// now the data
			sint32 w = param[0];
			sint32 h = param[1];
			if(param[2]!=255) {
				fclose(file);
				if(!update_only_values) {
					dbg->fatal("karte_t::load_heightfield()","Heightfield has wrong color depth %d", param[2] );
				}
				return false;
			}

			// report only values
			if(update_only_values) {
				fclose(file);
				ww = w;
				hh = h;
				return true;
			}

			// ok, now read them in
			hfield = new sint8[w*h];
			memset( hfield, grundwasser, w*h );

			for(sint16 y=0; y<h; y++) {
				for(sint16 x=0; x<w; x++) {
					int R = fgetc(file);
					int G = fgetc(file);
					int B = fgetc(file);
					hfield[x+(y*w)] =  (((R*2+G*3+B)/4 - 224) & 0xFFF0)/16;
				}
			}

			// success ...
			fclose(file);
			ww = w;
			hh = h;
			return true;
		}
	}
	return false;
}


/**
 * Hoehe eines Punktes der Karte mit "perlin noise"
 *
 * @param frequency in 0..1.0 roughness, the higher the rougher
 * @param amplitude in 0..160.0 top height of mountains, may not exceed 160.0!!!
 * @author Hj. Malthaner
 */
sint32 karte_t::perlin_hoehe(settings_t const* const sets, koord k, koord const size)
{
	// Hajo: to Markus: replace the fixed values with your
	// settings. Amplitude is the top highness of the
	// montains, frequency is something like landscape 'roughness'
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
	return ((int)(perlin_noise_2D(k.x, k.y, sets->get_map_roughness())*(double)sets->get_max_mountain_height())) / 16;
}


void karte_t::cleanup_karte( int xoff, int yoff )
{
	// we need a copy to smoothen the map to a realistic level
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
	// recalculate slopes and water tiles
	for(  j=0;  j<get_size().y;  j++  ) {
		for(  i=(j>=yoff)?0:xoff;  i<get_size().x;  i++  ) {
			planquadrat_t *pl = access(i,j);
			grund_t *gr = pl->get_kartenboden();
			koord k(i,j);
			uint8 slope = calc_natural_slope(k);
			gr->set_pos(koord3d(k,min_hgt(k)));
			bool recalc_water = false;
			if(  gr->get_typ()!=grund_t::wasser  &&  max_hgt(k) <= get_grundwasser()  ) {
				// below water but ground => convert
				pl->kartenboden_setzen(new wasser_t(this, gr->get_pos()) );
				recalc_water = true;
			}
			else if(  gr->get_typ()==grund_t::wasser  &&  max_hgt(k) > get_grundwasser()  ) {
				// water above ground => to ground
				pl->kartenboden_setzen(new boden_t(this, gr->get_pos(), slope ) );
				recalc_water = true;
			}
			else {
				gr->set_grund_hang( slope );
			}
			pl->get_kartenboden()->calc_bild();
			// recalc water ribis
			if (recalc_water) {
				if (i>0) {
					lookup_kartenboden(koord(i-1,j))->calc_bild();
				}
				if (j>0) {
					lookup_kartenboden(koord(i,j-1))->calc_bild();
				}
			}
		}
	}
}


void karte_t::destroy()
{
	is_sound = false; // karte_t::play_sound_area_clipped needs valid zeiger
DBG_MESSAGE("karte_t::destroy()", "destroying world");

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

	// alle convois aufraeumen
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

	// alle haltestellen aufraeumen
	old_progress += haltestelle_t::get_alle_haltestellen().get_count();
	haltestelle_t::destroy_all(this);
DBG_MESSAGE("karte_t::destroy()", "stops destroyed");
	ls.set_progress( old_progress );

	// remove all target cities (we can skip recalculation anyway)
	FOR(slist_tpl<fabrik_t*>, const f, fab_list) {
		f->clear_target_cities();
	}

	// delete towns first (will also delete all their houses)
	// for the next game we need to remember the desired number ...
	sint32 const no_of_cities = settings.get_anzahl_staedte();
	for(  uint32 i=0;  !stadt.empty();  i++  ) {
		rem_stadt(stadt.front());
		old_progress += 10;
		if(  (i&0x00F) == 0  ) {
			ls.set_progress( old_progress );
		}
	}
	settings.set_anzahl_staedte(no_of_cities);
DBG_MESSAGE("karte_t::destroy()", "towns destroyed");

	ls.set_progress( old_progress );
	old_progress += cached_size.x*cached_size.y;

	// removes all moving stuff from the sync_step
	while(!sync_list.empty()) {
#ifndef SYNC_VECTOR
		sync_steppable *ss = sync_list.remove_first();
		delete ss;
#else
		delete sync_list.back();
#endif
	}
	sync_list.clear();

	// now remove all pedestrians too ...
	while(!sync_eyecandy_list.empty()) {
		sync_steppable *ss = sync_eyecandy_list.remove_first();
		delete ss;
	}

	while(!sync_way_eyecandy_list.empty()) {
#ifndef SYNC_VECTOR
		sync_steppable *ss = sync_way_eyecandy_list.remove_first();
		delete ss;
#else
		delete sync_way_eyecandy_list.back();
#endif
	}

	ls.set_progress( old_progress );
DBG_MESSAGE("karte_t::destroy()", "sync list cleared");

	// dinge aufraeumen
	cached_grid_size.x = cached_grid_size.y = 1;
	cached_size.x = cached_size.y = 0;
	if(plan) {
		delete [] plan;
		plan = NULL;
	}
	DBG_MESSAGE("karte_t::destroy()", "planquadrat destroyed");

	old_progress += (cached_size.x*cached_size.y)/2;
	ls.set_progress( old_progress );

	// gitter aufraeumen
	if(grid_hgts) {
		delete [] grid_hgts;
		grid_hgts = NULL;
	}

	// marker aufraeumen
	marker.init(0,0);
DBG_MESSAGE("karte_t::destroy()", "marker destroyed");

	// spieler aufraeumen
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(spieler[i]) {
			delete spieler[i];
			spieler[i] = NULL;
		}
	}
DBG_MESSAGE("karte_t::destroy()", "player destroyed");

	old_progress += (cached_size.x*cached_size.y)/4;
	ls.set_progress( old_progress );

	// alle fabriken aufraeumen
	FOR(slist_tpl<fabrik_t*>, const f, fab_list) {
		delete f;
	}
	fab_list.clear();
DBG_MESSAGE("karte_t::destroy()", "factories destroyed");

	// hier nur entfernen, aber nicht loeschen
	ausflugsziele.clear();
DBG_MESSAGE("karte_t::destroy()", "attraction list destroyed");

	delete scenario;
	scenario = NULL;

assert( depot_t::get_depot_list().empty() );

DBG_MESSAGE("karte_t::destroy()", "world destroyed");
	dbg->important("World destroyed.");
}


void karte_t::add_convoi(convoihandle_t const cnv)
{
	assert(cnv.is_bound());
	convoi_array.append_unique(cnv);
}


void karte_t::rem_convoi(convoihandle_t const cnv)
{
	convoi_array.remove(cnv);
}


void karte_t::add_stadt(stadt_t *s)
{
	settings.set_anzahl_staedte(settings.get_anzahl_staedte() + 1);
	stadt.append(s, s->get_einwohner());

	// Knightly : add links between this city and other cities as well as attractions
	FOR(weighted_vector_tpl<stadt_t*>, const c, stadt) {
		c->add_target_city(s);
	}
	s->recalc_target_cities();
	s->recalc_target_attractions();
}


bool karte_t::rem_stadt(stadt_t *s)
{
	if(s == NULL  ||  stadt.empty()) {
		// no town there to delete ...
		return false;
	}

	// reduce number of towns
	if(s->get_name()) {
		DBG_MESSAGE("karte_t::rem_stadt()", "%s", s->get_name());
	}
	stadt.remove(s);
	DBG_DEBUG4("karte_t::rem_stadt()", "reduce city to %i", settings.get_anzahl_staedte() - 1);
	settings.set_anzahl_staedte(settings.get_anzahl_staedte() - 1);

	// Knightly : remove links between this city and other cities
	FOR(weighted_vector_tpl<stadt_t*>, const c, stadt) {
		c->remove_target_city(s);
	}

	// remove all links from factories
	DBG_DEBUG4("karte_t::rem_stadt()", "fab_list %i", fab_list.get_count() );
	FOR(slist_tpl<fabrik_t*>, const f, fab_list) {
		f->remove_target_city(s);
	}

	// ok, we can delete this
	DBG_MESSAGE("karte_t::rem_stadt()", "delete" );
	delete s;

	return true;
}


// just allocates space;
void karte_t::init_felder()
{
	assert(plan==0);

	uint32 const x = get_size().x;
	uint32 const y = get_size().y;
	plan      = new planquadrat_t[x * y];
	grid_hgts = new sint8[(x + 1) * (y + 1)];
	MEMZERON(grid_hgts, (x + 1) * (y + 1));

	marker.init(x, y);

	win_set_welt( this );
	reliefkarte_t::get_karte()->set_welt(this);

	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		// old default: AI 3 passenger, other goods
		spieler[i] = (i<2) ? new spieler_t(this,i) : NULL;
	}
	active_player = spieler[0];
	active_player_nr = 0;

	// defaults without timeline
	average_speed[0] = 60;
	average_speed[1] = 80;
	average_speed[2] = 40;
	average_speed[3] = 350;

	// clear world records
	max_road_speed.speed = 0;
	max_rail_speed.speed = 0;
	max_monorail_speed.speed = 0;
	max_maglev_speed.speed = 0;
	max_narrowgauge_speed.speed = 0;
	max_ship_speed.speed = 0;
	max_air_speed.speed = 0;

	// make timer loop invalid
	for( int i=0;  i<32;  i++ ) {
		last_frame_ms[i] = 0x7FFFFFFFu;
		last_step_nr[i] = 0xFFFFFFFFu;
	}
	last_frame_idx = 0;
	pending_season_change = 0;

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

	vehikel_basis_t::set_overtaking_offsets( get_settings().is_drive_left() );

	scenario = new scenario_t(this);

	nosave_warning = nosave = false;

	if (umgebung_t::server) {
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
	// First check, wether there is a canal:
	const weg_besch_t* river_besch = wegbauer_t::get_besch( umgebung_t::river_type[umgebung_t::river_types-1], 0 );
	if(  river_besch == NULL  ) {
		// should never reaching here ...
		dbg->warning("karte_t::create_rivers()","There is no river defined!\n");
		return;
	}

	// create a vector of the highest points
	vector_tpl<koord> water_tiles;
	weighted_vector_tpl<koord> mountain_tiles;

	sint8 last_height = 1;
	koord last_koord(0,0);
	const sint16 max_dist = cached_size.y+cached_size.x;

	// trunk of 16 will ensure that rivers are long enough apart ...
	for(  sint16 y = 8;  y < cached_size.y;  y+=16  ) {
		for(  sint16 x = 8;  x < cached_size.x;  x+=16  ) {
			koord k(x,y);
			grund_t *gr = lookup_kartenboden(k);
			const sint8 h = gr->get_hoehe()-get_grundwasser();
			if(  gr->ist_wasser()  ) {
				// may be good to start a river here
				water_tiles.append(k);
			}
			else if(  h>=last_height  ||  koord_distance(last_koord,k)>simrand(max_dist)  ) {
				// something worth to add here
				if(  h>last_height  ) {
					last_height = h;
				}
				last_koord = k;
				// using h*h as weight would give mountian sources more preferences
				// on the other hand most rivers do not string near summits ...
				mountain_tiles.append( k, h );
			}
		}
	}
	if (water_tiles.empty()) {
		dbg->message("karte_t::create_rivers()","There aren't any water tiles!\n");
		return;
	}

	// now make rivers
	sint16 retrys = number*2;
	while(  number > 0  &&  !mountain_tiles.empty()  &&  retrys>0  ) {

		// start with random coordinates
		koord const start = pick_any_weighted(mountain_tiles);
		mountain_tiles.remove( start );

		// build a list of matchin targets
		vector_tpl<koord> valid_water_tiles;
		for(  uint32 i=0;  i<water_tiles.get_count();  i++  ) {
			sint16 dist = koord_distance(start,water_tiles[i]);
			if(  settings.get_min_river_length() < dist  &&  dist < settings.get_max_river_length()  ) {
				valid_water_tiles.append( water_tiles[i] );
			}
		}

		// now try 256 random locations
		for(  sint32 i=0;  i<256  &&  !valid_water_tiles.empty();  i++  ) {
			koord const end = pick_any(valid_water_tiles);
			valid_water_tiles.remove( end );
			wegbauer_t riverbuilder(this, spieler[1]);
			riverbuilder.route_fuer(wegbauer_t::river, river_besch);
			sint16 dist = koord_distance(start,end);
			riverbuilder.set_maximum( dist*50 );
			riverbuilder.calc_route( lookup_kartenboden(end)->get_pos(), lookup_kartenboden(start)->get_pos() );
			if(  riverbuilder.get_count() >= (uint32)settings.get_min_river_length()  ) {
				// do not built too short rivers
				riverbuilder.baue();
				number --;
				retrys++;
				break;
			}
		}

		retrys--;
	}
	// we gave up => tell te user
	if(  number>0  ) {
		dbg->warning( "karte_t::create_rivers()","Too many rivers requested! (%i not constructed)", number );
	}
}


void karte_t::distribute_groundobjs_cities(int new_anzahl_staedte, sint32 new_mittlere_einwohnerzahl, sint16 old_x, sint16 old_y )
{
DBG_DEBUG("karte_t::distribute_groundobjs_cities()","distributing rivers");
	if (umgebung_t::river_types > 0 && settings.get_river_number() > 0) {
		create_rivers(settings.get_river_number());
	}

dbg->important("Creating cities ...");
DBG_DEBUG("karte_t::distribute_groundobjs_cities()","prepare cities");
	vector_tpl<koord> *pos = stadt_t::random_place(this, new_anzahl_staedte, old_x, old_y);

	if(  !pos->empty()  ) {
		const sint32 old_anzahl_staedte = stadt.get_count();
		new_anzahl_staedte = pos->get_count();
		dbg->important("Creating cities: %d", new_anzahl_staedte);

		// prissi if we could not generate enough positions ...
		settings.set_anzahl_staedte(old_anzahl_staedte);
		int old_progress = 16;

		// Ansicht auf erste Stadt zentrieren
		if(  old_x+old_y == 0  ) {
			change_world_position( koord3d((*pos)[0], min_hgt((*pos)[0])) );
		}
		loadingscreen_t ls( translator::translate( "distributing cities" ), 16 + 2 * (old_anzahl_staedte + new_anzahl_staedte) + 2 * new_anzahl_staedte + (old_x == 0 ? settings.get_factory_count() : 0), true, true );

		{
			// Loop only new cities:
#ifdef DEBUG
			uint32 tbegin = dr_time();
#endif
			for(  int i=0;  i<new_anzahl_staedte;  i++  ) {
				stadt_t* s = new stadt_t(spieler[1], (*pos)[i], 1 );
				DBG_DEBUG("karte_t::distribute_groundobjs_cities()","Erzeuge stadt %i with %ld inhabitants",i,(s->get_city_history_month())[HIST_CITICENS] );
				add_stadt(s);
			}

			delete pos;
			DBG_DEBUG("karte_t::distribute_groundobjs_cities()","took %lu ms for all towns", dr_time()-tbegin );

			uint32 game_start = current_month;
			// townhalls available since?
			FOR(vector_tpl<haus_besch_t const*>, const besch, *hausbauer_t::get_list(haus_besch_t::rathaus)) {
				uint32 intro_year_month = besch->get_intro_year_month();
				if(  intro_year_month<game_start  ) {
					game_start = intro_year_month;
				}
			}
			// streets since when?
			game_start = max( game_start, wegbauer_t::get_earliest_way(road_wt)->get_intro_year_month() );

			uint32 original_start_year = current_month;
			uint32 original_industry_gorwth = settings.get_industry_increase_every();
			settings.set_industry_increase_every( 0 );

			for(  uint32 i=old_anzahl_staedte;  i<stadt.get_count();  i++  ) {
				// Hajo: do final init after world was loaded/created
				stadt[i]->laden_abschliessen();

	//			int citizens=(int)(new_mittlere_einwohnerzahl*0.9);
	//			citizens = citizens/10+simrand(2*citizens+1);
				const sint32 citizens = (2500l * new_mittlere_einwohnerzahl) /(simrand(20000)+100);

				sint32 diff = (original_start_year-game_start)/2;
				sint32 growth = 32;
				sint32 current_bev = 1;

				/* grow gradually while aging
				 * the difference to the current end year will be halved,
				 * while the growth step is doubled
				 */
				current_month = game_start;
				bool not_updated = false;
				while(  current_bev < citizens  ) {
					growth = min( citizens-current_bev, growth*2 );
					current_bev += growth;
					stadt[i]->change_size( growth );
					if(  current_bev > citizens/2  &&  not_updated  ) {
						ls.set_progress( ++old_progress );
						not_updated = true;
					}
					current_month += diff;
					diff >>= 1;
				}

				// the growth is slow, so update here the progress bar
				ls.set_progress( ++old_progress );
			}

			current_month = original_start_year;
			settings.set_industry_increase_every( original_industry_gorwth );
			msg->clear();
		}
		finance_history_year[0][WORLD_TOWNS] = finance_history_month[0][WORLD_TOWNS] = stadt.get_count();
		finance_history_year[0][WORLD_CITICENS] = finance_history_month[0][WORLD_CITICENS] = last_month_bev;

		// Hajo: connect some cities with roads
		weg_besch_t const* besch = settings.get_intercity_road_type(get_timeline_year_month());
		if(besch == 0) {
			// Hajo: try some default (might happen with timeline ... )
			besch = wegbauer_t::weg_search(road_wt,80,get_timeline_year_month(),weg_t::type_flat);
		}

		wegbauer_t bauigel (this, spieler[1] );
		bauigel.route_fuer(wegbauer_t::strasse | wegbauer_t::terraform_flag, besch, tunnelbauer_t::find_tunnel(road_wt,15,get_timeline_year_month()), brueckenbauer_t::find_bridge(road_wt,15,get_timeline_year_month()) );
		bauigel.set_keep_existing_ways(true);
		bauigel.set_maximum(umgebung_t::intercity_road_length);

		// **** intercity road construction
		int count = 0;
		sint32 const n_cities  = settings.get_anzahl_staedte();
		int    const max_count = n_cities * (n_cities - 1) / 2 - old_anzahl_staedte * (old_anzahl_staedte - 1) / 2;
		// something to do??
		if(  max_count > 0  ) {
			// print("Building intercity roads ...\n");
			ls.set_max( 16 + 2 * (old_anzahl_staedte + new_anzahl_staedte) + 2 * new_anzahl_staedte + (old_x == 0 ? settings.get_factory_count() : 0) );
			// find townhall of city i and road in front of it
			vector_tpl<koord3d> k;
			for (int i = 0;  i < settings.get_anzahl_staedte(); ++i) {
				koord k1(stadt[i]->get_townhall_road());
				if (lookup_kartenboden(k1)  &&  lookup_kartenboden(k1)->hat_weg(road_wt)) {
					k.append(lookup_kartenboden(k1)->get_pos());
				}
				else {
					// look for a road near the townhall
					gebaeude_t const* const gb = ding_cast<gebaeude_t>(lookup_kartenboden(stadt[i]->get_pos())->first_obj());
					bool ok = false;
					if(  gb  &&  gb->ist_rathaus()  ) {
						koord pos = stadt[i]->get_pos() + koord(-1,-1);
						const koord size = gb->get_tile()->get_besch()->get_groesse(gb->get_tile()->get_layout());
						koord inc(1,0);
						// scan all adjacent tiles, take the first that has a road
						for(sint32 i=0; i<2*size.x+2*size.y+4  &&  !ok; i++) {
							grund_t *gr = lookup_kartenboden(pos);
							if (gr  &&  gr->hat_weg(road_wt)) {
								k.append(gr->get_pos());
								ok = true;
							}
							pos = pos + inc;
							if (i==size.x+1) {
								inc = koord(0,1);
							}
							else if (i==size.x+size.y+2) {
								inc = koord(-1,0);
							}
							else if (i==2*size.x+size.y+3) {
								inc = koord(0,-1);
							}
						}
					}
					if (!ok) {
						k.append( koord3d::invalid );
					}
				}
			}
			// compute all distances
			uint8 conn_comp=1; // current connection component for phase 0
			vector_tpl<uint8> city_flag; // city already connected to the graph? >0 nr of connection component
			array2d_tpl<sint32> city_dist(settings.get_anzahl_staedte(), settings.get_anzahl_staedte());
			for (sint32 i = 0; i < settings.get_anzahl_staedte(); ++i) {
				city_dist.at(i,i) = 0;
				for (sint32 j = i + 1; j < settings.get_anzahl_staedte(); ++j) {
					city_dist.at(i,j) = koord_distance(k[i], k[j]);
					city_dist.at(j,i) = city_dist.at(i,j);
					// count unbuildable connections to new cities
					if(  j>=old_anzahl_staedte && city_dist.at(i,j) >= umgebung_t::intercity_road_length  ) {
						count++;
					}
				}
				city_flag.append( i < old_anzahl_staedte ? conn_comp : 0 );

				// progress bar stuff
				ls.set_progress( 16 + 2 * new_anzahl_staedte + count * settings.get_anzahl_staedte() * 2 / max_count );
			}
			// mark first town as connected
			if (old_anzahl_staedte==0) {
				city_flag[0]=conn_comp;
			}

			// get a default vehikel
			route_t verbindung;
			vehikel_t* test_driver;
			vehikel_besch_t test_drive_besch(road_wt, 500, vehikel_besch_t::diesel );
			test_driver = vehikelbauer_t::baue(koord3d(), spieler[1], NULL, &test_drive_besch);
			test_driver->set_flag( ding_t::not_on_map );

			bool ready=false;
			uint8 phase=0;
			// 0 - first phase: built minimum spanning tree (edge weights: city distance)
			// 1 - second phase: try to complete the graph, avoid edges that
			// == have similar length then already existing connection
			// == lead to triangles with an angle >90 deg

			while( phase < 2  ) {
				ready = true;
				koord conn = koord::invalid;
				sint32 best = umgebung_t::intercity_road_length;

				if(  phase == 0  ) {
					// loop over all unconnected cities
					for (int i = 0; i < settings.get_anzahl_staedte(); ++i) {
						if(  city_flag[i] == conn_comp  ) {
							// loop over all connections to connected cities
							for (int j = old_anzahl_staedte; j < settings.get_anzahl_staedte(); ++j) {
								if(  city_flag[j] == 0  ) {
									ready=false;
									if(  city_dist.at(i,j) < best  ) {
										best = city_dist.at(i,j);
										conn = koord(i,j);
									}
								}
							}
						}
					}
					// did we completed a connection component?
					if(  !ready  &&  best == umgebung_t::intercity_road_length  ) {
						// next component
						conn_comp++;
						// try the first not connected city
						ready = true;
						for(  int i = old_anzahl_staedte;  i < settings.get_anzahl_staedte();  ++i  ) {
							if(  city_flag[i] ==0  ) {
								city_flag[i] = conn_comp;
								ready = false;
								break;
							}
						}
					}
				}
				else {
					// loop over all unconnected cities
					for (int i = 0; i < settings.get_anzahl_staedte(); ++i) {
						for (int j = max(old_anzahl_staedte, i + 1);  j < settings.get_anzahl_staedte(); ++j) {
							if(  city_dist.at(i,j) < best  &&  city_flag[i] == city_flag[j]  ) {
								bool ok = true;
								// is there a connection i..l..j ? forbid stumpfe winkel
								for (int l = 0; l < settings.get_anzahl_staedte(); ++l) {
									if(  city_flag[i] == city_flag[l]  &&  city_dist.at(i,l) == umgebung_t::intercity_road_length  &&  city_dist.at(j,l) == umgebung_t::intercity_road_length  ) {
										// cosine < 0 ?
										koord3d d1 = k[i]-k[l];
										koord3d d2 = k[j]-k[l];
										if(  d1.x*d2.x + d1.y*d2.y < 0  ) {
											city_dist.at(i,j) = umgebung_t::intercity_road_length+1;
											city_dist.at(j,i) = umgebung_t::intercity_road_length+1;
											ok = false;
											count ++;
											break;
										}
									}
								}
								if(ok) {
									ready = false;
									best = city_dist.at(i,j);
									conn = koord(i,j);
								}
							}
						}
					}
				}
				// valid connection?
				if(  conn.x >= 0  ) {
					// is there a connection already
					const bool connected = (  phase==1  &&  verbindung.calc_route( this, k[conn.x], k[conn.y], test_driver, 0, 0 )  );
					// build this connestion?
					bool build = false;
					// set appropriate max length for way builder
					if(  connected  ) {
						if(  2*verbindung.get_count() > (uint32)city_dist.at(conn)  ) {
							bauigel.set_maximum(verbindung.get_count() / 2);
							build = true;
						}
					}
					else {
						bauigel.set_maximum(umgebung_t::intercity_road_length);
						build = true;
					}

					if(  build  ) {
						bauigel.calc_route(k[conn.x],k[conn.y]);
					}

					if(  build  &&  bauigel.get_count() >= 2  ) {
						bauigel.baue();
						if (phase==0) {
							city_flag[ conn.y ] = conn_comp;
						}
						// mark as built
						city_dist.at(conn) =  umgebung_t::intercity_road_length;
						city_dist.at(conn.y, conn.x) =  umgebung_t::intercity_road_length;
						count ++;
					}
					else {
						// do not try again
						city_dist.at(conn) =  umgebung_t::intercity_road_length+1;
						city_dist.at(conn.y, conn.x) =  umgebung_t::intercity_road_length+1;
						count ++;

						if(  phase == 0  ) {
							// do not try to connect to this connected component again
							for(  int i = 0;  i < settings.get_anzahl_staedte();  ++i  ) {
								if(  city_flag[i] == conn_comp  && city_dist.at(i, conn.y)<umgebung_t::intercity_road_length) {
									city_dist.at(i, conn.y) =  umgebung_t::intercity_road_length+1;
									city_dist.at(conn.y, i) =  umgebung_t::intercity_road_length+1;
									count++;
								}
							}
						}
					}
				}

				// progress bar stuff
				ls.set_progress( 16 + 2 * new_anzahl_staedte + count * settings.get_anzahl_staedte() * 2 / max_count );

				// next phase?
				if(ready) {
					phase++;
					ready = false;
				}
			}
			delete test_driver;
		}
	}
	else {
		// could not generate any town
		if(pos) {
			delete pos;
		}
		settings.set_anzahl_staedte( stadt.get_count() ); // new number of towns (if we did not find enough positions)
	}

DBG_DEBUG("karte_t::distribute_groundobjs_cities()","distributing groundobjs");
	if(  umgebung_t::ground_object_probability > 0  ) {
		// add eyecandy like rocky, moles, flowers, ...
		koord k;
		sint32 queried = simrand(umgebung_t::ground_object_probability*2-1);
		for(  k.y=0;  k.y<get_size().y;  k.y++  ) {
			for(  k.x=(k.y<old_y)?old_x:0;  k.x<get_size().x;  k.x++  ) {
				grund_t *gr = lookup_kartenboden(k);
				if(  gr->get_typ()==grund_t::boden  &&  !gr->hat_wege()  ) {
					queried --;
					if(  queried<0  ) {
						const groundobj_besch_t *besch = groundobj_t::random_groundobj_for_climate( get_climate(gr->get_hoehe()), gr->get_grund_hang() );
						if(besch) {
							queried = simrand(umgebung_t::ground_object_probability*2-1);
							gr->obj_add( new groundobj_t( this, gr->get_pos(), besch ) );
						}
					}
				}
			}
		}
	}

DBG_DEBUG("karte_t::distribute_groundobjs_cities()","distributing movingobjs");
	if(  umgebung_t::moving_object_probability > 0  ) {
		// add animals and so on (must be done after growing and all other objects, that could change ground coordinates)
		koord k;
		bool has_water = movingobj_t::random_movingobj_for_climate( water_climate )!=NULL;
		sint32 queried = simrand(umgebung_t::moving_object_probability*2);
		// no need to test the borders, since they are mostly slopes anyway
		for(k.y=1; k.y<get_size().y-1; k.y++) {
			for(k.x=(k.y<old_y)?old_x:1; k.x<get_size().x-1; k.x++) {
				grund_t *gr = lookup_kartenboden(k);
				// flat ground or open water
				if(  gr->get_top()==0  &&  (  (gr->get_typ()==grund_t::boden  &&  gr->get_grund_hang()==hang_t::flach)  ||  (has_water  &&  gr->ist_wasser())  )  ) {
					queried --;
					if(  queried<0  ) {
						const groundobj_besch_t *besch = movingobj_t::random_movingobj_for_climate( get_climate(gr->get_hoehe()) );
						if(  besch  &&  ( besch->get_waytype()!=water_wt  ||  gr->get_hoehe()<=get_grundwasser() )  ) {
							if(besch->get_speed()!=0) {
								queried = simrand(umgebung_t::moving_object_probability*2);
								gr->obj_add( new movingobj_t( this, gr->get_pos(), besch ) );
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
	if (umgebung_t::networkmode) {
		if (umgebung_t::server) {
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
	}

	for(  uint i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		werkzeug[i] = werkzeug_t::general_tool[WKZ_ABFRAGE];
	}
	if(is_display_init()) {
		display_show_pointer(false);
	}
	change_world_position( koord(0,0), 0, 0 );

	settings = *sets;
	// names during creation time
	settings.set_name_language_iso(umgebung_t::language_iso);
	settings.set_use_timeline(settings.get_use_timeline() & 1);

	x_off = y_off = 0;

	ticks = 0;
	last_step_ticks = ticks;
	schedule_counter = 0;
	// ticks = 0x7FFFF800;  // Testing the 31->32 bit step

	last_month = 0;
	last_year = settings.get_starting_year();
	current_month = last_month + (last_year*12);
	set_ticks_per_world_month_shift(settings.get_bits_per_month());
	next_month_ticks =  karte_t::ticks_per_world_month;
	season=(2+last_month/3)&3; // summer always zero
	is_dragging = false;
	steps = 0;
	network_frame_count = 0;
	sync_steps = 0;
	map_counter = 0;
	recalc_average_speed();	// resets timeline
	koord::locality_factor = settings.get_locality_factor( last_year );

	grundwasser = (sint8)sets->get_grundwasser();      //29-Nov-01     Markus Weber    Changed
	grund_besch_t::calc_water_level( this, height_to_climate );
	snowline = sets->get_winter_snowline() + grundwasser;

	if(sets->get_beginner_mode()) {
		warenbauer_t::set_multiplier(settings.get_beginner_price_factor());
		settings.set_just_in_time( 0 );
	}
	else {
		warenbauer_t::set_multiplier( 1000 );
	}
	max_rail_speed.speed = max_monorail_speed.speed = max_maglev_speed.speed = max_narrowgauge_speed.speed = max_road_speed.speed = max_ship_speed.speed = max_air_speed.speed = 0;

	recalc_snowline();

	stadt.clear();

DBG_DEBUG("karte_t::init()","hausbauer_t::neue_karte()");
	// Call this before building cities
	hausbauer_t::neue_karte();

	cached_grid_size.x = 0;
	cached_grid_size.y = 0;

DBG_DEBUG("karte_t::init()","init_felder");
	init_felder();

	enlarge_map(&settings, h_field);

DBG_DEBUG("karte_t::init()","distributing trees");
	if (!settings.get_no_trees()) {
		baum_t::distribute_trees(this,3);
	}

DBG_DEBUG("karte_t::init()","built timeline");
	stadtauto_t::built_timeline_liste(this);

	nosave_warning = nosave = false;

	dbg->important("Creating factories ...");
	fabrikbauer_t::neue_karte(this);

	int consecutive_build_failures = 0;

	loadingscreen_t ls( translator::translate("distributing factories"), 16 + settings.get_anzahl_staedte() * 4 + settings.get_factory_count(), true, true );

	while(  fab_list.get_count() < (uint32)settings.get_factory_count()  ) {
		if(  !fabrikbauer_t::increase_industry_density( this, false )  ) {
			if(  ++consecutive_build_failures > 3  ) {
				// Industry chain building starts failing consecutively as map approaches full.
				break;
			}
		}
		else {
			consecutive_build_failures = 0;
		}
		ls.set_progress( 16 + settings.get_anzahl_staedte() * 4 + min(fab_list.get_count(),settings.get_factory_count()) );
	}

	settings.set_factory_count( fab_list.get_count() );
	finance_history_year[0][WORLD_FACTORIES] = finance_history_month[0][WORLD_FACTORIES] = fab_list.get_count();

	// tourist attractions
	fabrikbauer_t::verteile_tourist(this, settings.get_tourist_attractions());

	dbg->important("Preparing startup ...");
	if(zeiger == 0) {
		zeiger = new zeiger_t(this, koord3d::invalid, NULL );
	}

	// finishes the line preparation and sets id 0 to invalid ...
	spieler[0]->simlinemgmt.laden_abschliessen();

	set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE], get_active_player() );

	recalc_average_speed();

	for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
		if(  spieler[i]  ) {
			spieler[i]->set_active(settings.automaten[i]);
		}
	}

	active_player_nr = 0;
	active_player = spieler[0];
	werkzeug_t::update_toolbars(this);

	set_dirty();
	step_mode = PAUSE_FLAG;
	simloops = 60;
	reset_timer();

	if(is_display_init()) {
		display_show_pointer(true);
	}
	mute_sound(false);
}


void karte_t::enlarge_map(settings_t const* sets, sint8 const* const h_field)
{
	sint16 new_groesse_x = sets->get_groesse_x();
	sint16 new_groesse_y = sets->get_groesse_y();

	if(  cached_grid_size.y>0  &&  cached_grid_size.y!=new_groesse_y  ) {
		// to keep the labels
		grund_t::enlarge_map( new_groesse_x, new_groesse_y );
	}

	planquadrat_t *new_plan = new planquadrat_t[new_groesse_x*new_groesse_y];
	sint8 *new_grid_hgts = new sint8[(new_groesse_x+1)*(new_groesse_y+1)];

	memset(new_grid_hgts, grundwasser, sizeof(sint8)*(new_groesse_x+1)*(new_groesse_y+1));

	sint16 old_x = cached_grid_size.x;
	sint16 old_y = cached_grid_size.y;

	settings.set_groesse_x(new_groesse_x);
	settings.set_groesse_y(new_groesse_y);
	cached_grid_size.x = new_groesse_x;
	cached_grid_size.y = new_groesse_y;
	cached_size_max = max(cached_grid_size.x,cached_grid_size.y);
	cached_size.x = cached_grid_size.x-1;
	cached_size.y = cached_grid_size.y-1;

	intr_disable();

	bool reliefkarte = reliefkarte_t::is_visible;

	int max_display_progress;

	// If this is not called by karte_t::init
	if(  old_x != 0  ) {
		mute_sound(true);
		reliefkarte_t::is_visible = false;

		if(is_display_init()) {
			display_show_pointer(false);
		}

// Copy old values:
		for (sint16 ix = 0; ix<old_x; ix++) {
			for (sint16 iy = 0; iy<old_y; iy++) {
				uint32 nr = ix+(iy*old_x);
				uint32 nnr = ix+(iy*new_groesse_x);
				swap(new_plan[nnr], plan[nr]);
			}
		}
		for (sint16 ix = 0; ix<=old_x; ix++) {
			for (sint16 iy = 0; iy<=old_y; iy++) {
				uint32 nr = ix+(iy*(old_x+1));
				uint32 nnr = ix+(iy*(new_groesse_x+1));
				new_grid_hgts[nnr] = grid_hgts[nr];
			}
		}
		max_display_progress = 16 + sets->get_anzahl_staedte()*2 + stadt.get_count()*4;
	}
	else {
		max_display_progress = 16 + sets->get_anzahl_staedte() * 4 + settings.get_factory_count();
	}
	loadingscreen_t ls( translator::translate( old_x ? "enlarge map" : "Init map ..."), max_display_progress, true, true );

	delete [] plan;
	plan = new_plan;
	delete [] grid_hgts;
	grid_hgts = new_grid_hgts;

	setsimrand(0xFFFFFFFF, settings.get_karte_nummer());
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
		ls.set_progress(16);
	}
	else {
		if(  sets->get_rotation()==0  &&  sets->get_origin_x()==0  &&  sets->get_origin_y()==0) {
			// otherwise negative offsets may occur, so we cache only non-rotated maps
			init_perlin_map(new_groesse_x,new_groesse_y);
		}
		// loop only new tiles:
		for(  sint16 x = 0;  x<=new_groesse_x;  x++  ) {
			for(  sint16 y = (x>old_x)?0:old_y+1;  y<=new_groesse_y;  y++  ) {
				koord pos(x,y);
				sint16 const h = perlin_hoehe(&settings, pos, koord(old_x, old_y));
				set_grid_hgt( pos, (sint8) h);
			}
			ls.set_progress( (x*16)/new_groesse_x );
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
			gr = lookup_kartenboden(koord(i, old_y-1));
			h = gr->get_hoehe(hang_t::corner_SW);
			raise_grid_to(i, old_y+1, h);
		}
		for(i=0; i<old_y; i++) {
			gr = lookup_kartenboden(koord(old_x-1, i));
			h = gr->get_hoehe(hang_t::corner_NE);
			raise_grid_to(old_x+1, i, h);
		}
		for(i=0; i<old_x; i++) {
			gr = lookup_kartenboden(koord(i, old_y-1));
			h = gr->get_hoehe(hang_t::corner_SW);
			lower_grid_to(i, old_y+1, h );
		}
		for(i=0; i<old_y; i++) {
			gr = lookup_kartenboden(koord(old_x-1, i));
			h = gr->get_hoehe(hang_t::corner_NE);
			lower_grid_to(old_x+1, i, h);
		}
		gr = lookup_kartenboden(koord(old_x-1, old_y -1));
		h = gr ->get_hoehe(hang_t::corner_SE);
		raise_grid_to(old_x+1, old_y+1, h);
		lower_grid_to(old_x+1, old_y+1, h);
	}

	// create grounds on new part
	for (sint16 ix = 0; ix<new_groesse_x; ix++) {
		for (sint16 iy = (ix>=old_x)?0:old_y; iy<new_groesse_y; iy++) {
			koord k = koord(ix,iy);
			access(k)->kartenboden_setzen(new boden_t(this, koord3d(ix,iy,min_hgt(k)),0));
		}
	}

	// smooth the new part, reassign slopes on new part
	cleanup_karte( old_x, old_y );

	// smoothing the seam (if possible)
	/*
	 * SMOTHING THE SEAM IS NOT NECESSARY ANY LONGER, DELETE THIS CODE
	 */
/*	for (sint16 x=1; x<old_x; x++) {
		koord k(x,old_y);
		sint16 const height = perlin_hoehe(&settings, k, koord(old_x, old_y));
		// need to raise/lower more
		for(  sint16 dy=-abs(grundwasser-height);  dy<abs(grundwasser-height);  dy++  ) {
			koord pos(x,old_y+dy);
			sint16 const height = perlin_hoehe(&settings, pos, koord(old_x,old_y));
			while(lookup_hgt(pos)<height  &&  raise(pos)) ;
			while(lookup_hgt(pos)>height  &&  lower(pos)) ;
		}
	}
	for (sint16 y=1; y<old_y; y++) {
		koord k(old_x,y);
		sint16 const height = perlin_hoehe(&settings, k, koord(old_x, old_y));
		// need to raise/lower more
		for(  sint16 dx=-abs(grundwasser-height);  dx<abs(grundwasser-height);  dx++  ) {
			koord pos(old_x+dx,y);
			sint16 const height = perlin_hoehe(&settings, pos, koord(old_x, old_y));
			while(lookup_hgt(pos)<height  &&  raise(pos)) ;
			while(lookup_hgt(pos)>height  &&  lower(pos)) ;
		}
	}

	// now recalc the images of the old map near the seam ...
	for (sint16 x=0; x<old_x-20; x++) {
		for (sint16 y=max(old_y-20,0); y<old_y; y++) {
			plan[x+y*cached_grid_size.x].get_kartenboden()->calc_bild();
		}
	}
	for (sint16 x=max(old_x-20,0); x<old_x; x++) {
		for (sint16 y=0; y<old_y; y++) {
			plan[x+y*cached_grid_size.x].get_kartenboden()->calc_bild();
		}
	}
	*/

	// eventual update origin
	switch (settings.get_rotation()) {
		case 1:
			settings.set_origin_y(settings.get_origin_y() - new_groesse_y + old_y);
			break;
		case 2:
			settings.set_origin_x(settings.get_origin_x() - new_groesse_x + old_x);
			settings.set_origin_y(settings.get_origin_y() - new_groesse_y + old_y);
			break;
		case 3:
			settings.set_origin_x(settings.get_origin_x() - new_groesse_y + old_y);
			break;
	}

	// Resize marker_t:
	marker.init(new_groesse_x, new_groesse_y);

	distribute_groundobjs_cities( sets->get_anzahl_staedte(), sets->get_mittlere_einwohnerzahl(), old_x, old_y );

	// hausbauer_t::neue_karte(); <- this would reinit monuments! do not do this!
	fabrikbauer_t::neue_karte( this );
	set_schedule_counter();

	// Refresh the haltlist for the affected tiles / stations.
	// It is enough to check the tile just at the border ...
	uint16 const cov = settings.get_station_coverage();
	if(  old_y < new_groesse_y  ) {
		for(  sint16 x=0;  x<old_x;  x++  ) {
			for(  sint16 y=old_y-cov;  y<old_y;  y++  ) {
				halthandle_t h = plan[x+y*new_groesse_x].get_halt();
				if(  h.is_bound()  ) {
					for(  sint16 xp=max(0,x-cov);  xp<x+cov+1;  xp++  ) {
						for(  sint16 yp=y;  yp<y+cov+1;  yp++  ) {
							plan[xp+yp*new_groesse_x].add_to_haltlist(h);
						}
					}
				}
			}
		}
	}
	if(  old_x < new_groesse_x  ) {
		for(  sint16 x=old_x-cov;  x<old_x;  x++  ) {
			for(  sint16 y=0;  y<old_y;  y++  ) {
				halthandle_t h = plan[x+y*new_groesse_x].get_halt();
				if(  h.is_bound()  ) {
					for(  sint16 xp=x;  xp<x+cov+1;  xp++  ) {
						for(  sint16 yp=max(0,y-cov);  yp<y+cov+1;  yp++  ) {
							plan[xp+yp*new_groesse_x].add_to_haltlist(h);
						}
					}
				}
			}
		}
	}
	clear_random_mode( MAP_CREATE_RANDOM );

	if ( old_x != 0 ) {
		if(is_display_init()) {
			display_show_pointer(true);
		}
		mute_sound(false);

		reliefkarte_t::is_visible = reliefkarte;
		reliefkarte_t::get_karte()->set_welt( this );
		reliefkarte_t::get_karte()->calc_map();
		reliefkarte_t::get_karte()->set_mode( reliefkarte_t::get_karte()->get_mode() );

		set_dirty();
		reset_timer();
	}
	// update main menue
	werkzeug_t::update_toolbars(this);
}


karte_t::karte_t() :
	settings(umgebung_t::default_einstellungen),
	convoi_array(0),
	ausflugsziele(16),
	stadt(0),
	marker(0,0)
{
	// length of day and other time stuff
	ticks_per_world_month_shift = 20;
	ticks_per_world_month = (1 << ticks_per_world_month_shift);
	last_step_ticks = 0;
	server_last_announce_time = 0;
	last_interaction = dr_time();
	step_mode = PAUSE_FLAG;
	time_multiplier = 16;
	next_step_time = last_step_time = 0;
	fix_ratio_frame_time = 200;
	network_frame_count = 0;
	sync_steps = 0;

	for(  uint i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		werkzeug[i] = werkzeug_t::general_tool[WKZ_ABFRAGE];
	}

	follow_convoi = convoihandle_t();
	set_dirty();
	set_scroll_lock(false);

	// standard prices
	warenbauer_t::set_multiplier( 1000 );

	zeiger = 0;
	plan = 0;
	ij_off = koord::invalid;
	x_off = 0;
	y_off = 0;
	grid_hgts = 0;
	schedule_counter = 0;
	nosave_warning = nosave = false;
	loaded_rotation = 0;
	last_year = 1930;
	last_month = 0;

	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		spieler[i] = NULL;
		MEMZERO(player_password_hash[i]);
	}

	// no distance to show at first ...
	show_distance = koord3d::invalid;
	scenario = NULL;

	map_counter = 0;

	msg = new message_t(this);
	cached_size.x = 0;
	cached_size.y = 0;
}


karte_t::~karte_t()
{
	is_sound = false;

	destroy();

	// not deleting the werkzeuge of this map ...
	delete msg;
}


bool karte_t::can_lower_plan_to(sint16 x, sint16 y, sint8 h) const
{
	const planquadrat_t *plan = lookup(koord(x,y));

	if(  plan==NULL  ) {
		return false;
	}

	const sint8 hmax = plan->get_kartenboden()->get_hoehe();
	if(  hmax==h  &&  (plan->get_kartenboden()->get_grund_hang()==0  ||  is_plan_height_changeable(x, y))  ) {
		return true;
	}

	if(  !is_plan_height_changeable(x, y)  ) {
		return false;
	}

	// tunnel slope below?
	grund_t *gr = plan->get_boden_in_hoehe(h-1);
	if (gr && gr->get_grund_hang()!=hang_t::flach) {
		return false;
	}
	// tunnel below?
	while(h < hmax) {
		if(plan->get_boden_in_hoehe(h)) {
			return false;
		}
		h ++;
	}
	return true;
}


bool karte_t::can_raise_plan_to(sint16 x, sint16 y, sint8 h) const
{
	const planquadrat_t *plan = lookup(koord(x,y));
	if(  plan == 0  ||  !is_plan_height_changeable(x, y)  ) {
		return false;
	}

	// irgendwo eine Bruecke im Weg?
	int hmin = plan->get_kartenboden()->get_hoehe();
	while(h > hmin) {
		if(plan->get_boden_in_hoehe(h)) {
			return false;
		}
		h --;
	}
	return true;
}


bool karte_t::is_plan_height_changeable(sint16 x, sint16 y) const
{
	const planquadrat_t *plan = lookup(koord(x,y));
	bool ok = true;

	if(plan != NULL) {
		grund_t *gr = plan->get_kartenboden();

		ok = (gr->ist_natur() || gr->ist_wasser())  &&  !gr->hat_wege()  &&  !gr->is_halt();

		for(  int i=0; ok  &&  i<gr->get_top(); i++  ) {
			const ding_t *dt = gr->obj_bei(i);
			assert(dt != NULL);
			ok =
				dt->get_typ() == ding_t::baum  ||
				dt->get_typ() == ding_t::zeiger  ||
				dt->get_typ() == ding_t::wolke  ||
				dt->get_typ() == ding_t::sync_wolke  ||
				dt->get_typ() == ding_t::async_wolke  ||
				dt->get_typ() == ding_t::groundobj;
		}
	}

	return ok;
}


// since very large mountains can freeze the system
// we call ociasionally INT_CHECK()
static int raise_frame_counter = 0;


/* raise plan
 * new heights for each corner given
 * only test corners in ctest to avoid infinite loops
 */
bool karte_t::can_raise_to(sint16 x, sint16 y, bool keep_water, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw, uint8 ctest) const
{
	bool ok = false;
	if(is_within_limits(x,y)) {
		grund_t *gr = lookup_kartenboden(koord(x,y));
		const sint8 h0 = gr->get_hoehe();
		// which corners have to be raised?
		const sint8 h0_sw = corner1(ctest) ? (gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x,y+1)))   : h0 + corner1(gr->get_grund_hang()) ) : hsw+1;
		const sint8 h0_se = corner2(ctest) ? (gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x+1,y+1))) : h0 + corner2(gr->get_grund_hang()) ) : hse+1;
		const sint8 h0_ne = corner3(ctest) ? (gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x+1,y)))   : h0 + corner3(gr->get_grund_hang()) ) : hne+1;
		const sint8 h0_nw = corner4(ctest) ? (gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x,y)))     : h0 + corner4(gr->get_grund_hang()) ) : hnw+1;

		const sint8 max_hgt = max(max(hsw,hse),max(hne,hnw));

		if (gr->ist_wasser()  &&  keep_water  &&  max_hgt > grundwasser) {
			return false;
		}

		// more than 4096 calls => might need a screen update ...
		if(  ((++raise_frame_counter)&0x0FFF) == 0  ) {
			INT_CHECK( "somworld.cc" );
		}

		ok = can_raise_plan_to( x, y, max_hgt);
		// sw
		if (ok && h0_sw < hsw) {
			ok = can_raise_to( x-1, y+1, keep_water, hsw-1, hsw-1, hsw, hsw-1, 11);
		}
		// s
		if (ok && (h0_se < hse || h0_sw < hsw) && ((ctest&3)==3)) {
			const sint8 hs = max(hse, hsw) -1;
			ok = can_raise_to(x,y+1, keep_water, hs, hs, hse, hsw, 3);
		}
		// se
		if (ok && h0_se < hse) {
			ok = can_raise_to(x+1,y+1, keep_water, hse-1, hse-1, hse-1, hse, 7);
		}
		// e
		if (ok && (h0_se < hse || h0_ne < hne) && ((ctest&6)==6)) {
			const sint8 he = max(hse, hne) -1;
			ok = can_raise_to(x+1,y, keep_water, hse, he, he, hne, 6);
		}
		// ne
		if (ok && h0_ne < hne) {
			ok = can_raise_to(x+1,y-1, keep_water, hne, hne-1, hne-1, hne-1, 14);
		}
		// n
		if (ok && (h0_nw < hnw || h0_ne < hne) && ((ctest&12)==12)) {
			const sint8 hn = max(hnw, hne) -1;
			ok = can_raise_to(x,y-1, keep_water, hnw, hne, hn, hn, 12);
		}
		// nw
		if (ok && h0_nw < hnw) {
			ok = can_raise_to(x-1,y-1, keep_water, hnw-1, hnw, hnw-1, hnw-1, 13);
		}
		// w
		if (ok && (h0_nw < hnw || h0_sw < hsw) && ((ctest&9)==9)) {
			const sint8 hw = max(hnw, hsw) -1;
			ok = can_raise_to(x-1,y, keep_water, hw, hsw, hnw, hw, 9);
		}
	}
	else {
		ok = true;
	}
	return ok;
}


int karte_t::raise_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw)
{
	int n=0;
	if(is_within_limits(x,y)) {
		grund_t *gr = lookup_kartenboden(koord(x,y));
		const sint8 h0 = gr->get_hoehe();
		// old height
		const sint8 h0_sw = gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x,y+1)))   : h0 + corner1(gr->get_grund_hang());
		const sint8 h0_se = gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x+1,y+1))) : h0 + corner2(gr->get_grund_hang());
		const sint8 h0_ne = gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x+1,y)))   : h0 + corner3(gr->get_grund_hang());
		const sint8 h0_nw = gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x,y)))     : h0 + corner4(gr->get_grund_hang());

		// new height
		const sint8 hn_sw = max(hsw, h0_sw);
		const sint8 hn_se = max(hse, h0_se);
		const sint8 hn_ne = max(hne, h0_ne);
		const sint8 hn_nw = max(hnw, h0_nw);
		// nothing to do?
		if (!gr->ist_wasser()  &&  h0_sw >= hsw  &&  h0_se >= hse  &&  h0_ne >= hne  &&  h0_nw >= hnw) return 0;
		// calc new height and slope
		const sint8 hneu = min(min(hn_sw,hn_se), min(hn_ne,hn_nw));
		bool ok = ( (hn_sw-hneu<2) && (hn_se-hneu<2) && (hn_ne-hneu<2) && (hn_nw-hneu<2)); // may fail on water tiles since lookup_hgt might be modified from previous raise_to calls
		if (!ok && !gr->ist_wasser()) {
			assert(false);
		}
		const uint8 sneu = (hn_sw-hneu) | ((hn_se-hneu)<<1) | ((hn_ne-hneu)<<2) | ((hn_nw-hneu)<<3);
		// change height and slope, for water tiles only if they will become land
		if (!gr->ist_wasser() || (hneu + (sneu ? 1 : 0) > grundwasser)) {
			gr->set_pos( koord3d(x,y,hneu));
			gr->set_grund_hang( (hang_t::typ)sneu );
			access(x,y)->angehoben(this);
		}

		// update north point in grid
		set_grid_hgt(koord(x,y), hn_nw);
		if ( x == cached_size.x ) {
			// update eastern grid coordinates too if we are in the edge.
			set_grid_hgt(koord(x+1, y), hn_ne);
			set_grid_hgt(koord(x+1, y+1), hn_se);
		}
		if ( y == cached_size.y ) {
			// update southern grid coordinates too if we are in the edge.
			set_grid_hgt(koord(x, y+1), hn_sw);
			set_grid_hgt(koord(x+1, y+1), hn_se);
		}

		n += hn_sw-h0_sw + hn_se-h0_se + hn_ne-h0_ne + hn_nw-h0_nw;

		// sw
		if (h0_sw < hsw) {
			n += raise_to(x-1,y+1, hsw-1, hsw-1, hsw, hsw-1);
		}
		// s
		if (h0_sw < hsw  ||  h0_se < hse) {
			const sint8 hs = max(hse, hsw) -1;
			n += raise_to(x,y+1, hs, hs, hse, hsw);
		}
		// se
		if (h0_se < hse) {
			n += raise_to(x+1,y+1, hse-1, hse-1, hse-1, hse);
		}
		// e
		if (h0_se < hse  ||  h0_ne < hne) {
			const sint8 he = max(hse, hne) -1;
			n += raise_to(x+1,y, hse, he, he, hne);
		}
		// ne
		if (h0_ne < hne) {
			n += raise_to(x+1,y-1, hne, hne-1, hne-1, hne-1);
		}
		// n
		if (h0_nw < hnw  ||  h0_ne < hne) {
			const sint8 hn = max(hnw, hne) -1;
			n += raise_to(x,y-1, hnw, hne, hn, hn);
		}
		// nw
		if (h0_nw < hnw) {
			n += raise_to(x-1,y-1, hnw-1, hnw, hnw-1, hnw-1);
		}
		// w
		if (h0_sw < hsw  ||  h0_nw < hnw) {
			const sint8 hw = max(hnw, hsw) -1;
			n += raise_to(x-1,y, hw, hsw, hnw, hw);
		}
		lookup_kartenboden(koord(x,y))->calc_bild();
		if ( (x+1) < cached_size.x ) {
			lookup_kartenboden(koord(x+1,y))->calc_bild();
		}
		if ( (y+1) < cached_size.y ) {
			lookup_kartenboden(koord(x,y+1))->calc_bild();
		}
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
#ifndef DOUBLE_GROUNDS
			const sint8 hh = h-1;
#else
			const sint8 hh = h-2;
#endif
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


int karte_t::grid_raise(koord pos)
{
	int n = 0;

	if(is_within_grid_limits(pos)) {

		const grund_t *gr = lookup_kartenboden_gridcoords(pos);
		const hang_t::typ corner_to_raise = get_corner_to_operate(pos);

		const sint16 x = gr->get_pos().x;
		const sint16 y = gr->get_pos().y;
		const sint8 hgt = gr->get_hoehe(corner_to_raise);

		const sint8 hsw = hgt + corner1(corner_to_raise);
		const sint8 hse = hgt + corner2(corner_to_raise);
		const sint8 hne = hgt + corner3(corner_to_raise);
		const sint8 hnw = hgt + corner4(corner_to_raise);

		if (can_raise_to(x, y, false, hsw, hse, hne, hnw)) {
			n = raise_to(x, y, hsw, hse, hne, hnw);
			// force world full redraw, or background could be dirty.
			set_dirty();
		}
	}
	return (n+3)>>2;
}


// lower plan
// new heights for each corner given
// only test corners in ctest to avoid infinite loops
bool karte_t::can_lower_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw, uint8 ctest) const
{
	bool ok = false;
	if(is_within_limits(x,y)) {
		grund_t *gr = lookup_kartenboden(koord(x,y));
		const sint8 h0 = gr->get_hoehe();
		// which corners have to be raised?
		const sint8 h0_sw = corner1(ctest) ? (gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x,y+1)))   : h0 + corner1(gr->get_grund_hang()) ) : hsw-1;
		const sint8 h0_se = corner2(ctest) ? (gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x+1,y+1))) : h0 + corner2(gr->get_grund_hang()) ) : hse-1;
		const sint8 h0_ne = corner3(ctest) ? (gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x,y+1)))   : h0 + corner3(gr->get_grund_hang()) ) : hne-1;
		const sint8 h0_nw = corner4(ctest) ? (gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x,y)))     : h0 + corner4(gr->get_grund_hang()) ) : hnw-1;

		ok = can_lower_plan_to(x,y, min(min(hsw,hse),min(hne,hnw)));
		// sw
		if (ok && h0_sw > hsw) {
			ok = can_lower_to(x-1,y+1, hsw+1, hsw+1, hsw, hsw+1, 11);
		}
		// s
		if (ok && (h0_se > hse || h0_sw > hsw) && ((ctest&3)==3)) {
			const sint8 hs = min(hse, hsw) +1;
			ok = can_lower_to(x,y+1, hs, hs, hse, hsw, 3);
		}
		// se
		if (ok && h0_se > hse) {
			ok = can_lower_to(x+1,y+1, hse+1, hse+1, hse+1, hse, 7);
		}
		// e
		if (ok && (h0_se > hse || h0_ne > hne) && ((ctest&6)==6)) {
			const sint8 he = max(hse, hne) +1;
			ok = can_lower_to(x+1,y, hse, he, he, hne, 6);
		}
		// ne
		if (ok && h0_ne > hne) {
			ok = can_lower_to(x+1,y-1, hne, hne+1, hne+1, hne+1, 14);
		}
		// n
		if (ok && (h0_nw > hnw || h0_ne > hne) && ((ctest&12)==12)) {
			const sint8 hn = min(hnw, hne) +1;
			ok = can_lower_to(x,y-1, hnw, hne, hn, hn, 12);
		}
		// nw
		if (ok && h0_nw > hnw) {
			ok = can_lower_to(x-1,y-1, hnw+1, hnw, hnw+1, hnw+1, 13);
		}
		// w
		if (ok && (h0_nw > hnw || h0_sw > hsw) && ((ctest&9)==9)) {
			const sint8 hw = min(hnw, hsw) +1;
			ok = can_lower_to(x-1,y, hw, hsw, hnw, hw, 9);
		}
	}
	else {
		// border tile of map
		ok=true;
	}
	return ok;
}


int karte_t::lower_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw)
{
	int n=0;
	if(is_within_limits(x,y)) {
		grund_t *gr = lookup_kartenboden(koord(x,y));
		const sint8 h0 = gr->get_hoehe();
		// old height
		const sint8 h0_sw = gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x,y+1)))   : h0 + corner1(gr->get_grund_hang());
		const sint8 h0_se = gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x+1,y+1))) : h0 + corner2(gr->get_grund_hang());
		const sint8 h0_ne = gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x+1,y)))   : h0 + corner3(gr->get_grund_hang());
		const sint8 h0_nw = gr->ist_wasser() ? min(grundwasser, lookup_hgt(koord(x,y)))     : h0 + corner4(gr->get_grund_hang());
		// new height
		const sint8 hn_sw = min(hsw, h0_sw);
		const sint8 hn_se = min(hse, h0_se);
		const sint8 hn_ne = min(hne, h0_ne);
		const sint8 hn_nw = min(hnw, h0_nw);
		// nothing to do?
		if(  gr->ist_wasser()  ) {
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
		const sint8 hneu = min(min(hn_sw,hn_se), min(hn_ne,hn_nw));
		bool ok = ( (hn_sw-hneu<2) && (hn_se-hneu<2) && (hn_ne-hneu<2) && (hn_nw-hneu<2)); // may fail on water tiles since lookup_hgt might be modified from previous lower_to calls
		if(  !ok && !gr->ist_wasser()  ) {
			assert(false);
		}
		const uint8 sneu = (hn_sw-hneu) | ((hn_se-hneu)<<1) | ((hn_ne-hneu)<<2) | ((hn_nw-hneu)<<3);
		// change height and slope for land tiles only
		if (!gr->ist_wasser()) {
			gr->set_pos( koord3d(x,y,hneu));
			gr->set_grund_hang( (hang_t::typ)sneu );
			access(x,y)->abgesenkt(this);
		}
		// update north point in grid
		set_grid_hgt(koord(x, y), hn_nw);
		if ( x == cached_size.x ) {
			// update eastern grid coordinates too if we are in the edge.
			set_grid_hgt(koord(x+1, y), hn_ne);
			set_grid_hgt(koord(x+1, y+1), hn_se);
		}
		if ( y == cached_size.y ) {
			// update southern grid coordinates too if we are in the edge.
			set_grid_hgt(koord(x, y+1), hn_sw);
			set_grid_hgt(koord(x+1, y+1), hn_se);
		}

		n += h0_sw-hn_sw + h0_se-hn_se + h0_ne-hn_ne + h0_nw-hn_nw;

		// sw
		if (h0_sw > hsw) {
			n += lower_to(x-1,y+1, hsw+1, hsw+1, hsw, hsw+1);
		}
		// s
		if (h0_sw > hsw  ||  h0_se > hse) {
			const sint8 hs = min(hse, hsw) +1;
			n += lower_to(x,y+1, hs, hs, hse, hsw);
		}
		// se
		if (h0_se > hse) {
			n += lower_to(x+1,y+1, hse+1, hse+1, hse+1, hse);
		}
		// e
		if (h0_se > hse  ||  h0_ne > hne) {
			const sint8 he = min(hse, hne) +1;
			n += lower_to(x+1,y, hse, he, he, hne);
		}
		// ne
		if (h0_ne > hne) {
			n += lower_to(x+1,y-1, hne, hne+1, hne+1, hne+1);
		}
		// n
		if (h0_nw > hnw  ||  h0_ne > hne) {
			const sint8 hn = min(hnw, hne) +1;
			n += lower_to(x,y-1, hnw, hne, hn, hn);
		}
		// nw
		if (h0_nw > hnw) {
			n += lower_to(x-1,y-1, hnw+1, hnw, hnw+1, hnw+1);
		}
		// w
		if (h0_sw > hsw  ||  h0_nw > hnw) {
			const sint8 hw = min(hnw, hsw) +1;
			n += lower_to(x-1,y, hw, hsw, hnw, hw);
		}

		lookup_kartenboden(koord(x,y))->calc_bild();
		if( (x+1) < cached_size.x ) {
			lookup_kartenboden(koord(x+1,y))->calc_bild();
		}
		if( (y+1) < cached_size.y ) {
			lookup_kartenboden(koord(x,y+1))->calc_bild();
		}
	}
	return n;
}


void karte_t::lower_grid_to(sint16 x, sint16 y, sint8 h)
{
	if(is_within_grid_limits(x,y)) {
		const sint32 offset = x + y*(cached_grid_size.x+1);

		if(  grid_hgts[offset] > h  ) {
			grid_hgts[offset] = h;
#ifndef DOUBLE_GROUNDS
			sint8 hh = h+1;
#else
			sint8 hh = h+2;
#endif
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


int karte_t::grid_lower(koord pos)
{
	int n = 0;

	if(is_within_grid_limits(pos)) {

		const grund_t *gr = lookup_kartenboden_gridcoords(pos);
		const hang_t::typ corner_to_lower = get_corner_to_operate(pos);

		const sint16 x = gr->get_pos().x;
		const sint16 y = gr->get_pos().y;
		const sint8 hgt = gr->get_hoehe(corner_to_lower);

		const sint8 hsw = hgt - corner1(corner_to_lower);
		const sint8 hse = hgt - corner2(corner_to_lower);
		const sint8 hne = hgt - corner3(corner_to_lower);
		const sint8 hnw = hgt - corner4(corner_to_lower);

		if (can_lower_to(x, y, hsw, hse, hne, hnw)) {
			n = lower_to(x, y, hsw, hse, hne, hnw);
		}
	}
	return (n+3)>>2;
}


bool karte_t::can_ebne_planquadrat(koord pos, sint8 hgt, bool keep_water, bool make_underwater_hill) const
{
	const grund_t *gr = lookup_kartenboden(pos);
	const sint8 old_hgt = make_underwater_hill  &&  gr->ist_wasser() ? min_hgt(pos) : gr->get_hoehe();
	if (old_hgt>=hgt) {
		return can_lower_to(pos.x, pos.y, hgt, hgt, hgt, hgt);
	}
	else {
		return can_raise_to(pos.x, pos.y, keep_water, hgt, hgt, hgt, hgt);
	}
}


// make a flat leve at this position (only used for AI at the moment)
bool karte_t::ebne_planquadrat(spieler_t *sp, koord pos, sint8 hgt, bool keep_water, bool make_underwater_hill)
{
	int n = 0;
	bool ok = false;
	const grund_t *gr = lookup_kartenboden(pos);
	const sint8 old_hgt = make_underwater_hill  &&  gr->ist_wasser() ? min_hgt(pos) : gr->get_hoehe();
	if(  old_hgt>=hgt  ) {
		if(  can_lower_to(pos.x, pos.y, hgt, hgt, hgt, hgt)  ) {
			n = lower_to(pos.x, pos.y, hgt, hgt, hgt, hgt);
			ok = true;
		}
	}
	else {
		if(  can_raise_to(pos.x, pos.y, keep_water, hgt, hgt, hgt, hgt)  ) {
			n = raise_to(pos.x, pos.y, hgt, hgt, hgt, hgt);
			ok = true;
		}
	}
	// was changed => pay for it
	if(n>0) {
		n = (n+3) >> 2;
		spieler_t::book_construction_costs(sp, n * settings.cst_alter_land, pos, ignore_wt);
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
		if (spieler[i]) {
			spieler[i]->check_unlock(player_password_hash[i]);
		}
	}
}


void karte_t::rdwr_player_password_hashes(loadsave_t *file)
{
	pwd_hash_t dummy;
	for(  int i=0;  i<PLAYER_UNOWNED; i++  ) {
		pwd_hash_t *p = spieler[i] ? &spieler[i]->access_password_hash() : &dummy;
		for(  uint8 j=0; j<20; j++) {
			file->rdwr_byte( (*p)[j] );
		}
	}
}


void karte_t::call_change_player_tool(uint8 cmd, uint8 player_nr, uint16 param, bool scripted_call)
{
	if (umgebung_t::networkmode) {
		nwc_chg_player_t *nwc = new nwc_chg_player_t(sync_steps, map_counter, cmd, player_nr, param, scripted_call);

		network_send_server(nwc);
	}
	else {
		change_player_tool(cmd, player_nr, param, !get_spieler(1)->is_locked()  ||  scripted_call, true);
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
			if ( (param != spieler_t::HUMAN  &&  !public_player_unlocked)  ||  param >= spieler_t::MAX_AI) {
				return false;
			}
			// range check, player already existent?
			if ( player_nr >= PLAYER_UNOWNED  ||   get_spieler(player_nr) ) {
				return false;
			}
			if (exec) {
				new_spieler( player_nr, param );
				// activate/deactivate AI immediately
				spieler_t *sp = get_spieler(player_nr);
				if (param != spieler_t::HUMAN  &&  sp) {
					sp->set_active(true);
					settings.set_player_active(player_nr, sp->is_active());
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
			if ( player_nr >= PLAYER_UNOWNED  ||   get_spieler(player_nr)==NULL ) {
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


void karte_t::set_werkzeug( werkzeug_t *w, spieler_t *sp )
{
	if(  get_random_mode()&LOAD_RANDOM  ) {
		dbg->warning("karte_t::set_werkzeug", "Ignored tool %i during loading.", w->get_id() );
		return;
	}
	bool scripted_call = w->is_scripted();
	// check for scenario conditions
	if(  !scripted_call  &&  !scenario->is_tool_allowed(sp, w->get_id(), w->get_waytype())  ) {
		return;
	}
	// check for password-protected players
	if(  (!w->is_init_network_save()  ||  !w->is_work_network_save())  &&  !scripted_call  &&
		 !(w->get_id()==(WKZ_SET_PLAYER_TOOL|SIMPLE_TOOL)  ||  w->get_id()==(WKZ_ADD_MESSAGE_TOOL|SIMPLE_TOOL))  &&
		 sp  &&  sp->is_locked()  ) {
		// player is currently password protected => request unlock first
		create_win( -1, -1, new password_frame_t(sp), w_info, magic_pwd_t + sp->get_player_nr() );
		return;
	}
	w->flags |= event_get_last_control_shift();
	if(!umgebung_t::networkmode  ||  w->is_init_network_save()  ) {
		local_set_werkzeug(w, sp);
	}
	else {
		// queue tool for network
		nwc_tool_t *nwc = new nwc_tool_t(sp, w, zeiger->get_pos(), steps, map_counter, true);
		network_send_server(nwc);
	}
}


// set a new tool on our client, calls init
void karte_t::local_set_werkzeug( werkzeug_t *w, spieler_t * sp )
{
	w->flags |= werkzeug_t::WFL_LOCAL;

	if (get_scenario()->is_scripted()  &&  !get_scenario()->is_tool_allowed(sp, w->get_id()) ) {
		w->flags = 0;
		return;
	}
	// now call init
	bool init_result = w->init(this,sp);
	// for unsafe tools init() must return false
	assert(w->is_init_network_save()  ||  !init_result);

	if (init_result) {

		set_dirty();
		werkzeug_t *sp_wkz = werkzeug[sp->get_player_nr()];
		if(w != sp_wkz) {

			// reinit same tool => do not play sound twice
			sound_play(SFX_SELECT);

			// only exit, if it is not the same tool again ...
			sp_wkz->flags |= werkzeug_t::WFL_LOCAL;
			sp_wkz->exit(this, sp);
			sp_wkz->flags =0;
		}

		if(  sp==active_player  ) {
			// reset pointer
			koord3d zpos = zeiger->get_pos();
			// remove marks
			zeiger->change_pos( koord3d::invalid );
			// set new cursor properties
			w->init_cursor(zeiger);
			// .. and mark again (if the position is acceptable for the tool)
			if( w->check_valid_pos(this,zpos.get_2d())) {
				zeiger->change_pos( zpos );
			}
			else {
				zeiger->change_pos( koord3d::invalid );
			}
		}
		werkzeug[sp->get_player_nr()] = w;
	}
	w->flags = 0;
}


sint8 karte_t::min_hgt(const koord pos) const
{
	const sint8 h1 = lookup_hgt(pos);
	const sint8 h2 = lookup_hgt(pos+koord(1, 0));
	const sint8 h3 = lookup_hgt(pos+koord(1, 1));
	const sint8 h4 = lookup_hgt(pos+koord(0, 1));

	return min(min(h1,h2), min(h3,h4));
}


sint8 karte_t::max_hgt(const koord pos) const
{
	const sint8 h1 = lookup_hgt(pos);
	const sint8 h2 = lookup_hgt(pos+koord(1, 0));
	const sint8 h3 = lookup_hgt(pos+koord(1, 1));
	const sint8 h4 = lookup_hgt(pos+koord(0, 1));

	return max(max(h1,h2), max(h3,h4));
}

planquadrat_t *rotate90_new_plan;

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
}


void karte_t::rotate90()
{
DBG_MESSAGE( "karte_t::rotate90()", "called" );
	// asumme we can save this rotation
	nosave_warning = nosave = false;

	//announce current target rotation
	settings.rotate90();

	// clear marked region
	zeiger->change_pos( koord3d::invalid );

	// preprocessing, detach stops from factories to prevent crash
	FOR(slist_tpl<halthandle_t>, const s, haltestelle_t::get_alle_haltestellen()) {
		s->release_factory_links();
	}

	//rotate plans in parallel posix thread ...
	rotate90_new_plan = new planquadrat_t[cached_grid_size.y*cached_grid_size.x];

	world_xy_loop(&karte_t::rotate90_plans, false);

	grund_t::finish_rotate90();

	delete [] plan;
	plan = rotate90_new_plan;

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

	// fixed order fabrik, halts, convois
	FOR(slist_tpl<fabrik_t*>, const f, fab_list) {
		f->rotate90(cached_size.x);
	}
	// after rotation of factories, rotate everything that holds freight: stations and convoys
	FOR(slist_tpl<halthandle_t>, const s, haltestelle_t::get_alle_haltestellen()) {
		s->rotate90(cached_size.x);
	}

	FOR(vector_tpl<convoihandle_t>, const i, convoi_array) {
		i->rotate90(cached_size.x);
	}

	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  spieler[i]  ) {
			spieler[i]->rotate90( cached_size.x );
		}
	}

	// rotate label texts
	FOR(slist_tpl<koord>, & l, labels) {
		l.rotate90(cached_size.x);
	}

	// rotate view
	ij_off.rotate90( cached_size.x );

	// rotate messages
	msg->rotate90( cached_size.x );

	// rotate view in dialoge windows
	win_rotate90( cached_size.x );

	if( cached_grid_size.x != cached_grid_size.y ) {
		// the marking array and the map must be reinit
		marker.init( cached_grid_size.x, cached_grid_size.y );
		reliefkarte_t::get_karte()->set_welt( this );
	}

	//  rotate map search array
	fabrikbauer_t::neue_karte( this );

	// update minimap
	if(reliefkarte_t::is_visible) {
		reliefkarte_t::get_karte()->set_mode( reliefkarte_t::get_karte()->get_mode() );
	}

	get_scenario()->rotate90( cached_size.x );

	// finally recalculate schedules for goods in transit ...
	set_schedule_counter();

	set_dirty();
}
// -------- Verwaltung von Fabriken -----------------------------


bool karte_t::add_fab(fabrik_t *fab)
{
//DBG_MESSAGE("karte_t::add_fab()","fab = %p",fab);
	assert(fab != NULL);
	fab_list.insert( fab );
	goods_in_game.clear(); // Force rebuild of goods list
	return true;
}


// beware: must remove also links from stops and towns
bool karte_t::rem_fab(fabrik_t *fab)
{
	if(!fab_list.remove( fab )) {
		return false;
	}

	// Force rebuild of goods list
	goods_in_game.clear();

	// now all the interwoven connections must be cleared
	koord pos = fab->get_pos().get_2d();
	planquadrat_t* plan = access(pos);
	if(plan) {

		// we need a copy, since the verbinde fabriken is modifying the list
		halthandle_t list[16];
		const uint8 count = plan->get_haltlist_count();
		assert(count<16);
		memcpy( list, plan->get_haltlist(), count*sizeof(halthandle_t) );
		for( uint8 i=0;  i<count;  i++  ) {
			// first remove all the tiles that do not connect
			plan->remove_from_haltlist( this, list[i] );
			// then reconnect
			list[i]->verbinde_fabriken();
		}

		// remove all links from factories
		FOR(slist_tpl<fabrik_t*>, const fab, fab_list) {
			fab->rem_lieferziel(pos);
			fab->rem_supplier(pos);
		}

		// remove all links to cities
		fab->clear_target_cities();

		// finally delete it
		delete fab;

		// recalculate factory position map
		fabrikbauer_t::neue_karte(this);
	}
	return true;
}


/*----------------------------------------------------------------------------------------------------------------------*/
/* same procedure for tourist attractions */


void karte_t::add_ausflugsziel(gebaeude_t *gb)
{
	assert(gb != NULL);
	ausflugsziele.append( gb, gb->get_tile()->get_besch()->get_level() );

	// Knightly : add links between this attraction and all cities
	FOR(weighted_vector_tpl<stadt_t*>, const c, stadt) {
		c->add_target_attraction(gb);
	}
}


void karte_t::remove_ausflugsziel(gebaeude_t *gb)
{
	assert(gb != NULL);
	ausflugsziele.remove( gb );

	// Knightly : remove links between this attraction and all cities
	FOR(weighted_vector_tpl<stadt_t*>, const c, stadt) {
		c->remove_target_attraction(gb);
	}
}


// -------- Verwaltung von Staedten -----------------------------

stadt_t *karte_t::suche_naechste_stadt(const koord pos) const
{
	long min_dist = 99999999;
	stadt_t *best = NULL;

	if(is_within_limits(pos)) {
		FOR(weighted_vector_tpl<stadt_t*>, const s, stadt) {
			const koord k = s->get_pos();
			const long dist = (pos.x-k.x)*(pos.x-k.x) + (pos.y-k.y)*(pos.y-k.y);
			if(dist < min_dist) {
					min_dist = dist;
					best = s;
			}
		}
	}
	return best;
}


// -------- Verwaltung von synchronen Objekten ------------------

static volatile bool sync_step_running = false;
static volatile bool sync_step_eyecandy_running = false;
static volatile bool sync_way_eyecandy_running = false;

// handling animations and the like
bool karte_t::sync_eyecandy_add(sync_steppable *obj)
{
	if(  sync_step_eyecandy_running  ) {
		sync_eyecandy_add_list.insert( obj );
	}
	else {
		sync_eyecandy_list.put( obj, obj );
	}
	return true;
}


bool karte_t::sync_eyecandy_remove(sync_steppable *obj)	// entfernt alle dinge == obj aus der Liste
{
	if(  sync_step_eyecandy_running  ) {
		sync_eyecandy_remove_list.append(obj);
	}
	else {
		if(  sync_eyecandy_add_list.remove(obj)  ) {
			return true;
		}
		return sync_eyecandy_list.remove(obj)!=NULL;
	}
	return false;
}


void karte_t::sync_eyecandy_step(long delta_t)
{
	sync_step_eyecandy_running = true;
	// first add everything
	while(  !sync_eyecandy_add_list.empty()  ) {
		sync_steppable *obj = sync_eyecandy_add_list.remove_first();
		sync_eyecandy_list.put( obj, obj );
	}
	// now remove everything from last time
	sync_step_eyecandy_running = false;
	while(  !sync_eyecandy_remove_list.empty()  ) {
		sync_eyecandy_list.remove( sync_eyecandy_remove_list.remove_first() );
	}
	// now step ...
	sync_step_eyecandy_running = true;
	for(  ptrhashtable_tpl<sync_steppable*,sync_steppable*>::iterator iter = sync_eyecandy_list.begin();  iter != sync_eyecandy_list.end();  ) {
		// if false, then remove
		sync_steppable *ss = iter->key;
		if(!ss->sync_step(delta_t)) {
			iter = sync_eyecandy_list.erase(iter);
			delete ss;
		}
		else {
			++iter;
		}
	}
	// now remove everything from last time
	sync_step_eyecandy_running = false;
	while(  !sync_eyecandy_remove_list.empty()  ) {
		sync_eyecandy_list.remove( sync_eyecandy_remove_list.remove_first() );
	}
	sync_step_eyecandy_running = false;
}


// and now the same for pedestrians
bool karte_t::sync_way_eyecandy_add(sync_steppable *obj)
{
	if(  sync_way_eyecandy_running  ) {
		sync_way_eyecandy_add_list.insert( obj );
	}
	else {
		sync_way_eyecandy_list.append( obj );
	}
	return true;
}


bool karte_t::sync_way_eyecandy_remove(sync_steppable *obj)	// entfernt alle dinge == obj aus der Liste
{
	if(  sync_way_eyecandy_running  ) {
		sync_way_eyecandy_remove_list.append(obj);
	}
	else {
		if(  sync_way_eyecandy_add_list.remove(obj)  ) {
			return true;
		}
		return sync_way_eyecandy_list.remove(obj);
	}
	return false;
}


void karte_t::sync_way_eyecandy_step(long delta_t)
{
	sync_way_eyecandy_running = true;
	// first add everything
	while(  !sync_way_eyecandy_add_list.empty()  ) {
		sync_steppable *obj = sync_way_eyecandy_add_list.remove_first();
		sync_way_eyecandy_list.append( obj );
	}
	// now remove everything from last time
	sync_way_eyecandy_running = false;
	while(  !sync_way_eyecandy_remove_list.empty()  ) {
		sync_way_eyecandy_list.remove( sync_way_eyecandy_remove_list.remove_first() );
	}
	// now the actualy stepping
	sync_way_eyecandy_running = true;
#ifndef SYNC_VECTOR
	for(  slist_tpl<sync_steppable*>::iterator i=sync_way_eyecandy_list.begin();  !i.end();  ) {
		// if false, then remove
		sync_steppable *ss = *i;
		if(!ss->sync_step(delta_t)) {
			i = sync_list.erase(i);
			delete ss;
		}
		else {
			++i;
		}
	}
#else
	static vector_tpl<sync_steppable *> sync_way_eyecandy_list_copy;
	sync_way_eyecandy_list_copy.resize( sync_way_eyecandy_list.get_count()*1.1 );
	FOR(vector_tpl<sync_steppable*>, const ss, sync_way_eyecandy_list) {
		// if false, then remove
		if(!ss->sync_step(delta_t)) {
			delete ss;
		}
		else {
			sync_way_eyecandy_list_copy.append( ss );
		}
	}
	swap( sync_way_eyecandy_list_copy, sync_way_eyecandy_list );
	sync_way_eyecandy_list_copy.clear();
#endif
	// now remove everything from last time
	sync_way_eyecandy_running = false;
	while(  !sync_way_eyecandy_remove_list.empty()  ) {
		sync_way_eyecandy_list.remove( sync_way_eyecandy_remove_list.remove_first() );
	}
}


// ... and now all regular stuff, which needs to are in the same order on any plattform
// Thus we are using (slower) lists/vectors and no pointerhashtables
bool karte_t::sync_add(sync_steppable *obj)
{
	if(  sync_step_running  ) {
		sync_add_list.insert( obj );
	}
	else {
		sync_list.append( obj );
	}
	return true;
}


bool karte_t::sync_remove(sync_steppable *obj)	// entfernt alle dinge == obj aus der Liste
{
	if(  sync_step_running  ) {
		sync_remove_list.append(obj);
	}
	else {
		if(sync_add_list.remove(obj)) {
			return true;
		}
		return sync_list.remove(obj);
	}
	return false;
}


/*
 * this routine is called before an image is displayed
 * it moves vehicles and pedestrians
 * only time consuming thing are done in step()
 * everything else is done here
 */
void karte_t::sync_step(long delta_t, bool sync, bool display )
{
	set_random_mode( SYNC_STEP_RANDOM );
	if(sync) {
		// only omitted, when called to display a new frame during fast forward

		// just for progress
		if(  delta_t > 10000  ) {
			dbg->error( "karte_t::sync_step()", "delta_t too large: %li", delta_t );
			delta_t = 10000;
		}
		ticks += delta_t;

		set_random_mode( INTERACTIVE_RANDOM );

		/* animations do not require exact sync
		 * foundations etc are added removed freuently during city growth
		 * => they are now in a hastable!
		 */
		sync_eyecandy_step( delta_t );

		/* pedestrians do not require exact sync and are added/removed frequently
		 * => they are now in a hastable!
		 */
		sync_way_eyecandy_step( delta_t );

		clear_random_mode( INTERACTIVE_RANDOM );

		/* and now the rest for the other moving stuff */
		sync_step_running = true;
#ifndef SYNC_VECTOR
		// insert new objects created during last sync_step (eg vehicle smoke)
		if(!sync_add_list.empty()) {
			sync_list.append_list(sync_add_list);
		}
#else
		while(  !sync_add_list.empty()  ) {
			sync_list.append( sync_add_list.remove_first() );
		}
#endif

		// now remove everything from last time
		sync_step_running = false;
		while(  !sync_remove_list.empty()  ) {
			sync_list.remove( sync_remove_list.remove_first() );
		}

		sync_step_running = true;
#ifndef SYNC_VECTOR
		for(  slist_tpl<sync_steppable*>::iterator i=sync_list.begin();  !i.end();  ) {
			// if false, then remove
			sync_steppable *ss = *i;
			if(!ss->sync_step(delta_t)) {
				i = sync_list.erase(i);
				delete ss;
			}
			else {
				++i;
			}
		}
#else
		static vector_tpl<sync_steppable *> sync_list_copy;
		sync_list_copy.resize( sync_list.get_count() );
		FOR(vector_tpl<sync_steppable*>, const ss, sync_list) {
			// if false, then remove
			if(!ss->sync_step(delta_t)) {
				delete ss;
			}
			else {
				sync_list_copy.append( ss );
			}
		}
		swap( sync_list_copy, sync_list );
		sync_list_copy.clear();
#endif

		// now remove everything from this time
		sync_step_running = false;
		while(!sync_remove_list.empty()) {
			sync_list.remove( sync_remove_list.remove_first() );
		}

		sync_step_running = false;
	}

	if(display) {
		// only omitted in fast forward mode for the magic steps

		for(int x=0; x<MAX_PLAYER_COUNT-1; x++) {
			if(spieler[x]) {
				spieler[x]->age_messages(delta_t);
			}
		}

		// change view due to following a convoi?
		if(follow_convoi.is_bound()  &&  follow_convoi->get_vehikel_anzahl()>0) {
			vehikel_t const& v       = *follow_convoi->front();
			koord3d   const  new_pos = v.get_pos();
			if(new_pos!=koord3d::invalid) {
				const sint16 rw = get_tile_raster_width();
				int new_xoff = 0;
				int new_yoff = 0;
				v.get_screen_offset( new_xoff, new_yoff, get_tile_raster_width() );
				new_xoff -= tile_raster_scale_x(-v.get_xoff(), rw);
				new_yoff -= tile_raster_scale_y(-v.get_yoff(), rw) + tile_raster_scale_y(new_pos.z * TILE_HEIGHT_STEP, rw);
				change_world_position( new_pos.get_2d(), -new_xoff, -new_yoff );
			}
		}

		// display new frame with water animation
		intr_refresh_display( false );
		update_frame_sleep_time(delta_t);
	}
	clear_random_mode( SYNC_STEP_RANDOM );
}


// does all the magic about frame timing
void karte_t::update_frame_sleep_time(long /*delta*/)
{
	// get average frame time
	uint32 last_ms = dr_time();
	last_frame_ms[last_frame_idx] = last_ms;
	last_frame_idx = (last_frame_idx+1) % 32;
	if(last_frame_ms[last_frame_idx]<last_ms) {
		realFPS = (32000u) / (last_ms-last_frame_ms[last_frame_idx]);
	}
	else {
		realFPS = umgebung_t::fps;
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
		umgebung_t::simple_drawing = (umgebung_t::simple_drawing_normal >= get_tile_raster_width());

		// calaculate and activate fast redraw ..
		if(  realFPS > (umgebung_t::fps*17/16)  ) {
			// decrease fast tile zoom by one
			if(  umgebung_t::simple_drawing_normal > umgebung_t::simple_drawing_default  ) {
				umgebung_t::simple_drawing_normal --;
			}
		}
		else if(  realFPS < umgebung_t::fps/2  ) {
			// activate simple redraw
			umgebung_t::simple_drawing_normal = max( umgebung_t::simple_drawing_normal, get_tile_raster_width()+1 );
		}
		else if(  realFPS < (umgebung_t::fps*15)/16  )  {
			// increase fast tile redraw by one if below current tile size
			if(  umgebung_t::simple_drawing_normal <= (get_tile_raster_width()*3)/2  ) {
				umgebung_t::simple_drawing_normal ++;
			}
		}
		else if(  idle_time > 0  ) {
			// decrease fast tile zoom by one
			if(  umgebung_t::simple_drawing_normal > umgebung_t::simple_drawing_default  ) {
				umgebung_t::simple_drawing_normal --;
			}
		}
		umgebung_t::simple_drawing = (umgebung_t::simple_drawing_normal >= get_tile_raster_width());

		// way too slow => try to increase time ...
		if(  last_ms-last_interaction > 100  ) {
			if(  last_ms-last_interaction > 500  ) {
				set_frame_time( 1+get_frame_time() );
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
			if(realFPS>(umgebung_t::fps*17/16)) {
				increase_frame_time();
			}
			else if(realFPS<umgebung_t::fps) {
				if(  1000u/get_frame_time() < 2*realFPS  ) {
					if(  realFPS < (umgebung_t::fps/2)  ) {
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
		sint32 frame_intervall = max( 100, 1000/umgebung_t::fps );
		if(get_frame_time()>frame_intervall) {
			reduce_frame_time();
		}
		else {
			increase_frame_time();
		}
		// (de-)activate faster redraw
		umgebung_t::simple_drawing = umgebung_t::simple_drawing_fast_forward  ||  (umgebung_t::simple_drawing_normal >= get_tile_raster_width());
	}
}


// add an amout to a subcategory
void karte_t::buche(sint64 const betrag, player_cost const type)
{
	assert(type < MAX_WORLD_COST);
	finance_history_year[0][type] += betrag;
	finance_history_month[0][type] += betrag;
	// to do: check for dependecies
}


inline sint32 get_population(stadt_t const* const c)
{
	return c->get_einwohner();
}


void karte_t::new_month()
{
	bool need_locality_update = false;

	update_history();

	// advance history ...
	last_month_bev = finance_history_month[0][WORLD_CITICENS];
	for(  int hist=0;  hist<karte_t::MAX_WORLD_COST;  hist++  ) {
		for( int y=MAX_WORLD_HISTORY_MONTHS-1; y>0;  y--  ) {
			finance_history_month[y][hist] = finance_history_month[y-1][hist];
		}
	}

	current_month ++;
	last_month ++;
	if( last_month > 11 ) {
		last_month = 0;
		// check for changed distance weight
		uint32 old_locality_factor = koord::locality_factor;
		koord::locality_factor = settings.get_locality_factor( last_year+1 );
		need_locality_update = (old_locality_factor != koord::locality_factor);
	}
	DBG_MESSAGE("karte_t::neuer_monat()","Month (%d/%d) has started", (last_month%12)+1, last_month/12 );
	DBG_MESSAGE("karte_t::neuer_monat()","sync_step %u objects", sync_list.get_count() );

	// this should be done before a map update, since the map may want an update of the way usage
//	DBG_MESSAGE("karte_t::neuer_monat()","ways");
	FOR(slist_tpl<weg_t*>, const w, weg_t::get_alle_wege()) {
		w->neuer_monat();
	}

	// recalc old settings (and maybe update the staops with the current values)
	reliefkarte_t::get_karte()->neuer_monat();

	INT_CHECK("simworld 1701");

//	DBG_MESSAGE("karte_t::neuer_monat()","convois");
	// hsiegeln - call new month for convois
	FOR(vector_tpl<convoihandle_t>, const cnv, convoi_array) {
		cnv->new_month();
	}

	INT_CHECK("simworld 1701");

//	DBG_MESSAGE("karte_t::neuer_monat()","factories");
	FOR(slist_tpl<fabrik_t*>, const fab, fab_list) {
		fab->neuer_monat();
	}
	INT_CHECK("simworld 1278");


//	DBG_MESSAGE("karte_t::neuer_monat()","cities");
	stadt.update_weights(get_population);
	FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
		i->neuer_monat(need_locality_update);
	}

	INT_CHECK("simworld 1282");

	// spieler
	for(uint i=0; i<MAX_PLAYER_COUNT; i++) {
		if( i>=2  &&  last_month == 0  &&  !settings.is_freeplay() ) {
			// remove all player (but first and second) who went bankrupt during last year
			if(  spieler[i] != NULL  &&  spieler[i]->get_finance()->is_bancrupted()  )
			{
				remove_player(i);
			}
		}

		if(  spieler[i] != NULL  ) {
			// if returns false -> remove player
			if (!spieler[i]->neuer_monat()) {
				remove_player(i);
			}
		}
	}
	// update the window
	ki_kontroll_t* playerwin = (ki_kontroll_t*)win_get_magic(magic_ki_kontroll_t);
	if(  playerwin  ) {
		playerwin->update_data();
	}

	INT_CHECK("simworld 1289");

//	DBG_MESSAGE("karte_t::neuer_monat()","halts");
	FOR(slist_tpl<halthandle_t>, const s, haltestelle_t::get_alle_haltestellen()) {
		s->neuer_monat();
		INT_CHECK("simworld 1877");
	}

	INT_CHECK("simworld 2522");
	depot_t::neuer_monat();

	// now switch year to get the right year for all timeline stuff ...
	if( last_month == 0 ) {
		new_year();
		INT_CHECK("simworld 1299");
	}

	wegbauer_t::neuer_monat(this);
	INT_CHECK("simworld 1299");

	recalc_average_speed();
	INT_CHECK("simworld 1921");

	// update toolbars (i.e. new waytypes
	werkzeug_t::update_toolbars(this);


	if( !umgebung_t::networkmode  &&  umgebung_t::autosave>0  &&  last_month%umgebung_t::autosave==0 ) {
		char buf[128];
		sprintf( buf, "save/autosave%02i.sve", last_month+1 );
		save( buf, loadsave_t::autosave_mode, umgebung_t::savegame_version_str, true );
	}
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

DBG_MESSAGE("karte_t::new_year()","speedbonus for %d %i, %i, %i, %i, %i, %i, %i, %i", last_year,
			average_speed[0], average_speed[1], average_speed[2], average_speed[3], average_speed[4], average_speed[5], average_speed[6], average_speed[7] );

	cbuffer_t buf;
	buf.printf( translator::translate("Year %i has started."), last_year );
	msg->add_message(buf,koord::invalid,message_t::general,COL_BLACK,skinverwaltung_t::neujahrsymbol->get_bild_nr(0));

	FOR(vector_tpl<convoihandle_t>, const cnv, convoi_array) {
		cnv->neues_jahr();
	}

	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(  spieler[i] != NULL  ) {
			spieler[i]->neues_jahr();
		}
	}

}


// recalculated speed boni for different vehicles
// and takes care of all timeline stuff
void karte_t::recalc_average_speed()
{
	// retire/allocate vehicles
	stadtauto_t::built_timeline_liste(this);

	for(int i=road_wt; i<=narrowgauge_wt; i++) {
		const int typ = i==4 ? 3 : (i-1)&7;
		average_speed[typ] = vehikelbauer_t::get_speedbonus( this->get_timeline_year_month(), i==4 ? air_wt : (waytype_t)i );
	}

	//	DBG_MESSAGE("karte_t::recalc_average_speed()","");
	if(use_timeline()) {
		for(int i=road_wt; i<=air_wt; i++) {
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

			FOR(slist_tpl<vehikel_besch_t const*>, const info, vehikelbauer_t::get_info((waytype_t)i)) {
				const uint16 intro_month = info->get_intro_year_month();
				if(intro_month == current_month) {
					cbuffer_t buf;
					buf.printf( translator::translate("New %s now available:\n%s\n"), vehicle_type, translator::translate(info->get_name()) );
					msg->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,info->get_basis_bild());
				}

				const uint16 retire_month = info->get_retire_year_month();
				if(retire_month == current_month) {
					cbuffer_t buf;
					buf.printf( translator::translate("Production of %s has been stopped:\n%s\n"), vehicle_type, translator::translate(info->get_name()) );
					msg->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,info->get_basis_bild());
				}
			}
		}

		// city road (try to use always a timeline)
		if (weg_besch_t const* city_road_test = settings.get_city_road_type(current_month) ) {
			city_road = city_road_test;
		}
		else {
			DBG_MESSAGE("karte_t::neuer_monat()","Month %d has started", last_month);
			city_road = wegbauer_t::weg_search(road_wt,50,get_timeline_year_month(),weg_t::type_flat);
		}

	}
	else {
		// defaults
		city_road = settings.get_city_road_type(0);
		if(city_road==NULL) {
			city_road = wegbauer_t::weg_search(road_wt,50,0,weg_t::type_flat);
		}
	}
}


// returns the current speed record
sint32 karte_t::get_record_speed( waytype_t w ) const
{
	switch(w) {
		case road_wt: return max_road_speed.speed;
		case track_wt:
		case tram_wt: return max_rail_speed.speed;
		case monorail_wt: return max_monorail_speed.speed;
		case maglev_wt: return max_maglev_speed.speed;
		case narrowgauge_wt: return max_narrowgauge_speed.speed;
		case water_wt: return max_ship_speed.speed;
		case air_wt: return max_air_speed.speed;
		default: return 0;
	}
}


// sets the new speed record
void karte_t::notify_record( convoihandle_t cnv, sint32 max_speed, koord pos )
{
	speed_record_t *sr = NULL;
	switch (cnv->front()->get_waytype()) {
		case road_wt: sr = &max_road_speed; break;
		case track_wt:
		case tram_wt: sr = &max_rail_speed; break;
		case monorail_wt: sr = &max_monorail_speed; break;
		case maglev_wt: sr = &max_maglev_speed; break;
		case narrowgauge_wt: sr = &max_narrowgauge_speed; break;
		case water_wt: sr = &max_ship_speed; break;
		case air_wt: sr = &max_air_speed; break;
		default: assert(0);
	}

	// avoid the case of two convois with identical max speed ...
	if(cnv!=sr->cnv  &&  sr->speed+1==max_speed) {
		return;
	}

	// really new/faster?
	if(pos!=sr->pos  ||  sr->speed+1<max_speed) {
		// update it
		sr->cnv = cnv;
		sr->speed = max_speed-1;
		sr->year_month = current_month;
		sr->pos = pos;
		sr->besitzer = NULL; // will be set, when accepted
	}
	else {
		sr->cnv = cnv;
		sr->speed = max_speed-1;
		sr->pos = pos;

		// same convoi and same position
		if(sr->besitzer==NULL  &&  current_month!=sr->year_month) {
			// notfiy the world of this new record
			sr->speed = max_speed-1;
			sr->besitzer = cnv->get_besitzer();
			const char* msg;
			switch (cnv->front()->get_waytype()) {
				default: NOT_REACHED
				case road_wt:     msg = "New world record for motorcars: %.1f km/h by %s."; break;
				case track_wt:
				case tram_wt:     msg = "New world record for railways: %.1f km/h by %s.";  break;
				case monorail_wt: msg = "New world record for monorails: %.1f km/h by %s."; break;
				case maglev_wt: msg = "New world record for maglevs: %.1f km/h by %s."; break;
				case narrowgauge_wt: msg = "New world record for narrowgauges: %.1f km/h by %s."; break;
				case water_wt:    msg = "New world record for ship: %.1f km/h by %s.";      break;
				case air_wt:      msg = "New world record for planes: %.1f km/h by %s.";    break;
			}
			cbuffer_t buf;
			buf.printf( translator::translate(msg), (float)speed_to_kmh(10*sr->speed)/10.0, sr->cnv->get_name() );
			get_message()->add_message( buf, sr->pos, message_t::new_vehicle, PLAYER_FLAG|sr->besitzer->get_player_nr() );
		}
	}
}


void karte_t::set_schedule_counter()
{
	// do not call this from gui when playing in network mode!
	assert( (get_random_mode() & INTERACTIVE_RANDOM) == 0  );

	schedule_counter++;
}


void karte_t::step()
{
	DBG_DEBUG4("karte_t::step", "start step");
	unsigned long time = dr_time();

	// first: check for new month
	if(ticks > next_month_ticks) {

		if(  next_month_ticks > next_month_ticks+karte_t::ticks_per_world_month  ) {
			// avoid overflow here ...
			dbg->warning( "karte_t::step()", "Ticks were overflowing => resetted" );
			ticks %= karte_t::ticks_per_world_month;
			next_month_ticks %= karte_t::ticks_per_world_month;
			last_step_ticks %= karte_t::ticks_per_world_month;
		}
		next_month_ticks += karte_t::ticks_per_world_month;

		DBG_DEBUG4("karte_t::step", "calling neuer_monat");
		new_month();
	}

	DBG_DEBUG4("karte_t::step", "time calculations");
	const long delta_t = (long)ticks-(long)last_step_ticks;
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
		if(last_5_simloops<umgebung_t::max_acceleration) {
			if(idle_time>0) {
				idle_time --;
			}
		}
		else if(simloops>8u*umgebung_t::max_acceleration) {
			if((long)idle_time<get_frame_time()-10) {
				idle_time ++;
			}
		}
		// cap it ...
		if( (long)idle_time>=get_frame_time()-10) {
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

	// check for pending seasons change
	if(pending_season_change>0) {
		DBG_DEBUG4("karte_t::step", "pending_season_change");
		// process
		const uint32 end_count = min( cached_grid_size.x*cached_grid_size.y,  tile_counter + max( 16384, cached_grid_size.x*cached_grid_size.y/16 ) );
		while(  tile_counter < end_count  ) {
			plan[tile_counter].check_season(current_month);
			tile_counter ++;
			if((tile_counter&0x3FF)==0) {
				INT_CHECK("karte_t::step");
			}
		}

		if(  tile_counter >= (uint32)cached_grid_size.x*(uint32)cached_grid_size.y  ) {
			pending_season_change --;
			tile_counter = 0;
		}
	}

	// to make sure the tick counter will be updated
	INT_CHECK("karte_t::step");

	DBG_DEBUG4("karte_t::step", "step convois");
	// since convois will be deleted during stepping, we need to step backwards
	for (size_t i = convoi_array.get_count(); i-- != 0;) {
		convoihandle_t cnv = convoi_array[i];
		cnv->step();
		if((i&7)==0) {
			INT_CHECK("simworld 1947");
		}
	}

	// now step all towns (to generate passengers)
	DBG_DEBUG4("karte_t::step", "step cities");
	sint64 bev=0;
	FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
		i->step(delta_t);
		bev += i->get_finance_history_month(0, HIST_CITICENS);
	}

	// the inhabitants stuff
	finance_history_month[0][WORLD_CITICENS] = bev;

	DBG_DEBUG4("karte_t::step", "step factories");
	FOR(slist_tpl<fabrik_t*>, const f, fab_list) {
		f->step(delta_t);
	}
	finance_history_year[0][WORLD_FACTORIES] = finance_history_month[0][WORLD_FACTORIES] = fab_list.get_count();

	// step powerlines - required order: pumpe, senke, then powernet
	DBG_DEBUG4("karte_t::step", "step poweline stuff");
	pumpe_t::step_all( delta_t );
	senke_t::step_all( delta_t );
	powernet_t::step_all( delta_t );

	DBG_DEBUG4("karte_t::step", "step players");
	// then step all players
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  spieler[i] != NULL  ) {
			spieler[i]->step();
		}
	}

	DBG_DEBUG4("karte_t::step", "step halts");
	haltestelle_t::step_all();

	// ok, next step
	INT_CHECK("simworld 1975");

	if((steps%8)==0) {
		DBG_DEBUG4("karte_t::step", "checkmidi");
		check_midi();
	}

	// will also call all objects if needed ...
	if(  recalc_snowline()  ) {
		pending_season_change ++;
	}

	// number of playing clients changed
	if(  umgebung_t::server  &&  last_clients!=socket_list_t::get_playing_clients()  ) {
		if(  umgebung_t::server_announce  ) {
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
		buf.printf(translator::translate("Now %u clients connected.", settings.get_name_language_id()), last_clients);
		werkzeug_t *w = create_tool( WKZ_ADD_MESSAGE_TOOL | SIMPLE_TOOL );
		w->set_default_param( buf );
		set_werkzeug( w, NULL );
		// since init always returns false, it is safe to delete immediately
		delete w;
	}

	if(  get_scenario()->is_scripted() ) {
		get_scenario()->step();
	}
	DBG_DEBUG4("karte_t::step", "end");
}


// recalculates world statistics for older versions
void karte_t::restore_history()
{
	last_month_bev = -1;
	for(  int m=12-1;  m>0;  m--  ) {
		// now step all towns (to generate passengers)
		sint64 bev=0;
		sint64 total_pas = 1, trans_pas = 0;
		sint64 total_mail = 1, trans_mail = 0;
		sint64 total_goods = 1, supplied_goods = 0;
		FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
			bev            += i->get_finance_history_month(m, HIST_CITICENS);
			trans_pas      += i->get_finance_history_month(m, HIST_PAS_TRANSPORTED);
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
			if(  spieler[i]!=NULL  ) {
				transported += spieler[i]->get_finance()->get_history_veh_month( TT_ALL, m, ATV_TRANSPORTED );
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
			if(  spieler[i]  ) {
				transported_year += spieler[i]->get_finance()->get_history_veh_year( TT_ALL, y, ATV_TRANSPORTED );
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
	sint64 bev=0;
	sint64 total_pas = 1, trans_pas = 0;
	sint64 total_mail = 1, trans_mail = 0;
	sint64 total_goods = 1, supplied_goods = 0;
	sint64 total_pas_year = 1, trans_pas_year = 0;
	sint64 total_mail_year = 1, trans_mail_year = 0;
	sint64 total_goods_year = 1, supplied_goods_year = 0;
	FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
		bev                 += i->get_finance_history_month(0, HIST_CITICENS);
		trans_pas           += i->get_finance_history_month(0, HIST_PAS_TRANSPORTED);
		total_pas           += i->get_finance_history_month(0, HIST_PAS_GENERATED);
		trans_mail          += i->get_finance_history_month(0, HIST_MAIL_TRANSPORTED);
		total_mail          += i->get_finance_history_month(0, HIST_MAIL_GENERATED);
		supplied_goods      += i->get_finance_history_month(0, HIST_GOODS_RECIEVED);
		total_goods         += i->get_finance_history_month(0, HIST_GOODS_NEEDED);
		trans_pas_year      += i->get_finance_history_year( 0, HIST_PAS_TRANSPORTED);
		total_pas_year      += i->get_finance_history_year( 0, HIST_PAS_GENERATED);
		trans_mail_year     += i->get_finance_history_year( 0, HIST_MAIL_TRANSPORTED);
		total_mail_year     += i->get_finance_history_year( 0, HIST_MAIL_GENERATED);
		supplied_goods_year += i->get_finance_history_year( 0, HIST_GOODS_RECIEVED);
		total_goods_year    += i->get_finance_history_year( 0, HIST_GOODS_NEEDED);
	}

	finance_history_month[0][WORLD_GROWTH] = bev-last_month_bev;
	finance_history_year[0][WORLD_GROWTH] = bev - (finance_history_year[1][WORLD_CITICENS]==0 ? finance_history_month[0][WORLD_CITICENS] : finance_history_year[1][WORLD_CITICENS]);

	// the inhabitants stuff
	finance_history_year[0][WORLD_TOWNS] = finance_history_month[0][WORLD_TOWNS] = stadt.get_count();
	finance_history_year[0][WORLD_CITICENS] = finance_history_month[0][WORLD_CITICENS] = bev;
	finance_history_month[0][WORLD_GROWTH] = bev-last_month_bev;
	finance_history_year[0][WORLD_GROWTH] = bev - (finance_history_year[1][WORLD_CITICENS]==0 ? finance_history_month[0][WORLD_CITICENS] : finance_history_year[1][WORLD_CITICENS]);

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
		if(  spieler[i]!=NULL  ) {
			transported += spieler[i]->get_finance()->get_history_veh_month( TT_ALL, 0, ATV_TRANSPORTED_GOOD );
			transported_year += spieler[i]->get_finance()->get_history_veh_year( TT_ALL, 0, ATV_TRANSPORTED_GOOD );
		}
	}
	finance_history_month[0][WORLD_TRANSPORTED_GOODS] = transported;
	finance_history_year[0][WORLD_TRANSPORTED_GOODS] = transported_year;
}


void karte_t::set_scroll_lock(bool yesno)
{
	scroll_lock = yesno;
	if (yesno) {
		follow_convoi = convoihandle_t();
	}
}


koord karte_t::calculate_world_position( koord3d viewpos ) const
{
	// just calculate the offset from the z-position
	const sint16 raster = get_tile_raster_width();
	const sint16 new_yoff = tile_raster_scale_y(viewpos.z*TILE_HEIGHT_STEP,raster);
	sint16 lines = 0;
	if(new_yoff>0) {
		lines = (new_yoff + (raster/4))/(raster/2);
	}
	else {
		lines = (new_yoff - (raster/4))/(raster/2);
	}
	return viewpos.get_2d() - koord( lines, lines );
}


// change the center viewport position for a certain ground tile
// any possible convoi to follow will be disabled
void karte_t::change_world_position( koord3d new_ij )
{
	follow_convoi = convoihandle_t();
	change_world_position( calculate_world_position( new_ij ) );
}


// change the center viewport position
void karte_t::change_world_position( koord new_ij, sint16 new_xoff, sint16 new_yoff )
{
	const sint16 raster = get_tile_raster_width();
	// truncate new_xoff, modify new_ij.x
	new_ij.x -= new_xoff/raster;
	new_ij.y += new_xoff/raster;
	new_xoff %= raster;

	// truncate new_yoff, modify new_ij.y
	int lines = 0;
	if(new_yoff>0) {
		lines = (new_yoff + (raster/4))/(raster/2);
	}
	else {
		lines = (new_yoff - (raster/4))/(raster/2);
	}
	new_ij -= koord( lines, lines );
	new_yoff -= (raster/2)*lines;

	//position changed? => update and mark dirty
	if(new_ij!=ij_off  ||  new_xoff!=x_off  ||  new_yoff!=y_off) {
		ij_off = new_ij;
		x_off = new_xoff;
		y_off = new_yoff;
		set_dirty();
	}
}


void karte_t::move_view(event_t *ev)
{
	if(!scroll_lock) {
		koord new_ij = ij_off;

		sint16 new_xoff = x_off - (ev->mx - ev->cx) * umgebung_t::scroll_multi;
		sint16 new_yoff = y_off - (ev->my-ev->cy) * umgebung_t::scroll_multi;

		// this sets the new position and mark screen dirty
		// => with next refresh we will be at a new location
		change_world_position( new_ij, new_xoff, new_yoff );

		// move the mouse pointer back to starting location => infinite mouse movement
		if ((ev->mx - ev->cx)!=0  ||  (ev->my-ev->cy)!=0) {
#ifdef __BEOS__
			change_drag_start(ev->mx - ev->cx, ev->my - ev->cy);
#else
			display_move_pointer(ev->cx, ev->cy);
#endif
		}
	}
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


uint8 karte_t::recalc_natural_slope( const koord pos, sint8 &new_height ) const
{
	grund_t *gr = lookup_kartenboden(pos);
	if(!gr) {
		return hang_t::flach;
	}
	else {
		// now we check each four corners
		grund_t *gr1, *gr2, *gr3;

		// back right
		gr1 = lookup_kartenboden(pos+koord(0,-1));
		gr2 = lookup_kartenboden(pos+koord(1,-1));
		gr3 = lookup_kartenboden(pos+koord(1,0));
		// now get the correct height
		sint16 height1 = median (
			gr1 ? (sint16)gr1->get_hoehe()+corner2(gr1->get_grund_hang()) : grundwasser,
			gr2 ? (sint16)gr2->get_hoehe()+corner1(gr2->get_grund_hang()) : grundwasser,
			gr3 ? (sint16)gr3->get_hoehe()+corner4(gr3->get_grund_hang()) : grundwasser
		);
		// now we have height for corner one

		// front right
		gr1 = lookup_kartenboden(pos+koord(1,0));
		gr2 = lookup_kartenboden(pos+koord(1,1));
		gr3 = lookup_kartenboden(pos+koord(0,1));
		// now get the correct height
		sint16 height2 = median (
			gr1 ? (sint16)gr1->get_hoehe()+corner1(gr1->get_grund_hang()) : grundwasser,
			gr2 ? (sint16)gr2->get_hoehe()+corner4(gr2->get_grund_hang()) : grundwasser,
			gr3 ? (sint16)gr3->get_hoehe()+corner3(gr3->get_grund_hang()) : grundwasser
		);
		// now we have height for corner two

		// front left
		gr1 = lookup_kartenboden(pos+koord(0,1));
		gr2 = lookup_kartenboden(pos+koord(-1,1));
		gr3 = lookup_kartenboden(pos+koord(-1,0));
		// now get the correct height
		sint16 height3 = median (
			gr1 ? (sint16)gr1->get_hoehe()+corner4(gr1->get_grund_hang()) : grundwasser,
			gr2 ? (sint16)gr2->get_hoehe()+corner3(gr2->get_grund_hang()) : grundwasser,
			gr3 ? (sint16)gr3->get_hoehe()+corner2(gr3->get_grund_hang()) : grundwasser
		);
		// now we have height for corner three

		// back left
		gr1 = lookup_kartenboden(pos+koord(-1,0));
		gr2 = lookup_kartenboden(pos+koord(-1,-1));
		gr3 = lookup_kartenboden(pos+koord(0,-1));
		// now get the correct height
		sint16 height4 = median (
			gr1 ? (sint16)gr1->get_hoehe()+corner3(gr1->get_grund_hang()) : grundwasser,
			gr2 ? (sint16)gr2->get_hoehe()+corner2(gr2->get_grund_hang()) : grundwasser,
			gr3 ? (sint16)gr3->get_hoehe()+corner1(gr3->get_grund_hang()) : grundwasser
		);
		// now we have height for corner four

		// new height of that tile ...
		sint8 min_height = min(min(height1,height2), min(height3,height4));
		sint8 max_height = max(max(height1,height2), max(height3,height4));
#ifndef DOUBLE_GROUNDS
		/* check for an artificial slope on a steep sidewall */
		bool not_ok = abs(max_height-min_height)>2  ||  min_height == -128;

		sint8 old_height = gr->get_hoehe();
		new_height = min_height;

		// now we must make clear, that there is no ground above/below the slope
		if(  old_height!=new_height  ) {
			not_ok |= lookup(koord3d(pos,new_height))!=NULL;
			if(  old_height > new_height  ) {
				not_ok |= lookup(koord3d(pos,old_height-1))!=NULL;
			}
			if(  old_height < new_height  ) {
				not_ok |= lookup(koord3d(pos,old_height+1))!=NULL;
			}
			not_ok |= lookup(koord3d(pos,new_height))!=NULL;
		}

		if(  not_ok  ) {
			/* difference too high or ground above/below
			 * we just keep it as it was ...
			 */
			new_height = old_height;
			return gr->get_grund_hang();
		}
#if 0
		// Since we have now the correct slopes, we update the grid:
		sint8 *p = &grid_hgts[pos.x + pos.y*(sint32)(get_size().x+1)];
		*p = height4;
		*(p+1) = height3;
		*(p+get_size().x+2) = height2;
		*(p+get_size().x+1) = height1;
#endif

		// since slopes could be two unit height, we just return best effort ...
		return (height4-new_height>0 ? 8 : 0) + (height1-new_height>0 ? 4 : 0) + (height2-new_height ? 2 : 0) + (height3-new_height>0 ? 1 : 0);
#else
		if(  max_height-min_height>2  ) {
			/* this is an artificial slope on a steep sidewall
			 * we just keep it as it was ...
			 */
			new_height = gr->get_hoehe();
			return gr->get_grund_hang();
		}
		new_height = min_height;

		const int d1=height1-mini;
		const int d2=height2-mini;
		const int d3=height3-mini;
		const int d4=height4-mini;

		return (d1*27) + (d2*9) + (d3*3) + d4;
#endif
	}
	return 0;
}


uint8 karte_t::calc_natural_slope( const koord pos ) const
{
	if(is_within_grid_limits(pos.x, pos.y)) {

		const sint8 * p = &grid_hgts[pos.x + pos.y*(sint32)(get_size().x+1)];

		const int h1 = *p;
		const int h2 = *(p+1);
		const int h3 = *(p+get_size().x+2);
		const int h4 = *(p+get_size().x+1);

		const int mini = min(min(h1,h2), min(h3,h4));

		const int d1=h1-mini;
		const int d2=h2-mini;
		const int d3=h3-mini;
		const int d4=h4-mini;

#ifndef DOUBLE_GROUNDS
		return (d1<<3) | (d2<<2) | (d3<<1) | d4;
#else
		return (d1*27) + (d2*9) + (d3*3) + d4;
#endif
	}
	return 0;
}


bool karte_t::ist_wasser(koord pos, koord dim) const
{
	koord k;

	for(k.x = pos.x; k.x < pos.x + dim.x; k.x++) {
		for(k.y = pos.y; k.y < pos.y + dim.y; k.y++) {
			if(!is_within_grid_limits(k + koord(1,1))  ||  max_hgt(k) > get_grundwasser()) {
				return false;
			}
		}
	}
	return true;
}


bool karte_t::square_is_free(koord pos, sint16 w, sint16 h, int *last_y, climate_bits cl) const
{
	if(pos.x < 0  ||  pos.y < 0  ||  pos.x+w > get_size().x || pos.y+h > get_size().y) {
		return false;
	}

	grund_t *gr = lookup_kartenboden(pos);
	const sint16 platz_h = gr->get_grund_hang() ? max_hgt(pos) : gr->get_hoehe();	// remember the max height of the first tile

	koord k;
	for(k.y=pos.y+h-1; k.y>=pos.y; k.y--) {
		for(k.x=pos.x; k.x<pos.x+w; k.x++) {
			const grund_t *gr = lookup_kartenboden(k);

			// we can built, if: max height all the same, everything removable and no buildings there
			// since this is called very often, we us a trick:
			// if get_grund_hang()!=0 (non flat) then we add 127 (bigger than any slope) and substract it
			// will break with double slopes!
#ifdef DOUBLE_GROUNDS
#error "Fix this function!"
#endif
			if( !gr  ||  platz_h!=(gr->get_hoehe()+((gr->get_grund_hang()+127)/128))  ||  !gr->ist_natur() ||  gr->kann_alle_obj_entfernen(NULL) != NULL  ||  (cl&(1<<get_climate(gr->get_hoehe())))==0) {
				if(last_y) {
					*last_y = k.y;
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
 * Play a sound, but only if near enoungh.
 * Sounds are muted by distance and clipped completely if too far away.
 *
 * @author Hj. Malthaner
 */
bool karte_t::play_sound_area_clipped(koord const pos, uint16 const idx) const
{
	if(is_sound  &&  zeiger) {
		const int dist = koord_distance( pos, zeiger->get_pos() );

		if(dist < 100) {
			int xw = (2*display_get_width())/get_tile_raster_width();
			int yw = (4*display_get_height())/get_tile_raster_width();

			uint8 const volume = (uint8)(255U * (xw + yw) / (xw + yw + 64 * dist));
			if (volume > 8) {
				sound_play(idx, volume);
			}
		}
		return dist < 25;
	}
	return false;
}


void karte_t::save(const char *filename, loadsave_t::mode_t savemode, const char *version_str, bool silent )
{
DBG_MESSAGE("karte_t::speichern()", "saving game to '%s'", filename);
	loadsave_t  file;
	bool save_temp = strstart( filename, "save/" );
	const char *savename = save_temp ? "save/_temp.sve" : filename;

	display_show_load_pointer( true );
	if(!file.wr_open( savename, savemode, umgebung_t::objfilename.c_str(), version_str )) {
		create_win(new news_img("Kann Spielstand\nnicht speichern.\n"), w_info, magic_none);
		dbg->error("karte_t::speichern()","cannot open file for writing! check permissions!");
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
				remove( filename );
				rename( savename, filename );
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


void karte_t::save(loadsave_t *file,bool silent)
{
	bool needs_redraw = false;

	loadingscreen_t *ls = NULL;
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "start");
	if(!silent) {
		ls = new loadingscreen_t( translator::translate("Saving map ..."), get_size().y );
	}

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
			dbg->error( "karte_t::speichern()","Map cannot be saved in any rotation!" );
			create_win( new news_img("Map may be not saveable in any rotation!"), w_info, magic_none);
			// still broken, but we try anyway to save it ...
		}
	}
	// only broken buildings => just warn
	if(nosave_warning) {
		dbg->error( "karte_t::speichern()","Some buildings may be broken by saving!" );
	}

	/* If the current tool is a two_click_werkzeug, call cleanup() in order to delete dummy grounds (tunnel + monorail preview)
	 * THIS MUST NOT BE DONE IN NETWORK MODE!
	 */
	for(  uint8 sp_nr=0;  sp_nr<MAX_PLAYER_COUNT;  sp_nr++  ) {
		if(  two_click_werkzeug_t* tool = dynamic_cast<two_click_werkzeug_t*>(werkzeug[sp_nr]) ) {
			tool->cleanup( false );
		}
	}

	file->set_buffered(true);

	// do not set value for empyt player
	uint8 old_sp[MAX_PLAYER_COUNT];
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		old_sp[i] = settings.get_player_type(i);
		if(  spieler[i]==NULL  ) {
			settings.set_player_type(i, spieler_t::EMPTY);
		}
	}
	settings.rdwr(file);
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		settings.set_player_type(i, old_sp[i]);
	}

	file->rdwr_long(ticks);
	file->rdwr_long(last_month);
	file->rdwr_long(last_year);

	// rdwr cityrules for networkgames
	if(file->get_version()>102002) {
		bool do_rdwr = umgebung_t::networkmode;
		file->rdwr_bool(do_rdwr);
		if (do_rdwr) {
			stadt_t::cityrules_rdwr(file);
			if(file->get_version()>102003) {
				vehikelbauer_t::rdwr_speedbonus(file);
			}
		}
	}

	FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
		i->rdwr(file);
		if(silent) {
			INT_CHECK("saving");
		}
	}
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved cities ok");

	for(int j=0; j<get_size().y; j++) {
		for(int i=0; i<get_size().x; i++) {
			plan[i+j*cached_grid_size.x].rdwr(this, file, koord(i,j) );
		}
		if(silent) {
			INT_CHECK("saving");
		}
		else {
			ls->set_progress(j);
		}
	}
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved tiles");

	if(  file->get_version()<=102001  ) {
		// not needed any more
		for(int j=0; j<(get_size().y+1)*(sint32)(get_size().x+1); j++) {
			file->rdwr_byte(grid_hgts[j]);
		}
	DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved hgt");
	}

	sint32 fabs = fab_list.get_count();
	file->rdwr_long(fabs);
	FOR(slist_tpl<fabrik_t*>, const f, fab_list) {
		f->rdwr(file);
		if(silent) {
			INT_CHECK("saving");
		}
	}
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved fabs");

	sint32 haltcount=haltestelle_t::get_alle_haltestellen().get_count();
	file->rdwr_long(haltcount);
	FOR(slist_tpl<halthandle_t>, const s, haltestelle_t::get_alle_haltestellen()) {
		s->rdwr(file);
	}
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved stops");

	// svae number of convois
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
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved %i convois",convoi_array.get_count());

	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
// **** REMOVE IF SOON! *********
		if(file->get_version()<101000) {
			if(  i<8  ) {
				if(  spieler[i]  ) {
					spieler[i]->rdwr(file);
				}
				else {
					// simulate old ones ...
					spieler_t *sp = new spieler_t( this, i );
					sp->rdwr(file);
					delete sp;
				}
			}
		}
		else {
			if(  spieler[i]  ) {
				spieler[i]->rdwr(file);
			}
		}
	}
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved players");

	// saving messages
	if(  file->get_version()>=102005  ) {
		msg->rdwr(file);
	}
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved messages");

	// centered on what?
	sint32 dummy = ij_off.x;
	file->rdwr_long(dummy);
	dummy = ij_off.y;
	file->rdwr_long(dummy);

	if(file->get_version()>=99018) {
		// most recent version is 99018
		for (int year = 0;  year</*MAX_WORLD_HISTORY_YEARS*/12;  year++) {
			for (int cost_type = 0; cost_type</*MAX_WORLD_COST*/12; cost_type++) {
				file->rdwr_longlong(finance_history_year[year][cost_type]);
			}
		}
		for (int month = 0;month</*MAX_WORLD_HISTORY_MONTHS*/12;month++) {
			for (int cost_type = 0; cost_type</*MAX_WORLD_COST*/12; cost_type++) {
				file->rdwr_longlong(finance_history_month[month][cost_type]);
			}
		}
	}

	// finally a possible scenario
	scenario->rdwr( file );

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

	// clear hash table with missing paks (may cause some small memory loss though)
	missing_pak_names.clear();

	DBG_MESSAGE("karte_t::laden", "loading game from '%s'", filename);

	// reloading same game? Remeber pos
	const koord oldpos = settings.get_filename()[0]>0  &&  strncmp(filename,settings.get_filename(),strlen(settings.get_filename()))==0 ? ij_off : koord::invalid;

	if(  strstart(filename, "net:")  ) {
		// probably finish network mode?
		if(  umgebung_t::networkmode  ) {
			network_core_shutdown();
		}
		chdir( umgebung_t::user_dir );
		const char *err = network_connect(filename+4, this);
		if(err) {
			create_win( new news_img(err), w_info, magic_none );
			display_show_load_pointer(false);
			step_mode = NORMAL;
			return false;
		}
		else {
			umgebung_t::networkmode = true;
			name.printf( "client%i-network.sve", network_get_client_id() );
			restore_player_nr = strcmp( last_network_game.c_str(), filename )==0;
			if(  !restore_player_nr  ) {
				last_network_game = filename;
			}
		}
	}
	else {
		// probably finish network mode first?
		if(  umgebung_t::networkmode  ) {
			if (  umgebung_t::server  ) {
				char fn[256];
				sprintf( fn, "server%d-network.sve", umgebung_t::server );
				if(  strcmp(filename, fn) != 0  ) {
					// stay in networkmode, but disconnect clients
					dbg->warning("karte_t::laden","disconnecting all clients");
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
					dbg->warning("karte_t::laden","finished network mode");
					network_disconnect();
					finish_loop = false; // do not trigger intro screen
					// closing the socket will tell the server, I am away too
				}
			}
		}
		name.append(filename);
	}

	if(!file.rd_open(name)) {

		if(  (sint32)file.get_version()==-1  ||  file.get_version()>loadsave_t::int_version(SAVEGAME_VER_NR, NULL, NULL )  ) {
			dbg->warning("karte_t::laden()", translator::translate("WRONGSAVE") );
			create_win( new news_img("WRONGSAVE"), w_info, magic_none );
		}
		else {
			dbg->warning("karte_t::laden()", translator::translate("Kann Spielstand\nnicht laden.\n") );
			create_win(new news_img("Kann Spielstand\nnicht laden.\n"), w_info, magic_none);
		}
	} else if(file.get_version() < 84006) {
		// too old
		dbg->warning("karte_t::laden()", translator::translate("WRONGSAVE") );
		create_win(new news_img("WRONGSAVE"), w_info, magic_none);
	}
	else {
DBG_MESSAGE("karte_t::laden()","Savegame version is %d", file.get_version());

		load(&file);

		if(  umgebung_t::networkmode  ) {
			clear_command_queue();
		}

		if(  umgebung_t::server  ) {
			step_mode = FIX_RATIO;
			if(  umgebung_t::server  ) {
				// meaningless to use a locked map; there are passwords now
				settings.set_allow_player_change(true);
				// language of map becomes server language
				settings.set_name_language_iso(translator::get_lang()->iso_base);
			}

			if(  server_reload_pwd_hashes  ) {
				char fn[256];
				sprintf( fn, "server%d-pwdhash.sve", umgebung_t::server );
				loadsave_t pwdfile;
				if (pwdfile.rd_open(fn)) {
					rdwr_player_password_hashes( &pwdfile );
					// correct locking info
					nwc_auth_player_t::init_player_lock_server(this);
					pwdfile.close();
				}
				server_reload_pwd_hashes = false;
			}
		}
		else if(  umgebung_t::networkmode  ) {
			step_mode = PAUSE_FLAG|FIX_RATIO;
			switch_active_player( last_active_player_nr, true );
			if(  is_within_limits(oldpos)  ) {
				// go to position when last disconnected
				change_world_position( oldpos );
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
		else if(  !umgebung_t::networkmode  ||  !umgebung_t::restore_UI  ) {
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

		werkzeug_t::update_toolbars(this);
		set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE], get_active_player() );
	}
	settings.set_filename(filename);
	display_show_load_pointer(false);
	return ok;
}


// ugly hack, only needed for laden_abschliessen tree restorage
uint32 file_version = 0;

void karte_t::plans_laden_abschliessen( sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max )
{
	for(  int y = y_min;  y < y_max;  y++  ) {
		for(  int x = x_min; x < x_max;  x++  ) {
			const planquadrat_t *plan = lookup(koord(x,y));
			const int boden_count = plan->get_boden_count();
			for(  int schicht = 0;  schicht < boden_count;  schicht++  ) {
				grund_t *gr = plan->get_boden_bei(schicht);
				for(  int n = 0;  n < gr->get_top();  n++  ) {
					ding_t *d = gr->obj_bei(n);
					if(d) {
						d->laden_abschliessen();
					}
				}
				if(  file_version<=111000  &&  gr->ist_natur()  ) {
					gr->sort_trees();
				}
				gr->calc_bild();
			}
		}
	}
}


void karte_t::load(loadsave_t *file)
{
	char buf[80];

	intr_disable();
	dbg->message("karte_t::laden()", "Prepare for loading" );
	for(  uint i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		werkzeug[i] = werkzeug_t::general_tool[WKZ_ABFRAGE];
	}
	destroy_all_win(true);

	clear_random_mode(~LOAD_RANDOM);
	set_random_mode(LOAD_RANDOM);
	destroy();

	loadingscreen_t ls(translator::translate("Loading map ..."), 1, true, true );

	tile_counter = 0;
	simloops = 60;

	// zum laden vorbereiten -> tabelle loeschen
	powernet_t::neue_karte();
	pumpe_t::neue_karte();
	senke_t::neue_karte();

	file->set_buffered(true);

	// jetzt geht das laden los
	dbg->warning("karte_t::laden", "Fileversion: %d", file->get_version());
	settings = umgebung_t::default_einstellungen;
	settings.rdwr(file);
	loaded_rotation = settings.get_rotation();

	if(  umgebung_t::networkmode  ) {
		// to have games synchronized, transfer random counter too
		setsimrand(settings.get_random_counter(), 0xFFFFFFFFu );
		translator::init_custom_names(settings.get_name_language_id());
	}

	if(  !umgebung_t::networkmode  ||  (umgebung_t::server  &&  socket_list_t::get_playing_clients()==0)  ) {
		if (settings.get_allow_player_change() && umgebung_t::default_einstellungen.get_use_timeline() < 2) {
			// not locked => eventually switch off timeline settings, if explicitly stated
			settings.set_use_timeline(umgebung_t::default_einstellungen.get_use_timeline());
			DBG_DEBUG("karte_t::laden", "timeline: reset to %i", umgebung_t::default_einstellungen.get_use_timeline() );
		}
	}
	if (settings.get_beginner_mode()) {
		warenbauer_t::set_multiplier(settings.get_beginner_price_factor());
	}
	else {
		warenbauer_t::set_multiplier( 1000 );
	}

	grundwasser = (sint8)(settings.get_grundwasser());
DBG_DEBUG("karte_t::laden()","grundwasser %i",grundwasser);
	grund_besch_t::calc_water_level( this, height_to_climate );

	// just an initialisation for the loading
	season = (2+last_month/3)&3; // summer always zero
	snowline = settings.get_winter_snowline() + grundwasser;

	DBG_DEBUG("karte_t::laden", "settings loaded (groesse %i,%i) timeline=%i beginner=%i", settings.get_groesse_x(), settings.get_groesse_y(), settings.get_use_timeline(), settings.get_beginner_mode());

	// wird gecached, um den Pointerzugriff zu sparen, da
	// die groesse _sehr_ oft referenziert wird
	cached_grid_size.x = settings.get_groesse_x();
	cached_grid_size.y = settings.get_groesse_y();
	cached_size_max = max(cached_grid_size.x,cached_grid_size.y);
	cached_size.x = cached_grid_size.x-1;
	cached_size.y = cached_grid_size.y-1;
	x_off = y_off = 0;

	// Reliefkarte an neue welt anpassen
	reliefkarte_t::get_karte()->set_welt(this);

	ls.set_max( get_size().y*2+256 );
	init_felder();


	// reinit pointer with new pointer object and old values
	zeiger = new zeiger_t(this, koord3d::invalid, NULL );

	hausbauer_t::neue_karte();
	fabrikbauer_t::neue_karte(this);

DBG_DEBUG("karte_t::laden", "init felder ok");

	file->rdwr_long(ticks);
	file->rdwr_long(last_month);
	file->rdwr_long(last_year);
	if(file->get_version()<86006) {
		last_year += umgebung_t::default_einstellungen.get_starting_year();
	}
	// old game might have wrong month
	last_month %= 12;
	// set the current month count
	set_ticks_per_world_month_shift(settings.get_bits_per_month());
	current_month = last_month + (last_year*12);
	season = (2+last_month/3)&3; // summer always zero
	next_month_ticks = 	( (ticks >> karte_t::ticks_per_world_month_shift) + 1 ) << karte_t::ticks_per_world_month_shift;
	last_step_ticks = ticks;
	steps = 0;
	network_frame_count = 0;
	sync_steps = 0;
	step_mode = PAUSE_FLAG;

DBG_MESSAGE("karte_t::laden()","savegame loading at tick count %i",ticks);
	recalc_average_speed();	// resets timeline
	koord::locality_factor = settings.get_locality_factor( last_year );	// resets weigth factor
	// recalc_average_speed may have opened message windows
	destroy_all_win(true);

DBG_MESSAGE("karte_t::laden()", "init player");
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(  file->get_version()>=101000  ) {
			// since we have different kind of AIs
			delete spieler[i];
			spieler[i] = NULL;
			new_spieler(i, settings.spieler_type[i]);
		}
		else if(i<8) {
			// get the old player ...
			if(  spieler[i]==NULL  ) {
				new_spieler( i, (i==3) ? spieler_t::AI_PASSENGER : spieler_t::AI_GOODS );
			}
			settings.spieler_type[i] = spieler[i]->get_ai_id();
		}
	}
	// so far, player 1 will be active (may change in future)
	active_player = spieler[0];
	active_player_nr = 0;

	// rdwr cityrules, speedbonus for networkgames
	if(file->get_version()>102002) {
		bool do_rdwr = umgebung_t::networkmode;
		file->rdwr_bool(do_rdwr);
		if (do_rdwr) {
			stadt_t::cityrules_rdwr(file);
			if(file->get_version()>102003) {
				vehikelbauer_t::rdwr_speedbonus(file);
			}
		}
	}
	DBG_DEBUG("karte_t::laden", "init %i cities", settings.get_anzahl_staedte());
	stadt.clear();
	stadt.resize(settings.get_anzahl_staedte());
	for (int i = 0; i < settings.get_anzahl_staedte(); ++i) {
		stadt_t *s = new stadt_t(this, file);
		stadt.append( s, s->get_einwohner());
	}

	DBG_MESSAGE("karte_t::laden()","loading blocks");
	old_blockmanager_t::rdwr(this, file);

	DBG_MESSAGE("karte_t::laden()","loading tiles");
	for (int y = 0; y < get_size().y; y++) {
		for (int x = 0; x < get_size().x; x++) {
			plan[x+y*cached_grid_size.x].rdwr(this, file, koord(x,y) );
		}
		if(file->is_eof()) {
			dbg->fatal("karte_t::laden()","Savegame file mangled (too short)!");
		}
		ls.set_progress( y/2 );
	}

	if(file->get_version()<99005) {
		DBG_MESSAGE("karte_t::laden()","loading grid for older versions");
		for (int y = 0; y <= get_size().y; y++) {
			for (int x = 0; x <= get_size().x; x++) {
				sint32 hgt;
				file->rdwr_long(hgt);
				// old height step was 16!
				set_grid_hgt(koord(x, y), hgt/16 );
			}
		}
	}
	else if(  file->get_version()<=102001  )  {
		// hgt now bytes
		DBG_MESSAGE("karte_t::laden()","loading grid for older versions");
		for( sint32 i=0;  i<(get_size().y+1)*(sint32)(get_size().x+1);  i++  ) {
			file->rdwr_byte(grid_hgts[i]);
		}
	}
	else {
		// just restore the border as ground level
		// the heightfield is restored in grund_t::rdwr
		// but the south and east border of the map have to be resetted to water level
		DBG_MESSAGE("karte_t::laden()","calculating grid corners");
		for( sint16 i=0;  i<=cached_grid_size.x;  i++  ) {
			grid_hgts[i + cached_grid_size.y*(cached_grid_size.x+1)] = grundwasser;
		}
		for( sint16 j=0;  j<=cached_grid_size.y;  j++  ) {
			grid_hgts[cached_grid_size.x + j*(cached_grid_size.x+1)] = grundwasser;
		}
	}

	if(file->get_version()<88009) {
		DBG_MESSAGE("karte_t::laden()","loading slopes from older version");
		// Hajo: load slopes for older versions
		// now part of the grund_t structure
		for (int y = 0; y < get_size().y; y++) {
			for (int x = 0; x < get_size().x; x++) {
				sint8 slope;
				file->rdwr_byte(slope);
				access(x, y)->get_kartenboden()->set_grund_hang(slope);
			}
		}
	}

	if(file->get_version()<=88000) {
		// because from 88.01.4 on the foundations are handled differently
		for (int y = 0; y < get_size().y; y++) {
			for (int x = 0; x < get_size().x; x++) {
				koord k(x,y);
				grund_t *gr = access(x, y)->get_kartenboden();
				if(  gr->get_typ()==grund_t::fundament  ) {
					gr->set_hoehe( max_hgt(k) );
					gr->set_grund_hang( hang_t::flach );
					// transfer object to on new grund
					for(  int i=0;  i<gr->get_top();  i++  ) {
						gr->obj_bei(i)->set_pos( gr->get_pos() );
					}
				}
			}
		}
	}

	// Reliefkarte an neue welt anpassen
	DBG_MESSAGE("karte_t::laden()", "init relief");
	win_set_welt( this );
	reliefkarte_t::get_karte()->set_welt(this);

	sint32 fabs;
	file->rdwr_long(fabs);
	DBG_MESSAGE("karte_t::laden()", "prepare for %i factories", fabs);

	for(sint32 i = 0; i < fabs; i++) {
		// liste in gleicher reihenfolge wie vor dem speichern wieder aufbauen
		fabrik_t *fab = new fabrik_t(this, file);
		if(fab->get_besch()) {
			fab_list.append( fab );
		}
		else {
			dbg->error("karte_t::laden()","Unknown fabrik skipped!");
			delete fab;
		}
		if(i&7) {
			ls.set_progress( get_size().y/2+(128*i)/fabs );
		}
	}

	// load linemanagement status (and lines)
	// @author hsiegeln
	if (file->get_version() > 82003  &&  file->get_version()<88003) {
		DBG_MESSAGE("karte_t::laden()", "load linemanagement");
		get_spieler(0)->simlinemgmt.rdwr(this, file, get_spieler(0));
	}
	// end load linemanagement

	DBG_MESSAGE("karte_t::laden()", "load stops");
	// now load the stops
	// (the players will be load later and overwrite some values,
	//  like the total number of stops build (for the numbered station feature)
	if(file->get_version()>=99008) {
		sint32 halt_count;
		file->rdwr_long(halt_count);
		DBG_MESSAGE("karte_t::laden()","%d halts loaded",halt_count);
		for(int i=0; i<halt_count; i++) {
			halthandle_t halt = haltestelle_t::create( this, file );
			if(!halt->existiert_in_welt()) {
				dbg->warning("karte_t::laden()", "could not restore stop near %i,%i", halt->get_init_pos().x, halt->get_init_pos().y );
			}
			ls.set_progress( get_size().y/2+128+(get_size().y*i)/(2*halt_count) );
		}
	}

	DBG_MESSAGE("karte_t::laden()", "load convois");
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
		convoi_t *cnv = new convoi_t(this, file);
		convoi_array.append(cnv->self);

		if(cnv->in_depot()) {
			grund_t * gr = lookup(cnv->get_pos());
			depot_t *dep = gr ? gr->get_depot() : 0;
			if(dep) {
				cnv->betrete_depot(dep);
			}
			else {
				dbg->error("karte_t::laden()", "no depot for convoi, blocks may now be wrongly reserved!");
				cnv->destroy();
			}
		}
		else {
			sync_add( cnv );
		}
		if(  (convoi_array.get_count()&7) == 0  ) {
			ls.set_progress( get_size().y+(get_size().y*convoi_array.get_count())/(2*max_convoi)+128 );
		}
	}
DBG_MESSAGE("karte_t::laden()", "%d convois/trains loaded", convoi_array.get_count());

	// jetzt koennen die spieler geladen werden
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(  spieler[i]  ) {
			spieler[i]->rdwr(file);
			settings.automaten[i] = spieler[i]->is_active();
		}
		else {
			settings.automaten[i] = false;
		}
		ls.set_progress( (get_size().y*3)/2+128+16*i );
	}
DBG_MESSAGE("karte_t::laden()", "players loaded");

	// loading messages
	if(  file->get_version()>=102005  ) {
		msg->rdwr(file);
	}
	else if(  !umgebung_t::networkmode  ) {
		msg->clear();
	}
DBG_MESSAGE("karte_t::laden()", "messages loaded");

	// nachdem die welt jetzt geladen ist koennen die Blockstrecken neu
	// angelegt werden
	old_blockmanager_t::laden_abschliessen(this);
	DBG_MESSAGE("karte_t::laden()", "blocks loaded");

	file->rdwr_long(mi);
	file->rdwr_long(mj);
	DBG_MESSAGE("karte_t::laden()", "Setting view to %d,%d", mi,mj);
	if(is_within_limits(mi,mj)) {
		change_world_position( koord3d(mi,mj,min_hgt(koord(mi,mj))) );
	}
	else {
		change_world_position( koord3d(mi,mj,0) );
	}

	// right season for recalculations
	recalc_snowline();

DBG_MESSAGE("karte_t::laden()", "%d ways loaded",weg_t::get_alle_wege().get_count());

	ls.set_progress( (get_size().y*3)/2+256 );

	// ugly hack to pass file version to laden_abschliessen multithreaded
	file_version = file->get_version();
	world_xy_loop(&karte_t::plans_laden_abschliessen, true);
	ls.set_progress( (get_size().y*3)/2+256+get_size().y/8 );

DBG_MESSAGE("karte_t::laden()", "laden_abschliesen for tiles finished" );

	// must finish loading cities first before cleaning up factories
	weighted_vector_tpl<stadt_t*> new_weighted_stadt(stadt.get_count() + 1);
	FOR(weighted_vector_tpl<stadt_t*>, const s, stadt) {
		s->laden_abschliessen();
		s->recalc_target_cities();
		new_weighted_stadt.append(s, s->get_einwohner());
		INT_CHECK("simworld 1278");
	}
	swap(stadt, new_weighted_stadt);
	DBG_MESSAGE("karte_t::laden()", "cities initialized");

	ls.set_progress( (get_size().y*3)/2+256+get_size().y/4 );

	DBG_MESSAGE("karte_t::laden()", "clean up factories");
	FOR(slist_tpl<fabrik_t*>, const f, fab_list) {
		f->laden_abschliessen();
	}

DBG_MESSAGE("karte_t::laden()", "%d factories loaded", fab_list.get_count());

	// old versions did not save factory connections
	if(file->get_version()<99014) {
		sint32 const temp_min = settings.get_factory_worker_minimum_towns();
		sint32 const temp_max = settings.get_factory_worker_maximum_towns();
		// this needs to avoid the first city to be connected to all town
		settings.set_factory_worker_minimum_towns(0);
		settings.set_factory_worker_maximum_towns(stadt.get_count() + 1);
		FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
			i->verbinde_fabriken();
		}
		settings.set_factory_worker_minimum_towns(temp_min);
		settings.set_factory_worker_maximum_towns(temp_max);
	}
	ls.set_progress( (get_size().y*3)/2+256+get_size().y/3 );

	// resolve dummy stops into real stops first ...
	FOR(slist_tpl<halthandle_t>, const i, haltestelle_t::get_alle_haltestellen()) {
		if (i->get_besitzer() && i->existiert_in_welt()) {
			i->laden_abschliessen();
		}
	}

	// ... before removing dummy stops
	for(  slist_tpl<halthandle_t>::const_iterator i=haltestelle_t::get_alle_haltestellen().begin(); i!=haltestelle_t::get_alle_haltestellen().end();  ) {
		halthandle_t const h = *i;
		++i;
		if (!h->get_besitzer() || !h->existiert_in_welt()) {
			// this stop was only needed for loading goods ...
			haltestelle_t::destroy(h);	// remove from list
		}
	}

	ls.set_progress( (get_size().y*3)/2+256+(get_size().y*3)/8 );

	// adding lines and other stuff for convois
	for(unsigned i=0;  i<convoi_array.get_count();  i++ ) {
		convoihandle_t cnv = convoi_array[i];
		cnv->laden_abschliessen();
		// was deleted during loading => use same position again
		if(!cnv.is_bound()) {
			i--;
		}
	}

	// register all line stops and change line types, if needed
	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		if(  spieler[i]  ) {
			spieler[i]->laden_abschliessen();
		}
	}

#ifdef DEBUG
	long dt = dr_time();
#endif
	// recalculate halt connections
	haltestelle_t::reset_routing();
	do {
		haltestelle_t::step_all();
	} while (  haltestelle_t::get_rerouting_status()==RECONNECTING  );
#ifdef DEBUG
	DBG_MESSAGE("rebuild_destinations()","for all haltstellen_t took %ld ms", dr_time()-dt );
#endif

#if 0
	// reroute goods for benchmarking
	dt = dr_time();
	FOR(slist_tpl<halthandle_t>, const i, haltestelle_t::get_alle_haltestellen()) {
		sint16 dummy = 0x7FFF;
		i->reroute_goods(dummy);
	}
	DBG_MESSAGE("reroute_goods()","for all haltstellen_t took %ld ms", dr_time()-dt );
#endif

	// load history/create world history
	if(file->get_version()<99018) {
		restore_history();
	}
	else {
		// most recent savegame version is 99018
		for (int year = 0;  year</*MAX_WORLD_HISTORY_YEARS*/12;  year++) {
			for (int cost_type = 0; cost_type</*MAX_WORLD_COST*/12; cost_type++) {
				file->rdwr_longlong(finance_history_year[year][cost_type]);
			}
		}
		for (int month = 0;month</*MAX_WORLD_HISTORY_MONTHS*/12;month++) {
			for (int cost_type = 0; cost_type</*MAX_WORLD_COST*/12; cost_type++) {
				file->rdwr_longlong(finance_history_month[month][cost_type]);
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
	if(  !umgebung_t::networkmode  ) {
		for(  uint8 i=0;  i<PLAYER_UNOWNED;  i++  ) {
			if(  spieler[i]  ) {
				spieler[i]->check_unlock( player_password_hash[i] );
			}
		}
	}
	// initialize lock info for local server player
	// if call from sync command, lock info will be corrected there
	if(  umgebung_t::server) {
		nwc_auth_player_t::init_player_lock_server(this);
	}

	if(  file->get_version()>=102004  ) {
		if(  umgebung_t::restore_UI  ) {
			file->rdwr_byte( active_player_nr );
			active_player = spieler[active_player_nr];
			/* restore all open windows
			 * otherwise it will be ignored
			 * which is save, since it is the end of file
			 */
			rdwr_all_win( file );
		}
	}

	file->set_buffered(false);
	clear_random_mode(LOAD_RANDOM);
	dbg->warning("karte_t::laden()","loaded savegame from %i/%i, next month=%i, ticks=%i (per month=1<<%i)",last_month,last_year,next_month_ticks,ticks,karte_t::ticks_per_world_month_shift);
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
							plan[nr].get_boden_bei(i)->calc_bild();
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
					plan[nr].get_boden_bei(i)->calc_bild();
				}
			}
		}
	}
}


// recalcs all ground tiles on the map
void karte_t::update_map()
{
	DBG_MESSAGE( "karte_t::update_map()", "" );
	world_xy_loop(&karte_t::update_map_intern, true);
	set_dirty();
}


// return an index to a halt (or creates a new one)
// only used during loading
halthandle_t karte_t::get_halt_koord_index(koord k)
{
	if(!is_within_limits(k)) {
		return halthandle_t();
	}
	// already there?
	const halthandle_t h=lookup(k)->get_halt();
	if(!h.is_bound()) {
		// no => create
		return haltestelle_t::create( this, k, NULL );
	}
	return h;
}


uint8 karte_t::sp2num(spieler_t *sp)
{
	if(  sp==NULL  ) {
		return PLAYER_UNOWNED;
	}
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(spieler[i] == sp) {
			return i;
		}
	}
	dbg->fatal( "karte_t::sp2num()", "called with an invalid player!" );
}


void karte_t::load_heightfield(settings_t* const sets)
{
	sint16 w, h;
	sint8 *h_field;
	if(karte_t::get_height_data_from_file(sets->heightfield.c_str(), (sint8)(sets->get_grundwasser()), h_field, w, h, false )) {
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
				gr = lookup_kartenboden( koord(x,y) );
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

	if(  umgebung_t::networkmode  &&  (step_mode&PAUSE_FLAG)==0  ) {
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
	}
	else {
		// make timer loop invalid
		for( int i=0;  i<32;  i++ ) {
			last_frame_ms[i] = 0x7FFFFFFFu;
			last_step_nr[i] = 0xFFFFFFFFu;
		}
		last_frame_idx = 0;
		simloops = 60;

		set_frame_time( 1000/umgebung_t::fps );
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
	if(  umgebung_t::server  ) {
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
	current_month += 12;
	last_year ++;
	reset_timer();
	recalc_average_speed();
	koord::locality_factor = settings.get_locality_factor( last_year );
	FOR(weighted_vector_tpl<stadt_t*>, const i, stadt) {
		i->recalc_target_cities();
		i->recalc_target_attractions();
	}
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
	if(  !umgebung_t::networkmode  ) {
		time_multiplier += delta;
		if(time_multiplier<=0) {
			time_multiplier = 1;
		}
		if(step_mode!=NORMAL) {
			step_mode = NORMAL;
			reset_timer();
		}
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
	if(  !umgebung_t::networkmode  ) {
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


grund_t* karte_t::get_ground_on_screen_coordinate(const koord screen_pos, sint32 &found_i, sint32 &found_j, const bool intersect_grid) const
{
	const int rw1 = get_tile_raster_width();
	const int rw4 = rw1/4;

	/*
	* berechnung der basis feldkoordinaten in i und j
	* this would calculate raster i,j koordinates if there was no height
	*  die formeln stehen hier zur erinnerung wie sie in der urform aussehen

	int base_i = (screen_x+screen_y)/2;
	int base_j = (screen_y-screen_x)/2;

	int raster_base_i = (int)floor(base_i / 16.0);
	int raster_base_j = (int)floor(base_j / 16.0);

	*/

	const int i_off = ij_off.x+get_view_ij_offset().x;
	const int j_off = ij_off.y+get_view_ij_offset().y;

	bool found = false;
	// uncomment to: ctrl-key selects ground
	//bool select_karten_boden = event_get_last_control_shift()==2;

	// fallback: take kartenboden if nothing else found
	grund_t *bd = NULL;
	grund_t *gr = NULL;
	// for the calculation of hmin/hmax see simview.cc
	// for the definition of underground_level see grund_t::set_underground_mode
	const sint8 hmin = grund_t::underground_mode!=grund_t::ugm_all ? min(grundwasser, grund_t::underground_level) : get_minimumheight();
	const sint8 hmax = grund_t::underground_mode==grund_t::ugm_all ? get_maximumheight() : min(grund_t::underground_level, get_maximumheight());

	// find matching and visible grund
	for(sint8 hgt = hmax; hgt>=hmin; hgt--) {

		const int base_i = (screen_pos.x+screen_pos.y + tile_raster_scale_y((hgt*TILE_HEIGHT_STEP),rw1))/2;
		const int base_j = (screen_pos.y-screen_pos.x + tile_raster_scale_y((hgt*TILE_HEIGHT_STEP),rw1))/2;

		found_i = ((int)floor(base_i/(double)rw4)) + i_off;
		found_j = ((int)floor(base_j/(double)rw4)) + j_off;

		gr = lookup(koord3d(found_i,found_j,hgt));
		if(gr != NULL) {
			found = /*select_karten_boden ? gr->ist_karten_boden() :*/ gr->is_visible();
			if( ( gr->get_typ() == grund_t::tunnelboden || gr->get_typ() == grund_t::monorailboden ) && gr->get_weg_nr(0) == NULL && !gr->get_leitung()  &&  gr->find<zeiger_t>()) {
				// This is only a dummy ground placed by wkz_tunnelbau_t or wkz_wegebau_t as a preview.
				found = false;
			}
			if (found) {
				break;
			}

			if (bd==NULL && gr->ist_karten_boden()) {
				bd = gr;
			}
		}
		if (grund_t::underground_mode==grund_t::ugm_level && hgt==hmax) {
			// fallback in sliced mode, if no ground is under cursor
			bd = lookup_kartenboden(koord(found_i,found_j));
		}
		else if (intersect_grid){
			// We try to intersect with virtual nonexistent border tiles in south and east.
			if(  (gr = lookup_gridcoords( koord3d( found_i, found_j, hgt ) ))  ){
				found = true;
				break;
			}
		}

		// Last resort, try to intersect with the same tile +1 height, seems to be necessary on steep slopes
		// *NOTE* Don't do it on border tiles, since it will extend the range in which the cursor will be considered to be
		// inside world limits.
		if( found_i==(get_size().x-1)  ||  found_j == (get_size().y-1) ) {
			continue;
		}
		gr = lookup(koord3d(found_i,found_j,hgt+1));
		if(gr != NULL) {
			found = /*select_karten_boden ? gr->ist_karten_boden() :*/ gr->is_visible();
			if( ( gr->get_typ() == grund_t::tunnelboden || gr->get_typ() == grund_t::monorailboden ) && gr->get_weg_nr(0) == NULL && !gr->get_leitung()  &&  gr->find<zeiger_t>()) {
				// This is only a dummy ground placed by wkz_tunnelbau_t or wkz_wegebau_t as a preview.
				found = false;
			}
			if (found) {
				break;
			}
			if (bd==NULL && gr->ist_karten_boden()) {
				bd = gr;
			}
		}
	}

	if(found) {
		return gr;
	}
	else {
		if(bd!=NULL){
			found_i = bd->get_pos().x;
			found_j = bd->get_pos().y;
			return bd;
		}
		return NULL;
	}
}


koord3d karte_t::get_new_cursor_position(const event_t *ev ,bool grid_coordinates){
	const int rw1 = get_tile_raster_width();
	const int rw2 = rw1/2;
	const int rw4 = rw1/4;

	int screen_y = ev->my - y_off - rw2 - ((display_get_width()/rw1)&1)*rw4;
	int screen_x = (ev->mx - x_off - rw2)/2;

	if(zeiger->get_yoff() == Z_PLAN) {
		// already ok
	}
	else {
		// shifted by a quarter tile
		screen_y += rw4;
	}

	const grund_t *bd = get_ground_on_screen_coordinate(koord(screen_x, screen_y), mi, mj, grid_coordinates);

	// no suitable location found (outside map, ...)
	if (!bd) {
		return koord3d::invalid;
	}

	// offset needed for the raise / lower tool.
	sint8 groff;

	if( bd->is_visible() ) {
		groff = bd->get_hoehe(get_corner_to_operate(koord(mi, mj))) - bd->get_hoehe();
	}
	else {
		groff = 0;
	}

	// the new position - extra logic for raise / lower tool
	return koord3d(mi,mj, bd->get_disp_height() + (zeiger->get_yoff()==Z_GRID ? groff : 0));


}


void karte_t::move_cursor(const event_t *ev)
{
	if(!zeiger) {
		// No cursor to move, exit
		return;
	}

	static int mb_alt=0;

	werkzeug_t *wkz = werkzeug[get_active_player_nr()];

	const koord3d pos = get_new_cursor_position(ev, wkz->is_grid_tool());

	if( pos == koord3d::invalid ) {
		zeiger->change_pos(pos);
		return;
	}


	// move cursor
	const koord3d prev_pos = zeiger->get_pos();
	if(  (prev_pos != pos ||  ev->button_state != mb_alt)  ) {

		mb_alt = ev->button_state;

		zeiger->change_pos(pos);

		if(  !umgebung_t::networkmode  ||  wkz->is_move_network_save(get_active_player())) {
			wkz->flags = event_get_last_control_shift() | werkzeug_t::WFL_LOCAL;
			if(wkz->check_pos( this, get_active_player(), zeiger->get_pos() )==NULL) {
				if(  ev->button_state == 0  ) {
					is_dragging = false;
				}
				else if(ev->ev_class==EVENT_DRAG) {
					if(!is_dragging  &&  wkz->check_pos( this, get_active_player(), prev_pos )==NULL) {
						const char* err = get_scenario()->is_work_allowed_here(get_active_player(), wkz->get_id(), wkz->get_waytype(), prev_pos);
						if (err == NULL) {
							is_dragging = true;
						}
						else {
							is_dragging = false;
						}
					}
				}
				if (is_dragging) {
					const char* err = get_scenario()->is_work_allowed_here(get_active_player(), wkz->get_id(), wkz->get_waytype(), pos);
					if (err == NULL) {
						wkz->move( this, get_active_player(), is_dragging, pos );
					}
				}
			}
			wkz->flags = 0;
		}

		if(  (ev->button_state&7)==0  ) {
			// time, since mouse got here
			mouse_rest_time = dr_time();
			sound_wait_time = AMBIENT_SOUND_INTERVALL;	// 13s no movement: play sound
		}
	}
}


bool karte_t::is_background_visible() const
{

	sint32 i,j;

	if ( get_ground_on_screen_coordinate(koord(0,0),i,j)  &&  \
		 get_ground_on_screen_coordinate(koord(display_get_width()-1,0),i,j)  &&  \
		 get_ground_on_screen_coordinate(koord(0,display_get_height()-1),i,j)  &&  \
		 get_ground_on_screen_coordinate(koord(display_get_width()-1,display_get_height()-1),i,j)  ) {
		return false;
	}

	return true;
}


/* creates a new player with this type */
const char *karte_t::new_spieler(uint8 new_player, uint8 type)
{
	if(  new_player>=PLAYER_UNOWNED  ||  get_spieler(new_player)!=NULL  ) {
		return "Id invalid/already in use!";
	}
	switch( type ) {
		case spieler_t::EMPTY: break;
		case spieler_t::HUMAN: spieler[new_player] = new spieler_t(this,new_player); break;
		case spieler_t::AI_GOODS: spieler[new_player] = new ai_goods_t(this,new_player); break;
		case spieler_t::AI_PASSENGER: spieler[new_player] = new ai_passenger_t(this,new_player); break;
		default: return "Unknow AI type!";
	}
	settings.set_player_type(new_player, type);
	return NULL;
}


void karte_t::remove_player(uint8 player_nr)
{
	if ( player_nr!=1  &&  player_nr<PLAYER_UNOWNED  &&  spieler[player_nr]!=NULL) {
		spieler[player_nr]->ai_bankrupt();
		delete spieler[player_nr];
		spieler[player_nr] = 0;
		nwc_chg_player_t::company_removed(player_nr);
		// if default human, create new instace of it (to avoid crashes)
		if(  player_nr == 0  ) {
			spieler[0] = new spieler_t( this, 0 );
		}
		// if currently still active => reset to default human
		if(  player_nr == active_player_nr  ) {
			active_player_nr = 0;
			active_player = spieler[0];
			if(  !umgebung_t::server  ) {
				create_win( display_get_width()/2-128, 40, new news_img("Bankrott:\n\nDu bist bankrott.\n"), w_info, magic_none);
			}
		}
	}
}


/* goes to next active player */
void karte_t::switch_active_player(uint8 new_player, bool silent)
{
	for(  uint8 i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  spieler[(i+new_player)%MAX_PLAYER_COUNT] != NULL  ) {
			new_player = (i+new_player)%MAX_PLAYER_COUNT;
			break;
		}
	}
	koord3d old_zeiger_pos = zeiger->get_pos();

	// no cheating allowed?
	if (!settings.get_allow_player_change() && spieler[1]->is_locked()) {
		active_player_nr = 0;
		active_player = spieler[0];
		if(new_player!=0) {
			create_win( new news_img("On this map, you are not\nallowed to change player!\n"), w_time_delete, magic_none);
		}
	}
	else {
		zeiger->change_pos( koord3d::invalid ); // unmark area
		// exit active tool to remove pointers (for two_click_tool_t's, stop mover, factory linker)
		if(werkzeug[active_player_nr]) {
			werkzeug[active_player_nr]->exit(this, active_player);
		}
		active_player_nr = new_player;
		active_player = spieler[new_player];
		if(  !silent  ) {
			// tell the player
			cbuffer_t buf;
			buf.printf( translator::translate("Now active as %s.\n"), get_active_player()->get_name() );
			msg->add_message(buf, koord::invalid, message_t::ai | message_t::local_flag, PLAYER_FLAG|get_active_player()->get_player_nr(), IMG_LEER);
		}

		// update menue entries
		werkzeug_t::update_toolbars(this);
		set_dirty();
	}

	// update pointer image / area
	werkzeug[active_player_nr]->init_cursor(zeiger);
	// set position / mark area
	zeiger->change_pos( old_zeiger_pos );
}


void karte_t::interactive_event(event_t &ev)
{
	if(ev.ev_class == EVENT_KEYBOARD) {
		DBG_MESSAGE("karte_t::interactive_event()","Keyboard event with code %d '%c'", ev.ev_code, (ev.ev_code>=32  &&  ev.ev_code<=126) ? ev.ev_code : '?' );

		switch(ev.ev_code) {

			// cursor movements
			case '9':
				ij_off += koord::nord;
				dirty = true;
				break;
			case '1':
				ij_off += koord::sued;
				dirty = true;
				break;
			case '7':
				ij_off += koord::west;
				dirty = true;
				break;
			case '3':
				ij_off += koord::ost;
				dirty = true;
				break;
			case '6':
			case SIM_KEY_RIGHT:
				ij_off += koord(+1,-1);
				dirty = true;
				break;
			case '2':
			case SIM_KEY_DOWN:
				ij_off += koord(+1,+1);
				dirty = true;
				break;
			case '8':
			case SIM_KEY_UP:
				ij_off += koord(-1,-1);
				dirty = true;
				break;
			case '4':
			case SIM_KEY_LEFT:
				ij_off += koord(-1,+1);
				dirty = true;
				break;

			// closing windows
			case 27:
			case 127:
				// close topmost win
				destroy_win( win_get_top() );
				break;

			case SIM_KEY_F1:
				if(  gui_frame_t *win = win_get_top()  ) {
					if(  const char *helpfile = win->get_hilfe_datei()  ) {
						help_frame_t::open_help_on( helpfile );
						break;
					}
				}
				set_werkzeug( werkzeug_t::dialog_tool[WKZ_HELP], get_active_player() );
				break;

			// just ignore the key
			case 0:
				break;

			// distinguish between backspace and ctrl-H (both keycode==8), and enter and ctrl-M (both keycode==13)
			case 8:
			case 13:
				if(  !IS_CONTROL_PRESSED(&ev)  ) {
					// Control is _not_ pressed => Backspace or Enter pressed.
					if(  ev.ev_code == 8  ) {
						// Backspace
						sound_play(SFX_SELECT);
						destroy_all_win(false);
					}
					// Ignore Enter and Backspace but not Ctrl-H and Ctrl-M
					break;
				}

			default:
				{
					bool ok=false;
					FOR(vector_tpl<werkzeug_t*>, const i, werkzeug_t::char_to_tool) {
						if (i->command_key == ev.ev_code) {
							set_werkzeug(i, get_active_player());
							ok = true;
							break;
						}
					}
					if(!ok) {
						help_frame_t::open_help_on( "keys.txt" );
					}
				}
				break;
		}
	}

	if(  IS_LEFTRELEASE(&ev)  &&  ev.my < display_get_height() - 32 + (16*ticker::empty())  ) {

		DBG_MESSAGE("karte_t::interactive_event(event_t &ev)", "calling a tool");

		if(is_within_grid_limits(zeiger->get_pos().get_2d())) {
			const char *err = NULL;
			bool result = true;
			werkzeug_t *wkz = werkzeug[get_active_player_nr()];
			// first check for visibility etc
			err = wkz->check_pos( this, get_active_player(), zeiger->get_pos() );
			if (err==NULL) {
				wkz->flags = event_get_last_control_shift();
				if (!umgebung_t::networkmode  ||  wkz->is_work_network_save()  ||  wkz->is_work_here_network_save( this, get_active_player(), zeiger->get_pos() ) ) {
					// do the work
					wkz->flags |= werkzeug_t::WFL_LOCAL;
					// check allowance by scenario
					koord3d const& pos = zeiger->get_pos();
					if (get_scenario()->is_scripted()) {
						if (!get_scenario()->is_tool_allowed(get_active_player(), wkz->get_id(), wkz->get_waytype()) ) {
							err = "";
						}
						else {
							err = get_scenario()->is_work_allowed_here(get_active_player(), wkz->get_id(), wkz->get_waytype(), pos);
						}
					}
					if (err == NULL) {
						err = wkz->work(this, get_active_player(), zeiger->get_pos());
						if( err == NULL ) {
							// Check if we need to update pointer(zeiger) position.
							if ( wkz->update_pos_after_use() ) {
								// Cursor might need movement (screen has changed, we get a new one under the mouse pointer)
								const koord3d k = get_new_cursor_position(&ev ,wkz->is_grid_tool());
								zeiger->set_pos(k);
							}
						}
					}
				}
				else {
					// queue tool for network
					nwc_tool_t *nwc = new nwc_tool_t(get_active_player(), wkz, zeiger->get_pos(), steps, map_counter, false);
					network_send_server(nwc);
					result = false;
					// reset tool
					wkz->init(this, get_active_player());
				}
			}
			if (result) {
				// play sound / error message
				get_active_player()->tell_tool_result(wkz, zeiger->get_pos(), err, true);
			}
			wkz->flags = 0;
		}
	}

	// mouse wheel scrolled -> rezoom
	if (ev.ev_class == EVENT_CLICK) {
		const sint16 org_raster_width = get_tile_raster_width();
		if(ev.ev_code==MOUSE_WHEELUP) {
			if(win_change_zoom_factor(true)) {
				set_dirty();
			}
		}
		else if(ev.ev_code==MOUSE_WHEELDOWN) {
			if(win_change_zoom_factor(false)) {
				set_dirty();
			}
		}
		const sint16 new_raster_width = get_tile_raster_width();
		if (org_raster_width != new_raster_width) {
			// scale the fine offsets for displaying
			x_off = (x_off * new_raster_width) / org_raster_width;
			y_off = (y_off * new_raster_width) / org_raster_width;
		}
	}
	INT_CHECK("simworld 2117");
}


void karte_t::stop(bool exit_game)
{
	finish_loop = true;
	umgebung_t::quit_simutrans = exit_game;
}


void karte_t::network_game_set_pause(bool pause_, uint32 syncsteps_)
{
	if (umgebung_t::networkmode) {
		sync_steps = syncsteps_;
		steps = sync_steps / settings.get_frames_per_step();
		network_frame_count = sync_steps % settings.get_frames_per_step();
		dbg->warning("karte_t::network_game_set_pause", "steps=%d sync_steps=%d pause=%d", steps, sync_steps, pause_);
		if (pause_) {
			if (!umgebung_t::server) {
				reset_timer();
				step_mode = PAUSE_FLAG|FIX_RATIO;
			}
			else {
				// TODO
			}
		}
		else {
			step_mode = FIX_RATIO;
			if (!umgebung_t::server) {
				/* make sure, the server is really that far ahead
				 * Sleep() on windows often returns before!
				 */
				unsigned long const ms = dr_time() + (settings.get_server_frames_ahead() + (uint32)umgebung_t::additional_client_frames_behind) * fix_ratio_frame_time;
				while(  dr_time()<ms  ) {
					dr_sleep ( 10 );
				}
			}
			reset_timer();
		}
	}
	else {
		set_pause(pause_);
	}
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


bool karte_t::interactive(uint32 quit_month)
{
	event_t ev;
	finish_loop = false;
	bool swallowed = false;
	bool cursor_hidden = false;
	sync_steps = 0;

	network_frame_count = 0;
	vector_tpl<uint16>hashes_ok;	// bit set: this client can do something with this player

	if(  !scenario->rdwr_ok()  ) {
		// error during loading of savegame of scenario
		create_win( new news_img( scenario->get_error_text() ), w_info, magic_none);
		scenario->stop();
	}
	// only needed for network
	if(  umgebung_t::networkmode  ) {
		// clear the checklist history
		for(  int i=0;  i<LAST_CHECKLISTS_COUNT;  ++i  ) {
			last_checklists[i] = checklist_t();
		}
	}
	uint32 next_command_step = 0xFFFFFFFFu;
	sint32 ms_difference = 0;
	reset_timer();
	DBG_DEBUG4("karte_t::interactive", "welcome in this routine");

	if(  umgebung_t::server  ) {
		step_mode |= FIX_RATIO;

		reset_timer();
		// Announce server startup to the listing server
		if(  umgebung_t::server_announce  ) {
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
					sint16 id = NO_SOUND;
					if(  gr->ist_natur()  ||  gr->ist_wasser()  ) {
						if(  gr->get_pos().z >= get_snowline()  ) {
							id = sound_besch_t::climate_sounds[ arctic_climate ];
						}
						else {
							id = sound_besch_t::climate_sounds[ get_climate(zeiger->get_pos().z) ];
						}
						if(  id==NO_SOUND  ) {
							// try, if there is another sound ready
							if(  zeiger->get_pos().z==grundwasser  &&  !gr->ist_wasser()  ) {
								id = sound_besch_t::beach_sound;
							}
							else if(  gr->get_top()>0  &&  gr->obj_bei(0)->get_typ()==ding_t::baum  ) {
								id = sound_besch_t::forest_sound;
							}
						}
					}
					if(  id!=NO_SOUND  ) {
						sound_play(id);
					}
				}
				sound_wait_time *= 2;
			}
			DBG_DEBUG4("karte_t::interactive", "end of sound");
		}

		// get an event
		DBG_DEBUG4("karte_t::interactive", "calling win_poll_event");
		win_poll_event(&ev);

		if(ev.ev_class==EVENT_SYSTEM  &&  ev.ev_code==SYSTEM_QUIT) {
			// quit the program if this windows is closed
			destroy_all_win(true);
			umgebung_t::quit_simutrans = true;
			break;
		}

		if(ev.ev_class!=EVENT_NONE &&  ev.ev_class!=IGNORE_EVENT) {

			if(  umgebung_t::networkmode  ) {
				set_random_mode( INTERACTIVE_RANDOM );
			}

			DBG_DEBUG4("karte_t::interactive", "calling check_pos_win");
			swallowed = check_pos_win(&ev);

			if(  !swallowed  ) {
				if(IS_RIGHTCLICK(&ev)) {
					display_show_pointer(false);
					cursor_hidden = true;
				} else if(IS_RIGHTRELEASE(&ev)) {
					display_show_pointer(true);
					cursor_hidden = false;
				} else if(IS_RIGHTDRAG(&ev)) {
					// unset following
					follow_convoi = convoihandle_t();
					move_view(&ev);
				}
				else {
					if(cursor_hidden) {
						display_show_pointer(true);
						cursor_hidden = false;
					}
				}
			}

			DBG_DEBUG4("karte_t::interactive", "after check_pos_win");
			if((!swallowed  &&  (ev.ev_class==EVENT_DRAG  &&  ev.ev_code==MOUSE_LEFTBUTTON))  ||  (ev.button_state==0  &&  ev.ev_class==EVENT_MOVE)  ||  ev.ev_class==EVENT_RELEASE) {
				move_cursor(&ev);
			}

			if(  umgebung_t::networkmode  ) {
				clear_random_mode( INTERACTIVE_RANDOM );
			}
			DBG_DEBUG4("karte_t::interactive", "end of event handling");
		}

		if(  umgebung_t::networkmode  ) {
			// did we receive a new command?
			unsigned long ms = dr_time();
			network_command_t *nwc = network_check_activity( this, next_step_time>ms ? min( next_step_time-ms, 5) : 0 );
			if(  nwc==NULL  &&  !network_check_server_connection()  ) {
				dbg->warning("karte_t::interactive", "lost connection to server");
				network_disconnect();
			}

			// process all the received commands at once
			while (nwc) {
				// check timing
				if (nwc->get_id()==NWC_CHECK) {
					// checking for synchronisation
					nwc_check_t* nwcheck = (nwc_check_t*)nwc;
					// are we on time?
					ms_difference = 0;
					sint64 const difftime = (sint64)next_step_time - dr_time() + ((sint64)nwcheck->server_sync_step - sync_steps - settings.get_server_frames_ahead() - umgebung_t::additional_client_frames_behind) * fix_ratio_frame_time;
					if(  difftime<0  ) {
						// running ahead
						next_step_time += (uint32)(-difftime);
					}
					else {
						// more gentle catching up
						ms_difference = (sint32)difftime;
					}
					dbg->message("NWC_CHECK","time difference to server %lli",difftime);
				}
				// check random number generator states
				if(  umgebung_t::server  &&  nwc->get_id()==NWC_TOOL  ) {
					nwc_tool_t *nwt = dynamic_cast<nwc_tool_t *>(nwc);
					if(  nwt->is_from_initiator()  ) {
						if(  nwt->last_sync_step>sync_steps  ) {
							dbg->warning("karte_t::interactive", "client was too fast (skipping command)" );
							delete nwc;
							nwc = NULL;
						}
						// out of sync => drop client (but we can only compare if nwt->last_sync_step is not too old)
						else if(  is_checklist_available(nwt->last_sync_step)  &&  LCHKLST(nwt->last_sync_step)!=nwt->last_checklist  ) {
							// lost synchronisation -> server kicks client out actively
							char buf[256];
							const int offset = LCHKLST(nwt->last_sync_step).print(buf, "server");
							nwt->last_checklist.print(buf + offset, "initiator");
							dbg->warning("karte_t::interactive", "kicking client due to checklist mismatch : sync_step=%u %s", nwt->last_sync_step, buf);
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
			// when execute next command?
			if(  !command_queue.empty()  ) {
				next_command_step = command_queue.front()->get_sync_step();
			}
			else {
				next_command_step = 0xFFFFFFFFu;
			}

			// send data
			ms = dr_time();
			network_process_send_queues( next_step_time>ms ? min( next_step_time-ms, 5) : 0 );
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

		while(  !command_queue.empty()  &&  (next_command_step<=sync_steps/*  ||  step_mode&PAUSE_FLAG*/)  ) {
			network_world_command_t *nwc = command_queue.remove_first();
			if (nwc) {
				// want to execute something in the past?
				if (nwc->get_sync_step() < sync_steps) {
					if (!nwc->ignore_old_events()) {
						dbg->warning("karte_t::interactive", "wanted to do_command(%d) in the past", nwc->get_id());
						network_disconnect();
					}
				}
				// check map counter
				else if (nwc->get_map_counter() != map_counter) {
					dbg->warning("karte_t::interactive", "wanted to do_command(%d) from another world", nwc->get_id());
				}
				// check random counter?
				else if(  nwc->get_id()==NWC_CHECK  ) {
					nwc_check_t* nwcheck = (nwc_check_t*)nwc;
					// this was the random number at the previous sync step on the server
					const checklist_t &server_checklist = nwcheck->server_checklist;
					const uint32 server_sync_step = nwcheck->server_sync_step;
					char buf[256];
					const int offset = server_checklist.print(buf, "server");
					LCHKLST(server_sync_step).print(buf + offset, "client");
					dbg->warning("karte_t::interactive", "sync_step=%u  %s", server_sync_step, buf);
					if(  LCHKLST(server_sync_step)!=server_checklist  ) {
						dbg->warning("karte_t::interactive", "disconnecting due to checklist mismatch" );
						network_disconnect();
					}
				}
				else {
					if(  nwc->get_id()==NWC_TOOL  ) {
						nwc_tool_t *nwt = dynamic_cast<nwc_tool_t *>(nwc);
						if(  is_checklist_available(nwt->last_sync_step)  &&  LCHKLST(nwt->last_sync_step)!=nwt->last_checklist  ) {
							// lost synchronisation ...
							char buf[256];
							const int offset = nwt->last_checklist.print(buf, "server");
							LCHKLST(nwt->last_sync_step).print(buf + offset, "executor");
							dbg->warning("karte_t::interactive", "skipping command due to checklist mismatch : sync_step=%u %s", nwt->last_sync_step, buf);
							if(  !umgebung_t::server  ) {
								network_disconnect();
							}
							delete nwc;
							continue;
						}
					}
					nwc->do_command(this);
				}
				delete nwc;
			}
			// when execute next command?
			if(  !command_queue.empty()  ) {
				next_command_step = command_queue.front()->get_sync_step();
			}
			else {
				next_command_step = 0xFFFFFFFFu;
			}
		}

		// time for the next step?
		uint32 time = dr_time(); // - (umgebung_t::server ? 0 : 5000);
		if(  next_step_time<=time  ) {
			if(  step_mode&PAUSE_FLAG  ) {
				// only update display
				sync_step( 0, false, true );
				idle_time = 100;
			}
			else {
				if(  step_mode==FAST_FORWARD  ) {
					sync_step( 100, true, false );
					set_random_mode( STEP_RANDOM );
					step();
					clear_random_mode( STEP_RANDOM );
				}
				else if(  step_mode==FIX_RATIO  ) {
					next_step_time += fix_ratio_frame_time;
					if(  ms_difference>5  ) {
						next_step_time -= 5;
						ms_difference -= 5;
					}
					else if(  ms_difference<-5  ) {
						next_step_time += 5;
						ms_difference += 5;
					}
					sync_step( fix_ratio_frame_time, true, true );
					if (++network_frame_count == settings.get_frames_per_step()) {
						// ever fourth frame
						set_random_mode( STEP_RANDOM );
						step();
						clear_random_mode( STEP_RANDOM );
						network_frame_count = 0;
					}
					sync_steps = steps * settings.get_frames_per_step() + network_frame_count;
					LCHKLST(sync_steps) = checklist_t(get_random_seed(), halthandle_t::get_next_check(), linehandle_t::get_next_check(), convoihandle_t::get_next_check());
					// some serverside tasks
					if(  umgebung_t::networkmode  &&  umgebung_t::server  ) {
						// broadcast sync info
						if (  (network_frame_count==0  &&  (sint64)dr_time()-(sint64)next_step_time>fix_ratio_frame_time*2)
								||  (sync_steps % umgebung_t::server_sync_steps_between_checks)==0  ) {
							nwc_check_t* nwc = new nwc_check_t(sync_steps + 1, map_counter, LCHKLST(sync_steps), sync_steps);
							network_send_all(nwc, true);
						}
					}
#if DEBUG>4
					if(  umgebung_t::networkmode  &&  (sync_steps & 7)==0  &&  umgebung_t::verbose_debug>4  ) {
						dbg->message("karte_t::interactive", "time=%lu sync=%d  rand=%d", dr_time(), sync_steps, LRAND(sync_steps));
					}
#endif

					// no clients -> pause game
					if (  umgebung_t::networkmode  &&  umgebung_t::pause_server_no_clients  &&  socket_list_t::get_playing_clients() == 0  &&  !nwc_join_t::is_pending()  ) {
						set_pause(true);
					}
				}
				else {
					INT_CHECK( "karte_t::interactive()" );
					set_random_mode( STEP_RANDOM );
					step();
					clear_random_mode( STEP_RANDOM );
					idle_time = ((idle_time*7) + next_step_time - dr_time())/8;
					INT_CHECK( "karte_t::interactive()" );
				}
			}
		}

		// Interval-based server announcements
		if (  umgebung_t::server_announce  &&  umgebung_t::server_announce_interval > 0  &&
			dr_time() >= server_last_announce_time + (uint32)umgebung_t::server_announce_interval * 1000  ) {
			announce_server( 1 );
		}

		if (!swallowed) {
			DBG_DEBUG4("karte_t::interactive", "calling interactive_event");
			interactive_event(ev);
		}

		DBG_DEBUG4("karte_t::interactive", "point of loop return");
	} while(!finish_loop  &&  get_current_month()<quit_month);

	if(  get_current_month() >= quit_month  ) {
		umgebung_t::quit_simutrans = true;
	}

	// On quit announce server as being offline
	if(  umgebung_t::server  &&  umgebung_t::server_announce  ) {
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
	if(  umgebung_t::server_announce  ) {
		cbuffer_t buf;
		// Always send dns and port as these are used as the unique identifier for the server
		buf.append( "&dns=" );
		encode_URI( buf, umgebung_t::server_dns.c_str() );
		buf.printf( "&port=%u", umgebung_t::server );
		// Always send announce interval to allow listing server to predict next announce
		buf.printf( "&aiv=%u", umgebung_t::server_announce_interval );
		// Always send status, either online or offline
		if (  status == 0  ||  status == 1  ) {
			buf.append( "&st=1" );
		}
		else {
			buf.append( "&st=0" );
		}


		// Add fields sent only on server startup (cannot change during the course of a game)
		if (  status == 0  ) {
#ifndef REVISION
#	define REVISION 0
#endif
			// Simple revision used for matching (integer)
			buf.printf( "&rev=%d", atol( QUOTEME(REVISION) ) );
			// Complex version string used for display
			buf.printf( "&ver=Simutrans %s (r%s) built %s", QUOTEME(VERSION_NUMBER), QUOTEME(REVISION), QUOTEME(VERSION_DATE) );
			// Pakset version
			buf.append( "&pak=" );
			// Announce pak set, ideally get this from the copyright field of ground.Outside.pak
			char const* const copyright = grund_besch_t::ausserhalb->get_copyright();
			if (copyright && STRICMP("none", copyright) != 0) {
				// construct from outside object copyright string
				encode_URI( buf, copyright );
			}
			else {
				// construct from pak name
				std::string pak_name = umgebung_t::objfilename;
				pak_name.erase( pak_name.length() - 1 );
				encode_URI( buf, pak_name.c_str() );
			}
			// TODO - change this to be the start date of the current map
			buf.printf( "&start=%u,%u", settings.get_starting_month() + 1, settings.get_starting_year() );
			// Add server name for listing
			buf.append( "&name=" );
			encode_URI( buf, umgebung_t::server_name.c_str() );
			// Add server comments for listing
			buf.append( "&comments=" );
			encode_URI( buf, umgebung_t::server_comments.c_str() );
			// Add server maintainer email for listing
			buf.append( "&email=" );
			encode_URI( buf, umgebung_t::server_email.c_str() );
			// Add server pakset URL for listing
			buf.append( "&pakurl=" );
			encode_URI( buf, umgebung_t::server_pakurl.c_str() );
			// Add server info URL for listing
			buf.append( "&infurl=" );
			encode_URI( buf, umgebung_t::server_infurl.c_str() );

			// TODO send minimap data as well							// TODO
		}
		if (  status == 0  ||  status == 1  ) {
			// Now add the game data part
			uint8 active = 0, locked = 0;
			for(  uint8 i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
				if(  spieler[i]  &&  spieler[i]->get_ai_id()!=spieler_t::EMPTY  ) {
					active ++;
					if(  spieler[i]->is_locked()  ) {
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

		network_http_post( ANNOUNCE_SERVER, ANNOUNCE_URL, buf, NULL );

		// Record time of this announce
		server_last_announce_time = dr_time();
	}
}


void karte_t::network_disconnect()
{
	// force disconnect
	dbg->warning("karte_t::network_disconnect()", "Lost synchronisation with server.");
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
}


static bool sort_ware_by_name(const ware_besch_t* a, const ware_besch_t* b)
{
	int diff = strcmp(translator::translate(a->get_name()), translator::translate(b->get_name()));
	return diff < 0;
}


// Returns a list of goods produced by factories that exist in current game
const vector_tpl<const ware_besch_t*> &karte_t::get_goods_list()
{
	if (goods_in_game.empty()) {
		// Goods list needs to be rebuilt

		// Reset last vehicle filter in all depots, in case goods list has changed
		FOR(slist_tpl<depot_t*>, const d, depot_t::get_depot_list()) {
			d->selected_filter = VEHICLE_FILTER_RELEVANT;
		}

		FOR(slist_tpl<fabrik_t*>, const factory, get_fab_list()) {
			slist_tpl<ware_besch_t const*>* const produced_goods = factory->get_produced_goods();
			FOR(slist_tpl<ware_besch_t const*>, const good, *produced_goods) {
				goods_in_game.insert_unique_ordered(good, sort_ware_by_name);
			}
			delete produced_goods;
		}
		goods_in_game.insert_at(0, warenbauer_t::passagiere);
		goods_in_game.insert_at(1, warenbauer_t::post);
	}

	return goods_in_game;
}
