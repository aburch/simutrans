/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Hauptklasse fuer Simutrans, Datenstruktur die alles Zusammenhaelt
 * Hansjoerg Malthaner, 1997
 */

#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "path_explorer.h"
#ifdef _MSC_VER
#include <direct.h>
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
#include "simtools.h"
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

#include "gui/messagebox.h"
#include "gui/help_frame.h"
#include "gui/karte.h"

#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "dataobj/scenario.h"
#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
#include "dataobj/tabfile.h"
#include "dataobj/powernet.h"
#include "dataobj/network.h"

#include "utils/simstring.h"

#include "bauer/brueckenbauer.h"
#include "bauer/tunnelbauer.h"
#include "bauer/fabrikbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"
#include "bauer/vehikelbauer.h"

#include "besch/grund_besch.h"
#include "besch/sound_besch.h"
#include "besch/tunnel_besch.h"

#include "player/simplay.h"
#include "player/ai_passenger.h"
#include "player/ai_goods.h"



//#define DEMO
//#undef DEMO

// advance 201 ms per sync_step in fast forward mode
#define MAGIC_STEP (201)

// frame per second for fast forward
#define FF_PPS (10)


static bool is_dragging = false;



// changes the snowline height (for the seasons)
bool karte_t::recalc_snowline()
{
	static int mfactor[12] = { 99, 95, 80, 50, 25, 10, 0, 5, 20, 35, 65, 85 };
	static uint8 month_to_season[12] = { 2, 2, 2, 3, 3, 0, 0, 0, 0, 1, 1, 2 };

	// calculate snowline with day precicion
	// use linear interpolation
	const long ticks_this_month = get_zeit_ms() & (karte_t::ticks_per_tag-1);
	const long faktor = mfactor[letzter_monat] + (  ( (mfactor[(letzter_monat+1)%12]-mfactor[letzter_monat])*(ticks_this_month>>12) ) >> (karte_t::ticks_bits_per_tag-12) );

	// just remember them
	const sint16 old_snowline = snowline;
	const sint16 old_season = season;

	// and calculate new values
	season=month_to_season[letzter_monat];   //  (2+letzter_monat/3)&3; // summer always zero
	const int winterline = einstellungen->get_winter_snowline();
	const int summerline = einstellungen->get_climate_borders()[arctic_climate]+1;
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
#ifdef BIG_ENDIAN
			uint32 l;
			uint16 s;
			fread( &l, 4, 1, file );
			data_offset = endian_uint32(&l);
			fseek( file, 18, SEEK_SET );
			fread( &l, 4, 1, file );
			w = (sint32)endian_uint32(&l);
			fread( &l, 4, 1, file );
			h = (sint32)endian_uint32(&l);
			fseek( file, 28, SEEK_SET );
			fread( &s, 2, 1, file );
			bit_depth = (sint16)endian_uint16(&s);
			fread( &l, 4, 1, file );
			format = (sint32)endian_uint32(&l);
			fseek( file, 46, SEEK_SET );
			fread( &l, 4, 1, file );
			table = (sint32)endian_uint32(&l);
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
					read_line(buf, 255, file);
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
sint32 karte_t::perlin_hoehe( einstellungen_t *sets, koord k, koord size )
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

	// now lower the corners to ground level
	for(i=0; i<get_groesse_x(); i++) {
		lower_to(i, 0, grundwasser,false);
		lower_to(i, get_groesse_y(), grundwasser,false);
	}
	for(i=0; i<=get_groesse_y(); i++) {
		lower_to(0, i, grundwasser,false);
		lower_to(get_groesse_x(), i, grundwasser,false);
	}
	for(i=0; i<=get_groesse_x(); i++) {
		raise_to(i, 0, grundwasser,false);
		raise_to(i, get_groesse_y(), grundwasser,false);
	}
	for(i=0; i<=get_groesse_y(); i++) {
		raise_to(0, i, grundwasser,false);
		raise_to(get_groesse_x(), i, grundwasser,false);
	}

	// recalculate slopes and water tiles
	for(  j=0;  j<get_groesse_y();  j++  ) {
		for(  i=(j>=yoff)?0:xoff;  i<get_groesse_x();  i++  ) {
			planquadrat_t *pl = access(i,j);
			grund_t *gr = pl->get_kartenboden();
			koord k(i,j);
			uint8 slope = calc_natural_slope(k);
			gr->set_pos(koord3d(k,min_hgt(k)));
			if(  gr->get_typ()!=grund_t::wasser  &&  max_hgt(k) <= get_grundwasser()  ) {
				// below water but ground => convert
				pl->kartenboden_setzen(new wasser_t(this, gr->get_pos()) );
			}
			else if(  gr->get_typ()==grund_t::wasser  &&  max_hgt(k) > get_grundwasser()  ) {
				// water above ground => to ground
				pl->kartenboden_setzen(new boden_t(this, gr->get_pos(), slope ) );
			}
			else {
				gr->set_grund_hang( slope );
			}
			pl->get_kartenboden()->calc_bild();
		}
	}
}



void karte_t::destroy()
{
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

	// alle convois aufräumen
	while (!convoi_array.empty()) {
		convoihandle_t cnv = convoi_array.back();
		cnv->destroy();
	}
	convoi_array.clear();
DBG_MESSAGE("karte_t::destroy()", "convois destroyed");

	// alle haltestellen aufräumen
	haltestelle_t::destroy_all(this);
DBG_MESSAGE("karte_t::destroy()", "stops destroyed");

	// delete towns first (will also delete all their houses)
	// for the next game we need to remember the desired number ...
	sint32 no_of_cities=einstellungen->get_anzahl_staedte();
	while (!stadt.empty()) {
		rem_stadt(stadt.front());
	}
	einstellungen->set_anzahl_staedte(no_of_cities);
DBG_MESSAGE("karte_t::destroy()", "towns destroyed");

	while(!sync_list.empty()) {
		sync_steppable *ss = sync_list.remove_first();
		delete ss;
	}
	// entfernt alle synchronen objekte aus der liste
	sync_list.clear();
DBG_MESSAGE("karte_t::destroy()", "sync list cleared");

// dinge aufräumen
	cached_groesse_gitter_x = cached_groesse_gitter_y = 1;
	cached_groesse_karte_x = cached_groesse_karte_y = 0;
	if(plan) {
		delete [] plan;
		plan = NULL;
	}
	DBG_MESSAGE("karte_t::destroy()", "planquadrat destroyed");

	// gitter aufräumen
	if(grid_hgts) {
		delete [] grid_hgts;
		grid_hgts = NULL;
	}

	// marker aufräumen
	marker.init(0,0);
DBG_MESSAGE("karte_t::destroy()", "marker destroyed");

	// spieler aufräumen
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(spieler[i]) {
			delete spieler[i];
			spieler[i] = NULL;
		}
	}
DBG_MESSAGE("karte_t::destroy()", "player destroyed");

	// clear all ids
	simlinemgmt_t::init_line_ids();
DBG_MESSAGE("karte_t::destroy()", "lines destroyed");

	// alle fabriken aufräumen
	// "all factories clear up" (Babelfish)
	//slist_iterator_tpl<fabrik_t*> fab_iter(fab_list);
	//while(fab_iter.next()) {
	//for(sint16 i = fab_list.get_count() - 1; i >= 0; i --)
	ITERATE(fab_list, i)
	{
		//delete fab_iter.get_current();
		delete fab_list[i];
	}
	fab_list.clear();
DBG_MESSAGE("karte_t::destroy()", "factories destroyed");

	// hier nur entfernen, aber nicht löschen
	ausflugsziele.clear();
DBG_MESSAGE("karte_t::destroy()", "attraction list destroyed");

	delete scenario;
	scenario = NULL;


	bool empty_depot_list = depot_t::get_depot_list().empty();
	assert( empty_depot_list );

DBG_MESSAGE("karte_t::destroy()", "world destroyed");
	printf("World destroyed.\n");

	msg->clear();
	
	is_shutting_down = false;

	// Added by : Knightly
	path_explorer_t::finalise();
}



void karte_t::add_convoi(convoihandle_t &cnv)
{
	assert(cnv.is_bound());
	convoi_array.append_unique(cnv);
	cnv->get_besitzer()->buche( 1, COST_ALL_CONVOIS );
}



void karte_t::rem_convoi(convoihandle_t& cnv)
{
	convoi_array.remove(cnv);
	cnv->get_besitzer()->buche( -1, COST_ALL_CONVOIS );
}

/**
 * Zugriff auf das Städte Array.
 * Access to the cities array.
 * @author Hj. Malthaner
 */
const stadt_t *karte_t::get_random_stadt() const
{
	return stadt.at_weight(simrand(stadt.get_sum_weight()));
}

void karte_t::add_stadt(stadt_t *s)
{
	einstellungen->set_anzahl_staedte(einstellungen->get_anzahl_staedte()+1);
	stadt.append(s, s->get_einwohner(), 64);
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
	if(s->get_name()) { DBG_MESSAGE("karte_t::rem_stadt()", s->get_name() ); }
	stadt.remove(s);
	DBG_MESSAGE("karte_t::rem_stadt()", "reduce city to %i", einstellungen->get_anzahl_staedte()-1 );
	einstellungen->set_anzahl_staedte(einstellungen->get_anzahl_staedte()-1);

	// remove all links from factories
	DBG_MESSAGE("karte_t::rem_stadt()", "fab_list %i", fab_list.get_count() );
	//slist_iterator_tpl<fabrik_t *> iter(fab_list);
	//while(iter.next()) {
	for(sint16 i = fab_list.get_count() - 1; i >= 0; i --)
	{
		//(iter.get_current())->remove_arbeiterziel(s);
		fab_list[i]->remove_arbeiterziel(s);
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

	plan = new planquadrat_t[get_groesse_x()*get_groesse_y()];
	grid_hgts = new sint8[(get_groesse_x()+1)*(get_groesse_y()+1)];

	memset(grid_hgts, 0, sizeof(sint8)*(get_groesse_x()+1)*(get_groesse_y()+1));

	marker.init(get_groesse_x(),get_groesse_y());

	simlinemgmt_t::init_line_ids();

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

	sint8 last_height = 1;
	koord last_koord(0,0);
	const sint16 max_dist = cached_groesse_karte_y+cached_groesse_karte_x;

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
			else if(  h>=last_height  ||  koord_distance(last_koord,k)>simrand(max_dist)  ) {
				// something worth to add here
				if(  h>last_height  ) {
					last_height = h;
				}
				last_koord = k;
				mountain_tiles.append( k, h, 256 );
			}
		}
	}
	if(  water_tiles.get_count() == 0  ) {
		dbg->message("karte_t::create_rivers()","There aren't any water tiles!\n");
		return;
	}

	// now make rivers
	uint8 retrys = 0;
	while(  number>0  &&  mountain_tiles.get_count()>0  &&  retrys++<100  ) {
		koord start = mountain_tiles.at_weight( simrand(mountain_tiles.get_sum_weight()) );
		koord end = water_tiles[ simrand(water_tiles.get_count()) ];
		sint16 dist = koord_distance(start,end);
		if(  dist > einstellungen->get_min_river_length()  &&  dist < einstellungen->get_max_river_length()  ) {
			// should be at least of decent length
			wegbauer_t riverbuilder(this, spieler[1]);
			riverbuilder.route_fuer(wegbauer_t::river, river_besch);
			riverbuilder.set_maximum( dist*50 );
			riverbuilder.calc_route( lookup_kartenboden(end)->get_pos(), lookup_kartenboden(start)->get_pos() );
			if(  riverbuilder.get_count() >= (uint32)einstellungen->get_min_river_length()  ) {
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



void karte_t::distribute_groundobjs_cities(int new_anzahl_staedte, sint16 old_x, sint16 old_y)
{
DBG_DEBUG("karte_t::distribute_groundobjs_cities()","distributing groundobjs");

	if(  umgebung_t::river_types>0  &&  einstellungen->get_river_number()>0  ) {
		create_rivers( einstellungen->get_river_number() );
	}

	if(  umgebung_t::ground_object_probability > 0  ) {
		// add eyecandy like rocky, moles, flowers, ...
		koord k;
		sint32 queried = simrand(umgebung_t::ground_object_probability*2);
		for(  k.y=0;  k.y<get_groesse_y();  k.y++  ) {
			for(  k.x=(k.y<old_y)?old_x:0;  k.x<get_groesse_x();  k.x++  ) {
				grund_t *gr = lookup_kartenboden(k);
				if(  gr->get_typ()==grund_t::boden  &&  !gr->hat_wege()  ) {
					queried --;
					if(  queried<0  ) {
						const groundobj_besch_t *besch = groundobj_t::random_groundobj_for_climate( get_climate(gr->get_hoehe()), gr->get_grund_hang() );
						if(besch) {
							queried = simrand(umgebung_t::ground_object_probability*2);
							gr->obj_add( new groundobj_t( this, gr->get_pos(), besch ) );
						}
					}
				}
			}
		}
	}

printf("Creating cities ...\n");
DBG_DEBUG("karte_t::distribute_groundobjs_cities()","prepare cities");
	vector_tpl<koord> *pos = stadt_t::random_place(this, new_anzahl_staedte, old_x, old_y);

	if(  !pos->empty()  ) {
		const uint32 old_anzahl_staedte = stadt.get_count();
		new_anzahl_staedte = pos->get_count();

		// prissi if we could not generate enough positions ...
		einstellungen->set_anzahl_staedte( old_anzahl_staedte + new_anzahl_staedte );	// new number of towns (if we did not found enough positions) ...
		int old_progress = 16;
		const int max_display_progress=16+2*einstellungen->get_anzahl_staedte()
					+2*new_anzahl_staedte+((old_x==0)?einstellungen->get_land_industry_chains():0);

		// Ansicht auf erste Stadt zentrieren
		if (old_x+old_y == 0)
			change_world_position( koord3d((*pos)[0], min_hgt((*pos)[0])) );

		// Loop only new cities:
#ifdef DEBUG
		uint32 tbegin = dr_time();
#endif
		for(  int i=0;  i<new_anzahl_staedte;  i++  ) {
//			int citizens=(int)(einstellungen->get_mittlere_einwohnerzahl()*0.9);
//			citizens = citizens/10+simrand(2*citizens+1);
			int current_citicens = (2500l * einstellungen->get_mittlere_einwohnerzahl()) /(simrand(20000)+100);
			stadt_t* s = new stadt_t(spieler[1], (*pos)[i], current_citicens);
DBG_DEBUG("karte_t::distribute_groundobjs_cities()","Erzeuge stadt %i with %ld inhabitants",i,(s->get_city_history_month())[HIST_CITICENS] );
			stadt.append(s, s->get_einwohner(), 64);
			if(is_display_init()) {
				old_progress ++;
				display_progress(old_progress, max_display_progress);
			}
			else {
				printf("*");fflush(NULL);
			}
		}

		delete pos;
DBG_DEBUG("karte_t::distribute_groundobjs_cities()","took %lu ms for all towns", dr_time()-tbegin );

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
		const weg_besch_t* besch = umgebung_t::intercity_road_type ? wegbauer_t::get_besch(umgebung_t::intercity_road_type) : NULL;
		if(besch == 0) {
			dbg->warning("karte_t::init()", "road type '%s' not found", umgebung_t::intercity_road_type);
			// Hajo: try some default (might happen with timeline ... )
			besch = wegbauer_t::weg_search(road_wt,80,get_timeline_year_month(),weg_t::type_flat);
		}

		wegbauer_t bauigel (this, spieler[1] );
		bauigel.route_fuer(wegbauer_t::strasse, besch, tunnelbauer_t::find_tunnel(road_wt,15,get_timeline_year_month()), brueckenbauer_t::find_bridge(road_wt,15,get_timeline_year_month()) );
		bauigel.set_keep_existing_ways(true);
		bauigel.set_maximum(umgebung_t::intercity_road_length);

		// **** intercity road construction
		// progress bar data
		int old_progress_count = 16+2*new_anzahl_staedte;
		int count = 0;
		const int max_count=(einstellungen->get_anzahl_staedte()*(einstellungen->get_anzahl_staedte()-1))/2
					- (old_anzahl_staedte*(old_anzahl_staedte-1))/2;
		// something to do??
		if(  max_count > 0  ) {
			// print("Building intercity roads ...\n");
			// find townhall of city i and road in front of it
			vector_tpl<koord3d> k;
			for(  int i = 0;  i < einstellungen->get_anzahl_staedte();  i++  ) {
				koord k1 = stadt[i]->get_pos();
				const gebaeude_t* gb = dynamic_cast<gebaeude_t*>(lookup_kartenboden(k1)->first_obj());
				if(  gb  &&  gb->ist_rathaus()  ) {
					k1.y += gb->get_tile()->get_besch()->get_h(gb->get_tile()->get_layout());
					k.append( lookup_kartenboden(k1)->get_pos() );
				}
				else {
					k.append( koord3d::invalid );
				}
			}
			// compute all distances
			uint8 conn_comp=1; // current connection component for phase 0
			vector_tpl<uint8> city_flag; // city already connected to the graph? >0 nr of connection component
			array2d_tpl<sint32> city_dist(einstellungen->get_anzahl_staedte(), einstellungen->get_anzahl_staedte());
			for(  sint32 i = 0;  i < einstellungen->get_anzahl_staedte();  i++  ) {
				city_dist.at(i,i) = 0;
				for(  sint32 j = i + 1;  j < einstellungen->get_anzahl_staedte();  j++  ) {
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
					int progress_count = 16+ 2*new_anzahl_staedte+ (count*einstellungen->get_anzahl_staedte()*2)/max_count;
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
			fahrer_t* test_driver;
			vehikel_besch_t test_drive_besch(road_wt, 500, vehikel_besch_t::diesel );
			test_driver = vehikelbauer_t::baue(koord3d(), spieler[1], NULL, &test_drive_besch);

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
					for(  int i = 0;  i < einstellungen->get_anzahl_staedte();  i++  ) {
						if(  city_flag[i] == conn_comp  ) {
							// loop over all connections to connected cities
							for(  int j = old_anzahl_staedte; j < einstellungen->get_anzahl_staedte(); j++) {
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
						for(  int i = old_anzahl_staedte;  i < einstellungen->get_anzahl_staedte();  i++  ) {
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
					for(  int i = 0;  i < einstellungen->get_anzahl_staedte();  i++  ) {
						for(  int j = max(old_anzahl_staedte, i+1);  j < einstellungen->get_anzahl_staedte();  j++  ) {
							if(  city_dist.at(i,j) < best  &&  city_flag[i] == city_flag[j]  ) {
								bool ok = true;
								// is there a connection i..l..j ? forbid stumpfe winkel
								for(  int l = 0;  l < einstellungen->get_anzahl_staedte();  l++  ) {
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
						if(  2*verbindung.get_count() > city_dist.at(conn)  ) {
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
							for(  int i = 0;  i < einstellungen->get_anzahl_staedte();  i++  ) {
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
					int progress_count = 16+ 2*new_anzahl_staedte+ (count*einstellungen->get_anzahl_staedte()*2)/max_count;
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
		einstellungen->set_anzahl_staedte( stadt.get_count() );	// new number of towns (if we did not found enough positions) ...
	}

DBG_DEBUG("karte_t::distribute_groundobjs_cities()","distributing movingobjs");
	if(  umgebung_t::moving_object_probability > 0  ) {
		// add animals and so on (must be done after growing and all other objects, that could change ground coordinates)
		koord k;
		sint32 queried = simrand(umgebung_t::moving_object_probability*2);
		// no need to test the borders, since they are mostly slopes anyway
		for(k.y=1; k.y<get_groesse_y()-1; k.y++) {
			for(k.x=(k.y<old_y)?old_x:1; k.x<get_groesse_x()-1; k.x++) {
				grund_t *gr = lookup_kartenboden(k);
				if(gr->get_top()==0  &&  gr->get_typ()==grund_t::boden  &&  gr->get_grund_hang()==hang_t::flach) {
					queried --;
					if(  queried<0  ) {
						const groundobj_besch_t *besch = movingobj_t::random_movingobj_for_climate( get_climate(gr->get_hoehe()) );
						if(besch  &&  (besch->get_speed()==0  ||  (besch->get_waytype()!=water_wt  ||  gr->hat_weg(water_wt)  ||  gr->get_hoehe()<=get_grundwasser()) ) ) {
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



void karte_t::init(einstellungen_t* sets, sint8 *h_field)
{
	set_random_allowed( true );
	mute_sound(true);

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

	*einstellungen = *sets;
	// names during creation time
	einstellungen->set_name_language_iso( umgebung_t::language_iso );
	einstellungen->set_use_timeline( einstellungen->get_use_timeline()&1 );

	x_off = y_off = 0;

	ticks = 0;
	last_step_ticks = ticks;
	schedule_counter = 0;
	// ticks = 0x7FFFF800;  // Testing the 31->32 bit step

	letzter_monat = 0;
	letztes_jahr = einstellungen->get_starting_year();//umgebung_t::starting_year;
	current_month = letzter_monat + (letztes_jahr*12);
	set_ticks_bits_per_tag(einstellungen->get_bits_per_month());
	next_month_ticks =  karte_t::ticks_per_tag;
	season=(2+letzter_monat/3)&3; // summer always zero
	snowline = sets->get_winter_snowline()*Z_TILE_STEP + grundwasser;
	is_dragging = false;
	steps = 0;
	recalc_average_speed();	// resets timeline

	grundwasser = sets->get_grundwasser();      //29-Nov-01     Markus Weber    Changed
	grund_besch_t::calc_water_level( this, height_to_climate );
	snowline = sets->get_winter_snowline()*Z_TILE_STEP;

	if(sets->get_beginner_mode()) {
		warenbauer_t::set_multiplier( get_einstellungen()->get_starting_year() );
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
	enlarge_map(this->einstellungen, h_field);

DBG_DEBUG("karte_t::init()","distributing trees");
	if(!umgebung_t::no_tree) {
		baum_t::distribute_trees(this,3);
	}
	else {
		umgebung_t::no_tree = false;
	}

DBG_DEBUG("karte_t::init()","built timeline");
	stadtauto_t::built_timeline_liste(this);

	nosave_warning = nosave = false;

	fabrikbauer_t::neue_karte(this);
	// new system ...
	const int max_display_progress=16+einstellungen->get_anzahl_staedte()*4+einstellungen->get_land_industry_chains();
	for(  sint32 i=0;  i<einstellungen->get_land_industry_chains();  i++  ) {
		fabrikbauer_t::increase_industry_density( this, false );
		int progress_count = 16 + einstellungen->get_anzahl_staedte()*4 + i;
		display_progress(progress_count, max_display_progress );
	}
	finance_history_year[0][WORLD_FACTORIES] = finance_history_month[0][WORLD_FACTORIES] = fab_list.get_count();

	// tourist attractions
	fabrikbauer_t::verteile_tourist(this, einstellungen->get_tourist_attractions());

	printf("Preparing startup ...\n");
	if(zeiger == 0) {
		zeiger = new zeiger_t(this, koord3d::invalid, NULL );
	}

	// finishes the line preparation and sets id 0 to invalid ...
	spieler[0]->simlinemgmt.laden_abschliessen();

	set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE], get_active_player() );

	recalc_average_speed();

#ifndef DEMO
	for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
		if(  spieler[i]  ) {
			spieler[i]->set_active( einstellungen->automaten[i] );
		}
	}
#endif

	active_player_nr = 0;
	active_player = spieler[0];

	set_dirty();
	step_mode = PAUSE_FLAG;
	simloops = 60;
	reset_timer();

	if(is_display_init()) {
		display_show_pointer(true);
	}
	mute_sound(false);

	// Added by : Knightly
	if (einstellungen->get_default_path_option() == 2)
	{
		path_explorer_t::full_instant_refresh();
	}

	// Set the actual industry density and industry density proportion
	if(actual_industry_density <= 0)
	{
		double weight;
		ITERATE(fab_list, i)
		{
			const fabrik_besch_t* factory_type = fab_list[i]->get_besch();
			if(!factory_type->is_electricity_producer())
			{
				// Power stations are excluded from the target weight:
				// a different system is used for them.
				weight = factory_type->get_gewichtung();
				actual_industry_density += (1.0 / weight);
			}
		}
		industry_density_proportion = actual_industry_density / finance_history_month[0][WORLD_CITICENS];
	}
}


void karte_t::enlarge_map(einstellungen_t* sets, sint8 *h_field)
{
	sint16 new_groesse_x = sets->get_groesse_x();
	sint16 new_groesse_y = sets->get_groesse_y();
	planquadrat_t *new_plan = new planquadrat_t[new_groesse_x*new_groesse_y];
	sint8 *new_grid_hgts = new sint8[(new_groesse_x+1)*(new_groesse_y+1)];

	memset(new_grid_hgts, grundwasser, sizeof(sint8)*(new_groesse_x+1)*(new_groesse_y+1));

	sint16 old_x = cached_groesse_gitter_x;
	sint16 old_y = cached_groesse_gitter_y;

	einstellungen->set_groesse_x(new_groesse_x);
	einstellungen->set_groesse_y(new_groesse_y);
	cached_groesse_gitter_x = new_groesse_x;
	cached_groesse_gitter_y = new_groesse_y;
	cached_groesse_max = max(cached_groesse_gitter_x,cached_groesse_gitter_y);
	cached_groesse_karte_x = cached_groesse_gitter_x-1;
	cached_groesse_karte_y = cached_groesse_gitter_y-1;

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
		delete [] plan;
		delete [] grid_hgts;

		display_set_progress_text(translator::translate("enlarge map"));
		max_display_progress = 16 + sets->get_anzahl_staedte()*2 + stadt.get_count()*4;
	}
	else {
		max_display_progress = 16 + sets->get_anzahl_staedte()*4 + einstellungen->get_land_industry_chains();
	}

	plan = new_plan;
	grid_hgts = new_grid_hgts;

	display_progress(0,max_display_progress);
	setsimrand( 0xFFFFFFFF, einstellungen->get_karte_nummer() );
	if(  old_x==0  &&  einstellungen->heightfield.len() > 0  ){
		// init from file
		const int display_total = 16 + get_einstellungen()->get_anzahl_staedte()*4 + get_einstellungen()->get_land_industry_chains();

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
		if(  sets->get_rotation()==0  ) {
			// otherwise neagtive offsets may occur, so we cache only non-rotated maps
			init_perlin_map(new_groesse_x,new_groesse_y);
		}
		int next_progress, old_progress = 0;
		// loop only new tiles:
		for(  sint16 x = 0;  x<=new_groesse_x;  x++  ) {
			for(  sint16 y = (x>=old_x)?0:old_y;  y<=new_groesse_y;  y++  ) {
				koord pos(x,y);
				const sint16 h = perlin_hoehe( einstellungen, pos, koord(old_x,old_y) );
				set_grid_hgt( pos, h*Z_TILE_STEP);
			}
			next_progress = (x*16)/new_groesse_x;
			if ( next_progress > old_progress ){
				old_progress = next_progress;
				display_progress(old_progress, max_display_progress);
			}
		}
		exit_perlin_map();
		// now lower the corners and edge between new/old part to ground level
		sint16 i;
		for(  i=0;  i<=get_groesse_x();  i++  ) {
			lower_to(i, i<old_x?old_y:0,grundwasser,false);
			lower_to(i, get_groesse_y(), grundwasser,false);
		}
		for(  i=0;  i<=get_groesse_y();  i++  ) {
			lower_to(i<old_y?old_x:0, i,grundwasser,false);
			lower_to(get_groesse_x(), i, grundwasser,false);
		}
		for(i=0; i<=get_groesse_x(); i++) {
			raise_to(i, i<old_x?old_y:0,grundwasser,false);
			raise_to(i, get_groesse_y(), grundwasser,false);
		}
		for(i=0; i<=get_groesse_y(); i++) {
			raise_to(i<old_y?old_x:0, i,grundwasser,false);
			raise_to(get_groesse_x(), i, grundwasser,false);
		}
		raise_to(old_x, old_y, grundwasser,false);
		lower_to(old_x, old_y, grundwasser,false);
	}

//	int old_progress = 16;

	for (sint16 ix = 0; ix<new_groesse_x; ix++) {
		for (sint16 iy = (ix>=old_x)?0:old_y; iy<new_groesse_y; iy++) {
			koord k = koord(ix,iy);
			access(k)->kartenboden_setzen(new boden_t(this, koord3d(ix,iy,min_hgt(k)),0));
			access(k)->abgesenkt(this);
			grund_t *gr = lookup_kartenboden(k);
			gr->set_grund_hang(calc_natural_slope(k));
		}
	}

	// smoothing the seam (if possible)
	for (sint16 x=1; x<old_x; x++) {
		koord k(x,old_y);
		const sint16 height = perlin_hoehe( einstellungen, k, koord(old_x,old_y) )*Z_TILE_STEP;
		// need to raise/lower more
		for(  sint16 dy=-abs(grundwasser-height);  dy<abs(grundwasser-height);  dy++  ) {
			koord pos(x,old_y+dy);
			const sint16 height = perlin_hoehe( einstellungen, pos, koord(old_x,old_y) )*Z_TILE_STEP;
			while(lookup_hgt(pos)<height  &&  raise(pos)) ;
			while(lookup_hgt(pos)>height  &&  lower(pos)) ;
		}
	}
	for (sint16 y=1; y<old_y; y++) {
		koord k(old_x,y);
		const sint16 height = perlin_hoehe( einstellungen, k, koord(old_x,old_y) )*Z_TILE_STEP;
		// need to raise/lower more
		for(  sint16 dx=-abs(grundwasser-height);  dx<abs(grundwasser-height);  dx++  ) {
			koord pos(old_x+dx,y);
			const sint16 height = perlin_hoehe( einstellungen, pos, koord(old_x,old_y) )*Z_TILE_STEP;
			while(lookup_hgt(pos)<height  &&  raise(pos)) ;
			while(lookup_hgt(pos)>height  &&  lower(pos)) ;
		}
	}

	// new recalc the images of the old map near the seam ...
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

	cleanup_karte( old_x, old_y );

	// eventuall update origin
	switch( einstellungen->get_rotation() ) {
		case 1:
			einstellungen->set_origin_y( einstellungen->get_origin_y()-new_groesse_y+old_y );
			break;
		case 2:
			einstellungen->set_origin_x( einstellungen->get_origin_x()-new_groesse_x+old_x );
			einstellungen->set_origin_y( einstellungen->get_origin_y()-new_groesse_y+old_y );
			break;
		case 3:
			einstellungen->set_origin_x( einstellungen->get_origin_x()-new_groesse_y+old_y );
			break;
	}

	// Resize marker_t:
	marker.init(new_groesse_x, new_groesse_y);

	distribute_groundobjs_cities(sets->get_anzahl_staedte(),old_x, old_y);

	// hausbauer_t::neue_karte(); <- this would reinit monuments! do not do this!
	fabrikbauer_t::neue_karte( this );

	// Modified by : Knightly
	if ( einstellungen->get_default_path_option() == 2 )
	{
		path_explorer_t::refresh_all_categories(true);
	}
	else
	{
		set_schedule_counter();
	}

	// Refresh the haltlist for the affected tiles / stations.
	// It is enough to check the tile just at the border ...
	const sint8 cov = get_einstellungen()->get_station_coverage();
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
		simloops = 60;
		reset_timer();

		// make timer loop invalid
		for( int i=0;  i<32;  i++ ) {
			last_frame_ms[i] = 0x7FFFFFFFu;
			last_step_nr[i] = 0xFFFFFFFFu;
		}
		last_frame_idx = 0;
	}
}



karte_t::karte_t() : convoi_array(0), ausflugsziele(16), stadt(0), marker(0,0)
{
	is_shutting_down = false;

	// length of day and other time stuff
	ticks_bits_per_tag = 20;
#ifdef _MSC_VER
	ticks_per_tag = (1i64 << ticks_bits_per_tag);
#else
	ticks_per_tag = (1ll << ticks_bits_per_tag);
#endif
	last_step_ticks = 0;
	last_interaction = dr_time();
	step_mode = PAUSE_FLAG;
	time_multiplier = 16;
	next_step_time = last_step_time = 0;
	time_budget = 0;

	for(  uint i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		werkzeug[i] = werkzeug_t::general_tool[WKZ_ABFRAGE];
	}
	werkzeug_last_pos = koord3d::invalid;
	werkzeug_last_button = 0;

	follow_convoi = convoihandle_t();
	set_dirty();
	set_scroll_lock(false);

	einstellungen_t *sets = new einstellungen_t(umgebung_t::default_einstellungen);

	// standard prices
	warenbauer_t::set_multiplier( 1000 );

	zeiger = 0;
	plan = 0;
	ij_off = koord::invalid;
	x_off = 0;
	y_off = 0;
	grid_hgts = 0;
	einstellungen = sets;
	schedule_counter = 0;
	nosave_warning = nosave = false;
	actual_industry_density = industry_density_proportion = 0;

	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		spieler[i] = NULL;
	}

	// no distance to show at first ...
	show_distance = koord3d::invalid;
	scenario = NULL;

	msg = new message_t(this);

	base_pathing_counter = 0;

	outstanding_cars = 0;

	// Added by : Knightly
	path_explorer_t::initialise(this);

	//@author: jamespetts
	set_scale();
}


karte_t::~karte_t()
{
	is_sound = false;

	destroy();

	if(einstellungen) {
		delete einstellungen;
		einstellungen = NULL;
	}

	delete msg;
}

void karte_t::set_scale()
{
	const float scale_factor = get_einstellungen()->get_distance_per_tile();
	
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

	// Way objects

	vector_tpl<way_obj_besch_t * > * way_objects = wayobj_t::get_all_wayobjects();
	ITERATE_PTR(way_objects,j)
	{
		way_objects->get_element(j)->set_scale(scale_factor);
	}

	// Stations

	slist_iterator_tpl <halthandle_t> halt_pre_iter (haltestelle_t::get_alle_haltestellen());
	while( halt_pre_iter.next() ) 
	{
		koord3d t = halt_pre_iter.get_current()->get_basis_pos3d();
		grund_t *gr = lookup(t);
		gebaeude_t* gb = gr->find<gebaeude_t>();
		gb->get_tile()->get_modifiable_besch()->set_scale(scale_factor); 
	}

	// Goods

	const uint16 goods_count = warenbauer_t::get_waren_anzahl();
	for(uint16 i = 0; i < goods_count; i ++)
	{
		warenbauer_t::get_modifiable_info(i)->set_scale(scale_factor);
	}
}


bool karte_t::can_lower_plan_to(sint16 x, sint16 y, sint16 h) const
{
	const planquadrat_t *plan = lookup(koord(x,y));

	if(  plan==NULL  ) {
		return false;
	}

	const sint8 hmax = plan->get_kartenboden()->get_hoehe();
	if(  hmax==h  ) {
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


bool karte_t::can_raise_plan_to(sint16 x, sint16 y, sint16 h) const
{
	const planquadrat_t *plan = lookup(koord(x,y));
	if(plan == 0 || !is_plan_height_changeable(x, y)) {
		return false;
	}

	// irgendwo eine Brücke im Weg?
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
			if(dt != NULL) {
			ok =
				dt->get_typ() == ding_t::baum  ||
				dt->get_typ() == ding_t::zeiger  ||
				dt->get_typ() == ding_t::wolke  ||
				dt->get_typ() == ding_t::sync_wolke  ||
				dt->get_typ() == ding_t::async_wolke  ||
				dt->get_typ() == ding_t::groundobj;
			}
		}
	}

	return ok;
}

// raise plan
// new heights for each corner given
// only test corners in ctest to avoid infinite loops
bool karte_t::can_raise_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw, uint8 ctest) const
{
	bool ok;
	if(ist_in_kartengrenzen(x,y)) {
		grund_t *gr = lookup_kartenboden(koord(x,y));
		const sint8 h0 = gr->get_hoehe();
		// which corners have to be raised?
		const sint8 h0_sw = corner1(ctest) ? (gr->ist_wasser() ? lookup_hgt(koord(x,y+1))   : h0 + corner1(gr->get_grund_hang()) ) : hsw+1;
		const sint8 h0_se = corner2(ctest) ? (gr->ist_wasser() ? lookup_hgt(koord(x+1,y+1)) : h0 + corner2(gr->get_grund_hang()) ) : hse+1;
		const sint8 h0_ne = corner3(ctest) ? (gr->ist_wasser() ? lookup_hgt(koord(x+1,y))   : h0 + corner3(gr->get_grund_hang()) ) : hne+1;
		const sint8 h0_nw = corner4(ctest) ? (gr->ist_wasser() ? lookup_hgt(koord(x,y))     : h0 + corner4(gr->get_grund_hang()) ) : hnw+1;


		ok = can_raise_plan_to(x,y, max(max(hsw,hse),max(hne,hnw)));
		// sw
		if (ok && h0_sw < hsw) {
			ok = can_raise_to(x-1,y+1, hsw-1, hsw-1, hsw, hsw-1, 11);
		}
		// s
		if (ok && (h0_se < hse || h0_sw < hsw) && ((ctest&3)==3)) {
			const sint8 hs = max(hse, hsw) -1;
			ok = can_raise_to(x,y+1, hs, hs, hse, hsw, 3);
		}
		// se
		if (ok && h0_se < hse) {
			ok = can_raise_to(x+1,y+1, hse-1, hse-1, hse-1, hse, 7);
		}
		// e
		if (ok && (h0_se < hse || h0_ne < hne) && ((ctest&6)==6)) {
			const sint8 he = max(hse, hne) -1;
			ok = can_raise_to(x+1,y, hse, he, he, hne, 6);
		}
		// ne
		if (ok && h0_ne < hne) {
			ok = can_raise_to(x+1,y-1, hne, hne-1, hne-1, hne-1, 14);
		}
		// n
		if (ok && (h0_nw < hnw || h0_ne < hne) && ((ctest&12)==12)) {
			const sint8 hn = max(hnw, hne) -1;
			ok = can_raise_to(x,y-1, hnw, hne, hn, hn, 12);
		}
		// nw
		if (ok && h0_nw < hnw) {
			ok = can_raise_to(x-1,y-1, hnw-1, hnw, hnw-1, hnw-1, 13);
		}
		// w
		if (ok && (h0_nw < hnw || h0_sw < hsw) && ((ctest&9)==9)) {
			const sint8 hw = max(hnw, hsw) -1;
			ok = can_raise_to(x-1,y, hw, hsw, hnw, hw, 9);
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
	if(ist_in_kartengrenzen(x, y)) {
		grund_t *gr = lookup_kartenboden(koord(x,y));
		const sint8 hnew = gr->get_hoehe() + corner4(gr->get_grund_hang());

		return can_raise_to(x, y, hnew, hnew, hnew, hnew+1, 15/*all corners*/ );
	} else {
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
		const sint8 h0_sw = gr->ist_wasser() ? lookup_hgt(koord(x,y+1))   : h0 + corner1(gr->get_grund_hang());
		const sint8 h0_se = gr->ist_wasser() ? lookup_hgt(koord(x+1,y+1)) : h0 + corner2(gr->get_grund_hang());
		const sint8 h0_ne = gr->ist_wasser() ? lookup_hgt(koord(x+1,y))   : h0 + corner3(gr->get_grund_hang());
		const sint8 h0_nw = gr->ist_wasser() ? lookup_hgt(koord(x,y))     : h0 + corner4(gr->get_grund_hang());

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
int karte_t::raise_to(sint16 x, sint16 y, sint16 h, bool set_slopes /*always false*/)
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
	bool ok;
	if(ist_in_kartengrenzen(x,y)) {
		grund_t *gr = lookup_kartenboden(koord(x,y));
		const sint8 h0 = gr->get_hoehe();
		// which corners have to be raised?
		const sint8 h0_sw = corner1(ctest) ? (gr->ist_wasser() ? lookup_hgt(koord(x,y+1))   : h0 + corner1(gr->get_grund_hang()) ) : hsw-1;
		const sint8 h0_se = corner2(ctest) ? (gr->ist_wasser() ? lookup_hgt(koord(x+1,y+1)) : h0 + corner2(gr->get_grund_hang()) ) : hse-1;
		const sint8 h0_ne = corner3(ctest) ? (gr->ist_wasser() ? lookup_hgt(koord(x,y+1))   : h0 + corner3(gr->get_grund_hang()) ) : hne-1;
		const sint8 h0_nw = corner4(ctest) ? (gr->ist_wasser() ? lookup_hgt(koord(x,y))     : h0 + corner4(gr->get_grund_hang()) ) : hnw-1;

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
		const sint8 h0_sw = gr->ist_wasser() ? lookup_hgt(koord(x,y+1))   : h0 + corner1(gr->get_grund_hang());
		const sint8 h0_se = gr->ist_wasser() ? lookup_hgt(koord(x+1,y+1)) : h0 + corner2(gr->get_grund_hang());
		const sint8 h0_ne = gr->ist_wasser() ? lookup_hgt(koord(x+1,y))   : h0 + corner3(gr->get_grund_hang());
		const sint8 h0_nw = gr->ist_wasser() ? lookup_hgt(koord(x,y))     : h0 + corner4(gr->get_grund_hang());
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


int karte_t::lower_to(sint16 x, sint16 y, sint16 h, bool set_slopes /*always false*/)
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


static koord ebene_offsets[] = {koord(0,0), koord(1,0), koord(0,1), koord(1,1)};

bool karte_t::can_ebne_planquadrat(koord pos, sint16 hgt)
{
	if (lookup_kartenboden(pos)->get_hoehe()>=hgt) {
		return can_lower_to(pos.x, pos.y, hgt, hgt, hgt, hgt);
	}
	else {
		return can_raise_to(pos.x, pos.y, hgt, hgt, hgt, hgt);
	}
}



// make a flat leve at this position (only used for AI at the moment)
bool karte_t::ebne_planquadrat(spieler_t *sp, koord pos, sint16 hgt)
{
	int n = 0;
	bool ok = false;
	const sint8 old_hgt = lookup_kartenboden(pos)->get_hoehe();
	if(  old_hgt>=hgt  ) {
		if(  can_lower_to(pos.x, pos.y, hgt, hgt, hgt, hgt)  ) {
			n = lower_to(pos.x, pos.y, hgt, hgt, hgt, hgt);
			ok = true;
		}
	}
	else {
		if(  can_raise_to(pos.x, pos.y, hgt, hgt, hgt, hgt)  ) {
			n = raise_to(pos.x, pos.y, hgt, hgt, hgt, hgt);
			ok = true;
		}
	}
	// was changed => pay for it
	if(n>0) {
		spieler_t::accounting(sp, n*get_einstellungen()->cst_alter_land, pos, COST_CONSTRUCTION );
	}
	return ok;
}



// new tool definition
void karte_t::set_werkzeug( werkzeug_t *w, spieler_t *sp )
{
	if(!umgebung_t::networkmode  ||  w->is_init_network_save()  ) {

		if(w->init(this,sp)) {

			set_dirty();
			if(w!=werkzeug[sp->get_player_nr()]) {

				// reinit same tool => do not play sound twice
				struct sound_info info;
				info.index = SFX_SELECT;
				info.volume = 255;
				info.pri = 0;
				sound_play(info);

				// only exit, if it is not the same tool again ...
				werkzeug[sp->get_player_nr()]->exit(this,sp);
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
				werkzeug_last_pos = koord3d::invalid;
				werkzeug_last_button = 0;
			}
			
			werkzeug[sp->get_player_nr()] = w;
		}
	}
	else {
		// queue tool for network
		static char commandstring[4096];
		int len = sprintf( commandstring, NET_TO_SERVER NET_WKZ_INIT " %li,%lu,%hi,%hi,%hi,%hi,%hi,%s" NET_END_CMD, steps, get_random_seed(), w->id, get_active_player_nr(), zeiger->get_pos().x, zeiger->get_pos().y, zeiger->get_pos().z, w->default_param==NULL ? "" : w->default_param );
		network_send_server( commandstring, len );
	}
}



sint16 karte_t::min_hgt(const koord pos) const
{
	const int h1 = lookup_hgt(pos);
	const int h2 = lookup_hgt(pos+koord(1, 0));
	const int h3 = lookup_hgt(pos+koord(1, 1));
	const int h4 = lookup_hgt(pos+koord(0, 1));

	return min(min(h1,h2), min(h3,h4));
}


sint16 karte_t::max_hgt(const koord pos) const
{
	const int h1 = lookup_hgt(pos);
	const int h2 = lookup_hgt(pos+koord(1, 0));
	const int h3 = lookup_hgt(pos+koord(1, 1));
	const int h4 = lookup_hgt(pos+koord(0, 1));

	return max(max(h1,h2), max(h3,h4));
}


void karte_t::rotate90()
{
	// asumme we can save this rotation
	nosave_warning = nosave = false;

	//announce current target rotation
	einstellungen->rotate90();

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
	if ( einstellungen->get_default_path_option() == 2 )
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
		slist_iterator_tpl<stadt_t *> iter_city (fab->get_arbeiterziele());
		while(iter_city.next()) {
			iter_city.get_current()->remove_arbeiterziel(fab);
		}

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
		const int end = simrand( anz );
		result = fab_list[end];
	}
	return result;
}


/*----------------------------------------------------------------------------------------------------------------------*/
/* same procedure for tourist attractions */

void
karte_t::add_ausflugsziel(gebaeude_t *gb)
{
	assert(gb != NULL);
	ausflugsziele.append( gb, gb->get_tile()->get_besch()->get_level(), 16 );
//DBG_MESSAGE("karte_t::add_ausflugsziel()","appended ausflugsziel at %i",ausflugsziele.get_count() );
}

void
karte_t::remove_ausflugsziel(gebaeude_t *gb)
{
	assert(gb != NULL);
	ausflugsziele.remove( gb );
}



/* select a random target for a tourist; targets are weighted by their importance */
const gebaeude_t *
karte_t::get_random_ausflugsziel() const
{
	const unsigned long sum_pax=ausflugsziele.get_sum_weight();
	if (!ausflugsziele.empty() && sum_pax > 0) {
		return ausflugsziele.at_weight( simrand(sum_pax) );
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
		for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) 
		{
			stadt_t* c = *i;
			if(c->is_within_city_limits(pos))
			{
				city = c;
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
	bool old_allowed = set_random_allowed( true );
	if(sync) {
		// only omitted, when called to display a new frame during fast forward
		sync_step_running = true;

		// just for progress
		ticks += delta_t;

		// ingore calls by interrupt during fast forward ...
		while(!sync_add_list.empty()) {
			sync_steppable *ss = sync_add_list.remove_first();
			sync_list.insert( ss );
		}

		// now remove everything from last time
		while(!sync_remove_list.empty()) {
			sync_steppable *ss = sync_remove_list.remove_first();
			sync_list.remove( ss );
		}

		for(  slist_tpl<sync_steppable*>::iterator i=sync_list.begin();  i!=sync_list.end();  ) {
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
			const koord3d new_pos=follow_convoi->get_vehikel(0)->get_pos();
			if(new_pos!=koord3d::invalid) {
				const sint16 rw = get_tile_raster_width();
				int new_xoff = 0;
				int new_yoff = 0;
				follow_convoi->get_vehikel(0)->get_screen_offset( new_xoff, new_yoff );
				new_xoff -= tile_raster_scale_x(-follow_convoi->get_vehikel(0)->get_xoff(),rw);
				new_yoff -= tile_raster_scale_y(-follow_convoi->get_vehikel(0)->get_yoff(),rw) + tile_raster_scale_y(new_pos.z*TILE_HEIGHT_STEP/Z_TILE_STEP,rw);
				change_world_position( new_pos.get_2d(), -new_xoff, -new_yoff );
			}
		}

		// display new frame with water animation
		intr_refresh_display( false );
		update_frame_sleep_time(delta_t);
	}
	set_random_allowed( old_allowed );
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
		uint32 frame_intervall = max( 100, 1000/umgebung_t::fps );
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
void karte_t::buche(const sint64 betrag, enum player_cost type)
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

	INT_CHECK("simworld 1701");

//	DBG_MESSAGE("karte_t::neuer_monat()","convois");
	// hsiegeln - call new month for convoys
	for(unsigned i=0;  i<convoi_array.get_count();  i++ ) {
		convoihandle_t cnv = convoi_array[i];
		cnv->new_month();
	}

	base_pathing_counter ++;

	INT_CHECK("simworld 1701");


//	DBG_MESSAGE("karte_t::neuer_monat()","factories");
	sint16 number_of_factories = fab_list.get_count();
	fabrik_t * fab;
	for(sint16 i = number_of_factories - 1; i >= 0; i--)
	{
		fab = fab_list[i];
		fab->neuer_monat();
		// The number of factories might have diminished,
		// so must adjust i to prevent out of bounds errors.
		sint16 difference = number_of_factories - fab_list.get_count();
		i -= difference;
	}

	// Check to see whether more factories need to be added
	// to replace ones that have closed.
	// @author: jamespetts

	const double target_industry_density = get_target_industry_density();
	if(actual_industry_density < target_industry_density)
	{
		// Only add one per month, and randomise.
		const double proportion = ((target_industry_density - actual_industry_density) / target_industry_density) * 100;
		const uint8 chance = simrand(100);
		if(chance < proportion)
		{
			fabrikbauer_t::increase_industry_density(this, true, true);
		}
	}

	INT_CHECK("simworld 1278");

	//	DBG_MESSAGE("karte_t::neuer_monat()","cities");
	// roll city history and copy the new citicens (i.e. the new weight) into the stadt array
	// no INT_CHECK() here, or dialoges will go crazy!!!
	weighted_vector_tpl<stadt_t*> new_weighted_stadt(stadt.get_count() + 1);
	for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
		stadt_t* s = *i;
		s->neuer_monat();
		outstanding_cars += s->get_outstanding_cars();
		new_weighted_stadt.append(s, s->get_einwohner(), 64);
		INT_CHECK("simworld 1278");
	}
	swap(stadt, new_weighted_stadt);

	INT_CHECK("simworld 1282");

//	DBG_MESSAGE("karte_t::neuer_monat()","players");
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

	INT_CHECK("simworld 1289");

//	DBG_MESSAGE("karte_t::neuer_monat()","halts");
	slist_iterator_tpl <halthandle_t> halt_iter (haltestelle_t::get_alle_haltestellen());
	while( halt_iter.next() ) {
		halt_iter.get_current()->neuer_monat();
		INT_CHECK("simworld 1877");
	}

	// now switch year to get the right year for all timeline stuff ...
	if(letzter_monat==0) {
		neues_jahr();
		INT_CHECK("simworld 1299");
	}

	wegbauer_t::neuer_monat(this);
	INT_CHECK("simworld 1299");

	recalc_average_speed();
	INT_CHECK("simworld 1921");

	if(umgebung_t::autosave>0  &&  letzter_monat%umgebung_t::autosave==0) {
		char buf[128];
		sprintf( buf, "save/autosave%02i.sve", letzter_monat+1 );
		speichern( buf, true );
	}

	// Added by : Knightly
	// Note		: This should be done after all lines and convoys have rolled their statistics
	if ( einstellungen->get_default_path_option() == 2 )
	{
		path_explorer_t::refresh_all_categories(true);
	}
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

	for(int i=road_wt; i<=narrowgauge_wt; i++) {
		const int typ = i==4 ? 3 : (i-1)&7;
		const float speed_bonus_multiplier = get_einstellungen()->get_speed_bonus_multiplier();
		const uint32 base_speed_bonus = vehikelbauer_t::get_speedbonus( this->get_timeline_year_month(), i==4 ? air_wt : (waytype_t)i );
		average_speed[typ] = (float)base_speed_bonus * speed_bonus_multiplier;
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
				}
			}
		}

		// city road check
		const weg_besch_t* city_road_test = wegbauer_t::get_besch(get_einstellungen()->get_city_road_type(), get_timeline_year_month());
		if(city_road_test) {
			city_road = city_road_test;
		}
		else {
			DBG_MESSAGE("karte_t::neuer_monat()","Month %d has started", letzter_monat);
			city_road = wegbauer_t::weg_search(road_wt,50,get_timeline_year_month(),weg_t::type_flat);
		}

	}
	else {
		// defaults
		city_road = wegbauer_t::get_besch(get_einstellungen()->get_city_road_type(), 0);
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
	switch(cnv->get_vehikel(0)->get_waytype()) {
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
			switch(cnv->get_vehikel(0)->get_waytype()) {
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




void karte_t::step()
{
	unsigned long time = dr_time();

	// first: check for new month
	if(ticks > next_month_ticks) {

		next_month_ticks += karte_t::ticks_per_tag;

		// avoid overflow here ...
		// Should not overflow: now usint 64-bit values.
		//@jamespetts
		/*if(ticks>next_month_ticks) {
			ticks %= karte_t::ticks_per_tag;
			ticks += karte_t::ticks_per_tag;
			next_month_ticks = ticks+karte_t::ticks_per_tag;
			last_step_ticks %= karte_t::ticks_per_tag;
		}*/

		set_random_allowed( true );
		neuer_monat();
		set_random_allowed( !umgebung_t::networkmode );
	}

	const long delta_t = ticks - last_step_ticks;
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
		time_budget = 0;
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

	set_random_allowed( true );
	// check for pending seasons change
	if(pending_season_change>0) {
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
	if ( einstellungen->get_default_path_option() == 2 )
	{
		path_explorer_t::step();
		INT_CHECK("karte_t::step");
	}
	
	ITERATE(convoi_array,i)
	{
		convoihandle_t cnv = convoi_array[i];
		cnv->step();
		if((i&7)==0) {
			INT_CHECK("simworld 1947");
		}
	}

	// now step all towns (to generate passengers)
	sint64 bev = 0;
	for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
		(*i)->step(delta_t);
		bev += (*i)->get_finance_history_month( 0, HIST_CITICENS );
	}

	// the inhabitants stuff
	finance_history_month[0][WORLD_CITICENS] = bev;

	ITERATE(fab_list,i)
	{
		fab_list[i]->step(delta_t);
	}
	finance_history_year[0][WORLD_FACTORIES] = finance_history_month[0][WORLD_FACTORIES] = fab_list.get_count();

	// then step all players
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  spieler[i] != NULL  ) {
			spieler[i]->step();
		}
	}
	set_random_allowed( !umgebung_t::networkmode );

	haltestelle_t::step_all();

	// ok, next step
	INT_CHECK("simworld 1975");

	if((steps%8)==0) {
		check_midi();
	}

	// will also call all objects if needed ...
	if(  recalc_snowline()  ) {
		pending_season_change ++;
	}
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
			transported += spieler[i]->get_finance_history_month( 0, COST_ALL_TRANSPORTED );
			transported_year += spieler[i]->get_finance_history_year( 0, COST_ALL_TRANSPORTED );
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
		const int raster = get_tile_raster_width();
		koord new_ij = ij_off;

		sint16 new_xoff = x_off - (ev->mx - ev->cx) * umgebung_t::scroll_multi;
		new_ij.x -= new_xoff/raster;
		new_ij.y += new_xoff/raster;
		new_xoff %= raster;

		int lines = 0;
		sint16 new_yoff = y_off - (ev->my-ev->cy) * umgebung_t::scroll_multi;
		if(y_off>0) {
			lines = (new_yoff + (raster/4))/(raster/2);
		}
		else {
			lines = (new_yoff - (raster/4))/(raster/2);
		}
		new_ij -= koord( lines, lines );
		new_yoff -= (raster/2)*lines;

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
		return b<=c ? b : max(a,c);;
	}
	else {
		return b>c ? b : min(a,c);;
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



bool
karte_t::ist_platz_frei(koord pos, sint16 w, sint16 h, int *last_y, climate_bits cl) const
{
	koord k;

	if(pos.x<0 || pos.y<0 || pos.x+w>=get_groesse_x() || pos.y+h>=get_groesse_y()) {
		return false;
	}

	grund_t *gr = lookup_kartenboden(pos);
	const sint16 platz_h = gr->get_grund_hang() ? max_hgt(pos) : gr->get_hoehe();	// remember the max height of the first tile

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
			if(platz_h!=(gr->get_hoehe()+Z_TILE_STEP*((gr->get_grund_hang()+127)/128))  ||  !gr->ist_natur() ||  gr->get_halt().is_bound()  ||  gr->kann_alle_obj_entfernen(NULL) != NULL  ||  (cl&(1<<get_climate(gr->get_hoehe())))==0) {
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
				// Optimiert für größere Felder, hehe!
				// Die Idee: wenn bei 2x2 die untere Reihe nicht geht, können
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
bool
karte_t::play_sound_area_clipped(koord pos, sound_info info)
{
	if(is_sound) {
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



void karte_t::speichern(const char *filename,bool silent)
{
#ifndef DEMO
DBG_MESSAGE("karte_t::speichern()", "saving game to '%s'", filename);

	loadsave_t  file;

	display_show_load_pointer( true );
	if(!file.wr_open(filename,loadsave_t::save_mode,umgebung_t::objfilename)) {
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
				einstellungen->set_filename(filename);
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
		old_sp[i] = einstellungen->get_player_type(i);
		if(  spieler[i]==NULL  ) {
			einstellungen->set_player_type( i, spieler_t::EMPTY);
		}
	}
	einstellungen->rdwr(file);
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		einstellungen->set_player_type( i, old_sp[i] );
	}

	if(file->get_experimental_version() <= 1)
	{
		uint32 old_ticks = (uint32)ticks;
		file->rdwr_long(old_ticks, " ");
		ticks = old_ticks;
	}
	else
	{
		file->rdwr_longlong(ticks, " ");
	}
	file->rdwr_long(letzter_monat, " ");
	file->rdwr_long(letztes_jahr, "\n");

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
			file->rdwr_byte(grid_hgts[j], "\n");
		}
	DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved hgt");
	}

	sint32 fabs = fab_list.get_count();
	file->rdwr_long(fabs, "\n");
	//slist_iterator_tpl<fabrik_t*> fiter( fab_list );
	//while(fiter.next()) {
	for(sint16 i = fab_list.get_count() - 1; i >= 0; i --)
	{
		//(fiter.get_current())->rdwr(file);
		fab_list[i]->rdwr(file);
		if(silent) {
			INT_CHECK("saving");
		}
	}
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved fabs");

	sint32 haltcount=haltestelle_t::get_alle_haltestellen().get_count();
	file->rdwr_long(haltcount,"hc");
	slist_iterator_tpl<halthandle_t> iter (haltestelle_t::get_alle_haltestellen());
	while(iter.next()) {
		iter.get_current()->rdwr( file );
	}
DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved stops");

	// svae number of convois
	if(  file->get_version()>=101000  ) {
		uint16 i=convoi_array.get_count();
		file->rdwr_short( i, "" );
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

	// centered on what?
	sint32 dummy = ij_off.x;
	file->rdwr_long(dummy, " ");
	dummy = ij_off.y;
	file->rdwr_long(dummy, "\n");

	if(file->get_version()>=99018) {
		// most recent version is 99018
		for (int year = 0;  year</*MAX_WORLD_HISTORY_YEARS*/12;  year++) {
			for (int cost_type = 0; cost_type</*MAX_WORLD_COST*/12; cost_type++) {
				file->rdwr_longlong(finance_history_year[year][cost_type], " ");
			}
		}
		for (int month = 0;month</*MAX_WORLD_HISTORY_MONTHS*/12;month++) {
			for (int cost_type = 0; cost_type</*MAX_WORLD_COST*/12; cost_type++) {
				file->rdwr_longlong(finance_history_month[month][cost_type], " ");
			}
		}
	}

	// finally a possible scenario
	scenario->rdwr( file );

	if(file->get_experimental_version() >= 2)
	{
		file->rdwr_short(base_pathing_counter, "");
	}
	if(file->get_experimental_version() >= 7)
	{
		file->rdwr_double(industry_density_proportion);
	}

	if(needs_redraw) 
	{
		update_map();
	}
}



// LOAD, not save
// just the preliminaries, opens the file, checks the versions ...
bool karte_t::laden(const char *filename)
{
	const char *name = filename;
	bool ok=false;
	mute_sound(true);
	display_show_load_pointer(true);

#ifndef DEMO
	loadsave_t file;

	DBG_MESSAGE("karte_t::laden", "loading game from '%s'", filename);

	if(  strstr(filename,"net:")==filename  ) {
		name = "client-network.sve";
		SOCKET s;

		// open from network
		const char *err = network_open_address( filename+4, s );
		if(  err==NULL  ) {
			char buf[64];
			int len;
			dbg->message( "karte_t::laden", "send :" NET_TO_SERVER NET_GAME NET_END_CMD );
			len = send( s, NET_TO_SERVER NET_GAME NET_END_CMD, 9, 0 );
			err = "Server did not respond!";
			len = 64;
			network_add_client( s );
			if(  network_check_activity( 1000, buf, len )!=INVALID_SOCKET  ) {
				// wait for sync message to finish
				if(  memcmp( NET_FROM_SERVER NET_GAME, buf, 7 )!=0  ) {
					err = "Protocoll error (expecting " NET_GAME ")";
				}
				else {
					err = network_recieve_file( s, name, atol(buf+7) );
				}
			}
		}
		if(err) {
			create_win( new news_img(err), w_info, magic_none );
			network_close_socket( s );
			display_show_load_pointer(false);
			step_mode = NORMAL;
			return false;
		}
		else {
			chdir( umgebung_t::user_dir );
			umgebung_t::networkmode = true;
		}
	}
	else {
		// probably finish network mode?
		if(  umgebung_t::networkmode  &&  !umgebung_t::server  ) {
			// remain only in networkmode, if I am the server
			umgebung_t::networkmode = false;
			network_core_shutdown();
			// closing the socket will tell the server, I a away too
		}
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

		if(  umgebung_t::server  ) {
			step_mode = FIX_RATIO;
		}
		else if(  umgebung_t::networkmode  ) {
			network_send_server( NET_TO_SERVER NET_READY NET_END_CMD, 9 );
			step_mode = PAUSE_FLAG|FIX_RATIO;
		}
		else {
			step_mode = NORMAL;
		}

		ok = true;
		file.close();
		create_win( new news_img("Spielstand wurde\ngeladen!\n"), w_time_delete, magic_none);
		set_dirty();

		reset_timer();
		recalc_average_speed();
		mute_sound(false);

		set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE], get_active_player() );
	}
#endif
	einstellungen->set_filename(filename);
	display_show_load_pointer(false);

	weighted_vector_tpl<stadt_t*> new_weighted_stadt(stadt.get_count() + 1);
	for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) 
	{
		stadt_t* s = *i;
		outstanding_cars += s->get_outstanding_cars();
	}

	return ok;
}


// handles the actual loading
void karte_t::laden(loadsave_t *file)
{
	char buf[80];

	intr_disable();
	for(  uint i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		werkzeug[i] = werkzeug_t::general_tool[WKZ_ABFRAGE];
	}
	destroy_all_win();

	display_set_progress_text(translator::translate("Loading map ..."));
	display_progress(0, 100);	// does not matter, since fixed width

	destroy();


	// Added by : Knightly
	path_explorer_t::initialise(this);

	//fast_forward = false;

	tile_counter = 0;

	simloops = 60;

	// powernets zum laden vorbereiten -> tabelle loeschen
	powernet_t::neue_karte();

	const float old_scale_factor = get_einstellungen()->get_distance_per_tile();

	// jetzt geht das laden los
	DBG_MESSAGE("karte_t::laden", "Fileversion: %d, %p", file->get_version(), einstellungen);
	einstellungen->rdwr(file);

	if(  umgebung_t::networkmode  ) {
		// to have games synchronized, transfer random counter too
		setsimrand( einstellungen->get_random_counter(), 0xFFFFFFFFu );
	}

	if(  !umgebung_t::networkmode  ||  umgebung_t::server  ) {
		if(einstellungen->get_allow_player_change()  &&  umgebung_t::default_einstellungen.get_use_timeline()!=2) {
			// not locked => eventually switch off timeline settings, if explicitly stated
			einstellungen->set_use_timeline( umgebung_t::default_einstellungen.get_use_timeline() );
			DBG_DEBUG("karte_t::laden", "timeline: reset to %i", umgebung_t::default_einstellungen.get_use_timeline() );
		}
	}
	if(einstellungen->get_beginner_mode()) {
		warenbauer_t::set_multiplier( get_einstellungen()->get_starting_year() );
	}
	else {
		warenbauer_t::set_multiplier( 1000 );
	}

	if(old_scale_factor != get_einstellungen()->get_distance_per_tile())
	{
		set_scale();
	}

	// just an initialisation for the loading
	season = (2+letzter_monat/3)&3; // summer always zero
	snowline = einstellungen->get_winter_snowline()*Z_TILE_STEP + grundwasser;

DBG_DEBUG("karte_t::laden", "einstellungen loaded (groesse %i,%i) timeline=%i beginner=%i",einstellungen->get_groesse_x(),einstellungen->get_groesse_y(),einstellungen->get_use_timeline(),einstellungen->get_beginner_mode());

	// wird gecached, um den Pointerzugriff zu sparen, da
	// die groesse _sehr_ oft referenziert wird
	cached_groesse_gitter_x = einstellungen->get_groesse_x();
	cached_groesse_gitter_y = einstellungen->get_groesse_y();
	cached_groesse_max = max(cached_groesse_gitter_x,cached_groesse_gitter_y);
	cached_groesse_karte_x = cached_groesse_gitter_x-1;
	cached_groesse_karte_y = cached_groesse_gitter_y-1;
	x_off = y_off = 0;

	// Reliefkarte an neue welt anpassen
	reliefkarte_t::get_karte()->set_welt(this);

	//12-Jan-02     Markus Weber added
	grundwasser = einstellungen->get_grundwasser();
	grund_besch_t::calc_water_level( this, height_to_climate );

	DBG_DEBUG("karte_t::laden()","grundwasser %i",grundwasser);

	init_felder();

	// reinit pointer with new pointer object and old values
	zeiger = new zeiger_t(this, koord3d::invalid, NULL );

	hausbauer_t::neue_karte();
	fabrikbauer_t::neue_karte(this);

	DBG_DEBUG("karte_t::laden", "init felder ok");

	if(file->get_experimental_version() <= 1)
	{
		uint32 old_ticks = (uint32)ticks;
		file->rdwr_long(old_ticks, " ");
		ticks = old_ticks;
	}
	else
	{
		file->rdwr_longlong(ticks, "");
	}
	file->rdwr_long(letzter_monat, " ");
	file->rdwr_long(letztes_jahr, "\n");
	if(file->get_version()<86006) 
	{
		letztes_jahr += umgebung_t::default_einstellungen.get_starting_year();
	}
	// old game might have wrong month
	letzter_monat %= 12;
 	// set the current month count
	set_ticks_bits_per_tag(einstellungen->get_bits_per_month());
	current_month = letzter_monat + (letztes_jahr*12);
	season = (2+letzter_monat/3)&3; // summer always zero
	next_month_ticks = 	( (ticks >> karte_t::ticks_bits_per_tag) + 1 ) << karte_t::ticks_bits_per_tag;
	last_step_ticks = ticks;
	steps = 0;
	step_mode = PAUSE_FLAG;

DBG_MESSAGE("karte_t::laden()","savegame loading at tick count %i",ticks);
	recalc_average_speed();	// resets timeline

DBG_MESSAGE("karte_t::laden()", "init player");
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(  file->get_version()>=101000  ) {
			// since we have different kind of AIs
			delete spieler[i];
			spieler[i] = NULL;
			new_spieler( i, einstellungen->spieler_type[i] );
		}
		else if(i<8) {
			// get the old player ...
			if(  spieler[i]==NULL  ) {
				spieler[i] = (i==3) ? (spieler_t*)(new ai_passenger_t(this, i)) : (spieler_t*)(new ai_goods_t(this, i));
			}
			einstellungen->spieler_type[i] = spieler[i]->get_ai_id();
		}
	}
	// so far, player 1 will be active (may change in future)
	active_player = spieler[0];
	active_player_nr = 0;

DBG_DEBUG("karte_t::laden", "init %i cities",einstellungen->get_anzahl_staedte());
	stadt.clear();
	stadt.resize(einstellungen->get_anzahl_staedte());
	for(int i=0; i<einstellungen->get_anzahl_staedte(); i++) {
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
				file->rdwr_long(hgt, "\n");
				// old height step was 16!
				set_grid_hgt(koord(x, y), (hgt*Z_TILE_STEP)/16 );
			}
		}
	}
	else if(  file->get_version()<=102001  )  {
		// hgt now bytes
		DBG_MESSAGE("karte_t::laden()","loading grid for older versions");
		for( sint32 i=0;  i<(get_groesse_y()+1)*(get_groesse_x()+1);  i++  ) {
			file->rdwr_byte( grid_hgts[i], "\n" );
		}
	}
	else {
		// just restore the corners as ground level
		DBG_MESSAGE("karte_t::laden()","calculating grid corners");
		for( sint32 i=1;  i<get_groesse_x()+1;  i++  ) {
			grid_hgts[i] = grundwasser;
		}
		for( sint32 j=0;  j<get_groesse_y()+1;  j++  ) {
			grid_hgts[(get_groesse_x()+1)*j] = grundwasser;
		}
	}

	if(file->get_version()<88009) {
		DBG_MESSAGE("karte_t::laden()","loading slopes from older version");
		// Hajo: load slopes for older versions
		// now part of the grund_t structure
		for (int y = 0; y < get_groesse_y(); y++) {
			for (int x = 0; x < get_groesse_x(); x++) {
				sint8 slope;
				file->rdwr_byte(slope, ",");
				access(x, y)->get_kartenboden()->set_grund_hang(slope);
			}
		}
	}

	if(file->get_version()<=88000) {
		// because from 88.01.4 on the foundations are handled differently
		for (int y = 0; y < get_groesse_y(); y++) {
			for (int x = 0; x < get_groesse_x(); x++) {
				koord k(x,y);
				if(access(x,y)->get_kartenboden()->get_typ()==grund_t::fundament) {
					access(x,y)->get_kartenboden()->set_hoehe( max_hgt(k) );
					access(x,y)->get_kartenboden()->set_grund_hang(hang_t::flach);
				}
			}
		}
	}

	// Reliefkarte an neue welt anpassen
	DBG_MESSAGE("karte_t::laden()", "init relief");
	win_set_welt( this );
	reliefkarte_t::get_karte()->set_welt(this);

	sint32 fabs;
	file->rdwr_long(fabs, "\n");
	DBG_MESSAGE("karte_t::laden()", "prepare for %i factories", fabs);

	for(sint32 i = 0; i < fabs; i++) {
		// liste in gleicher reihenfolge wie vor dem speichern wieder aufbauen
		fabrik_t *fab = new fabrik_t(this, file);
		if(fab->get_besch()) {
			//fab_list.append( fab );
			fab_list.append(fab);
		}
		else {
			dbg->error("karte_t::laden()","Unknown fabrik skipped!");
		}
		if(i&7) {
			display_progress(get_groesse_y()+(24*i)/fabs, get_groesse_y()+stadt.get_count()+256);
		}
	}

	DBG_MESSAGE("karte_t::laden()", "clean up factories");
	//slist_iterator_tpl<fabrik_t*> fiter ( fab_list );
	//while(fiter.next()) {
	for(sint16 i = fab_list.get_count() - 1; i >= 0; i --)
	{
		//fiter.get_current()->laden_abschliessen();
		fab_list[i]->laden_abschliessen();
	}

	display_progress(get_groesse_y()+24, get_groesse_y()+256+stadt.get_count());

DBG_MESSAGE("karte_t::laden()", "%d factories loaded", fab_list.get_count());

	// must be done after reliefkarte is initialized
	int x = get_groesse_y() + 24;
	for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
		// old versions did not save factory connections
		if(file->get_version()<99014) {
			(*i)->verbinde_fabriken();
		}
		display_progress(x++, get_groesse_y() + 256 + stadt.get_count());
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
		file->rdwr_long(halt_count,"hc");
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
		file->rdwr_short( convoi_nr, "" );
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

	// jetzt können die spieler geladen werden
	display_progress(get_groesse_y()+24+stadt.get_count(), get_groesse_y()+256+stadt.get_count());
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(  spieler[i]  ) {
			spieler[i]->rdwr(file);
			einstellungen->automaten[i] = spieler[i]->is_active();
		}
		else {
			einstellungen->automaten[i] = false;
			einstellungen->password[i][0] = 0;
		}
		display_progress(get_groesse_y()+24+stadt.get_count()+(i*3), get_groesse_y()+256+stadt.get_count());
	}
DBG_MESSAGE("karte_t::laden()", "players loaded");

	// nachdem die welt jetzt geladen ist können die Blockstrecken neu
	// angelegt werden
	old_blockmanager_t::laden_abschliessen(this);
	DBG_MESSAGE("karte_t::laden()", "blocks loaded");

	file->rdwr_long(mi, " ");
	file->rdwr_long(mj, "\n");
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

	// adding lines and other stuff for convois
	for(unsigned i=0;  i<convoi_array.get_count();  i++ ) {
		convoihandle_t cnv = convoi_array[i];
		cnv->laden_abschliessen();
		// was deleted during loading => use same position again
		if(!cnv.is_bound()) {
			i--;
		}
	}

	// finish the loading of stops (i.e. assign the right good for these stops)
	for(  slist_tpl<halthandle_t>::const_iterator i=haltestelle_t::get_alle_haltestellen().begin(); i!=haltestelle_t::get_alle_haltestellen().end();  ) {
		if(  (*i)->get_besitzer()==NULL  ||  !(*i)->existiert_in_welt()  ) {
			// this stop was only needed for loading goods ...
			halthandle_t h = (*i);
			++i;	// goto next
			haltestelle_t::destroy(h);	// remove from list
		}
		else {
			(*i)->laden_abschliessen();
			++i;
		}
	}

	// register all line stops and change line types, if needed
	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		if(  spieler[i]  ) {
			spieler[i]->laden_abschliessen();
		}
	}

	// must resort them ...
	weighted_vector_tpl<stadt_t*> new_weighted_stadt(stadt.get_count() + 1);
	for (weighted_vector_tpl<stadt_t*>::const_iterator i = stadt.begin(), end = stadt.end(); i != end; ++i) {
		stadt_t* s = *i;
		s->laden_abschliessen();
		new_weighted_stadt.append(s, s->get_einwohner(), 64);
		INT_CHECK("simworld 1278");
	}
	swap(stadt, new_weighted_stadt);
	DBG_MESSAGE("karte_t::laden()", "cities initialized");

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
#endif

#if 0
	// reroute goods for benchmarking
	dt = dr_time();
	for(  slist_tpl<halthandle_t>::const_iterator i=haltestelle_t::get_alle_haltestellen().begin(); i!=haltestelle_t::get_alle_haltestellen().end();  ++i  ) {
		sint16 dummy = 0x7FFF;
		if((hnr++%64)==0) {
			display_progress(get_groesse_y()+48+stadt.get_count()+128+(hnr*40)/hmax, get_groesse_y()+256+stadt.get_count());
		}
		(*i)->reroute_goods(dummy);
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
				file->rdwr_longlong(finance_history_year[year][cost_type], " ");
			}
		}
		for (int month = 0;month</*MAX_WORLD_HISTORY_MONTHS*/12;month++) {
			for (int cost_type = 0; cost_type</*MAX_WORLD_COST*/12; cost_type++) {
				file->rdwr_longlong(finance_history_month[month][cost_type], " ");
			}
		}
		last_month_bev = finance_history_month[1][WORLD_CITICENS];
	}

#if 0
	// preserve tick counter ...
	ticks = ticks % karte_t::ticks_per_tag;
	next_month_ticks = karte_t::ticks_per_tag;
	letzter_monat %= 12;
#endif

	// finally: do we run a scenario?
	if(file->get_version()>=99018) {
		scenario->rdwr(file);
	}

	DBG_MESSAGE("karte_t::laden()","savegame from %i/%i, next month=%i, ticks=%i (per month=1<<%i)",letzter_monat,letztes_jahr,next_month_ticks,ticks,karte_t::ticks_bits_per_tag);

	if(file->get_experimental_version() >= 2)
	{
		file->rdwr_short(base_pathing_counter, "");
	}
	
	// Reconstruct the actual industry density.
	// @author: jamespetts
	
	if(actual_industry_density <= 0)
	{
		// Make sure that this is not double counted
		double weight;
		ITERATE(fab_list, i)
		{
			const fabrik_besch_t* factory_type = fab_list[i]->get_besch();
			if(!factory_type->is_electricity_producer())
			{
				// Power stations are excluded from the target weight:
				// a different system is used for them.
				weight = factory_type->get_gewichtung();
				actual_industry_density += (1.0 / weight);
			}
		}

		if(file->get_experimental_version() >= 7)
		{
			file->rdwr_double(industry_density_proportion);
		}
		else
		{
			industry_density_proportion = actual_industry_density / finance_history_month[0][WORLD_CITICENS];
		}
	}

	// Added by : Knightly
	if ( einstellungen->get_default_path_option() == 2 )
	{
		path_explorer_t::full_instant_refresh();
	}
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
 * @author Hansjörg Malthaner
 */
void karte_t::load_heightfield(einstellungen_t *sets)
{
	sint16 w, h;
	sint8 *h_field;
	if(karte_t::get_height_data_from_file(sets->heightfield, sets->get_grundwasser(), h_field, w, h, false )) {
		sets->set_groesse(w,h);
		// create map
		init(sets,h_field);
		delete [] h_field;
	}
	else {
		dbg->error("karte_t::load_heightfield()","Cant open file '%s'", (const char*)sets->heightfield);
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
void karte_t::mark_area( const koord3d pos, const koord size, const bool mark )
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
	DBG_MESSAGE("karte_t::reset_timer()","called");
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
	time_budget = 0;

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
		next_step_time = last_tick_sync+(1000/einstellungen->get_frames_per_second() );
		set_frame_time( 1000/einstellungen->get_frames_per_second() );
		intr_disable();
		// other stuff needed to synchronize
		tile_counter = 0;
		pending_season_change = 1;
	}
	else {
		set_frame_time( 1000/umgebung_t::fps );
		next_step_time = last_tick_sync+(3200/get_time_multiplier() );
		intr_enable();
	}
}



void karte_t::reset_interaction()
{
	last_interaction = dr_time();
}



// jump one year ahead
// (not updating history!)
void karte_t::step_year()
{
	DBG_MESSAGE("karte_t::step_year()","called");
//	ticks += 12*karte_t::ticks_per_tag;
//	next_month_ticks += 12*karte_t::ticks_per_tag;
	current_month += 12;
	letztes_jahr ++;
	reset_timer();
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

	// Pause: warten auf die nächste Taste
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
		sint8 groff; // offset for lower raise tool
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

		// prüfe richtung d.h. welches nachbarfeld ist am naechsten
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
			if(  !umgebung_t::networkmode  ) {
				if(  ev->button_state == 0  ) {
					is_dragging = false;
					if(  werkzeug_last_pos != pos  ) {
						werkzeug[get_active_player_nr()]->move( this, get_active_player(), 0, pos );
					}
				}
				else if(ev->ev_class==EVENT_DRAG  &&  werkzeug_last_pos!=pos) {
					if(!is_dragging  &&  ist_in_kartengrenzen(prev_pos.get_2d())) {
						werkzeug[get_active_player_nr()]->move( this, get_active_player(), 1, prev_pos );
					}
					is_dragging = true;
					werkzeug[get_active_player_nr()]->move( this, get_active_player(), 1, pos );
				}
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
void karte_t::new_spieler(uint8 new_player, uint8 type)
{
	assert(  spieler[new_player]==NULL  );
	switch( type ) {
		case spieler_t::EMPTY: break;
		case spieler_t::HUMAN: spieler[new_player] = new spieler_t(this,new_player); break;
		case spieler_t::AI_GOODS: spieler[new_player] = new ai_goods_t(this,new_player); break;
		case spieler_t::AI_PASSENGER: spieler[new_player] = new ai_passenger_t(this,new_player); break;
		default: dbg->fatal( "karte_t::new_spieler()","Unknow AI type %i!",type );
	}
}




/* goes to next active player */
void karte_t::switch_active_player(uint8 new_player)
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
	if(!einstellungen->get_allow_player_change()) {
		active_player_nr = 0;
		active_player = spieler[0];
		if(new_player!=0) {
			msg->add_message(translator::translate("On this map, you are not\nallowed to change player!\n"), koord::invalid, message_t::problems, PLAYER_FLAG|get_active_player()->get_player_nr(), IMG_LEER);
		}
	}
	else {
		koord3d old_zeiger_pos = zeiger->get_pos();
		zeiger->set_bild( IMG_LEER );	// unmarks also area
		zeiger->set_pos( koord3d::invalid );
		if(  dynamic_cast<two_click_werkzeug_t *>(werkzeug[active_player_nr])  ) {
			dynamic_cast<two_click_werkzeug_t *>(werkzeug[active_player_nr])->cleanup( active_player, false );
		}
		renew_menu = (active_player_nr==1  ||  new_player==1);
		active_player_nr = new_player;
		active_player = spieler[new_player];
		char buf[512];
		sprintf(buf, translator::translate("Now active as %s.\n"), get_active_player()->get_name() );
		msg->add_message(buf, koord::invalid, message_t::warnings, PLAYER_FLAG|get_active_player()->get_player_nr(), IMG_LEER);
		werkzeug_last_pos = koord3d::invalid;
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



void
karte_t::interactive_event(event_t &ev)
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
				if(  (ev.ev_key_mod & 2) == 0  ) {
					// Control is _not_ pressed => Backspace or Enter pressed.
					if(  ev.ev_code == 8  ) {
						// Backspace
						sound_play(click_sound);
						destroy_all_win();
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

	if(IS_LEFTRELEASE(&ev)) {
DBG_MESSAGE("karte_t::interactive_event(event_t &ev)", "calling a tool");

		const char *err = NULL;
		if(werkzeug_last_pos!=zeiger->get_pos()  &&  (!umgebung_t::networkmode  ||  werkzeug[get_active_player_nr()]->is_work_network_save())  ) {
			err = werkzeug[get_active_player_nr()]->work( this, get_active_player(), zeiger->get_pos() );
		}
		else {
			// queue tool for network
			static char commandstring[4096];
			int len = sprintf( commandstring, NET_TO_SERVER NET_WKZ_WORK " %li,%lu,%hi,%hi,%hi,%hi,%hi,%s" NET_END_CMD, steps, get_random_seed(), werkzeug[get_active_player_nr()]->id, get_active_player_nr(), zeiger->get_pos().x, zeiger->get_pos().y, zeiger->get_pos().z, werkzeug[get_active_player_nr()]->default_param==NULL ? "" : werkzeug[get_active_player_nr()]->default_param );
			network_send_server( commandstring, len );
		}

		/* tools can return three kinds of messages
		 * NULL = success
		 * "" = failure, but just do not try again
		 * "bla" error message, which should be shown
		 */
		if(err==NULL) {
			if(werkzeug[get_active_player_nr()]->ok_sound!=NO_SOUND) {
				struct sound_info info = {werkzeug[get_active_player_nr()]->ok_sound,255,0};
				sound_play(info);
			}
		}
		else if(*err!=0) {
			// something went really wrong
			struct sound_info info = {SFX_FAILURE,255,0};
			sound_play(info);
			create_win( new news_img(err), w_time_delete, magic_none);
		}
		werkzeug_last_pos = koord3d::invalid;
	}

	// mouse wheel scrolled -> rezoom
	if (ev.ev_class == EVENT_CLICK) {
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
	}
	INT_CHECK("simworld 2117");
}



void karte_t::beenden(bool b)
{
	finish_loop=true;
	umgebung_t::quit_simutrans = b;
}




bool karte_t::interactive(uint32 quit_month)
{
	event_t ev;
	finish_loop = false;
	bool swallowed = false;
	bool cursor_hidden = false;

	char network_buffer[4096];
	int len_last_command;

	uint32 last_randoms[16];
	uint8  network_frame_count = 0;

	// only needed for network
	slist_tpl<char *>command_queue;
	long next_command_step=-1;
	const uint32 frame_time = 1000/max(20,einstellungen->get_frames_per_second());
	reset_timer();

	if(  umgebung_t::server  ) {
		step_mode |= FIX_RATIO;
		reset_timer();
	}

	do {
		// check for too much time eaten by frame updates ...
		if(  step_mode==NORMAL  ) {
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
		}

		// get an event
		win_poll_event(&ev);

		if(ev.ev_class==EVENT_SYSTEM  &&  ev.ev_code==SYSTEM_QUIT) {
			// Beenden des Programms wenn das Fenster geschlossen wird.
			destroy_all_win();
			umgebung_t::quit_simutrans = true;
			return false;
		}

		if(ev.ev_class!=EVENT_NONE &&  ev.ev_class!=IGNORE_EVENT) {

			swallowed = check_pos_win(&ev);

			if(IS_RIGHTCLICK(&ev)) {
				display_show_pointer(false);
				cursor_hidden = true;
			} else if(IS_RIGHTRELEASE(&ev)) {
				display_show_pointer(true);
				cursor_hidden = false;
			} else if(!swallowed  &&  IS_RIGHTDRAG(&ev)) {
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

			if((!swallowed  &&  (ev.ev_class==EVENT_DRAG  &&  ev.ev_code==MOUSE_LEFTBUTTON))  ||  (ev.button_state==0  &&  ev.ev_class==EVENT_MOVE)  ||  ev.ev_class==EVENT_RELEASE) {
				bewege_zeiger(&ev);
			}
		}

		if(  umgebung_t::networkmode  ) {
			// did we recieved a new command?
			len_last_command = sizeof(network_buffer);
			SOCKET s = network_check_activity( min(5u,next_step_time-dr_time()), network_buffer, len_last_command );
			if(  s!=INVALID_SOCKET  &&  !network_check_server_connection()  ) {
				// pause for loosing of connection
				network_core_shutdown();
				umgebung_t::networkmode = false;
				len_last_command = 0;
				step_mode = PAUSE_FLAG;
				reset_timer();
				// and tell the player
				create_win( new news_img("Lost connection to server!"), w_info, magic_none );
			}

			if(  s!=INVALID_SOCKET  ) {
				assert( len_last_command>0 );

				if(  memcmp( network_buffer, NET_TO_SERVER, 4 )==0  ) {
					if(  !umgebung_t::server  ) {
						dbg->fatal("NETWORK","Only server should recieve this: \"%s\"!", network_buffer );
					}
					else {
						if(  memcmp( network_buffer+4, NET_INFO, 4 )==0  ) {
							// just transfer einstellungen_t
							chdir( umgebung_t::user_dir );
							loadsave_t file;
							remove("tmp");
							if(file.wr_open("tmp",loadsave_t::xml,umgebung_t::objfilename)) {
								einstellungen->rdwr(&file);
								file.close();
								network_send_file( s, "tmp" );
							}
							set_pause( 1 );
//							network_send_server( NET_TO_SERVER NET_READY NET_END_CMD, 9 );
						}
						else if(  memcmp( network_buffer+4, NET_GAME, 4 )==0  ) {
							// transfer game, all clients need to sync (save, reload, and pause)

							// now save and send
							chdir( umgebung_t::user_dir );
							speichern( "server-network.sve", false );
							long old_steps = steps;
							// ok, now sending game
							if(  network_send_file( s, "server-network.sve" )==NULL  ) {
								char cmd[128];
								int n = sprintf( cmd, NET_FROM_SERVER NET_SYNC " %li" NET_END_CMD, steps );
								network_send_all( cmd, n, true );
								laden( "server-network.sve" );
								steps = old_steps;
								for(  int clients_to_wait=network_get_clients();  clients_to_wait>0;  ) {
									len_last_command = sizeof(network_buffer);
									SOCKET s = network_check_activity( 5, network_buffer, len_last_command );
									if(  s!=INVALID_SOCKET  &&  len_last_command>0  &&  memcmp( network_buffer, NET_TO_SERVER NET_READY NET_END_CMD, 9 )==0  ) {
										clients_to_wait--;
									}
								}
								// we are now on time
								reset_timer();
								network_frame_count = 0;
								// unpause all clients
								static char commandstring[4096];
								int len = sprintf( commandstring, NET_FROM_SERVER NET_READY " %li" NET_END_CMD, steps );
								network_send_all( commandstring, len, true );
							}
						}
						else if(  memcmp( network_buffer+4, NET_WKZ_INIT, 4 )==0  ||  memcmp( network_buffer+4, NET_WKZ_WORK, 4 )==0  ) {
							const bool init = memcmp( network_buffer+4, NET_WKZ_INIT, 4 )==0;
							size_t i=8;
							long client_steps = atol( network_buffer+8 );
							while(  network_buffer[i]!=','  &&  i<len_last_command  ) {
								// skip ","
								i++;
							}
							i++;
							uint32 client_random = i<len_last_command ? atol( network_buffer+i ) : 0;
							while(  network_buffer[i]!=','  &&  i<len_last_command  ) {
								// skip ","
								i++;
							}
							// check for similar random counter!
							if(  steps>client_steps  &&  client_steps+16>steps  ) {
								if(  last_randoms[client_steps&15]!=client_random  ) {
									printf( "" );
								}
							}
							if(  i<len_last_command  ) {
								next_command_step = steps+umgebung_t::server_frames_ahead; // do this next xxx frames
								char command_string[4096];
								int len = sprintf( command_string, NET_FROM_SERVER "%s %i,%i%s", init ? NET_WKZ_INIT : NET_WKZ_WORK, next_command_step, get_random_seed(), network_buffer+i );
								command_queue.append( strdup( command_string+3 ) );
								network_send_all( command_string, len, true );
							}
						}
#ifdef DEBUG
						else if(  memcmp( network_buffer+4, NET_CHECK, 5 )==0  ) {
							const bool init = memcmp( network_buffer+4, NET_WKZ_INIT, 4 )==0;
							long client_steps = 0;
							uint32 client_random = 0;
							sscanf( network_buffer+10, "%li,%lu", &client_steps, &client_random );
							// check for identical random counter!
							if(  steps>=client_steps  &&  client_steps+16>steps  ) {
								if(  last_randoms[client_steps&15]!=client_random  ) {
									printf( "" );
								}
							}
						}
#endif
					}
				}
				else {
					// from server
					if(  strncmp( network_buffer, NET_FROM_SERVER, 3 )==0  ) {
						bool tool = true;
						bool work = false;
						if(  memcmp( network_buffer+3, NET_WKZ_INIT, 4 )==0  ) {
							work = false;
						}
						else if(  memcmp( network_buffer+3, NET_WKZ_WORK, 4 )==0  ) {
							work = true;
						}
						else if(  memcmp( network_buffer+3, NET_READY, 5 )==0  ) {
							tool = false;
							if(  steps==0  ) {
								sscanf( network_buffer+8, "%li", &steps );
							}
							assert( steps == atol(network_buffer+8) );
							step_mode = FIX_RATIO;
							// lagging at least one farme
							dr_sleep( (1000*umgebung_t::server_frames_ahead)/einstellungen->get_frames_per_second() );
							reset_timer();
							network_frame_count = 0;
						}
						else if(  memcmp( network_buffer+3, NET_SYNC, 4 )==0  ) {
							tool = false;
						}
						else {
							tool = false;
							dbg->error("NETWORK","Unknow command \"%s\"", network_buffer );
						}
						// need to collect them first ...
						if(  tool  ) {
							command_queue.append( strdup(network_buffer+3) );
							// eventually execute this next ...
							if(  next_command_step<steps  ||  command_queue.empty()  ) {
								next_command_step = atol( network_buffer+8 );
							}
						}
					}
				}
			}
		}
		else {
			// we wait here for maximum 9ms
			// average is 5 ms, so we usually
			// are quite responsive
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
		}

		while(  !command_queue.empty()  &&  (next_command_step==steps  ||  step_mode&PAUSE_FLAG)  ) {
			char *network_buffer = command_queue.remove_first();
			if(  memcmp( network_buffer, NET_SYNC, 4 )==0  ) {
				if(  !umgebung_t::server  ) {
					long old_steps = steps;
					// saving and reloading game, sending notification when ready
					chdir( umgebung_t::user_dir );
					speichern( "client-network.sve", false );
					// ok, now sending game
					laden( "client-network.sve" );
					steps = old_steps;
					reset_timer();
					network_send_server( NET_TO_SERVER NET_READY NET_END_CMD, 9 );
					step_mode = PAUSE_FLAG|FIX_RATIO;
				}
				// server needs to do nothing
			}
			else {
				const bool init = memcmp( network_buffer, NET_WKZ_INIT, 4 )==0;
				if(!umgebung_t::server) {
					DBG_MESSAGE("command","%s",network_buffer);
				}
				koord3d p = koord3d::invalid;
				uint16 id=0xFFFF;
				uint16 player_nr = PLAYER_UNOWNED;
				uint16 z_pos = -256;
				static char default_param[4096];
				long steps_nr = 0;
				uint32 random_counter;
				sscanf( network_buffer+5, "%li,%lu,%hi,%hi,%hi,%hi,%hi,%s" NET_END_CMD, &steps_nr, &random_counter, &id, &player_nr, &p.x, &p.y, &z_pos, default_param );
				size_t len = strlen(default_param);
				if(  len>0  &&  default_param[len-1]==';'  ) {
					default_param[len-1] = 0;
				}
				p.z = z_pos;
				if(  id==(SIMPLE_TOOL|WKZ_PAUSE)  ) {
					steps = steps_nr;
				}
				assert(  steps_nr==steps );
				werkzeug_t *wkz=NULL;
				if(  id&GENERAL_TOOL  ) {
					wkz = werkzeug_t::general_tool[id&0xFFF];
				}
				else if(  id&SIMPLE_TOOL  ) {
					wkz = werkzeug_t::simple_tool[id&0xFFF];
				}
				else if(  id&DIALOGE_TOOL  ) {
					wkz = werkzeug_t::dialog_tool[id&0xFFF];
				}
				if(  wkz  ) {
					const char *old_default_param = wkz->default_param;
					wkz->default_param = default_param;
					if(  init  ) {
						if(  wkz->init( this, spieler[player_nr] )  ) {
							set_dirty();
							if(wkz!=werkzeug[player_nr]) {
								// only exit, if it is not the same tool again ...
								werkzeug[player_nr]->exit(this,spieler[player_nr]);
							}
							werkzeug[player_nr] = wkz;
							// update cursor and lastpos
							if(  player_nr==active_player_nr  ) {
								zeiger->set_bild( wkz->cursor );
								werkzeug_last_pos = koord3d::invalid;
							}
						}
					}
					else {
						wkz->work( this, spieler[player_nr], p );
					}
					wkz->default_param = old_default_param;
				}

			}
			free( network_buffer );
			// when execute next command?
			if(  !command_queue.empty()  ) {
				next_command_step = atol( command_queue.front()+5 );
			}
		}

		// time for the next step?
		uint32 time = dr_time();
		if(  next_step_time<=time  ) {
			if(  step_mode&PAUSE_FLAG  ) {
				sync_step( 0, false, true );
				idle_time = 100;
			}
			else {
				if(  step_mode==FAST_FORWARD  ) {
					sync_step( 100, true, false );
					step();
				}
				else if(  step_mode==FIX_RATIO  ) {
					time_budget = next_step_time-time;
					next_step_time += frame_time;
					sync_step( frame_time, true, true );
					if(  network_frame_count++==4  ) {
						// ever fourth frame
						step();
						network_frame_count = 0;
						last_randoms[steps&15] = get_random_seed();
						if(  !umgebung_t::server  ) {
							int n = sprintf( network_buffer, NET_TO_SERVER NET_CHECK " %li,%lu", steps, last_randoms[steps&15] );
							network_send_server( network_buffer, n );
						}
					}
				}
				else {
					INT_CHECK( "karte_t::interactive()" );
					step();
					idle_time = ((idle_time*7) + next_step_time - dr_time())/8;
					INT_CHECK( "karte_t::interactive()" );
				}
			}
		}


		if (!swallowed) {
			interactive_event(ev);
		}

	} while(!finish_loop  &&  get_current_month()<quit_month);

	if(  get_current_month() >= quit_month  ) {
		umgebung_t::quit_simutrans = true;
	}

	return finish_loop;
}
