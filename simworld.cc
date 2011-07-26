/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Hauptklasse fuer Simutrans, Datenstruktur die alles Zusammenhaelt
 * Hansjoerg Malthaner, 1997
 */

#include <algorithm>
#include <limits>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "path_explorer.h"
#ifdef _MSC_VER
#include <direct.h>
#undef min
#else
#include <unistd.h>
#endif

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
#include "tpl/ordered_vector_tpl.h"
#include "tpl/stringhashtable_tpl.h"

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
#include "dataobj/tabfile.h"
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
#include "bauer/hausbauer.h"

#include "besch/grund_besch.h"
#include "besch/sound_besch.h"
#include "besch/tunnel_besch.h"
#include "besch/bruecke_besch.h"
#include "besch/stadtauto_besch.h"

#include "player/simplay.h"
#include "player/ai_passenger.h"
#include "player/ai_goods.h"

#ifdef DEBUG_SIMRAND_CALLS
bool karte_t::print_randoms = true;
#endif


//#define DEMO
//#undef DEMO

// advance 201 ms per sync_step in fast forward mode
#define MAGIC_STEP (201)

// frame per second for fast forward
#define FF_PPS (10)


static bool is_dragging = false;
static uint32 last_clients = -1;
static uint8 last_active_player_nr = 0;
static std::string last_network_game;


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

	// calculate snowline with day precicion
	// use linear interpolation
	const long ticks_this_month = get_zeit_ms() & (karte_t::ticks_per_world_month-1);
	const long faktor = mfactor[letzter_monat] + (  ( (mfactor[(letzter_monat+1)%12]-mfactor[letzter_monat])*(ticks_this_month>>12) ) >> (karte_t::ticks_per_world_month_shift-12) );

	// just remember them
	const sint16 old_snowline = snowline;
	const sint16 old_season = season;

	// and calculate new values
	season=month_to_season[letzter_monat];   //  (2+letzter_monat/3)&3; // summer always zero
	int const winterline = settings.get_winter_snowline();
	int const summerline = settings.get_climate_borders()[arctic_climate] + 1;
	snowline = summerline - (sint16)(((summerline-winterline)*faktor)/100);
	snowline = (snowline*Z_TILE_STEP) + grundwasser;

	// changed => we update all tiles ...
	return (old_snowline!=snowline  ||  old_season!=season);
}



// read height data from bmp or ppm files
bool karte_t::get_height_data_from_file( const char *filename, sint8 grundwasser, sint8 *&hfield, sint16 &ww, sint16 &hh, bool update_only_values )
{
	FILE *file = fopen(filename, "rb");
	if(file) {
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
					h_table[i] = ((((R*2+G*3+B)/4 - 224) & 0xFFF0)/16)*Z_TILE_STEP;
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
						hfield[x+offset] = ((((R*2+G*3+B)/4 - 224) & 0xFFF0)/16)*Z_TILE_STEP;
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
					hfield[x+(y*w)] =  ((((R*2+G*3+B)/4 - 224) & 0xFFF0)/16)*Z_TILE_STEP;
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

sint32 karte_t::perlin_hoehe(settings_t const* const sets, koord k, koord const size, const sint32 map_size)
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
	return ((int)(perlin_noise_2D(k.x, k.y, sets->get_map_roughness(), map_size)*(double)sets->get_max_mountain_height())) / 16;
}

void karte_t::cleanup_karte( int xoff, int yoff )
{
	// we need a copy to smoothen the map to a realistic level
	const sint32 grid_size = (get_groesse_x()+1)*(get_groesse_y()+1);
	sint8 *grid_hgts_cpy = new sint8[grid_size];
	memcpy( grid_hgts_cpy, grid_hgts, grid_size );

	// the trick for smoothing is to raise each tile by one
	sint32 i,j;
	for(j=0; j<=get_groesse_y(); j++) {
		for(i=j>=yoff?0:xoff; i<=get_groesse_x(); i++) {
			raise_to(i,j, (grid_hgts_cpy[i+j*(get_groesse_x()+1)]*Z_TILE_STEP)+Z_TILE_STEP, false );
		}
	}
	delete [] grid_hgts_cpy;

	// but to leave the map unchanged, we lower the height again
	for(j=0; j<=get_groesse_y(); j++) {
		for(i=j>=yoff?0:xoff; i<=get_groesse_x(); i++) {
			grid_hgts[i+j*(get_groesse_x()+1)] -= Z_TILE_STEP;
		}
	}

	// now lower the corners and edge between new/old part to ground level
	for(i=0; i<get_groesse_x(); i++) {
		lower_to(i, 0, grundwasser,false);
		lower_to(i, get_groesse_y(), grundwasser,false);
		if (i <= xoff) {
			lower_to(i, yoff, grundwasser,false);
		}
	}
	for(i=0; i<=get_groesse_y(); i++) {
		lower_to(0, i, grundwasser,false);
		lower_to(get_groesse_x(), i, grundwasser,false);
		if (i < yoff) {
			lower_to(xoff, i, grundwasser,false);
		}
	}
	for(i=0; i<=get_groesse_x(); i++) {
		raise_to(i, 0, grundwasser,false);
		raise_to(i, get_groesse_y(), grundwasser,false);
		if (i <= xoff) {
			raise_to(i, yoff, grundwasser,false);
		}
	}
	for(i=0; i<=get_groesse_y(); i++) {
		raise_to(0, i, grundwasser,false);
		raise_to(get_groesse_x(), i, grundwasser,false);
		if (i < yoff) {
			raise_to(xoff, i, grundwasser,false);
		}
	}

	// recalculate slopes and water tiles
	for(  j=0;  j<get_groesse_y();  j++  ) {
		for(  i=(j>=yoff)?0:xoff;  i<get_groesse_x();  i++  ) {
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

	is_shutting_down = true;

	// rotate the map until it can be saved
	nosave_warning = nosave = false;
	for( int i=0;  i<4  &&  nosave;  i++  ) {
DBG_MESSAGE("karte_t::destroy()", "rotating");
		rotate90();
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

	// alle convois aufraeumen
	while (!convoi_array.empty()) {
		convoihandle_t cnv = convoi_array.back();
		cnv->destroy();
	}
	convoi_array.clear();
DBG_MESSAGE("karte_t::destroy()", "convois destroyed");

	// alle haltestellen aufraeumen
	haltestelle_t::destroy_all(this);
DBG_MESSAGE("karte_t::destroy()", "stops destroyed");

	// delete towns first (will also delete all their houses)
	// for the next game we need to remember the desired number ...
	sint32 const no_of_cities = settings.get_anzahl_staedte();
	while (!stadt.empty()) {
		rem_stadt(stadt.front());
	}
	settings.set_anzahl_staedte(no_of_cities);

DBG_MESSAGE("karte_t::destroy()", "towns destroyed");

	while(!sync_list.empty()) {
		sync_steppable *ss = sync_list.remove_first();
		delete ss;
	}
	// entfernt alle synchronen objekte aus der liste
	sync_list.clear();
DBG_MESSAGE("karte_t::destroy()", "sync list cleared");

// dinge aufraeumen
	cached_groesse_gitter_x = cached_groesse_gitter_y = 1;
	cached_groesse_karte_x = cached_groesse_karte_y = 0;
	if(plan) {
		delete [] plan;
		plan = NULL;
	}
	DBG_MESSAGE("karte_t::destroy()", "planquadrat destroyed");

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

	// alle fabriken aufräumen
	// "all factories clear up" (Babelfish)
	ITERATE(fab_list, i)
	{
		delete fab_list[i];
	}
	fab_list.clear();
DBG_MESSAGE("karte_t::destroy()", "factories destroyed");

	// hier nur entfernen, aber nicht loeschen
	ausflugsziele.clear();
DBG_MESSAGE("karte_t::destroy()", "attraction list destroyed");

	delete scenario;
	scenario = NULL;

	senke_t::neue_karte();
	pumpe_t::neue_karte();

	bool empty_depot_list = depot_t::get_depot_list().empty();
	assert( empty_depot_list );

DBG_MESSAGE("karte_t::destroy()", "world destroyed");
	printf("World destroyed.\n");
	
	is_shutting_down = false;

	// Added by : Knightly
	path_explorer_t::finalise();
}



void karte_t::add_convoi(convoihandle_t &cnv)
{
	assert(cnv.is_bound());
	convoi_array.append_unique(cnv);
}



void karte_t::rem_convoi(convoihandle_t& cnv)
{
	convoi_array.remove(cnv);
}

/**
 * Zugriff auf das Städte Array.
 * Access to the cities array.
 * @author Hj. Malthaner
 */
const stadt_t *karte_t::get_random_stadt() const
{
	return pick_any_weighted(stadt);
}

void karte_t::add_stadt(stadt_t *s)
{
	settings.set_anzahl_staedte(settings.get_anzahl_staedte() + 1);
	stadt.append(s, s->get_einwohner(), 64);

	// Knightly : add links between this city and other cities as well as attractions
	for(  uint32 c=0;  c<stadt.get_count();  ++c  ) {
		stadt[c]->add_target_city(s);
	}
	s->recalc_target_cities();
	s->recalc_target_attractions();
}

/**
 * Removes town from map, houses will be left overs
 * @author prissi
 */
bool karte_t::rem_stadt(stadt_t *s)
{
	if(s == NULL  ||  stadt.empty()) {
		// no town there to delete ...
		return false;
	}

	// reduce number of towns
	if(s->get_name()) {
		DBG_MESSAGE("karte_t::rem_stadt()", s->get_name() );
	}
	stadt.remove(s);
	DBG_DEBUG4("karte_t::rem_stadt()", "reduce city to %i", settings.get_anzahl_staedte() - 1);
	settings.set_anzahl_staedte(settings.get_anzahl_staedte() - 1);

	// Knightly : remove links between this city and other cities
	for(  uint32 c=0;  c<stadt.get_count();  ++c  ) {
		stadt[c]->remove_target_city(s);
	}

	// remove all links from factories
	DBG_DEBUG4("karte_t::rem_stadt()", "fab_list %i", fab_list.get_count() );
	//slist_iterator_tpl<fabrik_t *> iter(fab_list);
	//while(iter.next()) {
	ITERATE(fab_list,i)
	{
		//(iter.get_current())->remove_target_city(s);
		fab_list[i]->remove_target_city(s);

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

	uint32 const x = get_groesse_x();
	uint32 const y = get_groesse_y();
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

	scenario = new scenario_t(this);

	nosave_warning = nosave = false;
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

	koord last_koord(0,0);

	// trunk of 16 will ensure that rivers are long enough apart ...
	for(  sint16 y = 8;  y < cached_groesse_karte_y;  y+=16  ) {
		for(  sint16 x = 8;  x < cached_groesse_karte_x;  x+=16  ) {
			koord k(x,y);
			grund_t *gr = lookup_kartenboden(k);
			const sint8 h = gr->get_hoehe()-get_grundwasser();
			if(  gr->ist_wasser()  ) {
				// may be good to start a river here
				water_tiles.append(k);
			}
			else {
				mountain_tiles.append( k, h * h, 256 );
			}
		}
	}
	if (water_tiles.empty()) {
		dbg->message("karte_t::create_rivers()","There aren't any water tiles!\n");
		return;
	}

	// now make rivers
	uint8 retrys = 0;
	while (number > 0 && !mountain_tiles.empty() && retrys++ < 100) {
		koord const start = pick_any_weighted(mountain_tiles);
		koord const end   = pick_any(water_tiles);
		sint16 dist = koord_distance(start,end);
		if (settings.get_min_river_length() < dist && dist < settings.get_max_river_length()) {
			// should be at least of decent length
			wegbauer_t riverbuilder(this, spieler[1]);
			riverbuilder.route_fuer(wegbauer_t::river, river_besch);
			riverbuilder.set_maximum( dist*50 );
			riverbuilder.calc_route( lookup_kartenboden(end)->get_pos(), lookup_kartenboden(start)->get_pos() );
			if (riverbuilder.get_count() >= (uint32)settings.get_min_river_length()) {
				// do not built too short rivers
				riverbuilder.baue();
				number --;
				retrys = 0;
			}
			mountain_tiles.remove( start );
		}
	}
	// we gave up => tell te user
	if(  number>0  ) {
		dbg->warning( "karte_t::create_rivers()","Too many rivers requested! (%i not constructed)", number );
	}
}



void karte_t::distribute_groundobjs_cities( settings_t const * const sets, sint16 old_x, sint16 old_y)
{
	DBG_DEBUG("karte_t::distribute_groundobjs_cities()","distributing groundobjs");

	double new_anzahl_staedte = sets->get_anzahl_staedte();
	unsigned number_of_big_cities = umgebung_t::number_of_big_cities;

	if (umgebung_t::river_types > 0 && settings.get_river_number() > 0) {
		create_rivers(settings.get_river_number());
	}

printf("Creating cities ...\n");
DBG_DEBUG("karte_t::distribute_groundobjs_cities()","prepare cities sizes");
	vector_tpl<sint32> *city_population = new vector_tpl<sint32>(new_anzahl_staedte);
	//double rank1_population = (2.0 * settings.get_mittlere_einwohnerzahl() * new_anzahl_staedte)/(1.0+new_anzahl_staedte);
	double rank1_population = sets->get_mittlere_einwohnerzahl();

	double adjusted_counter = 1;
	double population;
	double adjusted_city_size = 2.0 * new_anzahl_staedte;
	const int division_1 = new_anzahl_staedte / 3.25;
	const int division_2 = new_anzahl_staedte / 2.5;
	for(unsigned i = 0; i < new_anzahl_staedte; i++) 
	{
		do {	
			if (i < number_of_big_cities) 
			{
				population = (3.25 * rank1_population * new_anzahl_staedte) / (1.0 + new_anzahl_staedte);
			}
			else
			{
				adjusted_counter = i - number_of_big_cities + 2;
				population = rank1_population * ((new_anzahl_staedte - adjusted_counter) / adjusted_city_size);
			}
			/* now add some gaussian noise */
			double sigma;
			if(i < number_of_big_cities)
			{
				population /= 2.0;
				sigma = population;
			}
			else
			{
				population /= 4.0;
				sigma = population * 3.0;
			}
			population = simrand_gauss(population, sigma);
			if(i == division_1)
			{
				adjusted_city_size = 2.7 * new_anzahl_staedte;
			}
			else if(i == division_2)
			{
				adjusted_city_size = 3.8 * new_anzahl_staedte;
			}
		} while ((population < 0) || (population > std::numeric_limits<uint32>::max() ));
		city_population->append( uint32(population));
	}

#ifdef DEBUG
	for (unsigned i =0; i< new_anzahl_staedte; i++) 
	{
		DBG_DEBUG("karte_t::distribute_groundobjs_cities()", "City rank %d -- %d", i, (*city_population)[i]);
	}	

DBG_DEBUG("karte_t::distribute_groundobjs_cities()","prepare cities");
#endif 
	display_set_progress_text(translator::translate("Placing cities ..."));
	vector_tpl<koord> *pos = stadt_t::random_place(this, city_population, old_x, old_y);

	if(  !pos->empty()  ) {
		const sint32 old_anzahl_staedte = stadt.get_count();
		new_anzahl_staedte = pos->get_count();

		// prissi if we could not generate enough positions ...
		settings.set_anzahl_staedte(old_anzahl_staedte);
		int old_progress = 16;
		int const max_display_progress = 16 + 2 * (old_anzahl_staedte + new_anzahl_staedte) + 2 * new_anzahl_staedte + (old_x == 0 ? settings.get_land_industry_chains() : 0);

		// Ansicht auf erste Stadt zentrieren
		if (old_x+old_y == 0)
			change_world_position( koord3d((*pos)[0], min_hgt((*pos)[0])) );

		// Loop only new cities:
#ifdef DEBUG
		uint32 tbegin = dr_time();
#endif
		for(  unsigned i=0;  i<new_anzahl_staedte;  i++  ) {
			stadt_t* s = new stadt_t(spieler[1], (*pos)[i], (*city_population)[i]);
			DBG_DEBUG("karte_t::distribute_groundobjs_cities()","Erzeuge stadt %i with %ld inhabitants",i,(s->get_city_history_month())[HIST_CITICENS] );
			add_stadt(s);
			if(is_display_init()) {
				old_progress ++;
				display_progress(old_progress, max_display_progress);
			}
			else {
				printf("*");fflush(NULL);
			}
		}

		delete pos;
		delete city_population;
DBG_DEBUG("karte_t::distribute_groundobjs_cities()","took %lu ms for all towns", dr_time()-tbegin );
		display_set_progress_text(translator::translate("Growing cities ..."));
		for(  uint32 i=old_anzahl_staedte;  i<stadt.get_count();  i++  ) {
			// Hajo: do final init after world was loaded/created
			stadt[i]->laden_abschliessen();
			// the growth is slow, so update here the progress bar
			if(is_display_init()) {
				old_progress ++;
				display_progress(old_progress, max_display_progress);
			}
			else {
				printf("*");fflush(NULL);
			}
		}
		finance_history_year[0][WORLD_TOWNS] = finance_history_month[0][WORLD_TOWNS] = stadt.get_count();
		finance_history_year[0][WORLD_CITICENS] = finance_history_month[0][WORLD_CITICENS] = last_month_bev;

		// Hajo: connect some cities with roads
		display_set_progress_text(translator::translate("Connecting cities ..."));
		weg_besch_t const* besch = settings.get_intercity_road_type(get_timeline_year_month());
		if(besch == NULL) 
		{
			// Hajo: try some default (might happen with timeline ... )
			besch = wegbauer_t::weg_search(road_wt,80,5,get_timeline_year_month(),weg_t::type_flat);
		}

		wegbauer_t bauigel (this, spieler[1] );
		bauigel.route_fuer(wegbauer_t::strasse | wegbauer_t::terraform_flag, besch, tunnelbauer_t::find_tunnel(road_wt,15,get_timeline_year_month()), brueckenbauer_t::find_bridge(road_wt,15,get_timeline_year_month()) );
		bauigel.set_keep_existing_ways(true);
		bauigel.set_maximum(umgebung_t::intercity_road_length);

		// **** intercity road construction
		// progress bar data
		int old_progress_count = 16+2*new_anzahl_staedte;
		int count = 0;
		sint32 const n_cities  = settings.get_anzahl_staedte();
		int    const max_count = n_cities * (n_cities - 1) / 2 - old_anzahl_staedte * (old_anzahl_staedte - 1) / 2;
		// something to do??
		if(  max_count > 0  ) {
			// print("Building intercity roads ...\n");
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
							if (i==size.x+1) inc = koord(0,1);
							else if (i==size.x+size.y+2) inc = koord(-1,0);
							else if (i==2*size.x+size.y+3) inc = koord(0,-1);
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
				if(  is_display_init()  &&  count<=max_count  ) {
					int const progress_count = 16 + 2 * new_anzahl_staedte + count * settings.get_anzahl_staedte() * 2 / max_count;
					if(  old_progress_count != progress_count  ) {
						display_progress(progress_count, max_display_progress );
						old_progress_count = progress_count;
					}
				}
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
						for (int i = old_anzahl_staedte; i < settings.get_anzahl_staedte(); ++i) {
							if(  city_flag[i] ==0 ) {
								city_flag[i] = conn_comp;
								ready=false;
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
					const bool connected = phase==1 && verbindung.calc_route(this,k[conn.x],k[conn.y],  test_driver, 0, 0);
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

						if (phase==0) {
							// do not try to connect to this connected component again
							for (int i = 0; i < settings.get_anzahl_staedte(); ++i) {
								if (  city_flag[i] == conn_comp  && city_dist.at(i, conn.y)<umgebung_t::intercity_road_length) {
									city_dist.at(i, conn.y) =  umgebung_t::intercity_road_length+1;
									city_dist.at(conn.y, i) =  umgebung_t::intercity_road_length+1;
									count++;
								}
							}
						}
					}
				}
				//printf("IC-Road Progress : %d/%d\n", count, max_count);
				// progress bar stuff
				if(  is_display_init()  &&  count<=max_count  ) {
					int const progress_count = 16 + 2 * new_anzahl_staedte + count * settings.get_anzahl_staedte() * 2 / max_count;
					if(  old_progress_count != progress_count  ) {
						display_progress(progress_count, max_display_progress );
						old_progress_count = progress_count;
					}
				}
				// next phase?
				if (ready) {
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
		settings.set_anzahl_staedte(stadt.get_count()); // new number of towns (if we did not find enough positions)
	}

DBG_DEBUG("karte_t::distribute_groundobjs_cities()","distributing groundobjs");
	if(  umgebung_t::ground_object_probability > 0  ) {
		// add eyecandy like rocky, moles, flowers, ...
		koord k;
		sint32 queried = simrand(umgebung_t::ground_object_probability*2, "karte_t::distribute_groundobjs_cities()");
		for(  k.y=0;  k.y<get_groesse_y();  k.y++  ) {
			for(  k.x=(k.y<old_y)?old_x:0;  k.x<get_groesse_x();  k.x++  ) {
				grund_t *gr = lookup_kartenboden(k);
				if(  gr->get_typ()==grund_t::boden  &&  !gr->hat_wege()  ) {
					queried --;
					if(  queried<0  ) {
						const groundobj_besch_t *besch = groundobj_t::random_groundobj_for_climate( get_climate(gr->get_hoehe()), gr->get_grund_hang() );
						if(besch) {
							queried = simrand(umgebung_t::ground_object_probability*2, "karte_t::distribute_groundobjs_cities()");
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
		sint32 queried = simrand(umgebung_t::moving_object_probability*2, "karte_t::distribute_groundobjs_cities()");
		// no need to test the borders, since they are mostly slopes anyway
		for(k.y=1; k.y<get_groesse_y()-1; k.y++) {
			for(k.x=(k.y<old_y)?old_x:1; k.x<get_groesse_x()-1; k.x++) {
				grund_t *gr = lookup_kartenboden(k);
				if(gr->get_top()==0  &&  (gr->get_typ()==grund_t::boden  ||  gr->ist_wasser()) &&  gr->get_grund_hang()==hang_t::flach) {
					queried --;
					if(  queried<0  ) {
						const groundobj_besch_t *besch = movingobj_t::random_movingobj_for_climate( get_climate(gr->get_hoehe()) );
						if(besch  &&  (besch->get_waytype()!=water_wt  ||  gr->hat_weg(water_wt)  ||  gr->get_hoehe()<=get_grundwasser() ) ) {
							if(besch->get_speed()!=0) {
								queried = simrand(umgebung_t::moving_object_probability*2, "karte_t::distribute_groundobjs_cities()");
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

		// Added by : Knightly
		path_explorer_t::initialise(this);
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

	letzter_monat = 0;
	letztes_jahr = settings.get_starting_year();
	current_month = letzter_monat + (letztes_jahr*12);
	set_ticks_per_world_month_shift(settings.get_bits_per_month());
	next_month_ticks =  karte_t::ticks_per_world_month;
	season=(2+letzter_monat/3)&3; // summer always zero
	is_dragging = false;
	steps = 0;
	recalc_average_speed();	// resets timeline

	grundwasser = sets->get_grundwasser();      //29-Nov-01     Markus Weber    Changed
	grund_besch_t::calc_water_level( this, height_to_climate );
	snowline = sets->get_winter_snowline()*Z_TILE_STEP + grundwasser;

	if(sets->get_beginner_mode()) {
		warenbauer_t::set_multiplier(settings.get_beginner_price_factor());
		sets->set_just_in_time( 0 );
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

	cached_groesse_gitter_x = 0;
	cached_groesse_gitter_y = 0;

DBG_DEBUG("karte_t::init()","init_felder");
	init_felder();

	display_set_progress_text(translator::translate("Init map ..."));
	enlarge_map(&settings, h_field);

DBG_DEBUG("karte_t::init()","distributing trees");
	display_set_progress_text(translator::translate("Placing trees ..."));
	if (!settings.get_no_trees()) {
		baum_t::distribute_trees(this,3);
	}

DBG_DEBUG("karte_t::init()","built timeline");
	stadtauto_t::built_timeline_liste(this);

	nosave_warning = nosave = false;

	fabrikbauer_t::neue_karte(this);
	display_set_progress_text(translator::translate("Placing industries ..."));
	// new system ...
	int const max_display_progress = 16 + settings.get_anzahl_staedte() * 4 + settings.get_land_industry_chains();
	int chains=0;
	for (sint32 i = 0; i < settings.get_land_industry_chains(); ++i) {
		if (fabrikbauer_t::increase_industry_density( this, false )==0) {
			// building industry chain should fail max 10 times
			if (i-chains > 10) {
				break;
			}
		}
		else {
			chains++;
		}
		int const progress_count = 16 + settings.get_anzahl_staedte() * 4 + i;
		display_progress(progress_count, max_display_progress );
	}
	settings.set_land_industry_chains(chains);
	finance_history_year[0][WORLD_FACTORIES] = finance_history_month[0][WORLD_FACTORIES] = fab_list.get_count();

	// tourist attractions
	display_set_progress_text(translator::translate("Placing attractions ..."));
	fabrikbauer_t::verteile_tourist(this, settings.get_tourist_attractions());

	display_set_progress_text(translator::translate("Finalising ..."));
	printf("Preparing startup ...\n");
	if(zeiger == 0) {
		zeiger = new zeiger_t(this, koord3d::invalid, NULL );
	}

	// finishes the line preparation and sets id 0 to invalid ...
	spieler[0]->simlinemgmt.laden_abschliessen();

	set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE], get_active_player() );

	recalc_average_speed();

	// @author: jamespetts
	calc_generic_road_speed_city();
	calc_generic_road_speed_intercity();
	calc_max_road_check_depth();

#ifndef DEMO
	for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
		if(  spieler[i]  ) {
			spieler[i]->set_active(settings.automaten[i]);
		}
	}
#endif

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

	// Added by : Knightly
	if (settings.get_default_path_option() == 2)
	{
		path_explorer_t::full_instant_refresh();
	}

	// Set the actual industry density and industry density proportion
	actual_industry_density = 0;
	uint32 weight;
	ITERATE(fab_list, i)
	{
		const fabrik_besch_t* factory_type = fab_list[i]->get_besch();
		if(!factory_type->is_electricity_producer())
		{
			// Power stations are excluded from the target weight:
			// a different system is used for them.
			weight = factory_type->get_gewichtung();
			actual_industry_density += (100 / weight);
		}
	}
	// The population is not counted at this point, so cannot set this here.
	industry_density_proportion = 0;
}


void karte_t::enlarge_map(settings_t const* sets, sint8 const* const h_field)
{
	sint16 new_groesse_x = sets->get_groesse_x();
	sint16 new_groesse_y = sets->get_groesse_y();
	const sint32 map_size = max (new_groesse_x, new_groesse_y);
	planquadrat_t *new_plan = new planquadrat_t[new_groesse_x*new_groesse_y];
	sint8 *new_grid_hgts = new sint8[(new_groesse_x+1)*(new_groesse_y+1)];

	memset(new_grid_hgts, grundwasser, sizeof(sint8)*(new_groesse_x+1)*(new_groesse_y+1));

	sint16 old_x = cached_groesse_gitter_x;
	sint16 old_y = cached_groesse_gitter_y;

	settings.set_groesse_x(new_groesse_x);
	settings.set_groesse_y(new_groesse_y);
	cached_groesse_gitter_x = new_groesse_x;
	cached_groesse_gitter_y = new_groesse_y;
	cached_groesse_max = max(cached_groesse_gitter_x,cached_groesse_gitter_y);
	cached_groesse_karte_x = cached_groesse_gitter_x-1;
	cached_groesse_karte_y = cached_groesse_gitter_y-1;

	// Knightly : initialise the weighted list of distances
	stadt_t::init_distances( shortest_distance( koord(1, 1), koord( get_groesse_x(), get_groesse_y() ) ) );

	intr_disable();

	bool reliefkarte = reliefkarte_t::is_visible;

	int max_display_progress;

	// If this is not called by karte_t::init
	if ( old_x != 0 ) {
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
				new_plan[nnr] = plan[nr];
				plan[nr] = planquadrat_t();
			}
		}
		for (sint16 ix = 0; ix<=old_x; ix++) {
			for (sint16 iy = 0; iy<=old_y; iy++) {
				uint32 nr = ix+(iy*(old_x+1));
				uint32 nnr = ix+(iy*(new_groesse_x+1));
				new_grid_hgts[nnr] = grid_hgts[nr];
			}
		}

		display_set_progress_text(translator::translate("enlarge map"));
		max_display_progress = 16 + sets->get_anzahl_staedte()*2 + stadt.get_count()*4;
	}
	else {
		max_display_progress = 16 + sets->get_anzahl_staedte() * 4 + settings.get_land_industry_chains();
	}

	delete [] plan;
	plan = new_plan;
	delete [] grid_hgts;
	grid_hgts = new_grid_hgts;

	display_progress(0,max_display_progress);
	setsimrand(0xFFFFFFFF, settings.get_karte_nummer());
	clear_random_mode( 0xFFFF );
	set_random_mode( MAP_CREATE_RANDOM );

	if (old_x == 0 && settings.heightfield.size() > 0) {
		// init from file
		int const display_total = 16 + settings.get_anzahl_staedte()*4 + settings.get_land_industry_chains();

		for(int y=0; y<cached_groesse_gitter_y; y++) {
			for(int x=0; x<cached_groesse_gitter_x; x++) {
				grid_hgts[x + y*(cached_groesse_gitter_x+1)] = ((h_field[x+(y*(sint32)cached_groesse_gitter_x)]+1)/Z_TILE_STEP);
			}
			grid_hgts[cached_groesse_gitter_x + y*(cached_groesse_gitter_x+1)] = grid_hgts[cached_groesse_gitter_x-1 + y*(cached_groesse_gitter_x+1)];
		}
		// lower border
		memcpy( grid_hgts+(cached_groesse_gitter_x+1)*(sint32)cached_groesse_gitter_y, grid_hgts+(cached_groesse_gitter_x+1)*(sint32)(cached_groesse_gitter_y-1), cached_groesse_gitter_x+1 );
		display_progress(16, display_total);
	}
	else {
		if(  sets->get_rotation()==0  &&  sets->get_origin_x()==0  &&  sets->get_origin_y()==0) {
			// otherwise neagtive offsets may occur, so we cache only non-rotated maps
			init_perlin_map(new_groesse_x,new_groesse_y);
		}
		int next_progress, old_progress = 0;
		// loop only new tiles:
		for(  sint16 x = 0;  x<=new_groesse_x;  x++  ) {
			for(  sint16 y = (x>=old_x)?0:old_y;  y<=new_groesse_y;  y++  ) {
				koord pos(x,y);
				sint16 const h = perlin_hoehe(&settings, pos, koord(old_x, old_y), map_size);
				set_grid_hgt( pos, h*Z_TILE_STEP);
			}
			next_progress = (x*16)/new_groesse_x;
			if ( next_progress > old_progress ){
				old_progress = next_progress;
				display_progress(old_progress, max_display_progress);
			}
		}
		exit_perlin_map();
	}

	// create grounds on new part
	for (sint16 ix = 0; ix<new_groesse_x; ix++) {
		for (sint16 iy = (ix>=old_x)?0:old_y; iy<new_groesse_y; iy++) {
			koord k = koord(ix,iy);
			access(k)->kartenboden_setzen(new boden_t(this, koord3d(ix,iy,min_hgt(k)),0));
		}
	}

	// set borders to water level, smooth the new part, reassign slopes on new part
	cleanup_karte( old_x, old_y );

	// smoothing the seam (if possible)
	for (sint16 x=1; x<old_x; x++) {
		koord k(x,old_y);
		const sint16 height = perlin_hoehe( &settings, k, koord(old_x,old_y), map_size )*Z_TILE_STEP;
		// need to raise/lower more
		for(  sint16 dy=-abs(grundwasser-height);  dy<abs(grundwasser-height);  dy++  ) {
			koord pos(x,old_y+dy);
			const sint16 height = perlin_hoehe( &settings, pos, koord(old_x,old_y), map_size  )*Z_TILE_STEP;
			while(lookup_hgt(pos)<height  &&  raise(pos)) ;
			while(lookup_hgt(pos)>height  &&  lower(pos)) ;
		}
	}
	for (sint16 y=1; y<old_y; y++) {
		koord k(old_x,y);
		sint16 const height = perlin_hoehe(&settings, k, koord(old_x, old_y), map_size) * Z_TILE_STEP;
		// need to raise/lower more
		for(  sint16 dx=-abs(grundwasser-height);  dx<abs(grundwasser-height);  dx++  ) {
			koord pos(old_x+dx,y);
			sint16 const height = perlin_hoehe(&settings, pos, koord(old_x, old_y), map_size) * Z_TILE_STEP;
			while(lookup_hgt(pos)<height  &&  raise(pos)) ;
			while(lookup_hgt(pos)>height  &&  lower(pos)) ;
		}
	}

	// now recalc the images of the old map near the seam ...
	for (sint16 x=0; x<old_x-20; x++) {
		for (sint16 y=max(old_y-20,0); y<old_y; y++) {
			plan[x+y*cached_groesse_gitter_x].get_kartenboden()->calc_bild();
		}
	}
	for (sint16 x=max(old_x-20,0); x<old_x; x++) {
		for (sint16 y=0; y<old_y; y++) {
			plan[x+y*cached_groesse_gitter_x].get_kartenboden()->calc_bild();
		}
	}


	// eventuall update origin
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

	distribute_groundobjs_cities(sets, old_x, old_y);

	// hausbauer_t::neue_karte(); <- this would reinit monuments! do not do this!
	fabrikbauer_t::neue_karte( this );

	// Modified by : Knightly
	if ( settings.get_default_path_option() == 2 )
	{
		path_explorer_t::refresh_all_categories(true);
	}
	else
	{
		set_schedule_counter();
	}

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
}


karte_t::karte_t() :
	settings(umgebung_t::default_einstellungen),
	convoi_array(0),
	ausflugsziele(16),
	stadt(0),
	marker(0,0)
{
	is_shutting_down = false;

	// length of day and other time stuff
	ticks_per_world_month_shift = 20;
	ticks_per_world_month = (1LL << ticks_per_world_month_shift);
	last_step_ticks = 0;
	server_next_announce_month = 0xFFFFFFFFu;
	last_interaction = dr_time();
	step_mode = PAUSE_FLAG;
	time_multiplier = 16;
	next_step_time = last_step_time = 0;
	fix_ratio_frame_time = 200;

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
	recheck_road_connexions = true;
	actual_industry_density = industry_density_proportion = 0;

	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		spieler[i] = NULL;
		MEMZERO(player_password_hash[i]);
	}

	// no distance to show at first ...
	show_distance = koord3d::invalid;
	scenario = NULL;

	msg = new message_t(this);

	base_pathing_counter = 0;

	citycar_speed_average = 50;

	city_road = NULL;

	// Added by : Knightly
	path_explorer_t::initialise(this);

	// @author: jamespetts
	set_scale();

	next_private_car_update_month = 1;
}

#ifdef DEBUG_SIMRAND_CALLS
	fixed_list_tpl<const char*, 256> karte_t::random_callers;
#endif


karte_t::~karte_t()
{
	is_sound = false;

	destroy();

	// not deleting the werkzeuge of this map ...
	delete msg;
}

void karte_t::set_scale()
{
	const uint16 scale_factor = get_settings().get_meters_per_tile();
	
	// Vehicles
	for(int i = road_wt; i <= air_wt; i++) 
	{
		slist_tpl<vehikel_besch_t*>* vehicle_list = vehikelbauer_t::get_modifiable_info((waytype_t)i);
		if(vehicle_list != NULL)
		{
			slist_iterator_tpl<vehikel_besch_t*> vehinfo(vehicle_list);
			while (vehinfo.next()) 
			{
				vehikel_besch_t* info = vehinfo.get_current();
				info->set_scale(scale_factor);
			}
		}
	}

	// Ways

	stringhashtable_tpl <weg_besch_t *> * ways = wegbauer_t::get_all_ways();

	if(ways != NULL)
	{
		stringhashtable_iterator_tpl <weg_besch_t *> iter(ways);
		while(iter.next())
		{
			iter.access_current_value()->set_scale(scale_factor);
		}
	}

	// Tunnels

	stringhashtable_tpl <tunnel_besch_t *> * tunnels = tunnelbauer_t::get_all_tunnels();

	
	if(tunnels != NULL)
	{
		stringhashtable_iterator_tpl <tunnel_besch_t *> iter(tunnels);
		while(iter.next())
		{
			iter.access_current_value()->set_scale(scale_factor);
		}
	}

	// Bridges

	stringhashtable_tpl <bruecke_besch_t *> * bridges = brueckenbauer_t::get_all_bridges();

	if(bridges != NULL)
	{
		stringhashtable_iterator_tpl <bruecke_besch_t *> iter(bridges);
		while(iter.next())
		{
			iter.access_current_value()->set_scale(scale_factor);
		}
	}


	// Way objects

	stringhashtable_iterator_tpl<way_obj_besch_t *> wayobj_iter(wayobj_t::get_all_wayobjects());
	while(wayobj_iter.next()) 
	{
		wayobj_iter.access_current_value()->set_scale(scale_factor);
	}

	// Stations

	ITERATE(hausbauer_t::modifiable_station_buildings, n)
	{
		hausbauer_t::modifiable_station_buildings[n]->set_scale(scale_factor); 
	}

	// Goods

	const uint16 goods_count = warenbauer_t::get_waren_anzahl();
	for(uint16 i = 0; i < goods_count; i ++)
	{
		warenbauer_t::get_modifiable_info(i)->set_scale(scale_factor);
	}
}


bool karte_t::can_lower_plan_to(sint16 x, sint16 y, sint8 h) const
{
	const planquadrat_t *plan = lookup(koord(x,y));

	if(  plan==NULL  ) {
		return false;
	}

	const sint8 hmax = plan->get_kartenboden()->get_hoehe();
	if(  hmax==h  &&  (plan->get_kartenboden()->get_grund_hang()==0 ||  is_plan_height_changeable(x, y))) {
		return true;
	}

	if(  !is_plan_height_changeable(x, y)  ) {
		return false;
	}

	// tunnel slope below?
	grund_t *gr = plan->get_boden_in_hoehe(h-Z_TILE_STEP);
	if (gr && gr->get_grund_hang()!=hang_t::flach) {
		return false;
	}
	// tunnel below?
	while(h < hmax) {
		if(plan->get_boden_in_hoehe(h)) {
			return false;
		}
		h += Z_TILE_STEP;
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
		h -= Z_TILE_STEP;
	}
	return true;
}


bool karte_t::is_plan_height_changeable(sint16 x, sint16 y) const
{
	const planquadrat_t *plan = lookup(koord(x,y));
	bool ok = true;

	if(plan != NULL) {
		grund_t *gr = plan->get_kartenboden();

		ok = (gr->ist_natur() || gr->ist_wasser())  &&  !gr->hat_wege();

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
	if(ist_in_kartengrenzen(x,y)) {
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
		if (x<0) ok = hne <= grundwasser && hse <= grundwasser;
		if (y<0) ok = hsw <= grundwasser && hse <= grundwasser;
		if (x>=cached_groesse_karte_x) ok = hsw <= grundwasser && hnw <= grundwasser;
		if (y>=cached_groesse_karte_y) ok = hnw <= grundwasser && hne <= grundwasser;
	}
	return ok;
}


// nw-ecke corner4 anheben
bool karte_t::can_raise(sint16 x, sint16 y) const
{
	raise_frame_counter = 0;
	if(ist_in_kartengrenzen(x, y)) {
		grund_t *gr = lookup_kartenboden(koord(x,y));
		const sint8 hnew = gr->get_hoehe() + corner4(gr->get_grund_hang());

		return can_raise_to(x, y, false, hnew, hnew, hnew, hnew+1, 15/*all corners*/ );
	}
	else {
		return true;
	}
}


// raise plan
// new heights for each corner given
// clear tile, reset water/land type, calc reliefkarte pixel
int karte_t::raise_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw)
{
	int n=0;
	if(ist_in_kartengrenzen(x,y)) {
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
		set_grid_hgt(koord(x,y),hn_nw);

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
		if ((x+1)<cached_groesse_karte_x) {
			lookup_kartenboden(koord(x+1,y))->calc_bild();
		}
		if ((y+1)<cached_groesse_karte_y) {
			lookup_kartenboden(koord(x,y+1))->calc_bild();
		}
	}
	return n;
}

// raise height in the hgt-array
int karte_t::raise_to(sint16 x, sint16 y, sint8 h, bool set_slopes /*always false*/)
{
	int n = 0;
	if(ist_in_gittergrenzen(x,y)) {
		const sint32 offset = x + y*(cached_groesse_gitter_x+1);

		if(  grid_hgts[offset]*Z_TILE_STEP < h  ) {
			grid_hgts[offset] = h/Z_TILE_STEP;
			n = 1;

#ifndef DOUBLE_GROUNDS
			n += raise_to(x-1, y-1, h-Z_TILE_STEP,set_slopes);
			n += raise_to(x  , y-1, h-Z_TILE_STEP,set_slopes);
			n += raise_to(x+1, y-1, h-Z_TILE_STEP,set_slopes);
			n += raise_to(x-1, y  , h-Z_TILE_STEP,set_slopes);

			n += raise_to(x, y, h,set_slopes);

			n += raise_to(x+1, y  , h-Z_TILE_STEP,set_slopes);
			n += raise_to(x-1, y+1, h-Z_TILE_STEP,set_slopes);
			n += raise_to(x  , y+1, h-Z_TILE_STEP,set_slopes);
			n += raise_to(x+1, y+1, h-Z_TILE_STEP,set_slopes);
#else
			n += raise_to(x-1, y-1, h-Z_TILE_STEP*2,set_slopes);
			n += raise_to(x  , y-1, h-Z_TILE_STEP*2,set_slopes);
			n += raise_to(x+1, y-1, h-Z_TILE_STEP*2,set_slopes);
			n += raise_to(x-1, y  , h-Z_TILE_STEP*2,set_slopes);

			n += raise_to(x, y, h,set_slopes);

			n += raise_to(x+1, y  , h-Z_TILE_STEP*2,set_slopes);
			n += raise_to(x-1, y+1, h-Z_TILE_STEP*2,set_slopes);
			n += raise_to(x  , y+1, h-Z_TILE_STEP*2,set_slopes);
			n += raise_to(x+1, y+1, h-Z_TILE_STEP*2,set_slopes);
#endif
		}
	}

	return n;
}


int karte_t::raise(koord pos)
{
	bool ok = can_raise(pos.x, pos.y);
	int n = 0;
	if(ok && ist_in_kartengrenzen(pos)) {
		grund_t *gr = lookup_kartenboden(pos);
		const sint8 hnew = gr->get_hoehe() + corner4(gr->get_grund_hang());

		n = raise_to(pos.x, pos.y, hnew, hnew, hnew, hnew+1);
	}
	return (n+3)>>2;
}


// lower plan
// new heights for each corner given
// only test corners in ctest to avoid infinite loops
bool karte_t::can_lower_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw, uint8 ctest) const
{
	bool ok = false;
	if(ist_in_kartengrenzen(x,y)) {
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
		if (x<0) ok = hne >= grundwasser && hse >= grundwasser;
		if (y<0) ok = hsw >= grundwasser && hse >= grundwasser;
		if (x>=cached_groesse_karte_x) ok = hsw >= grundwasser && hnw >= grundwasser;
		if (y>=cached_groesse_karte_y) ok = hnw >= grundwasser && hne >= grundwasser;
	}
	return ok;
}


// nw-ecke corner4 absenken
bool karte_t::can_lower(sint16 x, sint16 y) const
{
	if(ist_in_kartengrenzen(x, y)) {
		grund_t *gr = lookup_kartenboden(koord(x,y));
		const sint8 hnew = gr->get_hoehe() + corner4(gr->get_grund_hang());

		return can_lower_to(x, y, hnew, hnew, hnew, hnew-1, 15/*all corners*/ );
	} else {
		return true;
	}
}


// lower plan
// new heights for each corner given
// cleartile=true: clear tile, reset water/land type, calc reliefkarte pixel
int karte_t::lower_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw)
{
	int n=0;
	if(ist_in_kartengrenzen(x,y)) {
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
		set_grid_hgt(koord(x,y),hn_nw);

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
		if(  (x+1)<cached_groesse_karte_x  ) {
			lookup_kartenboden(koord(x+1,y))->calc_bild();
		}
		if(  (y+1)<cached_groesse_karte_y  ) {
			lookup_kartenboden(koord(x,y+1))->calc_bild();
		}
	}
	return n;
}


int karte_t::lower_to(sint16 x, sint16 y, sint8 h, bool set_slopes /*always false*/)
{
	int n = 0;
	if(ist_in_gittergrenzen(x,y)) {
		const sint32 offset = x + y*(cached_groesse_gitter_x+1);

		if(  grid_hgts[offset]*Z_TILE_STEP > h  ) {
			grid_hgts[offset] = h/Z_TILE_STEP;
			n = 1;

#ifndef DOUBLE_GROUNDS
			n += lower_to(x-1, y-1, h+Z_TILE_STEP,set_slopes);
			n += lower_to(x  , y-1, h+Z_TILE_STEP,set_slopes);
			n += lower_to(x+1, y-1, h+Z_TILE_STEP,set_slopes);
			n += lower_to(x-1, y  , h+Z_TILE_STEP,set_slopes);

			n += lower_to(x, y, h,set_slopes);

			n += lower_to(x+1, y  , h+Z_TILE_STEP,set_slopes);
			n += lower_to(x-1, y+1, h+Z_TILE_STEP,set_slopes);
			n += lower_to(x  , y+1, h+Z_TILE_STEP,set_slopes);
			n += lower_to(x+1, y+1, h+Z_TILE_STEP,set_slopes);
#else
			n += lower_to(x-1, y-1, h+Z_TILE_STEP*2,set_slopes);
			n += lower_to(x  , y-1, h+Z_TILE_STEP*2,set_slopes);
			n += lower_to(x+1, y-1, h+Z_TILE_STEP*2,set_slopes);
			n += lower_to(x-1, y  , h+Z_TILE_STEP*2,set_slopes);

			n += lower_to(x, y, h,set_slopes);

			n += lower_to(x+1, y  , h+Z_TILE_STEP*2,set_slopes);
			n += lower_to(x-1, y+1, h+Z_TILE_STEP*2,set_slopes);
			n += lower_to(x  , y+1, h+Z_TILE_STEP*2,set_slopes);
			n += lower_to(x+1, y+1, h+Z_TILE_STEP*2,set_slopes);
#endif
		}
	}
  return n;
}



int karte_t::lower(koord pos)
{
	bool ok = can_lower(pos.x, pos.y);
	int n = 0;
	if(ok && ist_in_kartengrenzen(pos)) {
		grund_t *gr = lookup_kartenboden(pos);
		const sint8 hnew = gr->ist_wasser() ? lookup_hgt(pos) : gr->get_hoehe() + corner4(gr->get_grund_hang());
		n = lower_to(pos.x, pos.y, hnew, hnew, hnew, hnew-1);
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
		spieler_t::accounting(sp, n * settings.cst_alter_land, pos, COST_CONSTRUCTION);
	}
	return ok;
}


void karte_t::set_player_password_hash( uint8 player_nr, uint8 *hash )
{
	memcpy( player_password_hash[player_nr], hash, 20 );
}


void karte_t::clear_player_password_hashes()
{
	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		MEMZERO(player_password_hash[i]);
		if (spieler[i]) {
			spieler[i]->set_unlock(player_password_hash[i]);
		}
	}
}


// new tool definition
void karte_t::set_werkzeug( werkzeug_t *w, spieler_t *sp )
{
	if(  get_random_mode()&LOAD_RANDOM  ) {
		dbg->warning("karte_t::set_werkzeug", "Ignored tool %i during loading.", w->get_id() );
		return;
	}

	if(  (!w->is_init_network_save()  ||  !w->is_work_network_save())  &&
		 !(w->get_id()==(WKZ_PWDHASH_TOOL|SIMPLE_TOOL)  ||  w->get_id()==(WKZ_SET_PLAYER_TOOL|SIMPLE_TOOL))  &&
		 sp  &&  sp->set_unlock(player_password_hash[sp->get_player_nr()])  ) {
		// player is currently password protected => request unlock first
		create_win( -1, -1, new password_frame_t(sp), w_info, (long)(player_password_hash[sp->get_player_nr()]) );
		return;
	}
	w->flags = event_get_last_control_shift();
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

	bool init_result = w->init(this,sp);
	// for unsafe tools init() must return false
	assert(w->is_init_network_save()  ||  !init_result);

	if (init_result) {

		set_dirty();
		werkzeug_t *sp_wkz = werkzeug[sp->get_player_nr()];
		if(w != sp_wkz) {

			// reinit same tool => do not play sound twice
			struct sound_info info;
			info.index = SFX_SELECT;
			info.volume = 255;
			info.pri = 0;
			sound_play(info);

			// only exit, if it is not the same tool again ...

			sp_wkz->flags |= werkzeug_t::WFL_LOCAL;
			sp_wkz->exit(this, sp);
			sp_wkz->flags =0;
		}
		else {
			// init again, to interrupt dragging
			werkzeug[sp->get_player_nr()]->init(this,active_player);
		}
		
		if(  sp==active_player  ) {
			// reset pointer
			koord3d zpos = zeiger->get_pos();
			zeiger->set_bild( w->cursor );
			zeiger->set_yoff( w->offset );
			if(!zeiger->area_changed()) {
				// reset to default 1,1 size
				zeiger->set_area( koord(1,1), false );
			}
			zeiger->change_pos( zpos );
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


void karte_t::rotate90()
{
	// asumme we can save this rotation
	nosave_warning = nosave = false;

	//announce current target rotation
	settings.rotate90();

	// clear marked region
	zeiger->change_pos( koord3d::invalid );

	// preprocessing, detach stops from factories to prevent crash
	slist_iterator_tpl <halthandle_t> halt_pre_iter (haltestelle_t::get_alle_haltestellen());
	while( halt_pre_iter.next() ) {
		halt_pre_iter.get_current()->release_factory_links();
	}

	// first: rotate all things on the map
	planquadrat_t *new_plan = new planquadrat_t[cached_groesse_gitter_y*cached_groesse_gitter_x];
	for( int x=0;  x<cached_groesse_gitter_x;  x++  ) {
		for( int y=0;  y<cached_groesse_gitter_y;  y++  ) {
			int nr = x+(y*cached_groesse_gitter_x);
			int new_nr = (cached_groesse_karte_y-y)+(x*cached_groesse_gitter_y);
			new_plan[new_nr] = plan[nr];
			plan[nr] = planquadrat_t();

			// now rotate everything on the ground(s)
			for(  uint i=0;  i<new_plan[new_nr].get_boden_count();  i++  ) {
				new_plan[new_nr].get_boden_bei(i)->rotate90();
			}
		}
	}
	delete [] plan;
	plan = new_plan;

	// rotate heightmap
	sint8 *new_hgts = new sint8[(cached_groesse_gitter_x+1)*(cached_groesse_gitter_y+1)];
	for( int x=0;  x<=cached_groesse_gitter_x;  x++  ) {
		for( int y=0;  y<=cached_groesse_gitter_y;  y++  ) {
			int nr = x+(y*(cached_groesse_gitter_x+1));
			int new_nr = (cached_groesse_gitter_y-y)+(x*(cached_groesse_gitter_y+1));
			new_hgts[new_nr] = grid_hgts[nr];
		}
	}
	delete [] grid_hgts;
	grid_hgts = new_hgts;

	// rotate borders
	sint16 xw = cached_groesse_karte_x;
	cached_groesse_karte_x = cached_groesse_karte_y;
	cached_groesse_karte_y = xw;

	int wx = cached_groesse_gitter_x;
	cached_groesse_gitter_x = cached_groesse_gitter_y;
	cached_groesse_gitter_y = wx;

	// now step all towns (to generate passengers)
	for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
		(*i)->rotate90( cached_groesse_karte_x );
	}

	//slist_iterator_tpl<fabrik_t *> iter(fab_list);
	//while(iter.next()) {
	for(sint16 i = fab_list.get_count() - 1; i >= 0; i --)
	{
		//iter.get_current()->rotate90( cached_groesse_karte_x );
		fab_list[i]->rotate90( cached_groesse_karte_x );
	}

	slist_iterator_tpl <halthandle_t> halt_iter (haltestelle_t::get_alle_haltestellen());
	while( halt_iter.next() ) {
		halt_iter.get_current()->rotate90( cached_groesse_karte_x );
	}

	// rotate all other objects like factories and convois
	for(unsigned i=0; i<convoi_array.get_count();  i++) {
		convoi_array[i]->rotate90( cached_groesse_karte_x );
	}

	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  spieler[i]  ) {
			spieler[i]->rotate90( cached_groesse_karte_x );
		}
	}

	// rotate label texts
	slist_iterator_tpl <koord> label_iter (labels);
	while( label_iter.next() ) {
		label_iter.access_current().rotate90( cached_groesse_karte_x );
	}

	// rotate view
	ij_off.rotate90( cached_groesse_karte_x );

	// rotate messages
	msg->rotate90( cached_groesse_karte_x );

	// rotate view in dialoge windows
	win_rotate90( cached_groesse_karte_x );

	if(cached_groesse_gitter_x != cached_groesse_gitter_y) {
		// the marking array and the map must be reinit
		marker.init( cached_groesse_gitter_x, cached_groesse_gitter_y );
		reliefkarte_t::get_karte()->set_welt( this );
	}

	//  rotate map search array
	fabrikbauer_t::neue_karte( this );

	// update minimap
	if(reliefkarte_t::is_visible) {
		reliefkarte_t::get_karte()->set_mode( reliefkarte_t::get_karte()->get_mode() );
	}

	// finally recalculate schedules for goods in transit ...
	// Modified by : Knightly
	if ( settings.get_default_path_option() == 2 )
	{
		path_explorer_t::refresh_all_categories(true);
	}
	else
	{
		set_schedule_counter();
	}

	set_dirty();
}
// -------- Verwaltung von Fabriken -----------------------------


bool
karte_t::add_fab(fabrik_t *fab)
{
//DBG_MESSAGE("karte_t::add_fab()","fab = %p",fab);
	assert(fab != NULL);
	//fab_list.insert( fab );
	fab_list.append(fab);
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

		// remove all links to cities
		fab->clear_target_cities();

		// finally delete it
		delete fab;

		// recalculate factory position map
		fabrikbauer_t::neue_karte(this);
	}
	return true;
}

fabrik_t *
karte_t::get_random_fab() const
{
	const int anz = fab_list.get_count();
	fabrik_t *result = NULL;

	if(anz > 0) {
		const int end = simrand( anz, "fabrik_t *karte_t::get_random_fab" );
		result = fab_list[end];
	}
	return result;
}


/*----------------------------------------------------------------------------------------------------------------------*/
/* same procedure for tourist attractions */


void karte_t::add_ausflugsziel(gebaeude_t *gb)
{
	assert(gb != NULL);
	ausflugsziele.append( gb, gb->get_tile()->get_besch()->get_level(), 16 );

	// Knightly : add links between this attraction and all cities
	for(  uint32 c=0;  c<stadt.get_count();  ++c  ) {
		stadt[c]->add_target_attraction(gb);
	}
}


void karte_t::remove_ausflugsziel(gebaeude_t *gb)
{
	assert(gb != NULL);
	ausflugsziele.remove( gb );

	// Knightly : remove links between this attraction and all cities
	for(  uint32 c=0;  c<stadt.get_count();  ++c  ) {
		stadt[c]->remove_target_attraction(gb);
	}
}


/* select a random target for a tourist; targets are weighted by their importance */
const gebaeude_t *
karte_t::get_random_ausflugsziel() const
{
	if (!ausflugsziele.empty()) {
		return pick_any_weighted(ausflugsziele);
	}
	// so there are no destinations ... should never occur ...
	dbg->fatal("karte_t::get_random_ausflugsziel()","nothing found.");
	return NULL;
}


// -------- Verwaltung von Staedten -----------------------------
// "look for next city" (Babelfish)

stadt_t *karte_t::suche_naechste_stadt(const koord pos) const
{
	long min_dist = 99999999;
	stadt_t *best = NULL;

	if(ist_in_kartengrenzen(pos)) {
		for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
			stadt_t* s = *i;
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


stadt_t *karte_t::get_city(const koord pos) const
{
	stadt_t* city = NULL;
	if(pos == koord::invalid)
	{
		return NULL;
	}

	if(ist_in_kartengrenzen(pos)) 
	{
		uint16 cities = 0;
		for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) 
		{
			stadt_t* c = *i;
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

static bool sync_step_running = false;

bool karte_t::sync_add(sync_steppable *obj)
{
	if(  sync_step_running  ) {
		sync_add_list.insert( obj );
	}
	else {
		sync_list.insert( obj );
	}
	return true;
}


bool karte_t::sync_remove(sync_steppable *obj)	// entfernt alle dinge == obj aus der Liste
{
	if(  sync_step_running  ) {
		if(sync_add_list.remove(obj)) {
			return true;
		}
		else {
			sync_remove_list.append(obj);
		}
	}
	else {
		if(sync_add_list.remove(obj)) {
			return true;
		}
		else {
			sync_list.remove(obj);
		}
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
		sync_step_running = true;

		// just for progress
		ticks += delta_t;

		// insert new objects created during last sync_step (eg vehicle smoke)
		if(!sync_add_list.empty()) {
			sync_list.append_list(sync_add_list);
		}

		// now remove everything from last time
		while(!sync_remove_list.empty()) {
			sync_steppable *ss = sync_remove_list.remove_first();
			sync_list.remove( ss );
		}

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

		// now remove everything from this time
		while(!sync_remove_list.empty()) {
			sync_steppable *ss = sync_remove_list.remove_first();
			sync_list.remove( ss );
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
				new_yoff -= tile_raster_scale_y(-v.get_yoff(), rw) + tile_raster_scale_y(new_pos.z * TILE_HEIGHT_STEP / Z_TILE_STEP, rw);
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
	else {
		// try to get 10 fps or lower rate (if set)
		sint32 frame_intervall = max( 100, 1000/umgebung_t::fps );
		if(get_frame_time()>frame_intervall) {
			reduce_frame_time();
		}
		else {
			increase_frame_time();
		}
		// calculate current speed
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



void karte_t::neuer_monat()
{
	update_history();
	// advance history ...
	last_month_bev = finance_history_month[0][WORLD_CITICENS];
	for(  int hist=0;  hist<karte_t::MAX_WORLD_COST;  hist++  ) {
		for( int y=MAX_WORLD_HISTORY_MONTHS-1; y>0;  y--  ) {
			finance_history_month[y][hist] = finance_history_month[y-1][hist];
		}
	}

	current_month ++;
	letzter_monat ++;
	if(letzter_monat>11) {
		letzter_monat = 0;
	}
	DBG_MESSAGE("karte_t::neuer_monat()","Month (%d/%d) has started", (letzter_monat%12)+1, letzter_monat/12 );
	DBG_MESSAGE("karte_t::neuer_monat()","sync_step %u objects", sync_list.get_count() );

	// this should be done before a map update, since the map may want an update of the way usage
//	DBG_MESSAGE("karte_t::neuer_monat()","ways");
	slist_iterator_tpl <weg_t *> weg_iter (weg_t::get_alle_wege());
	while( weg_iter.next() ) {
		weg_iter.get_current()->neuer_monat();
	}

//	DBG_MESSAGE("karte_t::neuer_monat()","depots");
	// Bernd Gabriel - call new month for depots
	slist_iterator_tpl <depot_t *> depot_iter(depot_t::get_depot_list());
	while( depot_iter.next() ) {
		depot_iter.get_current()->neuer_monat();
	}

	// recalc old settings (and maybe update the staops with the current values)
	reliefkarte_t::get_karte()->neuer_monat();

	INT_CHECK("simworld 3042");

//	DBG_MESSAGE("karte_t::neuer_monat()","convois");
	// hsiegeln - call new month for convoys
	for(unsigned i=0;  i<convoi_array.get_count();  i++ ) {
		convoihandle_t cnv = convoi_array[i];
		cnv->new_month();
	}

	base_pathing_counter ++;

	INT_CHECK("simworld 3053");


//	DBG_MESSAGE("karte_t::neuer_monat()","factories");
	sint16 number_of_factories = fab_list.get_count();
	fabrik_t * fab;
	uint32 total_electric_demand = 1;
	uint32 electric_productivity = 0;
	sint16 difference = 0;
	for(sint16 i = number_of_factories - 1; i >= 0; i--)
	{
		fab = fab_list[i];
		fab->neuer_monat();
		// The number of factories might have diminished,
		// so must adjust i to prevent out of bounds errors.
		difference = number_of_factories - fab_list.get_count();
		if(difference == 0)
		{
			// Check to see whether the factory has closed down - if so, the pointer will be dud.
			if(fab->get_besch()->is_electricity_producer()) 
			{
				electric_productivity += fab->get_scaled_electric_amount();
			}
			else 
			{
				total_electric_demand += fab->get_scaled_electric_amount();
			}
			number_of_factories = fab_list.get_count();
		}
		i -= difference;
		number_of_factories -= difference;
	}

	// Check to see whether more factories need to be added
	// to replace ones that have closed.
	// @author: jamespetts

	if(industry_density_proportion == 0 && finance_history_month[0][WORLD_CITICENS] > 0)
	{
		// Set the industry density proportion for the first time when the number of citizens is populated.
		industry_density_proportion = ((sint64)actual_industry_density * 10000ll) / finance_history_month[0][WORLD_CITICENS];
	}
	const uint32 target_industry_density = get_target_industry_density();
	if(actual_industry_density < target_industry_density)
	{
		// Only add one per month, and randomise.
		const uint32 percentage = ((target_industry_density - actual_industry_density) * 100) / target_industry_density;
		const uint8 chance = simrand(10000, "void karte_t::neuer_monat()");
		if(chance < percentage)
		{
			fabrikbauer_t::increase_industry_density(this, true, true);
		}
	}

	INT_CHECK("simworld 3105");

	//	DBG_MESSAGE("karte_t::neuer_monat()","cities");
	// roll city history and copy the new citicens (i.e. the new weight) into the stadt array
	// no INT_CHECK() here, or dialoges will go crazy!!!
	weighted_vector_tpl<stadt_t*> new_weighted_stadt(stadt.get_count() + 1);
	sint32 outstanding_cars = 0;
	for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
		stadt_t* s = *i;
		s->neuer_monat(recheck_road_connexions);
		outstanding_cars += s->get_outstanding_cars();
		new_weighted_stadt.append(s, s->get_einwohner(), 64);
		INT_CHECK("simworld 3117");
		total_electric_demand += (*i)->get_power_demand();
	}
	recheck_road_connexions = false;
	swap(stadt, new_weighted_stadt);

	if(fabrikbauer_t::power_stations_available(this) && (((sint64)electric_productivity * 4000l) / total_electric_demand) < (sint64)get_settings().get_electric_promille())
	{
		// Add industries if there is a shortage of electricity - power stations will be built.
		// Also (8.1 and onwards) - check whether power stations are available, or else large quantities of other industries will
		// be built instead every month.
		fabrikbauer_t::increase_industry_density(this, true, true, true);
	}

	INT_CHECK("simworld 3130");

//	DBG_MESSAGE("karte_t::neuer_monat()","players");
	if (letzter_monat == 0 && !settings.is_freeplay()) {
		// remove all player (but first and second) who went bankrupt during last year
		for(int i=2; i<MAX_PLAYER_COUNT-1; i++) {
			if(  spieler[i] != NULL  &&
				spieler[i]->get_finance_history_year(0,COST_NETWEALTH)<=0  &&
				spieler[i]->get_finance_history_year(0,COST_MAINTENANCE)==0  &&
				spieler[i]->get_maintenance(spieler_t::MAINT_VEHICLE)==0  &&
				spieler[i]->get_finance_history_year(0,COST_ALL_CONVOIS)==0  )
			{
				delete spieler[i];
				spieler[i] = 0;
				// if currently still active => reset to default human
				if(  i == active_player_nr  ) {
					i = 0;
					active_player = spieler[0];
				}
			}
		}
		// update the window
		ki_kontroll_t* playerwin = (ki_kontroll_t*)win_get_magic(magic_ki_kontroll_t);
		if (playerwin) {
			playerwin->update_data();
		}
	}
	// spieler
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(  spieler[i] != NULL  ) {
			spieler[i]->neuer_monat();
		}
	}

	stadtauto_t* car;
	while(!unassigned_cars.empty() && (sint32)unassigned_cars.get_count() > outstanding_cars)
	{
		//Make sure that there are not too many cars on the roads. 
		car = unassigned_cars.remove_first();
		if(car != NULL)
		{
			car->kill();
		}
	}
	car = NULL;

	INT_CHECK("simworld 3175");

//	DBG_MESSAGE("karte_t::neuer_monat()","halts");
	slist_iterator_tpl <halthandle_t> halt_iter (haltestelle_t::get_alle_haltestellen());
	while( halt_iter.next() ) {
		halt_iter.get_current()->neuer_monat();
		INT_CHECK("simworld 1877");
	}

	INT_CHECK("simworld 2522");
	slist_iterator_tpl<depot_t*> iter = depot_t::get_depot_list();
	while(iter.next())
	{
		iter.get_current()->neuer_monat();
	}

	// now switch year to get the right year for all timeline stuff ...
	if(letzter_monat==0) {
		neues_jahr();
		INT_CHECK("simworld 1299");
	}

	wegbauer_t::neuer_monat(this);
	INT_CHECK("simworld 1299");

	// Check whether downstream substations have become engulfed by
	// an expanding city.
	slist_iterator_tpl<senke_t *> senke_iter( senke_t::senke_list );
	while(senke_iter.next()) 
	{
		// This will add a city if the city has engulfed the substation, and remove a city if
		// the city has been deleted or become smaller. 
		senke_t* const substation = senke_iter.access_current();
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

	if(  !umgebung_t::networkmode  &&  umgebung_t::autosave>0  &&  letzter_monat%umgebung_t::autosave==0  ) {
		char buf[128];
		sprintf( buf, "save/autosave%02i.sve", letzter_monat+1 );
		speichern( buf, umgebung_t::savegame_version_str, umgebung_t::savegame_ex_version_str, true );
	}

	// Added by : Knightly
	// Note		: This should be done after all lines and convoys have rolled their statistics
	if ( settings.get_default_path_option() == 2 )
	{
		path_explorer_t::refresh_all_categories(true);
	}

	set_citycar_speed_average();
	calc_generic_road_speed_city();
	calc_generic_road_speed_intercity();
	calc_max_road_check_depth();
}



void karte_t::neues_jahr()
{
	letztes_jahr = current_month/12;

	// advance history ...
	for(  int hist=0;  hist<karte_t::MAX_WORLD_COST;  hist++  ) {
		for( int y=MAX_WORLD_HISTORY_YEARS-1; y>0;  y--  ) {
			finance_history_year[y][hist] = finance_history_year[y-1][hist];
		}
	}

DBG_MESSAGE("karte_t::neues_jahr()","speedbonus for %d %i, %i, %i, %i, %i, %i, %i, %i", letztes_jahr,
			average_speed[0], average_speed[1], average_speed[2], average_speed[3], average_speed[4], average_speed[5], average_speed[6], average_speed[7] );

	char buf[256];
	sprintf(buf,translator::translate("Year %i has started."),letztes_jahr);
	msg->add_message(buf,koord::invalid,message_t::general,COL_BLACK,skinverwaltung_t::neujahrsymbol->get_bild_nr(0));

	for(unsigned i=0;  i<convoi_array.get_count();  i++ ) {
		convoihandle_t cnv = convoi_array[i];
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

	const uint32 speed_bonus_percent = get_settings().get_speed_bonus_multiplier_percent();
	for(int i=road_wt; i<=narrowgauge_wt; i++) {
		const int typ = i==4 ? 3 : (i-1)&7;
		const uint32 base_speed_bonus = vehikelbauer_t::get_speedbonus( this->get_timeline_year_month(), i==4 ? air_wt : (waytype_t)i );
		average_speed[typ] = (base_speed_bonus * speed_bonus_percent) / 100;
	}

	//	DBG_MESSAGE("karte_t::recalc_average_speed()","");
	if(use_timeline()) {

		char	buf[256];
		for(int i=road_wt; i<=air_wt; i++) {
			slist_tpl<vehikel_besch_t*>* cl = vehikelbauer_t::get_info((waytype_t)i);
			if(cl) {
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
				}
				vehicle_type = translator::translate( vehicle_type );

				slist_iterator_tpl<vehikel_besch_t*> vehinfo(cl);
				while (vehinfo.next()) {
					const vehikel_besch_t* info = vehinfo.get_current();
					const uint16 intro_month = info->get_intro_year_month();
					if(intro_month == current_month) 
					{
						if(info->is_available_only_as_upgrade())
						{
							sprintf(buf,
								translator::translate("Upgrade to %s now available:\n%s\n"),
								vehicle_type,
								translator::translate(info->get_name()));
								msg->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,info->get_basis_bild());
						}
						else
						{
							sprintf(buf,
								translator::translate("New %s now available:\n%s\n"),
								vehicle_type,
								translator::translate(info->get_name()));
								msg->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,info->get_basis_bild());
						}
					}

					const uint16 retire_month = info->get_retire_year_month();
					if(retire_month == current_month) 
					{
						sprintf(buf,
							translator::translate("Production of %s has been stopped:\n%s\n"),
							vehicle_type,
							translator::translate(info->get_name()));
						msg->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,info->get_basis_bild());
					}

					const uint16 obsolete_month = info->get_obsolete_year_month(this);
					if(obsolete_month == current_month) 
					{
						sprintf(buf,
							translator::translate("The following %s has become obsolete:\n%s\n"),
							vehicle_type,
							translator::translate(info->get_name()));
							msg->add_message(buf,koord::invalid,message_t::new_vehicle,COL_DARK_BLUE,info->get_basis_bild());
					}
				}
			}
		}

		// city road check
		if (weg_besch_t const* city_road_test = settings.get_city_road_type(get_timeline_year_month())) {
			city_road = city_road_test;
		}
		else {
			DBG_MESSAGE("karte_t::neuer_monat()","Month %d has started", letzter_monat);
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
			char text[1024];
			sprintf( text, translator::translate(msg), (float)speed_to_kmh(10*sr->speed)/10.0F, sr->cnv->get_name() );
			get_message()->add_message(text, sr->pos, message_t::new_vehicle, PLAYER_FLAG|sr->besitzer->get_player_nr() );
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

		next_month_ticks += karte_t::ticks_per_world_month;

		// avoid overflow here ...
		// Should not overflow: now usint 64-bit values.
		// @author: jamespetts
/*
		if(ticks>next_month_ticks) {
			ticks %= karte_t::ticks_per_world_month;
			ticks += karte_t::ticks_per_world_month;
			next_month_ticks = ticks+karte_t::ticks_per_world_month;
			last_step_ticks %= karte_t::ticks_per_world_month;
		}
*/

		DBG_DEBUG4("karte_t::step", "calling neuer_monat");
		neuer_monat();
	}

	const long delta_t = ticks - last_step_ticks;
	DBG_DEBUG4("karte_t::step", "time calculations");
	if(  step_mode==NORMAL  ) {
		/* Try to maintain a decent pause, with a step every 170-250 ms (~5,5 simloops/s)
		 * Also avoid too large or negative steps
		 */

		// needs plausibility check?!?
		if(delta_t>10000  || delta_t<0) {
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
			if(idle_time<get_frame_time()-10) {
				idle_time ++;
			}
		}
		// cap it ...
		if(idle_time>=get_frame_time()-10) {
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
		const uint32 end_count = min( cached_groesse_gitter_x*cached_groesse_gitter_y,  tile_counter + max( 16384, cached_groesse_gitter_x*cached_groesse_gitter_y/16 ) );
		while(  tile_counter < end_count  ) {
			plan[tile_counter].check_season(current_month);
			tile_counter ++;
			if((tile_counter&0x3FF)==0) {
				INT_CHECK("karte_t::step");
			}
		}
		if(  tile_counter >= (uint32)cached_groesse_gitter_x*(uint32)cached_groesse_gitter_y  ) {
			pending_season_change --;
			tile_counter = 0;
		}
	}

	// to make sure the tick counter will be updated
	INT_CHECK("karte_t::step");


	// Knightly : calling global path explorer
	if ( settings.get_default_path_option() == 2 )
	{
		path_explorer_t::step();
		INT_CHECK("karte_t::step");
	}
	
	
	DBG_DEBUG4("karte_t::step", "step convois");
	// since convois will be deleted during stepping, we need to step backwards
	for(sint32 i=convoi_array.get_count()-1;  i>=0;  i--  ) {
		convoihandle_t cnv = convoi_array[i];
		cnv->step();
		if((i&7)==0) {
			INT_CHECK("simworld 1947");
		}
	}

	// now step all towns (to generate passengers)
	DBG_DEBUG4("karte_t::step", "step cities");
	sint64 bev = 0;
	for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
		(*i)->step(delta_t);
		bev += (*i)->get_finance_history_month( 0, HIST_CITICENS );
	}

	// the inhabitants stuff
	finance_history_month[0][WORLD_CITICENS] = bev;

	DBG_DEBUG4("karte_t::step", "step factories");
	ITERATE(fab_list,i)
	{
		fab_list[i]->step(delta_t);
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
		last_clients = socket_list_t::get_playing_clients();
		if(  umgebung_t::announce_server  &&  umgebung_t::announce_server_intervall<0  ) {
			// inform the master server
			announce_server();
		}
		// add message via tool
		cbuffer_t buf;
		buf.printf(translator::translate("Now %u clients connected.", settings.get_name_language_id()), last_clients);
		werkzeug_t *w = create_tool( WKZ_ADD_MESSAGE_TOOL | SIMPLE_TOOL );
		w->set_default_param( buf );
		set_werkzeug( w, NULL );
		// since init always returns false, it is save to delete immediately
		delete w;
#ifdef DEBUG_SIMRAND_CALLS
		if(last_clients == 0)
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
		for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
			bev += (*i)->get_finance_history_month( m, HIST_CITICENS );
			trans_pas += (*i)->get_finance_history_month( m, HIST_PAS_TRANSPORTED );
			total_pas += (*i)->get_finance_history_month( m, HIST_PAS_GENERATED );
			trans_mail += (*i)->get_finance_history_month( m, HIST_MAIL_TRANSPORTED );
			total_mail += (*i)->get_finance_history_month( m, HIST_MAIL_GENERATED );
			supplied_goods += (*i)->get_finance_history_month( m, HIST_GOODS_RECIEVED );
			total_goods += (*i)->get_finance_history_month( m, HIST_GOODS_NEEDED );
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
				transported += spieler[i]->get_finance_history_month( m, COST_ALL_TRANSPORTED );
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
		for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
			bev += (*i)->get_finance_history_year( y, HIST_CITICENS );
			trans_pas_year += (*i)->get_finance_history_year( y, HIST_PAS_TRANSPORTED );
			total_pas_year += (*i)->get_finance_history_year( y, HIST_PAS_GENERATED );
			trans_mail_year += (*i)->get_finance_history_year( y, HIST_MAIL_TRANSPORTED );
			total_mail_year += (*i)->get_finance_history_year( y, HIST_MAIL_GENERATED );
			supplied_goods_year += (*i)->get_finance_history_year( y, HIST_GOODS_RECIEVED );
			total_goods_year += (*i)->get_finance_history_year( y, HIST_GOODS_NEEDED );
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
				transported_year += spieler[i]->get_finance_history_year( y, COST_ALL_TRANSPORTED );
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
	for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
		bev += (*i)->get_finance_history_month( 0, HIST_CITICENS );
		trans_pas += (*i)->get_finance_history_month( 0, HIST_PAS_TRANSPORTED );
		total_pas += (*i)->get_finance_history_month( 0, HIST_PAS_GENERATED );
		trans_mail += (*i)->get_finance_history_month( 0, HIST_MAIL_TRANSPORTED );
		total_mail += (*i)->get_finance_history_month( 0, HIST_MAIL_GENERATED );
		supplied_goods += (*i)->get_finance_history_month( 0, HIST_GOODS_RECIEVED );
		total_goods += (*i)->get_finance_history_month( 0, HIST_GOODS_NEEDED );
		trans_pas_year += (*i)->get_finance_history_year( 0, HIST_PAS_TRANSPORTED );
		total_pas_year += (*i)->get_finance_history_year( 0, HIST_PAS_GENERATED );
		trans_mail_year += (*i)->get_finance_history_year( 0, HIST_MAIL_TRANSPORTED );
		total_mail_year += (*i)->get_finance_history_year( 0, HIST_MAIL_GENERATED );
		supplied_goods_year += (*i)->get_finance_history_year( 0, HIST_GOODS_RECIEVED );
		total_goods_year += (*i)->get_finance_history_year( 0, HIST_GOODS_NEEDED );
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
			transported += spieler[i]->get_finance_history_month( 0, COST_TRANSPORTED_GOOD );
			transported_year += spieler[i]->get_finance_history_year( 0, COST_TRANSPORTED_GOOD );
		}
	}
	finance_history_month[0][WORLD_TRANSPORTED_GOODS] = transported;
	finance_history_year[0][WORLD_TRANSPORTED_GOODS] = transported_year;
}



/**
 * If this is true, the map will not be scrolled
 * on right-drag
 * @author Hj. Malthaner
 */
void karte_t::set_scroll_lock(bool yesno)
{
	scroll_lock = yesno;
	if(yesno  &&  follow_convoi.is_bound()) {
		follow_convoi = convoihandle_t();
	}
}



// change the center viewport position for a certain ground tile
// any possible convoi to follow will be disabled
void karte_t::change_world_position( koord3d new_ij )
{
	const sint16 rw = get_tile_raster_width();
	change_world_position( new_ij.get_2d(), 0, tile_raster_scale_y(new_ij.z*TILE_HEIGHT_STEP/Z_TILE_STEP,rw) );
	follow_convoi = convoihandle_t();
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



void karte_t::blick_aendern(event_t *ev)
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



/**
 * returns the natural slope at a position using the actual slopes
 * @author prissi
 */
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
		sint8 *p = &grid_hgts[pos.x + pos.y*(get_groesse_x()+1)];
		*p = height4;
		*(p+1) = height3;
		*(p+get_groesse_x()+2) = height2;
		*(p+get_groesse_x()+1) = height1;
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



/**
 * returns the natural slope a a position using the grid
 * @author prissi
 */
uint8 karte_t::calc_natural_slope( const koord pos ) const
{
	if(ist_in_gittergrenzen(pos.x, pos.y)) {

		const sint8 * p = &grid_hgts[pos.x + pos.y*(get_groesse_x()+1)];

		const int h1 = *p;
		const int h2 = *(p+1);
		const int h3 = *(p+get_groesse_x()+2);
		const int h4 = *(p+get_groesse_x()+1);

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
			if(max_hgt(k) > get_grundwasser()) {
				return false;
			}
		}
	}
	return true;
}



bool karte_t::ist_platz_frei(koord pos, sint16 w, sint16 h, int *last_y, climate_bits cl) const
{
	if(pos.x<0 || pos.y<0 || pos.x+w>=get_groesse_x() || pos.y+h>=get_groesse_y()) {
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
			if(platz_h!=(gr->get_hoehe()+Z_TILE_STEP*((gr->get_grund_hang()+127)/128))  ||  !gr->ist_natur() ||  gr->kann_alle_obj_entfernen(NULL) != NULL  ||  (cl&(1<<get_climate(gr->get_hoehe())))==0) {
				if(last_y) {
					*last_y = k.y;
				}
				return false;
			}
		}
	}
	return true;
}

slist_tpl<koord> *karte_t::finde_plaetze(sint16 w, sint16 h, climate_bits cl, sint16 old_x, sint16 old_y) const
{
	slist_tpl<koord> * list = new slist_tpl<koord>();
	koord start;
	int last_y;

DBG_DEBUG("karte_t::finde_plaetze()","for size (%i,%i) in map (%i,%i)",w,h,get_groesse_x(),get_groesse_y() );
	for(start.x=0; start.x<get_groesse_x()-w; start.x++) {
		for(start.y=start.x<old_x?old_y:0; start.y<get_groesse_y()-h; start.y++) {
			if(ist_platz_frei(start, w, h, &last_y, cl)) {
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
bool karte_t::play_sound_area_clipped(koord pos, sound_info info) const
{
	if(is_sound  &&  zeiger) {
		const int dist = koord_distance( pos, zeiger->get_pos() );

		if(dist < 100) {
			int xw = (2*display_get_width())/get_tile_raster_width();
			int yw = (4*display_get_height())/get_tile_raster_width();

			info.volume = (255l*(xw+yw))/(xw+yw+(64*dist));
			if(  info.volume>8  ) {
				sound_play(info);
			}
		}
		return dist < 25;
	}
	return false;
}



void karte_t::speichern(const char *filename, const char *version_str, const char *ex_version_str, bool silent )
{
#ifndef DEMO
DBG_MESSAGE("karte_t::speichern()", "saving game to '%s'", filename);

	loadsave_t  file;

	display_show_load_pointer( true );
	if(!file.wr_open(filename, loadsave_t::save_mode, umgebung_t::objfilename.c_str(), version_str, ex_version_str )) {
		create_win(new news_img("Kann Spielstand\nnicht speichern.\n"), w_info, magic_none);
		dbg->error("karte_t::speichern()","cannot open file for writing! check permissions!");
	}
	else {
		speichern(&file,silent);
		const char *success = file.close();
		if(success) {
			static char err_str[512];
			sprintf( err_str, translator::translate("Error during saving:\n%s"), success );
			create_win( new news_img(err_str), w_time_delete, magic_none);
		}
		else {
			if(!silent) {
				create_win( new news_img("Spielstand wurde\ngespeichert!\n"), w_time_delete, magic_none);
				// update the filename, if no autosave
				settings.set_filename(filename);
			}
		}
		reset_interaction();
	}
	display_show_load_pointer( false );
#endif
}


void karte_t::speichern(loadsave_t *file,bool silent)
{
	bool needs_redraw = false;

DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "start");
	if(!silent) {
		display_set_progress_text(translator::translate("Saving map ..."));
		display_progress(0,get_groesse_y());
	}

	// rotate the map until it can be saved completely
	nosave_warning = nosave = false;
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
			tool->cleanup( spieler[sp_nr], false );
		}
	}

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

	if(file->get_experimental_version() <= 1)
	{
		uint32 old_ticks = (uint32)ticks;
		file->rdwr_long(old_ticks);
		ticks = old_ticks;
	}
	else
	{
		file->rdwr_longlong(ticks);
	}
	file->rdwr_long(letzter_monat);
	file->rdwr_long(letztes_jahr);

	// rdwr cityrules (and associated settings) for networkgames
	if(file->get_version()>102002 && (file->get_experimental_version() == 0 || file->get_experimental_version() >= 9))
	{
		bool do_rdwr = umgebung_t::networkmode;
		file->rdwr_bool(do_rdwr);
		if (do_rdwr) 
		{
			if(file->get_experimental_version() >= 9)
			{
				stadt_t::cityrules_rdwr(file);
				stadt_t::privatecar_rdwr(file);
			}
			stadt_t::electricity_consumption_rdwr(file);
			if(file->get_version()>102003 && (file->get_experimental_version() == 0 || file->get_experimental_version() >= 9)) 
			{
				vehikelbauer_t::rdwr_speedbonus(file);
			}
		}
	}

	for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
		(*i)->rdwr(file);
		if(silent) {
			INT_CHECK("saving");
		}
	}
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved cities ok");

	for(int j=0; j<get_groesse_y(); j++) {
		for(int i=0; i<get_groesse_x(); i++) {
			plan[i+j*cached_groesse_gitter_x].rdwr(this, file, koord(i,j) );
		}
		if(silent) {
			INT_CHECK("saving");
		}
		else {
			display_progress(j, get_groesse_y());
		}
	}
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved tiles");

	if(  file->get_version()<=102001  ) {
		// not needed any more
		for(int j=0; j<(get_groesse_y()+1)*(get_groesse_x()+1); j++) {
			file->rdwr_byte(grid_hgts[j]);
		}
	DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved hgt");
	}

	sint32 fabs = fab_list.get_count();
	file->rdwr_long(fabs);
	for(sint16 i = fab_list.get_count() - 1; i >= 0; i --)
	{
		fab_list[i]->rdwr(file);
		if(silent) {
			INT_CHECK("saving");
		}
	}
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved fabs");

	sint32 haltcount=haltestelle_t::get_alle_haltestellen().get_count();
	file->rdwr_long(haltcount);
	slist_iterator_tpl<halthandle_t> iter (haltestelle_t::get_alle_haltestellen());
	while(iter.next()) {
		iter.get_current()->rdwr( file );
	}
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved stops");

	// svae number of convois
	if(  file->get_version()>=101000  ) {
		uint16 i=convoi_array.get_count();
		file->rdwr_short(i);
	}
	for(unsigned i=0;  i<convoi_array.get_count();  i++ ) {
		// one MUST NOT call INT_CHECK here or else the convoi will be broken during reloading!
		convoihandle_t cnv = convoi_array[i];
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

	if(file->get_experimental_version() >= 2)
	{
		file->rdwr_short(base_pathing_counter);
	}
	if(file->get_experimental_version() >= 7 && file->get_experimental_version() < 9 && file->get_version() < 110006)
	{
		double old_proportion = (double)industry_density_proportion / 10000.0;
		file->rdwr_double(old_proportion);
		industry_density_proportion = old_proportion * 10000.0;
	}
	else if(file->get_experimental_version() >= 9 && file->get_version() >= 110006)
	{
		file->rdwr_long(industry_density_proportion);
	}

	if(file->get_experimental_version() >=9 && file->get_version() >= 110000)
	{
		file->rdwr_byte(next_private_car_update_month);
		
		// Existing values now saved in order to prevent network desyncs
		file->rdwr_long(citycar_speed_average);
		file->rdwr_bool(recheck_road_connexions);
		file->rdwr_short(generic_road_speed_city);
		file->rdwr_short(generic_road_speed_intercity);
		file->rdwr_long(max_road_check_depth);
		if(file->get_experimental_version() < 10)
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

	// save all open windows (upon request)
	file->rdwr_byte( active_player_nr );
	rdwr_all_win(file);

	if(needs_redraw) 
	{
		update_map();
	}
}



// LOAD, not save
// just the preliminaries, opens the file, checks the versions ...
bool karte_t::laden(const char *filename)
{
	cbuffer_t name;
	bool ok = false;
	bool restore_player_nr = false;
	mute_sound(true);
	display_show_load_pointer(true);

#ifndef DEMO
	loadsave_t file;

	DBG_MESSAGE("karte_t::laden", "loading game from '%s'", filename);

	if(  strstr(filename,"net:")==filename  ) {
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

		if(  (sint32)file.get_version()==-1  ||  file.get_version()>loadsave_t::int_version(SAVEGAME_VER_NR, NULL, NULL).version  ) {
			create_win( new news_img("WRONGSAVE"), w_info, magic_none );
		}
		else {
			create_win(new news_img("Kann Spielstand\nnicht laden.\n"), w_info, magic_none);
		}
	} else if(file.get_version() < 84006) {
		// too old
		create_win(new news_img("WRONGSAVE"), w_info, magic_none);
	}
	else 
	{
/*
		event_t ev;
		ev.ev_class=EVENT_NONE;
		check_pos_win(&ev);
*/
		DBG_MESSAGE("karte_t::laden()","Savegame version is %d", file.get_version());

		laden(&file);

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
		}
		else if(  umgebung_t::networkmode  ) {
			step_mode = PAUSE_FLAG|FIX_RATIO;
			switch_active_player( last_active_player_nr, true );
		}
		else {
			step_mode = NORMAL;
		}

		ok = true;
		file.close();
		if(  !umgebung_t::networkmode  ||  !umgebung_t::restore_UI  ) {
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
#endif
	settings.set_filename(filename);
	display_show_load_pointer(false);

	calc_generic_road_speed_city();
	calc_generic_road_speed_intercity();
	calc_max_road_check_depth();

	return ok;
}


// handles the actual loading
void karte_t::laden(loadsave_t *file)
{
	char buf[80];

	intr_disable();
	dbg->message("karte_t::laden()", "Prepare for loading" );
	for(  uint i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		werkzeug[i] = werkzeug_t::general_tool[WKZ_ABFRAGE];
	}
	destroy_all_win(true);

	display_set_progress_text(translator::translate("Loading map ..."));
	display_progress(0, 100);	// does not matter, since fixed width

	clear_random_mode(~LOAD_RANDOM);
	set_random_mode(LOAD_RANDOM);

	destroy();


	// Added by : Knightly
	path_explorer_t::initialise(this);

	//fast_forward = false;

	tile_counter = 0;

	simloops = 60;

	// zum laden vorbereiten -> tabelle loeschen
	powernet_t::neue_karte();
	pumpe_t::neue_karte();
	senke_t::neue_karte();

	const uint16 old_scale_factor = get_settings().get_meters_per_tile();

	// jetzt geht das laden los
	dbg->warning("karte_t::laden", "Fileversion: %d", file->get_version());
	settings = umgebung_t::default_einstellungen;
	settings.rdwr(file);

	if(  umgebung_t::networkmode  ) {
		// to have games synchronized, transfer random counter too
		setsimrand(settings.get_random_counter(), 0xFFFFFFFFu );
		translator::init_city_names(settings.get_name_language_id());
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

	if(old_scale_factor != get_settings().get_meters_per_tile())
	{
		set_scale();
	}

	grundwasser = settings.get_grundwasser();
DBG_DEBUG("karte_t::laden()","grundwasser %i",grundwasser);
	grund_besch_t::calc_water_level( this, height_to_climate );

	// just an initialisation for the loading
	season = (2+letzter_monat/3)&3; // summer always zero
	snowline = settings.get_winter_snowline() * Z_TILE_STEP + grundwasser;

	DBG_DEBUG("karte_t::laden", "settings loaded (groesse %i,%i) timeline=%i beginner=%i", settings.get_groesse_x(), settings.get_groesse_y(), settings.get_use_timeline(), settings.get_beginner_mode());

	// wird gecached, um den Pointerzugriff zu sparen, da
	// die groesse _sehr_ oft referenziert wird
	cached_groesse_gitter_x = settings.get_groesse_x();
	cached_groesse_gitter_y = settings.get_groesse_y();
	cached_groesse_max = max(cached_groesse_gitter_x,cached_groesse_gitter_y);
	cached_groesse_karte_x = cached_groesse_gitter_x-1;
	cached_groesse_karte_y = cached_groesse_gitter_y-1;
	x_off = y_off = 0;

	// Knightly : initialise the weighted list of distances
	stadt_t::init_distances( shortest_distance( koord(1, 1), koord( get_groesse_x(), get_groesse_y() ) ) );

	// Reliefkarte an neue welt anpassen
	reliefkarte_t::get_karte()->set_welt(this);

	init_felder();

	// reinit pointer with new pointer object and old values
	zeiger = new zeiger_t(this, koord3d::invalid, NULL );

	hausbauer_t::neue_karte();
	fabrikbauer_t::neue_karte(this);

	DBG_DEBUG("karte_t::laden", "init felder ok");

	if(file->get_experimental_version() <= 1)
	{
		uint32 old_ticks = (uint32)ticks;
		file->rdwr_long(old_ticks);
		ticks = (sint64)old_ticks;
	}
	else
	{
		file->rdwr_longlong(ticks);
	}
	file->rdwr_long(letzter_monat);
	file->rdwr_long(letztes_jahr);
	if(file->get_version()<86006) 
	{
		letztes_jahr += umgebung_t::default_einstellungen.get_starting_year();
	}
	// old game might have wrong month
	letzter_monat %= 12;
	// set the current month count
	set_ticks_per_world_month_shift(settings.get_bits_per_month());
	current_month = letzter_monat + (letztes_jahr*12);
	season = (2+letzter_monat/3)&3; // summer always zero
	next_month_ticks = 	( (ticks >> karte_t::ticks_per_world_month_shift) + 1 ) << karte_t::ticks_per_world_month_shift;
	last_step_ticks = ticks;
	steps = 0;
	step_mode = PAUSE_FLAG;

DBG_MESSAGE("karte_t::laden()","savegame loading at tick count %i",ticks);
	recalc_average_speed();	// resets timeline
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

	// rdwr cityrules for networkgames
	if(file->get_version() > 102002 && (file->get_experimental_version() == 0 || file->get_experimental_version() >= 9)) {
		bool do_rdwr = umgebung_t::networkmode;
		file->rdwr_bool(do_rdwr);
		if (do_rdwr) 
		{
			stadt_t::cityrules_rdwr(file);
			if(file->get_experimental_version() >= 9)
			{
				stadt_t::privatecar_rdwr(file);
				stadt_t::electricity_consumption_rdwr(file);
			}
			if(file->get_version()>102003 && (file->get_experimental_version() == 0 || file->get_experimental_version() >= 9)) 
			{
				vehikelbauer_t::rdwr_speedbonus(file);
			}
		}
	}
	DBG_DEBUG("karte_t::laden", "init %i cities", settings.get_anzahl_staedte());
	stadt.clear();
	stadt.resize(settings.get_anzahl_staedte());
	for (int i = 0; i < settings.get_anzahl_staedte(); ++i) {
		stadt_t *s = new stadt_t(this, file);
		stadt.append( s, s->get_einwohner(), 64 );
	}

	DBG_MESSAGE("karte_t::laden()","loading blocks");
	old_blockmanager_t::rdwr(this, file);

	DBG_MESSAGE("karte_t::laden()","loading tiles");
	for (int y = 0; y < get_groesse_y(); y++) {
		for (int x = 0; x < get_groesse_x(); x++) {
			if(file->is_eof()) {
				dbg->fatal("karte_t::laden()","Savegame file mangled (too short)!");
			}
			plan[x+y*cached_groesse_gitter_x].rdwr(this, file, koord(x,y) );
		}
		display_progress(y, get_groesse_y()+stadt.get_count()+256);
	}

	if(file->get_version()<99005) {
		DBG_MESSAGE("karte_t::laden()","loading grid for older versions");
		for (int y = 0; y <= get_groesse_y(); y++) {
			for (int x = 0; x <= get_groesse_x(); x++) {
				sint32 hgt;
				file->rdwr_long(hgt);
				// old height step was 16!
				set_grid_hgt(koord(x, y), (hgt*Z_TILE_STEP)/16 );
			}
		}
	}
	else if(  file->get_version()<=102001  )  {
		// hgt now bytes
		DBG_MESSAGE("karte_t::laden()","loading grid for older versions");
		for( sint32 i=0;  i<(get_groesse_y()+1)*(get_groesse_x()+1);  i++  ) {
			file->rdwr_byte(grid_hgts[i]);
		}
	}
	else {
		// just restore the border as ground level
		// the heightfield is restored in grund_t::rdwr
		// but the south and east border of the map have to be resetted to water level
		DBG_MESSAGE("karte_t::laden()","calculating grid corners");
		for( sint16 i=0;  i<=cached_groesse_gitter_x;  i++  ) {
			grid_hgts[i + cached_groesse_gitter_y*(cached_groesse_gitter_x+1)] = grundwasser;
		}
		for( sint16 j=0;  j<=cached_groesse_gitter_y;  j++  ) {
			grid_hgts[cached_groesse_gitter_x + j*(cached_groesse_gitter_x+1)] = grundwasser;
		}
	}

	if(file->get_version()<88009) {
		DBG_MESSAGE("karte_t::laden()","loading slopes from older version");
		// Hajo: load slopes for older versions
		// now part of the grund_t structure
		for (int y = 0; y < get_groesse_y(); y++) {
			for (int x = 0; x < get_groesse_x(); x++) {
				sint8 slope;
				file->rdwr_byte(slope);
				access(x, y)->get_kartenboden()->set_grund_hang(slope);
			}
		}
	}

	if(file->get_version()<=88000) {
		// because from 88.01.4 on the foundations are handled differently
		for (int y = 0; y < get_groesse_y(); y++) {
			for (int x = 0; x < get_groesse_x(); x++) {
				koord k(x,y);
				grund_t *gr1 = access(x, y)->get_kartenboden();
				if(gr1->get_typ()==grund_t::fundament) {
					gr1->set_hoehe( max_hgt(k) );
					// get new grund
					grund_t *gr2 = access(x, y)->get_kartenboden();
					gr2->set_grund_hang(hang_t::flach);
					// transfer object to on new grund
					for(  int i=0;  i<gr1->get_top();  i++  ) {
						gr1->obj_bei(i)->set_pos( gr2->get_pos() );
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
			fab_list.append(fab);
		}
		else {
			dbg->error("karte_t::laden()","Unknown fabrik skipped!");
		}
		if(i&7) {
			display_progress(get_groesse_y()+(24*i)/fabs, get_groesse_y()+stadt.get_count()+256);
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
	if(file->get_version()>=99008) 
	{
		sint32 halt_count;
		file->rdwr_long(halt_count);
		for(int i=0; i<halt_count; i++) 
		{
			halthandle_t halt = haltestelle_t::create( this, file );
			if(halt->existiert_in_welt())
			{
				halt->get_besitzer()->halt_add(halt);
			}
			else 
			{
				dbg->warning("karte_t::laden()", "could not restore stop near %i,%i", halt->get_init_pos().x, halt->get_init_pos().y );
			}
		}
		DBG_MESSAGE("karte_t::laden()","%d halts loaded",halt_count);
	}

	DBG_MESSAGE("karte_t::laden()", "load convois");
	uint16 convoi_nr = 65535;
	if(  file->get_version()>=101000  ) {
		file->rdwr_short(convoi_nr);
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
	}
DBG_MESSAGE("karte_t::laden()", "%d convois/trains loaded", convoi_array.get_count());

	// jetzt koennen die spieler geladen werden
	display_progress(get_groesse_y()+24+stadt.get_count(), get_groesse_y()+256+stadt.get_count());
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(  spieler[i]  ) {
			spieler[i]->rdwr(file);
			settings.automaten[i] = spieler[i]->is_active();
		}
		else {
			settings.automaten[i] = false;
		}
		display_progress(get_groesse_y()+24+stadt.get_count()+(i*3), get_groesse_y()+256+stadt.get_count());
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
	if(ist_in_kartengrenzen(mi,mj)) {
		change_world_position( koord3d(mi,mj,min_hgt(koord(mi,mj))) );
	}
	else {
		change_world_position( koord3d(mi,mj,0) );
	}

	// right season for recalculations
	recalc_snowline();

DBG_MESSAGE("karte_t::laden()", "%d ways loaded",weg_t::get_alle_wege().get_count());
	for (int y = 0; y < get_groesse_y(); y++) {
		for (int x = 0; x < get_groesse_x(); x++) {
			const planquadrat_t *plan = lookup(koord(x,y));
			const int boden_count = plan->get_boden_count();
			for(int schicht=0; schicht<boden_count; schicht++) {
				grund_t *gr = plan->get_boden_bei(schicht);
				for(int n=0; n<gr->get_top(); n++) {
					ding_t *d = gr->obj_bei(n);
					if(d) {
						d->laden_abschliessen();
					}
				}
				gr->calc_bild();
			}
		}
		display_progress(get_groesse_y()+48+stadt.get_count()+(y*128)/get_groesse_y(), get_groesse_y()+256+stadt.get_count());
	}

	// must finish loading cities first before cleaning up factories
	weighted_vector_tpl<stadt_t*> new_weighted_stadt(stadt.get_count() + 1);
	for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
		stadt_t* s = *i;
		s->laden_abschliessen();
		s->recalc_target_cities();
		new_weighted_stadt.append(s, s->get_einwohner(), 64);
		INT_CHECK("simworld 1278");
	}
	swap(stadt, new_weighted_stadt);
	DBG_MESSAGE("karte_t::laden()", "cities initialized");

	DBG_MESSAGE("karte_t::laden()", "clean up factories");
	ITERATE(fab_list, n)
	{
		fab_list[n]->laden_abschliessen();
	}
	/*slist_iterator_tpl<fabrik_t*> fiter ( fab_list );
	while(fiter.next()) {
		fiter.get_current()->laden_abschliessen();
	}*/

DBG_MESSAGE("karte_t::laden()", "%d factories loaded", fab_list.get_count());

	// old versions did not save factory connections
	if(file->get_version()<99014) {
		sint32 const temp_min = settings.get_factory_worker_minimum_towns();
		sint32 const temp_max = settings.get_factory_worker_maximum_towns();
		// this needs to avoid the first city to be connected to all town
		settings.set_factory_worker_minimum_towns(0);
		settings.set_factory_worker_maximum_towns(stadt.get_count() + 1);
		for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
			(*i)->verbinde_fabriken();
		}
		settings.set_factory_worker_minimum_towns(temp_min);
		settings.set_factory_worker_maximum_towns(temp_max);
	}

	// resolve dummy stops into real stops first ...
	for(  slist_tpl<halthandle_t>::const_iterator i=haltestelle_t::get_alle_haltestellen().begin(); i!=haltestelle_t::get_alle_haltestellen().end();  ++i  ) {
		if(  (*i)->get_besitzer()  &&  (*i)->existiert_in_welt()  ) {
			(*i)->laden_abschliessen();
		}
	}

	// ... before removing dummy stops
	for(  slist_tpl<halthandle_t>::const_iterator i=haltestelle_t::get_alle_haltestellen().begin(); i!=haltestelle_t::get_alle_haltestellen().end();  ) {
		if(  (*i)->get_besitzer()==NULL  ||  !(*i)->existiert_in_welt()  ) {
			// this stop was only needed for loading goods ...
			halthandle_t h = (*i);
			++i;	// goto next
			haltestelle_t::destroy(h);	// remove from list
		}
		else {
			++i;
		}
	}

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
	set_schedule_counter();
	int hnr=0, hmax=haltestelle_t::get_alle_haltestellen().get_count();
	for(  slist_tpl<halthandle_t>::const_iterator i=haltestelle_t::get_alle_haltestellen().begin(); i!=haltestelle_t::get_alle_haltestellen().end();  ++i  ) {
		if((hnr++%64)==0) {
			display_progress(get_groesse_y()+48+stadt.get_count()+128+(hnr*80)/hmax, get_groesse_y()+256+stadt.get_count());
		}
	}
#ifdef DEBUG
	DBG_MESSAGE("rebuild_destinations()","for all haltstellen_t took %ld ms", dr_time()-dt );

	// reroute goods for benchmarking
	dt = dr_time();
#endif
	// reroute_goods needs long time in large game
	// we must resolve all 'Error in routing' here.
	for(  slist_tpl<halthandle_t>::const_iterator i=haltestelle_t::get_alle_haltestellen().begin(); i!=haltestelle_t::get_alle_haltestellen().end();  ++i  ) {
		if((hnr++%64)==0) {
			display_progress(get_groesse_y()+48+stadt.get_count()+128+(hnr*40)/hmax, get_groesse_y()+256+stadt.get_count());
		}
		(*i)->reroute_goods();
	}
#ifdef DEBUG
	DBG_MESSAGE("reroute_goods()","for all haltstellen_t took %ld ms", dr_time()-dt );
#endif

	// load history/create world history
	if(file->get_version()<99018) {
		restore_history();
	}
	else {
		// most recent savegame version is 99018
		for (int year = 0;  year<MAX_WORLD_HISTORY_YEARS;  year++) {
			for (int cost_type = 0; cost_type<MAX_WORLD_COST; cost_type++) {
				file->rdwr_longlong(finance_history_year[year][cost_type]);
			}
		}
		for (int month = 0;month<MAX_WORLD_HISTORY_MONTHS;month++) {
			for (int cost_type = 0; cost_type<MAX_WORLD_COST; cost_type++) {
				file->rdwr_longlong(finance_history_month[month][cost_type]);
			}
		}
		last_month_bev = finance_history_month[1][WORLD_CITICENS];
	}

#if 0
	// preserve tick counter ...
	ticks = ticks % karte_t::ticks_per_world_month;
	next_month_ticks = karte_t::ticks_per_world_month;
	letzter_monat %= 12;
#endif

	// finally: do we run a scenario?
	if(file->get_version()>=99018) {
		scenario->rdwr(file);
	}

	// restore locked state
	for(  uint8 i=0;  i<PLAYER_UNOWNED;  i++  ) {
		if(  spieler[i]  ) {
			spieler[i]->set_unlock( player_password_hash[i] );
		}
	}

	if(file->get_experimental_version() >= 2)
	{
		file->rdwr_short(base_pathing_counter);
	}
	
	if((file->get_experimental_version() >= 7 && file->get_experimental_version() < 9 && file->get_version() < 110006))
	{
		double old_proportion = industry_density_proportion / 10000.0;
		file->rdwr_double(old_proportion);
		industry_density_proportion = old_proportion * 10000.0;
	}
	else if(file->get_experimental_version() >= 9 && file->get_version() >= 110006)
	{
		file->rdwr_long(industry_density_proportion);
	}
	else if(file->is_loading())
	{
		// Reconstruct the actual industry density.
		// @author: jamespetts			
		// Loading a game - must set this to zero here and recalculate.
		actual_industry_density = 0;
		uint32 weight;
		ITERATE(fab_list, i)
		{
			const fabrik_besch_t* factory_type = fab_list[i]->get_besch();
			if(!factory_type->is_electricity_producer())
			{
				// Power stations are excluded from the target weight:
				// a different system is used for them.
				weight = factory_type->get_gewichtung();
				actual_industry_density += (100 / weight);
			}
		}
		industry_density_proportion = ((sint64)actual_industry_density * 10000ll) / finance_history_month[0][WORLD_CITICENS];
	}

	if(file->get_experimental_version() >=9 && file->get_version() >= 110000)
	{
		file->rdwr_byte(next_private_car_update_month);
		
		// Existing values now saved in order to prevent network desyncs
		file->rdwr_long(citycar_speed_average);
		file->rdwr_bool(recheck_road_connexions);
		file->rdwr_short(generic_road_speed_city);
		file->rdwr_short(generic_road_speed_intercity);
		file->rdwr_long(max_road_check_depth);
		if(file->get_experimental_version() < 10)
		{
			double old_density = actual_industry_density / 100.0;
			file->rdwr_double(old_density);
			actual_industry_density = old_density * 100.0;
		}
		else
		{
			file->rdwr_long(actual_industry_density);
		}
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

	// Added by : Knightly
	if ( settings.get_default_path_option() == 2 )
	{
		path_explorer_t::full_instant_refresh();
	}

	clear_random_mode(LOAD_RANDOM);
	
	dbg->warning("karte_t::laden()","loaded savegame from %i/%i, next month=%i, ticks=%i (per month=1<<%i)",letzter_monat,letztes_jahr,next_month_ticks,ticks,karte_t::ticks_per_world_month_shift);
}

// recalcs all ground tiles on the map
void karte_t::update_map()
{
	for(  int i=0;  i<cached_groesse_gitter_x*cached_groesse_gitter_y;  i++  ) {
		const int boden_count = plan[i].get_boden_count();
		for(int schicht=0; schicht<boden_count; schicht++) {
			grund_t *gr = plan[i].get_boden_bei(schicht);
			gr->calc_bild();
		}
	}
	set_dirty();
}


// return an index to a halt (or creates a new one)
// only used during loading
halthandle_t karte_t::get_halt_koord_index(koord k)
{
	if(!ist_in_kartengrenzen(k)) {
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




/**
 * Creates a map from a heightfield
 * @param sets game settings
 * @author Hj. Malthaner
 */
void karte_t::load_heightfield(settings_t* const sets)
{
	sint16 w, h;
	sint8 *h_field;
	if(karte_t::get_height_data_from_file(sets->heightfield.c_str(), sets->get_grundwasser(), h_field, w, h, false )) {
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


static void warte_auf_mausklick_oder_taste(event_t *ev)
{
	do {
		win_get_event(ev);
	}while( !(IS_LEFTRELEASE(ev)  ||  (ev->ev_class == EVENT_KEYBOARD  &&  (ev->ev_code >= 32  &&  ev->ev_code < 256) ) ) );
}



/**
 * marks an area using the grund_t mark flag
 * @author prissi
 */
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
//	ticks += 12*karte_t::ticks_per_world_month;
//	next_month_ticks += 12*karte_t::ticks_per_world_month;
	current_month += 12;
	letztes_jahr ++;
	reset_timer();
	recalc_average_speed();
}



// jump one or more months ahead
// (updating history!)
void karte_t::step_month( sint16 months )
{
	while(  months-->0  ) {
		neuer_monat();
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



void karte_t::do_freeze()
{
	dr_prepare_flush();

	display_fillbox_wh(display_get_width()/2-100, display_get_height()/2-50, 200,100, MN_GREY2, true);
	display_ddd_box(display_get_width()/2-100, display_get_height()/2-50, 200,100, MN_GREY4, MN_GREY0);
	display_ddd_box(display_get_width()/2-92, display_get_height()/2-42, 200-16,100-16, MN_GREY0, MN_GREY4);
	display_proportional(display_get_width()/2, display_get_height()/2-5, translator::translate("GAME PAUSED"), ALIGN_MIDDLE, COL_BLACK, false);

	// Pause: warten auf die naechste Taste
	event_t ev;
	dr_flush();
	warte_auf_mausklick_oder_taste(&ev);
	reset_timer();
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


void karte_t::bewege_zeiger(const event_t *ev)
{
	static int mb_alt=0;

	if(zeiger) {
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

		// berechnung der basis feldkoordinaten in i und j

		/*  this would calculate raster i,j koordinates if there was no height
		*  die formeln stehen hier zur erinnerung wie sie in der urform aussehen

		int base_i = (screen_x+screen_y)/2;
		int base_j = (screen_y-screen_x)/2;

		int raster_base_i = (int)floor(base_i / 16.0);
		int raster_base_j = (int)floor(base_j / 16.0);

		*/

		const int i_off = ij_off.x+get_ansicht_ij_offset().x;
		const int j_off = ij_off.y+get_ansicht_ij_offset().y;

		bool found = false;
		// uncomment to: ctrl-key selects ground
		//bool select_karten_boden = event_get_last_control_shift()==2;

		sint8 hgt; // trial height
		sint8 groff=0; // offset for lower raise tool
		// fallback: take kartenboden if nothing else found
		const grund_t *bd = NULL;
		// for the calculation of hmin/hmax see simview.cc
		// for the definition of underground_level see grund_t::set_underground_mode
		const sint8 hmin = grund_t::underground_mode!=grund_t::ugm_all ? min(grundwasser, grund_t::underground_level) : grundwasser-10;
		const sint8 hmax = grund_t::underground_mode==grund_t::ugm_all ? 32 : min(grund_t::underground_level, 32);

		// find matching and visible grund
		for(hgt = hmax; hgt>=hmin; hgt-=Z_TILE_STEP) {

			const int base_i = (screen_x+screen_y + tile_raster_scale_y((hgt*TILE_HEIGHT_STEP)/Z_TILE_STEP,rw1) )/2;
			const int base_j = (screen_y-screen_x + tile_raster_scale_y((hgt*TILE_HEIGHT_STEP)/Z_TILE_STEP,rw1))/2;

			mi = ((int)floor(base_i/(double)rw4)) + i_off;
			mj = ((int)floor(base_j/(double)rw4)) + j_off;

			const grund_t *gr = lookup(koord3d(mi,mj,hgt));
			if(gr != NULL) {
				found = /*select_karten_boden ? gr->ist_karten_boden() :*/ gr->is_visible();
				if( ( gr->get_typ() == grund_t::tunnelboden || gr->get_typ() == grund_t::monorailboden ) && gr->get_weg_nr(0) == NULL ) {
					// This is only a dummy ground placed by wkz_tunnelbau_t or wkz_wegebau_t as a preview.
					found = false;
				}
				if (found) {
					groff = corner4(gr->get_grund_hang());
					break;
				}

				if (bd==NULL && gr->ist_karten_boden()) {
					bd = gr;
				}
			}
			else if (grund_t::underground_mode==grund_t::ugm_level && hgt==hmax) {
				// fallback in sliced mode, if no ground is under cursor
				bd = lookup_kartenboden(koord(mi,mj));
			}
		}
		// try kartenboden?
		if (!found && bd!=NULL) {
			mi = bd->get_pos().x;
			mj = bd->get_pos().y;
			hgt= bd->get_disp_height();
			groff = bd->is_visible() ? corner4(bd->get_grund_hang()) : 0;
			found = true;
		}
		// no suitable location found (outside map, ...)
		if (!found) {
			return;
		}

		// the new position - extra logic for raise / lower tool
		const koord3d pos = koord3d(mi,mj, hgt + (zeiger->get_yoff()==Z_GRID ? groff : 0));

		// rueckwaerttransformation um die zielkoordinaten
		// mit den mauskoordinaten zu vergleichen
		int neu_x = ((mi-i_off) - (mj-j_off))*rw2 + display_get_width()/2 + rw2;

		// pruefe richtung d.h. welches nachbarfeld ist am naechsten
		if(ev->mx-x_off < neu_x) {
			zeiger->set_richtung(ribi_t::west);
		}
		else {
			zeiger->set_richtung(ribi_t::nord);
		}

		// zeiger bewegen
		const koord3d prev_pos = zeiger->get_pos();
		if(  (prev_pos != pos ||  ev->button_state != mb_alt)  ) {

			mb_alt = ev->button_state;

			zeiger->change_pos(pos);
			werkzeug_t *wkz = werkzeug[get_active_player_nr()];
			if(  !umgebung_t::networkmode  ||  wkz->is_move_network_save(get_active_player())) {
				wkz->flags = event_get_last_control_shift() | werkzeug_t::WFL_LOCAL;
				if(wkz->check( this, get_active_player(), zeiger->get_pos() )==NULL) {
					if(  ev->button_state == 0  ) {
						is_dragging = false;
					}
					else if(ev->ev_class==EVENT_DRAG) {
						if(!is_dragging  &&  wkz->check( this, get_active_player(), prev_pos )==NULL) {
							wkz->move( this, get_active_player(), 1, prev_pos );
							is_dragging = true;
						}
					}
					wkz->move( this, get_active_player(), is_dragging, pos );
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


/* goes to next active player */
void karte_t::switch_active_player(uint8 new_player, bool silent)
{
	// cheat: play as AI
	bool renew_menu=false;

	for(  uint8 i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  spieler[(i+new_player)%MAX_PLAYER_COUNT] != NULL  ) {
			new_player = (i+new_player)%MAX_PLAYER_COUNT;
			break;
		}
	}

	// no cheating allowed?
	if (!settings.get_allow_player_change() && spieler[1]->is_locked()) {
		active_player_nr = 0;
		active_player = spieler[0];
		if(new_player!=0) {
			create_win( new news_img("On this map, you are not\nallowed to change player!\n"), w_time_delete, magic_none);
		}
	}
	else {
		koord3d old_zeiger_pos = zeiger->get_pos();
		zeiger->set_bild( IMG_LEER );	// unmarks also area
		zeiger->set_pos( koord3d::invalid );
		// exit active tool to remove pointers (for two_click_tool_t's, stop mover, factory linker)
		if(werkzeug[active_player_nr]) {
			werkzeug[active_player_nr]->exit(this, active_player);
		}
		renew_menu = (active_player_nr==1  ||  new_player==1);
		active_player_nr = new_player;
		active_player = spieler[new_player];
		if(  !silent  ) {
			// tell the player
			char buf[512];
			sprintf(buf, translator::translate("Now active as %s.\n"), get_active_player()->get_name() );
			msg->add_message(buf, koord::invalid, message_t::ai | message_t::local_flag, PLAYER_FLAG|get_active_player()->get_player_nr(), IMG_LEER);
		}
		zeiger->set_area( koord(1,1), false );
		zeiger->set_pos( old_zeiger_pos );
	}

	// update menue entries (we do not want player1 to run anything)
	if(renew_menu) {
		werkzeug_t::update_toolbars(this);
		set_dirty();
	}

	zeiger->set_bild( werkzeug[active_player_nr]->cursor );
}


void karte_t::interactive_event(event_t &ev)
{
	struct sound_info click_sound;
	click_sound.index = SFX_SELECT;
	click_sound.volume = 255;
	click_sound.pri = 0;

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
						sound_play(click_sound);
						destroy_all_win(false);
					}
					// Ignore Enter and Backspace but not Ctrl-H and Ctrl-M
					break;
				}

			default:
				{
					bool ok=false;
					for (vector_tpl<werkzeug_t *>::const_iterator iter = werkzeug_t::char_to_tool.begin(), end = werkzeug_t::char_to_tool.end(); iter != end; ++iter) {
						if(  (*iter)->command_key==ev.ev_code  ) {
							set_werkzeug( *iter, get_active_player() );
							ok = true;
							break;
						}
					}
					if(!ok) {
						// key help dialoge
						create_win(new help_frame_t("keys.txt"), w_info, magic_keyhelp);
					}
				}
				break;
		}
	}

	if(  IS_LEFTRELEASE(&ev)
		&&  ev.my < display_get_height()-32+(16*ticker::empty())
	) {

		DBG_MESSAGE("karte_t::interactive_event(event_t &ev)", "calling a tool");

		if(ist_in_kartengrenzen(zeiger->get_pos().get_2d())) {
			const char *err = NULL;
			bool result = true;
			werkzeug_t *wkz = werkzeug[get_active_player_nr()];
			// first check for visibility etc
			err = wkz->check( this, get_active_player(), zeiger->get_pos() );
			if (err==NULL) {
				wkz->flags = event_get_last_control_shift();
				if (!umgebung_t::networkmode  ||  wkz->is_work_network_save()  ||  wkz->is_work_here_network_save( this, get_active_player(), zeiger->get_pos() ) ) {
					// do the work
					wkz->flags |= werkzeug_t::WFL_LOCAL;
					err = wkz->work( this, get_active_player(), zeiger->get_pos() );
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


void karte_t::beenden(bool b)
{
	finish_loop = true;
	umgebung_t::quit_simutrans = b;
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
			buf.printf("%02X", (unsigned char)c);
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
		// announce me on server
		if(  umgebung_t::announce_server  ) {
			cbuffer_t buf;
#ifndef REVISION
#	define REVISION 0
#endif
			buf.printf("/serverlist_ex/slist.php?ID=%u&st=on&ip=%s&port=%u&rev=" QUOTEME(REVISION) "&pak=", umgebung_t::announce_server, umgebung_t::server_name.c_str(), umgebung_t::server);
			// announce ak set
			char const* const copyright = grund_besch_t::ausserhalb->get_copyright();
			if (copyright && STRICMP("none", copyright) != 0) {
				// construct from outside object copyright string
				encode_URI(buf, copyright);
			}
			else {
				// construct from pak name
				std::string pak_name = umgebung_t::objfilename;
				pak_name.erase( pak_name.length()-1 );
				buf.append( pak_name.c_str() );
			}
			buf.append( "&name=" );
			// add comment
			encode_URI(buf, umgebung_t::server_comment.c_str());
			network_download_http( ANNOUNCE_SERVER, buf, NULL );
			// and now the details (resetting month for automatic messages)
			server_next_announce_month = 0;
			announce_server();
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
						struct sound_info ambient_sound;
						ambient_sound.index = id;
						ambient_sound.volume = 255;
						ambient_sound.pri = 0;
						sound_play( ambient_sound );
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
					if(follow_convoi.is_bound()) {
						follow_convoi = convoihandle_t();
					}
					blick_aendern(&ev);
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
				bewege_zeiger(&ev);
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

			// Knightly : send changed limits to server where necessary
			if(  settings.get_default_path_option()==2  &&  path_explorer_t::are_local_limits_changed()  ) {
				path_explorer_t::limit_set_t local_limits = path_explorer_t::get_local_limits();
				network_send_server( new nwc_routesearch_t(sync_steps, map_counter, local_limits, false) );
				path_explorer_t::reset_local_limits_state();
				dbg->warning("karte_t::interactive", "nwc_routesearch_t object created and sent to server: sync_step=%u map_counter=%u limits=(%u, %u, %u, %llu, %u)",
					sync_steps, map_counter, local_limits.rebuild_connexions, local_limits.filter_eligible, local_limits.fill_matrix, local_limits.explore_paths, local_limits.reroute_goods);
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
					if(  difftime < 0) {
						// running ahead
						next_step_time -= difftime;
					}
					else {
						// more gentle catching up
						ms_difference = (sint32 )difftime;
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

			// Knightly : check if changed limits, if any, have to be transmitted to all clients
			if(  settings.get_default_path_option()==2  &&  umgebung_t::server  ) {
				nwc_routesearch_t::check_for_transmission( this );
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
						printf("Desync for attempting past execution\n");
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
						printf("Desync due to checklist mismatch\nsync_step=%u  %s", server_sync_step, buf);
						network_disconnect();
#ifdef DEBUG_SIMRAND_CALLS
						ticker::add_msg( buf, koord::invalid, COL_LIGHT_BLUE );
						printf("disconnecting due to checklist mismatch (%s)\n", buf);
#endif
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
								printf("Desync due to checklist mismatch\nsync_step=%u  %s", nwt->last_sync_step, buf);
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
						dbg->message("karte_t::interactive", "time=%lu sync=%d"/*  rand=%d"*/, dr_time(), sync_steps/*, LRAND(sync_steps)*/);
					}
#endif
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

		// monthly announcements
		if(  get_current_month() >= server_next_announce_month  ) {
			announce_server();
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

	if(  umgebung_t::server  &&  umgebung_t::announce_server  ) {
		cbuffer_t buf;
		buf.printf( "/serverlist_ex/slist.php?ID=%u&st=off", umgebung_t::announce_server );
		network_download_http( ANNOUNCE_SERVER, buf, NULL );
	}

	intr_enable();
	display_show_pointer(true);
	return finish_loop;
#undef LRAND
}


// if announce_server has a valid ID, it will be announced on the list
void karte_t::announce_server()
{
	// announce game info to server, format is
	// gd=time3.1923:size256x256:Player2:locked2:Clients1:Towns15:citicens3245:Factories33:Convoys56:Stops17
	if(  umgebung_t::announce_server  ) {
		// now send the status
		cbuffer_t buf;
		buf.printf( "/serverlist_ex/map.php?ID=%u", umgebung_t::announce_server );
		buf.printf( "&gd=time%u.%u:size%ux%u:", (get_current_month()%12)+1, get_current_month()/12, get_groesse_x(), get_groesse_y() );
		uint8 player=0, locked = 0;
		for(  uint8 i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
			if(  spieler[i]  &&  spieler[i]->get_ai_id()!=spieler_t::EMPTY  ) {
				player ++;
				if(  spieler[i]->is_locked()  ) {
					locked ++;
				}
			}
		}
		last_clients = socket_list_t::get_playing_clients();
		buf.printf( "Players%u:locked%u:Clients%u:", player, locked, last_clients );
		buf.printf( "Towns%u:citicens%u:Factories%u:Convoys%u:Stops%u", stadt.get_count(), stadt.get_sum_weight(), fab_list.get_count(), get_convoi_count(), haltestelle_t::get_alle_haltestellen().get_count() );
		network_download_http( ANNOUNCE_SERVER, buf, NULL );
		if(  umgebung_t::announce_server_intervall > 0  ) {
			server_next_announce_month = current_month + umgebung_t::announce_server_intervall;
		}
		else {
			server_next_announce_month = 0xFFFFFFFFu;
		}
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

	beenden(false);

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
	if(stadtauto_t::table.empty())
	{
		// No city cars - use default speed.
		citycar_speed_average = 50;
		return;
	}
	stringhashtable_iterator_tpl<const stadtauto_besch_t*> iter(&stadtauto_t::table);
	sint32 vehicle_speed_sum = 0;
	sint32 count = 0;
	while(iter.next())
	{
		// Take into account the *chance* of vehicles, too: fewer people have sports cars than Minis. 
		vehicle_speed_sum += (speed_to_kmh(iter.get_current_value()->get_geschw())) * iter.get_current_value()->get_gewichtung();
		count += iter.get_current_value()->get_gewichtung();
	}
	citycar_speed_average = vehicle_speed_sum / count;
}

void karte_t::calc_generic_road_speed_intercity()
{
	// This method is used only when private car connexion
	// checking is turned off.
	
	// Adapted from the method used to build city roads in the first place, written by Hajo.
	const weg_besch_t* besch = settings.get_intercity_road_type(get_timeline_year_month());
	if(besch == NULL) 
	{
		// Hajo: try some default (might happen with timeline ... )
		besch = wegbauer_t::weg_search(road_wt,80,get_timeline_year_month(),weg_t::type_flat);
	}
	generic_road_speed_intercity = (uint16)calc_generic_road_speed(besch);
}

sint32 karte_t::calc_generic_road_speed(const weg_besch_t* besch)
{
	sint32 speed_average = citycar_speed_average;
	if(besch)
	{
		const sint32 road_speed_limit = besch->get_topspeed();
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
	
	return ((6 * 15) * settings.get_meters_per_tile()) /  (speed_average * 100);
}

void karte_t::calc_max_road_check_depth()
{
	sint32 max_road_speed = 0;
	stringhashtable_tpl <weg_besch_t *> * ways = wegbauer_t::get_all_ways();

	if(ways != NULL)
	{
		stringhashtable_iterator_tpl <weg_besch_t *> iter(ways);
		while(iter.next())
		{
			if(iter.get_current_value()->get_wtyp() != road_wt || iter.get_current_value()->get_intro_year_month() > current_month || iter.get_current_value()->get_retire_year_month() > current_month)
			{
				continue;
			}
			if(iter.get_current_value()->get_topspeed() > max_road_speed)
			{
				max_road_speed = iter.get_current_value()->get_topspeed();
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
	max_road_check_depth = ((uint32)settings.get_max_longdistance_tolerance() * 100) / (settings.get_meters_per_tile() * 6) * min(citycar_speed_average, max_road_speed);
}
