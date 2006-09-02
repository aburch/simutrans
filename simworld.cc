/*
 * simworld.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * simworld.cc
 *
 * Hauptklasse fuer Simutrans, Datenstruktur die alles Zusammenhaelt
 * Hansjoerg Malthaner, 1997
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef simdebug_h
#include "simdebug.h"
#endif

#ifndef tpl_vector_h
#include "tpl/vector_tpl.h"
#endif


#include "boden/boden.h"
#include "boden/wasser.h"
#include "simplay.h"
#include "simfab.h"
#include "simconvoi.h"
#include "simcity.h"
#include "simskin.h"
#include "simwin.h"
#include "simhalt.h"
#include "simdepot.h"
#include "simversion.h"
#include "simmesg.h"
#include "simcolor.h"
#include "simlinemgmt.h"

#include "simtime.h"
#include "simintr.h"
#include "simio.h"

#include "simimg.h"
#include "old_blockmanager.h"
#include "simvehikel.h"
#include "simverkehr.h"
#include "simworld.h"

#include "simwerkz.h"
#include "simtools.h"
#include "simsound.h"

#include "simgraph.h"
#include "simdisplay.h"

#include "dings/zeiger.h"
#include "dings/baum.h"
#include "dings/signal.h"
#include "dings/roadsign.h"
#include "dings/wayobj.h"
#include "dings/gebaeude.h"

#include "gui/welt.h"
#include "gui/karte.h"
#include "gui/map_frame.h"
#include "gui/optionen.h"
#include "gui/player_frame_t.h"
#include "gui/werkzeug_parameter_waehler.h"
#include "gui/messagebox.h"
#include "gui/loadsave_frame.h"
#include "gui/money_frame.h"
#include "gui/schedule_list.h"
#include "gui/convoi_frame.h"
#include "gui/halt_list_frame.h"        //30-Dec-01     Markus Weber    Added
#include "gui/citylist_frame_t.h"
#include "gui/message_frame_t.h"
#include "gui/help_frame.h"
#include "gui/goods_frame_t.h"

#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
#include "dataobj/tabfile.h"
#include "dataobj/powernet.h"

#include "utils/simstring.h"

#include "bauer/brueckenbauer.h"
#include "bauer/tunnelbauer.h"
#include "bauer/fabrikbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"
#include "bauer/vehikelbauer.h"

#include "besch/grund_besch.h"
#include "besch/sound_besch.h"

#include "ifc/sync_steppable.h"

#ifdef AUTOTEST
#include "test/worldtest.h"
#include "test/testtool.h"
//#include "test/buildings_frame_t.h"
#endif

static bool fast_forward = false;


//static schedule_list_gui_t * schedule_list_gui = 0;


//#define DEMO
//#undef DEMO

/**
 * anzahl ticks pro tag in bits
 * @see ticks_per_tag
 * @author Hj. Malthaner
 */
uint32 karte_t::ticks_bits_per_tag = 20;

/**
 * anzahl ticks pro tag
 * @author Hj. Malthaner
 */
uint32 karte_t::ticks_per_tag = (1 << 20);

// offsets for mouse pointer
const int karte_t::Z_PLAN      = 8;
const int karte_t::Z_GRID      = 0;
const int karte_t::Z_LINES     = 4;


void
karte_t::setze_scroll_multi(int n)
{
    einstellungen->setze_scroll_multi(n);
}



/**
 * Read a heightfield from file
 * @param filename name of heightfield file
 * @author Hj. Malthaner
 */
void
karte_t::calc_hoehe_mit_heightfield(const cstring_t & filename)
{
	FILE *file = fopen(filename, "rb");
	if(file) {
		const int display_total = 16 + gib_einstellungen()->gib_anzahl_staedte()*4 + gib_einstellungen()->gib_land_industry_chains() + gib_einstellungen()->gib_city_industry_chains();
		char buf [256];
		int w, h;

		read_line(buf, 255, file);

		if(strncmp(buf, "P6", 2)) {
			dbg->fatal("karte_t::load_heightfield()","Heightfield has wrong image type %s", buf);
		}

		read_line(buf, 255, file);
		sscanf(buf, "%d %d", &w, &h);
		if(w != gib_groesse_x()  || h != gib_groesse_y()) {
			dbg->fatal("karte_t::load_heightfield()","Heightfield has wrong size %s", buf);
		}

		read_line(buf, 255, file);
		if(strncmp(buf, "255", 2)) {
			dbg->fatal("karte_t::load_heightfield()","Heightfield has wrong color depth %s", buf);
		}

		int y;
		for(y=0; y<=gib_groesse_y(); y++) {
			for(int x=0; x<=gib_groesse_x(); x++) {
				setze_grid_hgt(koord(x,y), grundwasser);
			}
		}

		for(y=0; y<gib_groesse_y(); y++) {
			for(int x=0; x<gib_groesse_x(); x++) {
				int R = fgetc(file);
				int G = fgetc(file);
				int B = fgetc(file);

				setze_grid_hgt( koord(x,y), ((R*2+G*3+B)/4 - 224) & 0xFFF0 );
			}

			if(is_display_init()) {
				if(y==0) {
					display_proportional((display_get_width()-display_total)/2,display_get_height()/2-20,translator::translate("Init map ..."),ALIGN_LEFT,COL_WHITE,0);
				}
				display_progress((y*16)/gib_groesse_y(), display_total);
				display_flush(IMG_LEER, 0, 0, 0, "", "", 0, 0);
			}
		}

		fclose(file);
	}
	else {
		perror("Error:");
	}
}



/**
 * Hoehe eines Punktes der Karte mit "perlin noise"
 *
 * @param frequency in 0..1.0 roughness, the higher the rougher
 * @param amplitude in 0..160.0 top height of mountains, may not exceed 160.0!!!
 * @author Hj. Malthaner
 */
int
karte_t::perlin_hoehe(const int x, const int y,
                      const double frequency, const double amplitude)
{
//    double perlin_noise_2D(double x, double y, double persistence);
//    return ((int)(perlin_noise_2D(x, y, 0.6)*160.0)) & 0xFFFFFFF0;
    return ((int)(perlin_noise_2D(x, y, frequency)*amplitude)) & 0xFFFFFFF0;
}



void
karte_t::calc_hoehe_mit_perlin()
{
	const int display_total = 16 + gib_einstellungen()->gib_anzahl_staedte()*4 + gib_einstellungen()->gib_land_industry_chains() + gib_einstellungen()->gib_city_industry_chains();
	for(int y=0; y<=gib_groesse_y(); y++) {

		for(int x=0; x<=gib_groesse_x(); x++) {
			// Hajo: to Markus: replace the fixed values with your
			// settings. Amplitude is the top highness of the
			// montains, frequency is something like landscape 'roughness'
			// amplitude may not be greater than 160.0 !!!
			// please don't allow frequencies higher than 0.8 it'll
			// break the AI's pathfinding. Frequency values of 0.5 .. 0.7
			// seem to be ok, less is boring flat, more is too crumbled
			// the old defaults are given here: f=0.6, a=160.0
			const int h=perlin_hoehe(x,y,einstellungen->gib_map_roughness(),einstellungen->gib_max_mountain_height() );
			setze_grid_hgt(koord(x,y),h );
			//	  DBG_MESSAGE("karte_t::calc_hoehe_mit_perlin()","%i",h);
		}

		if(is_display_init()) {
			if(y==0) {
				display_proportional((display_get_width()-display_total)/2,display_get_height()/2-20,translator::translate("Init map ..."),ALIGN_LEFT,COL_WHITE,0);
			}
			display_progress((y*16)/gib_groesse_y(), display_total);
			display_flush(IMG_LEER, 0, 0, 0, "", "", 0, 0);
		}
		else {
			printf("X");fflush(NULL);
		}
	}
	print(" - ok\n");fflush(NULL);
}


void karte_t::raise_clean(int x, int y, int h)
{
  if(ist_in_gittergrenzen(x, y)) {
    const koord k (x,y);

    if(lookup_hgt(k) < h) {
      setze_grid_hgt(k, h);

#ifndef DOUBLE_GROUNDS
      raise_clean(x-1, y-1, h-16);
      raise_clean(x  , y-1, h-16);
      raise_clean(x+1, y-1, h-16);
      raise_clean(x-1, y  , h-16);
      // Punkt selbst hat schon die neue Hoehe
      raise_clean(x+1, y  , h-16);
      raise_clean(x-1, y+1, h-16);
      raise_clean(x  , y+1, h-16);
      raise_clean(x+1, y+1, h-16);
#else
      raise_clean(x-1, y-1, h-32);
      raise_clean(x  , y-1, h-32);
      raise_clean(x+1, y-1, h-32);
      raise_clean(x-1, y  , h-32);
      // Punkt selbst hat schon die neue Hoehe
      raise_clean(x+1, y  , h-32);
      raise_clean(x-1, y+1, h-32);
      raise_clean(x  , y+1, h-32);
      raise_clean(x+1, y+1, h-32);
#endif

    }
  }
}


void karte_t::cleanup_karte()
{
	// we need a copy to smoothen the map to a realistic level
	sint8 *grid_hgts_cpy = new sint8[(gib_groesse_x()+1)*(gib_groesse_y()+1)];
	memcpy(grid_hgts_cpy,grid_hgts,(gib_groesse_x()+1)*(gib_groesse_y()+1));
	// now connect the heights
	sint16 i,j;
	for(j=0; j<=gib_groesse_y(); j++) {
		for(i=0; i<=gib_groesse_x(); i++) {
			raise_clean(i,j, (grid_hgts_cpy[j*(gib_groesse_x()+1)+i]<<4)+16 );
		}
	}
	delete [] grid_hgts_cpy;

	// now lower the corners to ground level
	for(i=0; i<gib_groesse_x(); i++) {
		lower_to(i, 0, grundwasser,false);
		lower_to(i, gib_groesse_y(), grundwasser,false);
	}
	for(i=0; i<=gib_groesse_y(); i++) {
		lower_to(0, i, grundwasser,false);
		lower_to(gib_groesse_x(), i, grundwasser,false);
	}
	for(i=0; i<=gib_groesse_x(); i++) {
		raise_to(i, 0, grundwasser,false);
		raise_to(i, gib_groesse_y(), grundwasser,false);
	}
	for(i=0; i<=gib_groesse_y(); i++) {
		raise_to(0, i, grundwasser,false);
		raise_to(gib_groesse_x(), i, grundwasser,false);
	}
}



void karte_t::verteile_baeume(int dichte)
{
  // at first deafault values for forest creation will be set
  // Base forest size - minimal size of forest - map independent
  unsigned char forest_base_size = 36;

  // Map size divisor - smaller it is the larger are individual forests
  unsigned char forest_map_size_divisor = 38;

  // Forest count divisor - smaller it is, the more forest are generated
  unsigned char forest_count_divisor = 16;

  // Forest boundary sharpenss: 0 - perfectly sharp boundaries, 20 - very blurred
  unsigned char forest_boundary_blur = 6;

  // Forest boundary thickness  - determines how thick will the boundary line be
  unsigned char forest_boundary_thickness = 2;

  // Determins how often are spare trees going to be planted (works inversly)
  unsigned char forest_inverse_spare_tree_density = 5;

  //	Number of trees on square 2 - minimal usable, 3 good, 5 very nice looking
  setze_max_no_of_trees_on_square (3);

  //now forest configuration data will be read form forrestconf.tab
  tabfile_t simuconf;

  if(simuconf.open("config/forrestconf.tab")) {
    tabfileobj_t contents;

    simuconf.read(contents);

    forest_base_size = contents.get_int("forest_base_size", 36);
    forest_map_size_divisor = contents.get_int("forest_map_size_divisor", 38);
    forest_count_divisor = contents.get_int("forest_count_divisor", 16);
    forest_boundary_blur = contents.get_int("forest_boundary_blur", 6);
    forest_boundary_thickness = contents.get_int("forest_boundary_thickness", 2);
    forest_inverse_spare_tree_density = contents.get_int("forest_inverse_spare_tree_density", 5);


    setze_max_no_of_trees_on_square (contents.get_int("max_no_of_trees_on_square", 3));

    simuconf.close();
  } else {
    // nothing here, if reading fails, default values will remain
  }

  // now we can proceed to tree palnting routine itself
  // best forests results are produced if forest size is tied to map size -
  // but there is some nonlinearity to ensure good forests on small maps

  const unsigned int t_forest_size =
    (unsigned int)pow( gib_groesse_max()>>7 , 0.5)*forest_base_size + (gib_groesse_max()/forest_map_size_divisor);
  const unsigned char c_forest_count = gib_groesse_max()/forest_count_divisor;
  unsigned int  x_tree_pos, y_tree_pos, distance, tree_probability;
  unsigned char c2;

	DBG_MESSAGE("verteile_baeume()","c_forest_count %i",c_forest_count);

  for (unsigned char c1 = 0 ; c1 < c_forest_count ; c1++) {
    const unsigned int xpos_f = simrand(gib_groesse_x());
    const unsigned int ypos_f = simrand(gib_groesse_y());
    const unsigned int c_coef_x = 1+simrand(2);
    const unsigned int c_coef_y = 1+simrand(2);
    const unsigned int x_forest_boundary = t_forest_size*c_coef_x;
    const unsigned int y_forest_boundary = t_forest_size*c_coef_y;
    unsigned int i, j;

    for(j = 0; j < x_forest_boundary; j++) {
      for(i = 0; i < y_forest_boundary; i++) {

	x_tree_pos = (j-(t_forest_size>>1)); // >>1 works like 2 but is faster
	y_tree_pos = (i-(t_forest_size>>1));

	distance = 1 + ((int) sqrt( (double)(x_tree_pos*x_tree_pos/c_coef_x + y_tree_pos*y_tree_pos/c_coef_y)));

	tree_probability = (t_forest_size<<4) / distance; //is same as = ( 32 * (t_forest_size / 2) ) / distance

	for (c2 = 0 ; c2 < gib_max_no_of_trees_on_square() ; c2++) {
	  const unsigned int rating =
	    simrand(forest_boundary_blur) + 38 + c2*forest_boundary_thickness;

	  if (rating < tree_probability ) {
	    const koord pos ((short int)(xpos_f+x_tree_pos),
			     (short int)(ypos_f+y_tree_pos));

	    baum_t::plant_tree_on_coordinate(this,
					     pos,
					     gib_max_no_of_trees_on_square());
	  }
	}
      }
    }
  }

	DBG_MESSAGE("verteile_baeume()","distributing single trees");

	koord pos;
	for(pos.y=0;pos.y<gib_groesse_y(); pos.y++) {
		for(pos.x=0; pos.x<gib_groesse_x(); pos.x++) {
			//plant spare trees, (those with low preffered density)
			if(simrand(forest_inverse_spare_tree_density*dichte) < 100) {
				baum_t::plant_tree_on_coordinate(this, pos, 1);
			}
		}
	}
}




// karte_t methoden

void
karte_t::destroy()
{
DBG_MESSAGE("karte_t::destroy()", "destroying world");

	printf("Destroying world ... ");

	labels.clear();

	unsigned int i,j;

	// alle convois aufräumen
	while(convoi_array.get_count() > 0) {
		convoihandle_t cnv = convoi_array.at(convoi_array.get_count()-1);
		delete cnv.get_rep();	// since the convoi unbinds himself
	}
	convoi_array.clear();
DBG_MESSAGE("karte_t::destroy()", "convois destroyed");

	// alle haltestellen aufräumen
	haltestelle_t::destroy_all();
DBG_MESSAGE("karte_t::destroy()", "stops destroyed");

	// städte aufräumen
	if(stadt) {
		weighted_vector_tpl <stadt_t *> * tmp = stadt;
		stadt = 0;
			for(i=0; i<tmp->get_count(); i++) {
		delete tmp->at(i);
		}
		tmp->clear();
		delete tmp;
		tmp = 0;
	}
DBG_MESSAGE("karte_t::destroy()", "towns destroyed");

	// entfernt alle synchronen objekte aus der liste
	sync_list.clear();
DBG_MESSAGE("karte_t::destroy()", "sync list cleared");

	// dinge aufräumen
	if(plan) {
		for(j=0; j<(unsigned int)gib_groesse_y(); j++) {
		for(i=0; i<(unsigned int)gib_groesse_x(); i++) {
			access(i, j)->destroy(NULL);
//			DBG_MESSAGE("karte_t::destroy()", "planquadrat %i,%i destroyed",i,j);
		}
	}

	delete [] plan;
	plan = 0;
	}
	DBG_MESSAGE("karte_t::destroy()", "planquadrat destroyed");

	// gitter aufräumen
	if(grid_hgts) {
		delete [] grid_hgts;
		grid_hgts = 0;
	}

	// marker aufräumen
	marker.init(0,0);
DBG_MESSAGE("karte_t::destroy()", "marker destroyed");

	// spieler aufräumen
	for(i=0; i<MAX_PLAYER_COUNT ; i++) {
		if(spieler[i]) {
			delete spieler[i];
			spieler[i] = 0;
		}
	}
DBG_MESSAGE("karte_t::destroy()", "player destroyed");

	// clear all ids
	simlinemgmt_t::init_line_ids();
DBG_MESSAGE("karte_t::destroy()", "lines destroyed");

	// alle fabriken aufräumen
	slist_iterator_tpl<fabrik_t*> fab_iter(fab_list);
	while(fab_iter.next()) {
		delete fab_iter.get_current();
	}
	fab_list.clear();
DBG_MESSAGE("karte_t::destroy()", "factories destroyed");

	// hier nur entfernen, aber nicht löschen
	ausflugsziele.clear();
DBG_MESSAGE("karte_t::destroy()", "attraction list destroyed");

//    delete schedule_list_gui;
//    schedule_list_gui = 0;

DBG_MESSAGE("karte_t::destroy()", "world destroyed");
	printf("destroyed.\n");
}



/**
 * Zugriff auf das Städte Array.
 * @author Hj. Malthaner
 */
const stadt_t *karte_t::get_random_stadt() const
{
	return stadt->at_weight( simrand(stadt->get_sum_weight()) );
}

void karte_t::add_stadt(stadt_t *s)
{
	einstellungen->setze_anzahl_staedte(einstellungen->gib_anzahl_staedte()+1);
	stadt->append(s,s->gib_einwohner(),64);
}

/**
 * Removes town from map, houses will be left overs
 * @author prissi
 */
bool karte_t::rem_stadt(stadt_t *s)
{
	if(stadt->get_count()==0  ||  s==NULL) {
		// now twon there to delete ...
		return false;
	}
	// ok, we can delete this
	stadt->remove( s );
	delete s;
	// reduce number of towns
	einstellungen->setze_anzahl_staedte(einstellungen->gib_anzahl_staedte()-1);
	// remove all links from factories
	slist_iterator_tpl<fabrik_t *> iter (fab_list);
	while(iter.next()) {
		iter.get_current()->rem_arbeiterziel(s);
	}
	return true;
}


void
karte_t::init_felder()
{
    plan   = new planquadrat_t[gib_groesse_x()*gib_groesse_y()];
    grid_hgts = new sint8[(gib_groesse_x()+1)*(gib_groesse_y()+1)];

    memset(grid_hgts, 0, sizeof(sint8)*(gib_groesse_x()+1)*(gib_groesse_y()+1));

    marker.init(gib_groesse_x(),gib_groesse_y());

    simlinemgmt_t::init_line_ids();

    win_setze_welt( this );
    reliefkarte_t::gib_karte()->setze_welt(this);

    for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
        spieler[i] = new spieler_t(this, i*4, i);
/********************************************************************** automat
        if(i>=2) {
        	// otherwise during loading things are not belonging to an AI ...
        	spieler[i]->automat = true;
        }
*/
    }
    active_player = spieler[0];
    active_player_nr = 0;

	// defaults without timeline
	average_speed[0] = 60;
	average_speed[1] = 80;
	average_speed[2] = 40;
	average_speed[3] = 350;
}


void
karte_t::init(einstellungen_t *sets)
{
	mute_sound(true);
	int i, j;

	destroy();
	intr_disable();

	if(is_display_init()) {
		display_show_pointer(false);
	}
	setze_ij_off(koord(0,0));

	x_off = y_off = 0;

	ticks = 0;
	schedule_counter = 0;
	// ticks = 0x7FFFF800;  // Testing the 31->32 bit step

	letzter_monat = 0;
	letztes_jahr = sets->gib_starting_year();//umgebung_t::starting_year;
	current_month = letzter_monat + (letztes_jahr*12);
	setze_ticks_bits_per_tag(sets->gib_bits_per_month());
	next_month_ticks =  karte_t::ticks_per_tag;
	season = 2;
	steps = 0;

	einstellungen = sets;

	grundwasser = sets->gib_grundwasser();      //29-Nov-01     Markus Weber    Changed

	if(sets->gib_beginner_mode()) {
		warenbauer_t::set_multiplier( umgebung_t::beginner_price_factor );
		sets->setze_just_in_time( 0 );
	}

	// wird gecached, um den Pointerzugriff zu sparen, da
	// die groesse _sehr_ oft referenziert wird
	cached_groesse_gitter_x = einstellungen->gib_groesse_x();
	cached_groesse_gitter_y = einstellungen->gib_groesse_y();
	cached_groesse_max = max(cached_groesse_gitter_x,cached_groesse_gitter_y);
	cached_groesse_karte_x = cached_groesse_gitter_x-1;
	cached_groesse_karte_y = cached_groesse_gitter_y-1;

DBG_DEBUG("karte_t::init()","init_felder");
	init_felder();

DBG_DEBUG("karte_t::init()","setze_grid_hgt");
	for(j=0; j<=gib_groesse_y(); j++) {
		for(i=0; i<=gib_groesse_x(); i++) {
			setze_grid_hgt(koord(i, j), 0);
		}
	}

DBG_DEBUG("karte_t::init()","kartenboden_setzen");
	for(sint16 y=0; y<gib_groesse_y(); y++) {
		for(sint16 x=0; x<gib_groesse_x(); x++) {
			access(x,y)->kartenboden_setzen( new boden_t(this, koord3d(x,y, 0), 0 ), false);
		}
	}

	print("Creating landscape shape...\n");
	// calc_hoehe(0, 0, gib_groesse()-1, gib_groesse()-1);

DBG_DEBUG("karte_t::init()","calc_hoehe_mit_heightfield");
	setsimrand( 0xFFFFFFFF, einstellungen->gib_karte_nummer() );
	if(einstellungen->heightfield.len() > 0) {
		calc_hoehe_mit_heightfield(einstellungen->heightfield);
	}
	else {
		calc_hoehe_mit_perlin();
	}

DBG_DEBUG("karte_t::init()","cleanup karte");
	cleanup_karte();

DBG_DEBUG("karte_t::init()","set ground");
	// Hajo: init slopes
	koord k;
	for(k.y=0; k.y<gib_groesse_y(); k.y++) {
		for(k.x=0; k.x<gib_groesse_x(); k.x++) {
			access(k)->abgesenkt(this);
			lookup(k)->gib_kartenboden()->setze_grund_hang( calc_natural_slope(k) );
		}
	}

DBG_DEBUG("karte_t::init()","distributing trees");
    verteile_baeume(3);

DBG_DEBUG("karte_t::init()","built timeline");
    stadtauto_t::built_timeline_liste(this);

    print("Creating cities ...\n");

DBG_DEBUG("karte_t::init()","hausbauer_t::neue_karte()");
    hausbauer_t::neue_karte();

	// make sure, we have a city road ...
	city_road = wegbauer_t::gib_besch(umgebung_t::city_road_type->chars(),get_timeline_year_month());
	if(!city_road) {
		city_road = wegbauer_t::weg_search(weg_t::strasse,30,get_timeline_year_month());
	}


DBG_DEBUG("karte_t::init()","prepare cities");
	stadt = new weighted_vector_tpl <stadt_t *>(0);
	vector_tpl<koord> *pos = stadt_t::random_place(this, einstellungen->gib_anzahl_staedte());

	if(pos!=NULL  &&  pos->get_count()>0) {
		// prissi if we could not generate enough positions ...
		einstellungen->setze_anzahl_staedte( pos->get_count() );	// new number of towns (if we did not found enough positions) ...
		const int max_display_progress=16+einstellungen->gib_anzahl_staedte()*4+einstellungen->gib_land_industry_chains()+einstellungen->gib_city_industry_chains();

		// Ansicht auf erste Stadt zentrieren
		setze_ij_off(pos->at(0) + koord(-8, -8));

		for(i=0; i<einstellungen->gib_anzahl_staedte(); i++) {

//			int citizens=(int)(einstellungen->gib_mittlere_einwohnerzahl()*0.9);
//			citizens = citizens/10+simrand(2*citizens+1);

			int current_citicens = (2500l * einstellungen->gib_mittlere_einwohnerzahl()) /(simrand(20000)+100);

			stadt_t *s = new stadt_t(this, spieler[1], pos->at(i), current_citicens );
DBG_DEBUG("karte_t::init()","Erzeuge stadt %i with %ld inhabitants",i,(s->get_city_history_month())[HIST_CITICENS] );
			stadt->append( s, current_citicens, 64 );
		}

		for(i=0; i<einstellungen->gib_anzahl_staedte(); i++) {
			// Hajo: do final init after world was loaded/created
			if(stadt->at(i)) {
				stadt->at(i)->laden_abschliessen();
			}
			// the growth is slow, so update here the progress bar
			if(is_display_init()) {
				display_progress(16+i*2, max_display_progress);
				display_flush(IMG_LEER,0, 0, 0, "", "", 0, 0);
			}
			else {
				printf("*");fflush(NULL);
			}
		}

		// Hajo: connect some cities with roads
		const weg_besch_t * besch = wegbauer_t::gib_besch(umgebung_t::intercity_road_type->chars());
		if(besch == 0) {
			dbg->warning("karte_t::init()","road type '%s' not found",umgebung_t::intercity_road_type->chars());
			// Hajo: try some default (might happen with timeline ... )
			besch = wegbauer_t::weg_search(weg_t::strasse,80,get_timeline_year_month());
		}

		// Hajo: No owner so that roads can be removed!
		wegbauer_t bauigel (this, 0);
		bauigel.baubaer = false;
		bauigel.route_fuer(wegbauer_t::strasse, besch);
		bauigel.set_keep_existing_ways(true);
		bauigel.set_maximum(umgebung_t::intercity_road_length);

		// Hajo: search for road offset
		koord roff (0,1);
		if(lookup(pos->at(0)+roff)->gib_kartenboden()->gib_weg(weg_t::strasse) == 0) {
			roff = koord(0,2);
		}

		int old_progress_count = 16+2*einstellungen->gib_anzahl_staedte();
		int count = 0;
		const int max_count=(einstellungen->gib_anzahl_staedte()*(einstellungen->gib_anzahl_staedte()-1))/2;

		for(i=0; i<einstellungen->gib_anzahl_staedte(); i++) {
			for(int j=i+1; j<einstellungen->gib_anzahl_staedte(); j++) {
				const koord k1 = pos->at(i) + roff;
				const koord k2 = pos->at(j) + roff;
				const koord diff = k1-k2;
				const int d = diff.x*diff.x + diff.y*diff.y;

				if(d < umgebung_t::intercity_road_length) {
//DBG_DEBUG("karte_t::init()","built route fom city %d to %d", i, j);
					bauigel.calc_route(k1, k2);
					if(bauigel.max_n >= 1) {
						bauigel.baue();
					}
					else {
//DBG_DEBUG("karte_t::init()","no route found fom city %d to %d", i, j);
					}
				}
				else {
//DBG_DEBUG("karte_t::init()","cites %d and %d are too far away", i, j);
				}
				count ++;
				// how much we continued?
				if(is_display_init()) {
					int progress_count = 16+einstellungen->gib_anzahl_staedte()*2+ (count*einstellungen->gib_anzahl_staedte()*2)/max_count;
					if(old_progress_count!=progress_count) {
						display_progress(progress_count, max_display_progress );
						display_flush(IMG_LEER,0, 0, 0, "", "", 0, 0);
						old_progress_count = progress_count;
					}
				}
			} //for j
		} // for i

		delete pos;
	}
	pos = NULL;

    // prissi: completely change format
     fabrikbauer_t::verteile_industrie(this, gib_spieler(1),	einstellungen->gib_land_industry_chains(),false);
     fabrikbauer_t::verteile_industrie(this, gib_spieler(1), einstellungen->gib_city_industry_chains(),true);
	// crossconnect all?
	if(umgebung_t::crossconnect_factories) {
		slist_iterator_tpl <fabrik_t *> iter (this->fab_list);
		while( iter.next() ) {
			iter.get_current()->add_all_suppliers();
		}
	}

	// tourist attractions
     fabrikbauer_t::verteile_tourist(this, gib_spieler(1), einstellungen->gib_tourist_attractions());

    print("Preparing startup ...\n");

    if(zeiger == 0) {
      zeiger = new zeiger_t(this, koord3d(0,0,0), spieler[0]);
    }

	// finishes the line preparation and sets id 0 to invalid ...
	spieler[0]->simlinemgmt.laden_abschliessen();

    mouse_funk = NULL;
    setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), Z_PLAN,  NO_SOUND, NO_SOUND );

    recalc_average_speed();

#ifndef DEMO
    for(i = 0; i < 6; i++) {
	spieler[i + 2]->set_active( umgebung_t::automaten[i] );
    }
#endif

	// change grounds to winter?
	const int current_season=(2+letzter_monat/3)&3; // summer always zero
	boden_t::toggle_season( current_season );
	setze_dirty();

    intr_enable();

	if(is_display_init()) {
		display_show_pointer(true);
	}
	mute_sound(false);
}

karte_t::karte_t() : convoi_array(0), ausflugsziele(16), quick_shortcuts(15), marker(0,0)
{
	for (unsigned int i=0; i<15; i++) {
//DBG_MESSAGE("karte_t::karte_t()","Append NULL to quick_shortcuts at %d\n",i);
		quick_shortcuts.append(NULL);
	}

	follow_convoi = convoihandle_t();
	setze_dirty();
	//    set_rotation(0);
	set_scroll_lock(false);

	einstellungen_t * sets = new einstellungen_t();

	if(umgebung_t::testlauf) {
		sets->setze_groesse(256,384);
		sets->setze_anzahl_staedte(16);
		sets->setze_land_industry_chains(8);
		sets->setze_city_industry_chains(4);
		sets->setze_tourist_attractions(8);
		sets->setze_verkehr_level(7);
		sets->setze_karte_nummer( 33 );
		sets->setze_station_coverage( umgebung_t::station_coverage_size );
	}
	else {
		sets->setze_groesse(64,64);
		sets->setze_anzahl_staedte(1);
		sets->setze_land_industry_chains(1);
		sets->setze_city_industry_chains(0);
		sets->setze_tourist_attractions(1);
		sets->setze_verkehr_level(7);
		sets->setze_karte_nummer( 33 );
		sets->setze_station_coverage( umgebung_t::station_coverage_size );
	}

	sets->setze_starting_year( umgebung_t::starting_year );
	sets->setze_use_timeline( umgebung_t::use_timeline==1 );
	sets->setze_bits_per_month( umgebung_t::bits_per_month );
	sets->setze_just_in_time( umgebung_t::just_in_time );

	// standard prices
	warenbauer_t::set_multiplier( 1000 );

	stadt = 0;
	zeiger = 0;
	plan = 0;
	grid_hgts = 0;
	einstellungen = 0;
	schedule_counter = 0;

	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		spieler[i] = 0;
	}

	init(sets);
}

karte_t::~karte_t()
{
    destroy();

    if(einstellungen) {
        delete einstellungen;
        einstellungen = NULL;
    }
}

bool karte_t::can_lower_plan_to(int x, int y, int h) const
{
    const planquadrat_t *plan = lookup(koord(x,y));

    if(plan == 0 || !is_plan_height_changeable(x, y)) {
	return false;
    }

    int hmax = plan->gib_kartenboden()->gib_hoehe();
    //
    // irgendwo ein Tunnel vergraben?
    //
    while(h < hmax) {
	if(plan->gib_boden_in_hoehe(h)) {
	    return false;
	}
	h += 16;
    }
    return true;
}

bool karte_t::can_raise_plan_to(int x, int y, int h) const
{
    const planquadrat_t *plan = lookup(koord(x,y));

    if(plan == 0 || !is_plan_height_changeable(x, y)) {
	return false;
    }

    int hmin = plan->gib_kartenboden()->gib_hoehe();
    //
    // irgendwo eine Brücke im Weg?
    //
    while(h > hmin) {
	if(plan->gib_boden_in_hoehe(h)) {
	    return false;
	}
	h -= 16;
    }
    return true;
}


bool karte_t::is_plan_height_changeable(int x, int y) const
{
    const planquadrat_t *plan = lookup(koord(x,y));
    bool ok = true;

    if(plan != NULL) {
	grund_t *gr = plan->gib_kartenboden();

	ok = (gr->ist_natur() || gr->ist_wasser())  &&  !gr->hat_wege();

	for(int i=0; ok && i<gr->gib_top(); i++) {
	    const ding_t *dt = gr->obj_bei(i);
	    if(dt != NULL) {
		ok =
		    dt->gib_typ() == ding_t::baum  ||
		    dt->gib_typ() == ding_t::zeiger  ||
		    dt->gib_typ() == ding_t::wolke  ||
		    dt->gib_typ() == ding_t::sync_wolke  ||
		    dt->gib_typ() == ding_t::async_wolke;
	    }
	}
    }

    return ok;
}


bool karte_t::can_raise_to(int x, int y, int h) const
{
    const planquadrat_t *plan = lookup(koord(x,y));
    bool ok = true;		// annahme, es geht, pruefung ob nicht

    if(plan) {
	if(lookup_hgt(koord(x, y)) < h) {
	    ok =
		// Nachbar-Planquadrate testen
		can_raise_plan_to(x-1,y-1, h) &&
		can_raise_plan_to(x,y-1, h)   &&
		can_raise_plan_to(x-1,y, h)   &&
		can_raise_plan_to(x,y, h)     &&
#ifndef DOUBLE_GROUNDS
		// Nachbar-Gridpunkte testen
		can_raise_to(x-1, y-1, h-16) &&
		can_raise_to(x  , y-1, h-16) &&
		can_raise_to(x+1, y-1, h-16) &&
		can_raise_to(x-1, y  , h-16) &&
		can_raise_to(x+1, y  , h-16) &&
		can_raise_to(x-1, y+1, h-16) &&
		can_raise_to(x  , y+1, h-16) &&
		can_raise_to(x+1, y+1, h-16);
#else
		// Nachbar-Gridpunkte testen
		can_raise_to(x-1, y-1, h-32) &&
		can_raise_to(x  , y-1, h-32) &&
		can_raise_to(x+1, y-1, h-32) &&
		can_raise_to(x-1, y  , h-32) &&
		can_raise_to(x+1, y  , h-32) &&
		can_raise_to(x-1, y+1, h-32) &&
		can_raise_to(x  , y+1, h-32) &&
		can_raise_to(x+1, y+1, h-32);
#endif
	}
    } else {
	ok = false;
    }
    return ok;
}

bool karte_t::can_raise(int x, int y) const
{
	if(ist_in_gittergrenzen(x, y)) {
		return can_raise_to(x, y, lookup_hgt(koord(x, y))+16);
	} else {
		return true;
	}
}

int karte_t::raise_to(sint16 x, sint16 y, sint16 h,bool set_slopes)
{
	const koord pos (x,y);
	int n = 0;

	if(ist_in_gittergrenzen(x,y)) {
		if(lookup_hgt(pos) < h) {
			setze_grid_hgt(pos, h);

			n = 1;

#ifndef DOUBLE_GROUNDS
			n += raise_to(x-1, y-1, h-16,set_slopes);
			n += raise_to(x  , y-1, h-16,set_slopes);
			n += raise_to(x+1, y-1, h-16,set_slopes);
			n += raise_to(x-1, y  , h-16,set_slopes);

			n += raise_to(x, y, h,set_slopes);

			n += raise_to(x+1, y  , h-16,set_slopes);
			n += raise_to(x-1, y+1, h-16,set_slopes);
			n += raise_to(x  , y+1, h-16,set_slopes);
			n += raise_to(x+1, y+1, h-16,set_slopes);
#else
			n += raise_to(x-1, y-1, h-32,set_slopes);
			n += raise_to(x  , y-1, h-32,set_slopes);
			n += raise_to(x+1, y-1, h-32,set_slopes);
			n += raise_to(x-1, y  , h-32,set_slopes);

			n += raise_to(x, y, h);

			n += raise_to(x+1, y  , h-32,set_slopes);
			n += raise_to(x-1, y+1, h-32,set_slopes);
			n += raise_to(x  , y+1, h-32,set_slopes);
			n += raise_to(x+1, y+1, h-32,set_slopes);
#endif
			if(set_slopes) {
				planquadrat_t *plan;
				if((plan=access(x,y))) {
					plan->gib_kartenboden()->setze_grund_hang( calc_natural_slope(pos) );
					plan->angehoben( this );
				}

				if((plan = access(x-1,y))) {
					plan->gib_kartenboden()->setze_grund_hang( calc_natural_slope(pos+koord(-1,0)) );
					plan->angehoben( this );
				}

				if((plan = access(x,y-1))) {
					plan->gib_kartenboden()->setze_grund_hang( calc_natural_slope(pos+koord(0,-1)) );
					plan->angehoben( this );
				}

				if((plan = access(x-1,y-1))) {
					plan->gib_kartenboden()->setze_grund_hang( calc_natural_slope(pos+koord(-1,-1)) );
					plan->angehoben( this );
				}
			}

		}
	}

	return n;
}


int karte_t::raise(koord pos)
{
	bool ok = can_raise(pos.x, pos.y);
	int n = 0;
	if(ok && ist_in_gittergrenzen(pos)) {
		n = raise_to(pos.x, pos.y, lookup_hgt(pos)+16,true);
	}
	return n;
}

bool karte_t::can_lower_to(int x, int y, int h) const
{
    const planquadrat_t *plan = lookup(koord(x,y));
    bool ok = true;		// annahme, es geht, pruefung ob nicht

    if(plan) {
	if(lookup_hgt(koord(x, y)) > h) {
	    ok =
		// Nachbar-Planquadrate testen
		can_lower_plan_to(x-1,y-1, h) &&
		can_lower_plan_to(x,y-1, h)   &&
		can_lower_plan_to(x-1,y, h)   &&
		can_lower_plan_to(x,y, h)     &&
#ifndef DOUBLE_GROUNDS
		// Nachbar-Gridpunkte testen
		can_lower_to(x-1, y-1, h+16) &&
		can_lower_to(x  , y-1, h+16) &&
		can_lower_to(x+1, y-1, h+16) &&
		can_lower_to(x-1, y  , h+16) &&
		can_lower_to(x+1, y  , h+16) &&
		can_lower_to(x-1, y+1, h+16) &&
		can_lower_to(x  , y+1, h+16) &&
		can_lower_to(x+1, y+1, h+16);
#else
		// Nachbar-Gridpunkte testen
		can_lower_to(x-1, y-1, h+32) &&
		can_lower_to(x  , y-1, h+32) &&
		can_lower_to(x+1, y-1, h+32) &&
		can_lower_to(x-1, y  , h+32) &&
		can_lower_to(x+1, y  , h+32) &&
		can_lower_to(x-1, y+1, h+32) &&
		can_lower_to(x  , y+1, h+32) &&
		can_lower_to(x+1, y+1, h+32);
#endif
	}
    } else {
	ok = false;
    }
    return ok;
}



bool karte_t::can_lower(int x, int y) const
{
    if(ist_in_gittergrenzen(x, y)) {
	return can_lower_to(x, y, lookup_hgt(koord(x, y))-16);
    } else {
	return true;
    }
}



int karte_t::lower_to(sint16 x, sint16 y, sint16 h,bool set_slopes)
{
	int n = 0;
	if(ist_in_gittergrenzen(x,y)) {
		const koord pos (x, y);

		if(lookup_hgt(pos) > h) {
			setze_grid_hgt(pos, h);

			n = 1;

#ifndef DOUBLE_GROUNDS
			n += lower_to(x-1, y-1, h+16,set_slopes);
			n += lower_to(x  , y-1, h+16,set_slopes);
			n += lower_to(x+1, y-1, h+16,set_slopes);
			n += lower_to(x-1, y  , h+16,set_slopes);

			n += lower_to(x, y, h,set_slopes);

			n += lower_to(x+1, y  , h+16,set_slopes);
			n += lower_to(x-1, y+1, h+16,set_slopes);
			n += lower_to(x  , y+1, h+16,set_slopes);
			n += lower_to(x+1, y+1, h+16,set_slopes);
#else
			n += lower_to(x-1, y-1, h+32,set_slopes);
			n += lower_to(x  , y-1, h+32,set_slopes);
			n += lower_to(x+1, y-1, h+32,set_slopes);
			n += lower_to(x-1, y  , h+32,set_slopes);

			n += lower_to(x, y, h,set_slopes);

			n += lower_to(x+1, y  , h+32,set_slopes);
			n += lower_to(x-1, y+1, h+32,set_slopes);
			n += lower_to(x  , y+1, h+32,set_slopes);
			n += lower_to(x+1, y+1, h+32,set_slopes);
#endif
			if(set_slopes) {
				planquadrat_t *plan;
				if((plan=access(x,y))) {
					plan->gib_kartenboden()->setze_grund_hang( calc_natural_slope(pos) );
					plan->abgesenkt( this );
				}

				if((plan = access(x-1,y))) {
					plan->gib_kartenboden()->setze_grund_hang( calc_natural_slope(pos+koord(-1,0)) );
					plan->abgesenkt( this );
				}

				if((plan = access(x,y-1))) {
					plan->gib_kartenboden()->setze_grund_hang( calc_natural_slope(pos+koord(0,-1)) );
					plan->abgesenkt( this );
				}

				if((plan = access(x-1,y-1))) {
					plan->gib_kartenboden()->setze_grund_hang( calc_natural_slope(pos+koord(-1,-1)) );
					plan->abgesenkt( this );
				}
			}
		}
	}
  return n;
}



int karte_t::lower(koord pos)
{
	bool ok = can_lower(pos.x, pos.y);
	int n = 0;
	if(ok && ist_in_gittergrenzen(pos.x, pos.y)) {
		n = lower_to(pos.x, pos.y, lookup_hgt(pos)-16,true);
	}
	return n;
}



bool
karte_t::ebne_planquadrat(koord pos, int hgt)
{
	koord offsets[] = {koord(0,0), koord(1,0), koord(0,1), koord(1,1)};
	bool ok = true;

	for(int i=0; i<4; i++) {
		koord p = pos + offsets[i];

		if(lookup_hgt(p) > hgt) {

			if(can_lower_to(p.x, p.y, hgt)) {
				lower_to(p.x, p.y, hgt,true);
			} else {
				ok = false;
			}

		} else if(lookup_hgt(p) < hgt) {

			if(can_raise_to(p.x, p.y, hgt)) {
				raise_to(p.x, p.y, hgt,true);
			} else {
				ok = false;
			}
		}
	}
	return ok;
}



void
karte_t::setze_maus_funktion(int (* funktion)(spieler_t *, karte_t *, koord),
                             int zeiger_bild,
			     int zeiger_versatz,
			     int ok_sound,
			     int ko_sound)
{
    setze_maus_funktion(
	(int (*)(spieler_t *, karte_t *, koord, value_t))funktion,
	zeiger_bild, zeiger_versatz, 0l, ok_sound, ko_sound);
}



/**
 * Spezialvarainte mit einem Parameter, der immer übergeben wird
 * Hajo: changed parameter type from long to value_t because some
 *       parts of the code pass pointers
 * @author V. Meyer, Hj. Malthaner
 */
void
karte_t::setze_maus_funktion(int (* funktion)(spieler_t *, karte_t *, koord,
					      value_t param),
                             int zeiger_bild,
			     int zeiger_versatz,
			     value_t param,
			     int ok_sound,
			     int ko_sound)
{
    // gibt es eien neue funktion ?
    if(funktion != NULL) {
	// gab es eine alte funktion ?
	if(mouse_funk != NULL) {
	    mouse_funk(get_active_player(), this, EXIT, mouse_funk_param);
	}

	struct sound_info info;
	info.index = SFX_SELECT;
	info.volume = 255;
	info.pri = 0;
	sound_play(info);

	mouse_funk = funktion;
	mouse_funk_param = param;
	zeiger->setze_yoff(zeiger_versatz);
	zeiger->setze_bild(0, zeiger_bild);
	zeiger->set_flag(ding_t::dirty);

	mouse_funk(get_active_player(), this, INIT, mouse_funk_param);
    }

    mouse_funk_ok_sound = ok_sound;
    mouse_funk_ko_sound = ko_sound;
}


void karte_t::new_mountain(int x, int y, int w, int h, int t)
{
    int i,j,n;

    DBG_MESSAGE("karte_t::new_mountain()",
		 "New mountain %d+%d, %d+%d, %d",x,w,y,h,t);

    for(n=0; n<abs(t); n++){
	for(i=x; i<=x+w; i++) {
	    for(j=y; j<=y+h; j++) {
		if(t < 0) {
		    lower(koord(i,j));
		} else {
		    raise(koord(i,j));
		}
	    }
	}
    }
}



/**
 * Sets grid height.
 * Never set grid_hgts manually, always
 * use this method.
 * @author Hj. Malthaner
 */
void karte_t::setze_grid_hgt(koord k, int hgt)
{
  if(ist_in_gittergrenzen(k.x, k.y)) {
    grid_hgts[k.x + k.y*(gib_groesse_x()+1)] = (hgt >> 4);
  }
}


int
karte_t::min_hgt(const koord pos) const
{
    const int h1 = lookup_hgt(pos);
    const int h2 = lookup_hgt(pos+koord(1, 0));
    const int h3 = lookup_hgt(pos+koord(1, 1));
    const int h4 = lookup_hgt(pos+koord(0, 1));

    return min(min(h1,h2), min(h3,h4));
}


int
karte_t::max_hgt(const koord pos) const
{
    const int h1 = lookup_hgt(pos);
    const int h2 = lookup_hgt(pos+koord(1, 0));
    const int h3 = lookup_hgt(pos+koord(1, 1));
    const int h4 = lookup_hgt(pos+koord(0, 1));

    return max(max(h1,h2), max(h3,h4));
}


// -------- Verwaltung von Fabriken -----------------------------


bool
karte_t::add_fab(fabrik_t *fab)
{
//DBG_MESSAGE("karte_t::add_fab()","fab = %p",fab);
	assert(fab != NULL);
	fab_list.insert( fab );
	return true;
}

bool
karte_t::rem_fab(fabrik_t *fab)
{
	assert(fab != NULL);
	fab_list.remove( fab );
	return true;
}

fabrik_t *
karte_t::get_random_fab() const
{
	const int anz = fab_list.count();
	fabrik_t *result = NULL;

	if(anz > 0) {
		const int end = simrand( anz );
		result = (fabrik_t *)fab_list.at(end);
	}
	return result;
}


/* return all factories in this area
 * @author prissi
 */
vector_tpl<fabrik_t *> &
karte_t::find_all_factories( koord pos, koord extent )
{
	static vector_tpl <fabrik_t*> fablist(32);
	fablist.clear();

	slist_iterator_tpl<fabrik_t *> iter (this->fab_list);
	while(iter.next()) {
		fabrik_t * fab = iter.get_current();
		if(fab->is_fabrik(pos,extent)) {
			fablist.append(fab);
//DBG_MESSAGE("karte_t::find_all_factories()","append %s at (%i,%i)",fab->gib_name(),fab->gib_pos().x,fab->gib_pos().y);
		}
	}
	return fablist;
}

/*----------------------------------------------------------------------------------------------------------------------*/
/* same procedure for tourist attractions */

void
karte_t::add_ausflugsziel(gebaeude_t *gb)
{
	assert(gb != NULL);
	ausflugsziele.append( gb, gb->gib_tile()->gib_besch()->gib_level(), 16 );
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
karte_t::gib_random_ausflugsziel() const
{
	const unsigned long sum_pax=ausflugsziele.get_sum_weight();
	if(ausflugsziele.get_count()>0  &&  sum_pax>0) {
		return ausflugsziele.at_weight( simrand(sum_pax) );
	}
	// so there are no destinations ... should never occur ...
	dbg->fatal("karte_t::gib_random_ausflugsziel()","nothing found.");
	return NULL;
}


// -------- Verwaltung von Staedten -----------------------------

stadt_t *
karte_t::suche_naechste_stadt(const koord pos) const
{
    long min_dist = 99999999;
    stadt_t * best = NULL;

    for(unsigned n=0; n<stadt->get_count(); n++) {
	    const koord k = stadt->at(n)->gib_pos();

	    const long dist = (pos.x-k.x)*(pos.x-k.x) + (pos.y-k.y)*(pos.y-k.y);

	    if(dist < min_dist) {

//		printf("dist %d  stadt %d\n", dist, n);
		min_dist = dist;
		best = stadt->at(n);
	    }
    }
    return best;
}

stadt_t *
karte_t::suche_naechste_stadt(const koord pos, const stadt_t *letzte) const
{
    int min_dist = 99999999;
    stadt_t * best = NULL;
    const koord letzte_pos = letzte->gib_pos();
    const int letzte_dist = (letzte_pos.x-pos.x)*(letzte_pos.x-pos.x) + (letzte_pos.y-pos.y)*(letzte_pos.y-pos.y);
    bool letzte_gefunden = false;

    for(unsigned n=0; n<stadt->get_count(); n++) {
	    const koord k = stadt->at(n)->gib_pos();

	    const int dist = (pos.x-k.x)*(pos.x-k.x) + (pos.y-k.y)*(pos.y-k.y);

	    if(stadt->at(n) == letzte) {
		letzte_gefunden = true;
	    }
	    else if(letzte_gefunden ? dist >= letzte_dist : dist > letzte_dist) {
		if(dist < min_dist) {
//		    printf("dist %d  stadt %d\n", dist, n);
		    min_dist = dist;
		    best = stadt->at(n);
		}
	    }
    }
    return best;
}



// -------- Verwaltung von synchronen Objekten ------------------

bool
karte_t::sync_add(sync_steppable *obj)
{
	assert(obj != NULL);
	sync_add_list.insert( obj );
	return true;
}

bool
karte_t::sync_remove(sync_steppable *obj)	// entfernt alle dinge == obj aus der Liste
{
	assert(obj != NULL);
	if(sync_add_list.remove(obj)) {
		return true;
	}
	return sync_list.remove( obj );
}

void
karte_t::sync_prepare()
{
	if(sync_add_list.count()>0) {
		slist_iterator_tpl<sync_steppable *> iter (sync_add_list);
		while(iter.next()) {
			sync_steppable *ss = iter.get_current();
			ss->sync_prepare();
			sync_list.insert( ss );
		}
//		DBG_MESSAGE("karte_t::sync_prepare()","added %d obj(s) to sync_list",sync_add_list.count());
		sync_add_list.clear();
	}
}

void
karte_t::sync_step(const long dt)
{
	// ingore calls by interrupt ...
	if(fast_forward  &&  dt!=-987) {
		return;
	}

	const long delta_t = (dt<0) ? 200 : (dt * get_time_multi()) >> 4;
	slist_iterator_tpl<sync_steppable*> iter (sync_list);

	// Hajo: we use a slight hack here to remove the current
	// object from the list without wrecking the iterator
	bool ok = iter.next();
	while(ok) {
		sync_steppable *ss = iter.get_current();

		// Hajo: advance iterator, so that we can remove the current object
		// safely
		ok = iter.next();

		if(ss->sync_step(delta_t) == false) {
			sync_list.remove(ss);
			delete ss;
		}

	}

	for(int x=0; x<8; x++) {
		gib_spieler(x)->age_messages(delta_t);
	}

	ticks += delta_t;

	// change view due to following a convoi?
	if(follow_convoi.is_bound()) {
		const koord old_pos=gib_ij_off();
		const koord new_pos=follow_convoi->gib_vehikel(0)->gib_pos().gib_2d()-koord(8,8);
		const int rw=get_tile_raster_width();
		const int new_xoff=tile_raster_scale_x(-follow_convoi->gib_vehikel(0)->gib_xoff(),rw);
		const int new_yoff=tile_raster_scale_x(-follow_convoi->gib_vehikel(0)->gib_yoff(),rw)+tile_raster_scale_y(follow_convoi->gib_vehikel(0)->gib_pos().z,rw);
		if(new_pos!=old_pos  ||  new_xoff!=gib_x_off()  ||  new_yoff!=gib_y_off()) {
			//position changed => update
			setze_ij_off( new_pos );
			x_off = new_xoff;
			y_off = new_yoff;
			intr_refresh_display( true );
		}
	}

	// Hajo: ein weiterer Frame
	thisFPS++;
}


void
karte_t::neuer_monat()
{
	current_month ++;
	letzter_monat ++;
	if(letzter_monat>11) {
		letzter_monat = 0;
	}
	DBG_MESSAGE("karte_t::neuer_monat()","Month %d has started", letzter_monat);

	const weg_besch_t *city_road_test = wegbauer_t::gib_besch(umgebung_t::city_road_type->chars(),get_timeline_year_month());
	if(city_road_test) {
		city_road = city_road_test;
	}
	else {
		DBG_MESSAGE("karte_t::neuer_monat()","Month %d has started", letzter_monat);
		city_road = wegbauer_t::weg_search(weg_t::strasse,30,get_timeline_year_month());
	}
	INT_CHECK("simworld 1701");

	// this should be done before a map update, since the map may want an update of the way usage
//	DBG_MESSAGE("karte_t::neuer_monat()","ways");
	slist_iterator_tpl <weg_t *> weg_iter (weg_t::gib_alle_wege());
	while( weg_iter.next() ) {
		weg_iter.get_current()->neuer_monat();
	}
	INT_CHECK("simworld 1890");	// change grounds to winter?
	const int current_season=(2+letzter_monat/3)&3; // summer always zero
	if(  season!=current_season  ) {
		season = current_season;
		boden_t::toggle_season( current_season );
	}

//	DBG_MESSAGE("karte_t::neuer_monat()","process seasons");
	// recalc old settings (and maybe update the staops with the current values)
	reliefkarte_t::gib_karte()->neuer_monat();

	INT_CHECK("simworld 1701");

//	DBG_MESSAGE("karte_t::neuer_monat()","convois");
	// hsiegeln - call new month for convois
	for(unsigned i=0;  i<convoi_array.get_count();  i++ ) {
		convoihandle_t cnv = convoi_array.at(i);
		cnv->new_month();
	}

	INT_CHECK("simworld 1701");

//	DBG_MESSAGE("karte_t::neuer_monat()","factories");
	slist_iterator_tpl<fabrik_t*> iter (fab_list);
	while(iter.next()) {
		fabrik_t * fab = iter.get_current();
		fab->neuer_monat();
	}
	INT_CHECK("simworld 1278");


//	DBG_MESSAGE("karte_t::neuer_monat()","cities");
	// roll city history and copy the new citicens (i.e. the new weight) into the stadt array
	// no INT_CHECK() here, or dialoges will go crazy!!!
	weighted_vector_tpl<stadt_t *>  *new_weighted_stadt = new weighted_vector_tpl <stadt_t *> ( stadt->get_count()+1 );
	for(unsigned i=0; i<stadt->get_count(); i++) {
		stadt_t *s = stadt->at(i);
		s->neuer_monat();
		new_weighted_stadt->append( s, s->gib_einwohner(), 64 );
		INT_CHECK("simworld 1278");
	}
	delete stadt;
	stadt = new_weighted_stadt;

	INT_CHECK("simworld 1282");

//	DBG_MESSAGE("karte_t::neuer_monat()","players");
	// spieler
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(spieler[i] != NULL) {
			spieler[i]->neuer_monat();
		}
	}

	INT_CHECK("simworld 1289");

//	DBG_MESSAGE("karte_t::neuer_monat()","halts");
	slist_iterator_tpl <halthandle_t> halt_iter (haltestelle_t::gib_alle_haltestellen());
	while( halt_iter.next() ) {
		halt_iter.get_current()->neuer_monat();
		INT_CHECK("simworld 1877");
	}

	// now switch year to get the right year for all timeline stuff ...
	if(letzter_monat==0) {
		neues_jahr();
	}

//	DBG_MESSAGE("karte_t::neuer_monat()","citycars");
	stadtauto_t::built_timeline_liste(this);
	INT_CHECK("simworld 1299");

	DBG_MESSAGE("karte_t::neuer_monat()","timeline");

	// prissi: check for new/retired vehicles
	if(use_timeline()) {
		char	buf[256];

		for(int i=weg_t::strasse; i<weg_t::luft; i++) {
			const vehikel_besch_t *info;
			for(  int j=0;  (info = vehikelbauer_t::gib_info((weg_t::typ)i, j));  j++  ) {

				const uint16 intro_month = info->get_intro_year_month();
				if(intro_month == current_month) {
					sprintf(buf,
						translator::translate("New vehicle now available:\n%s\n"),
						translator::translate(info->gib_name()));
						message_t::get_instance()->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,info->gib_basis_bild());
				}

				const uint16 retire_month = info->get_retire_year_month();
				if(retire_month == current_month) {
					sprintf(buf,
						translator::translate("Production of vehicle has been stopped:\n%s\n"),
						translator::translate(info->gib_name()));
						message_t::get_instance()->add_message(buf,koord::invalid,message_t::new_vehicle,NEW_VEHICLE,info->gib_basis_bild());
				}
			}
		}
	}
	INT_CHECK("simworld 1921");

	recalc_average_speed();
	INT_CHECK("simworld 1921");

	if(umgebung_t::autosave>0  &&  letzter_monat%umgebung_t::autosave==0) {
		char buf[128];
		sprintf( buf, "save/autosave%02i.sve", letzter_monat+1 );
		speichern( buf, true );
	}
}



// recalculated speed boni for different vehicles
void karte_t::recalc_average_speed()
{
//	DBG_MESSAGE("karte_t::recalc_average_speed()","");
	if(use_timeline()) {
		int num_averages[4]={0,0,0,0};
		int speed_sum[4]={0,0,0,0};
		const uint16 timeline_month = get_timeline_year_month();
		for(int i=weg_t::strasse; i<=weg_t::luft; i++) {
			// check for speed
			const vehikel_besch_t *info;
			const int speed_bonus_categorie = (i>=4  &&  i<=7) ? 2 : (i==weg_t::luft ? 4 : i );
			for(  int j=0;  (info = vehikelbauer_t::gib_info((weg_t::typ)i, j));  j++  ) {
				if(info->gib_leistung()>0  &&  !info->is_future(timeline_month)  &&  !info->is_retired(timeline_month)) {
					speed_sum[speed_bonus_categorie-1] += info->gib_geschw();
					num_averages[speed_bonus_categorie-1] ++;
				}
			}
		}
		// recalculate them
		for(int i=0;  i<4;  i++) {
			if(num_averages[i]>0) {
				average_speed[i] = speed_sum[i]/num_averages[i];
			}
		}
	}
	else {
		// defaults
		average_speed[0] = 60;
		average_speed[1] = 80;
		average_speed[2] = 40;
		average_speed[3] = 350;
	}
}



int karte_t::get_average_speed(weg_t::typ typ) const
{
	switch(typ) {
		case weg_t::strasse: return average_speed[0];
		case weg_t::schiene:
		case weg_t::monorail:
		case weg_t::schiene_maglev:
		case weg_t::schiene_strab: return average_speed[1];
		case weg_t::wasser: return average_speed[2];
		case weg_t::luft: return average_speed[3];
		default: return 1;
	}
}



void karte_t::neues_jahr()
{
	letztes_jahr = current_month/12;

DBG_MESSAGE("karte_t::neues_jahr()","Year %d has started", letztes_jahr);

	char buf[256];
	sprintf(buf,translator::translate("Year %i has started."),letztes_jahr);
	message_t::get_instance()->add_message(buf,koord::invalid,message_t::general,COL_BLACK,skinverwaltung_t::neujahrsymbol->gib_bild_nr(0));

	for(unsigned i=0;  i<convoi_array.get_count();  i++ ) {
		convoihandle_t cnv = convoi_array.at(i);
		cnv->neues_jahr();
	}

	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		gib_spieler(i)->neues_jahr();
	}
}


#define GRUPPEN 14
static long step_group_times[GRUPPEN];



void karte_t::step(const long delta_t)
{
	// needs plausibility check?!?
	if(delta_t<=0  ||  delta_t>10000) {
		return;
	}

	const int step_group = steps % GRUPPEN;
	unsigned int i;

	// due to the scheduling in several parts, we must remember the last number of intervalls
	for(i=0; i<GRUPPEN; i++) {
		step_group_times[i] += delta_t;
	}
	// the we use this accumulated value
	const long delta_t_sum = step_group_times[step_group];

	// did the at least 25 ms passed (this makes maximum simloop something below 40)
	if(delta_t_sum>350) {

		// and reset it
		step_group_times[step_group] = 0;
		// how many steps passed?
		const int step_group_step = (steps / GRUPPEN);
		// true delta t ...
//		const long all_delta_t = delta_t_sum-step_group_times[(step_group+1)%GRUPPEN];

		int check_counter = 0;
		const int cached_groesse2 = cached_groesse_gitter_x*cached_groesse_gitter_y;
		for(int i=step_group; i<cached_groesse2; i+=GRUPPEN) {
			plan[i].step(delta_t_sum, step_group_step);

			if(((++check_counter) & 63) == 0) {	// every 64 one update ...
				INT_CHECK("simworld 1076");
				interactive_update();
			}
		}

		// Hajo: Convois need extra frequent steps to avoid unneccesary
		// long waiting times
		// the convois goes alternating up and down to avoid a preferred
		// order and loading only of the train highest in the game
		for(unsigned i=0; i<convoi_array.get_count();  i++) {
			convoihandle_t cnv=convoi_array.get(i);
			cnv->step();
			if((i&7)==0) {
				INT_CHECK("simworld 1947");
			}
		}
		interactive_update();

		// now step all towns (to generate passengers)
		for(unsigned int n=0; n<stadt->get_count(); n++) {
			stadt->at(n)->step(delta_t);
			INT_CHECK("simworld 1959");
		}
		interactive_update();

		// then step all players
		INT_CHECK("simworld 1975");
		spieler[steps & 7]->step();
		interactive_update();

		// ok, next step
		steps ++;
		INT_CHECK("simworld 1975");

		if(ticks > next_month_ticks) {

			next_month_ticks += karte_t::ticks_per_tag;

			// avoid overflow here ...
			if(ticks>next_month_ticks) {
				ticks %= karte_t::ticks_per_tag;
				next_month_ticks = ticks+karte_t::ticks_per_tag;
			}

			// now handle new month (and maybe new year too)
			neuer_monat();
		}
		interactive_update();
	}
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


/* functions for following a convoi on the map
 * give an unbound handle to unset
 */
void karte_t::set_follow_convoi(convoihandle_t cnv)
{
	follow_convoi = cnv;
}



void
karte_t::blick_aendern(event_t *ev)
{
  if(!scroll_lock) {
    const int raster = get_tile_raster_width();
    const int xmask = raster - 1;
    const int ymask = raster/2 - 1;

    int xd = -x_off/2;
    int yd = -y_off/2;

    xd += (ev->mx - ev->cx) * einstellungen->gib_scroll_multi();
    yd += (ev->my - ev->cy) * einstellungen->gib_scroll_multi();

    const int dx = (xd & ~xmask) / (raster/2);
    const int dy = (yd & ~ymask) / (raster/4);

    x_off = -(xd & xmask)*2;
    y_off = -(yd & ymask)*2;

    setze_ij_off(gib_ij_off() + koord(dx+dy, dy-dx));


    if ((ev->mx - ev->cx) != 0 || (ev->my - ev->cy) != 0) {
      intr_refresh_display( true );

#ifdef __BEOS__
      change_drag_start(ev->mx - ev->cx, ev->my - ev->cy);
#else
      display_move_pointer(ev->cx, ev->cy);
#endif
    }
  }
}


// nordhang = 3
// suedhang = 12

/**
 * returns the natural slope a a position
 * @author prissi
 */
uint8	karte_t::calc_natural_slope( const koord pos ) const
{
	if(ist_in_kartengrenzen(pos.x, pos.y)) {

		const sint8 * p = &grid_hgts[pos.x + pos.y*(gib_groesse_x()+1)];

		const int h1 = *p;
		const int h2 = *(p+1);
		const int h3 = *(p+gib_groesse_x()+2);
		const int h4 = *(p+gib_groesse_x()+1);

		const int mini = min(min(h1,h2), min(h3,h4));

		const int d1=h1-mini;
		const int d2=h2-mini;
		const int d3=h3-mini;
		const int d4=h4-mini;

#ifndef DOUBLE_GROUNDS
		const int slope = (d1<<3) + (d2<<2) + (d3<<1) + d4;
#else
		const int slope = (d1*27) + (d2*9) + (d3*3) + d4;
#endif
		return slope;
	}
	return 0;
}



bool
karte_t::ist_wasser(koord pos, koord dim) const
{
    koord k;

    for(k.x = pos.x; k.x < pos.x + dim.x; k.x++) {
	for(k.y = pos.y; k.y < pos.y + dim.y; k.y++) {
	    if(lookup_hgt(k) > gib_grundwasser()) {
		return false;
	    }
	}
    }
    return true;
}

bool
karte_t::ist_platz_frei(koord pos, int w, int h, int *last_y, bool slope_check ) const
{
    koord k;

    if(pos.x<0 || pos.y<0 || pos.x+w>=gib_groesse_x() || pos.y+h>=gib_groesse_y()) {
//	if(pos.x | pos.y | (gib_groesse_x()-(pos.x+w)) | (gib_groesse_y()-(pos.y+h)) < 0) {
		return false;
	}
	// ACHTUNG: Schleifen sind mit finde_plaetze koordiniert, damit wir ein
	// paar Abfragen einsparen können bei h > 1!
	// V. Meyer
	for(k.y=pos.y+h-1; k.y>=pos.y; k.y--) {
		for(k.x=pos.x; k.x<pos.x+w; k.x++) {
			const grund_t *gr = lookup(k)->gib_kartenboden();

			if(  (slope_check && gr->gib_grund_hang() != hang_t::flach) ||  !gr->ist_natur() ||
			/*gr->ist_wasser() || *//*Wasser liefert als ist_natur() false */
			   gr->kann_alle_obj_entfernen(NULL) != NULL
			) {
				if(last_y) {
					*last_y = k.y;
				}
				return false;
			}
		}
	}
	return true;
}



slist_tpl<koord> *
karte_t::finde_plaetze(int w, int h) const
{
	slist_tpl<koord> * list = new slist_tpl<koord>();
	koord start;
	int last_y;

DBG_DEBUG("karte_t::finde_plaetze()","for size (%i,%i) in map (%i,%i)",w,h,gib_groesse_x(),gib_groesse_y() );
	for(start.x=0; start.x<gib_groesse_x()-w; start.x++) {
		for(start.y=0; start.y<gib_groesse_y()-h; start.y++) {
			if(ist_platz_frei(start, w, h, &last_y)) {
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
		const int center = display_get_width() >> 7;
		const int dist = abs((pos.x-center) - gib_ij_off().x) + abs((pos.y-center) - gib_ij_off().y);

		if(dist < 25) {
			info.volume = 255-dist*9;
			sound_play(info);
		}
		return dist < 25;
	}
	return false;
}



//void
//karte_t::beenden()    // 02-Nov-2001  Markus Weber    Added parameter

void
karte_t::beenden(bool quit_simutrans)
{
    doit = false;
    m_quit_simutrans = quit_simutrans; // 02-Nov-200    Markus Weber    Added
}

void
karte_t::speichern(const char *filename,bool silent)
{
#ifndef DEMO
DBG_MESSAGE("karte_t::speichern()", "saving game to '%s'", filename);

	loadsave_t  file;

	display_show_load_pointer( true );
	if(!file.wr_open(filename)) {
		create_win(-1, -1, new nachrichtenfenster_t(this, "Kann Spielstand\nnicht speichern.\n"), w_autodelete);
		dbg->error("karte_t::speichern()","cannot open file for writing! check permissions!");
	}
	else {
		speichern(&file,silent);
		file.close();
		if(!silent) {
			create_win(-1, -1, 30, new nachrichtenfenster_t(this, "Spielstand wurde\ngespeichert!\n"), w_autodelete);
			// update the filename, if no autosave
			einstellungen->setze_filename(filename);
		}
	}
	display_show_load_pointer( false );
#endif
}


void
karte_t::speichern(loadsave_t *file,bool silent)
{
    int i,j;

    einstellungen->rdwr(file);

	file->rdwr_long(ticks, " ");
	file->rdwr_long(letzter_monat, " ");
	file->rdwr_long(letztes_jahr, "\n");

	DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "save cities");
    for(i=0; i<einstellungen->gib_anzahl_staedte(); i++) {
	stadt->at(i)->rdwr(file);
	if(silent) {
		INT_CHECK("saving");
	}
    }
	DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved cities ok");

	DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "save tiles");
    for(j=0; j<gib_groesse_y(); j++) {
	for(i=0; i<gib_groesse_x(); i++) {
	    access(i, j)->rdwr(this, file);
	}
	if(silent) {
		INT_CHECK("saving");
	}
	else {
		display_progress(j, gib_groesse_y());
		display_flush(IMG_LEER,0, 0, 0, "", "", 0, 0);
	}
    }
	DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved access");

    for(j=0; j<=gib_groesse_y(); j++) {
	for(i=0; i<=gib_groesse_x(); i++) {
            int hgt = lookup_hgt(koord(i, j));
	    file->rdwr_long(hgt, "\n");
	}
    }
	DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved hgt");
	if(silent) {
		INT_CHECK("saving");
	}

    int fabs = fab_list.count();

    file->rdwr_long(fabs, "\n");

    slist_iterator_tpl<fabrik_t*> fiter ( fab_list );
    while(fiter.next()) {
	(fiter.get_current())->rdwr(file);
	if(silent) {
		INT_CHECK("saving");
	}
    }
	DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved fabs");

	for(unsigned i=0;  i<convoi_array.get_count();  i++ ) {
		convoihandle_t cnv = convoi_array.at(i);
		cnv->rdwr(file);
	}
	if(silent) {
		INT_CHECK("saving");
	}
	file->wr_obj_id("Ende Convois");
	DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved %i convois",convoi_array.get_count());

    for(i=0; i<MAX_PLAYER_COUNT; i++) {
	spieler[i]->rdwr(file);
	if(silent) {
		INT_CHECK("saving");
	}
    }
	DBG_MESSAGE("karte_t::speichern(loadsave_t *file)", "saved players");

    i = gib_ij_off().x;
    j = gib_ij_off().y;
    file->rdwr_delim("View ");
    file->rdwr_long(i, " ");
    file->rdwr_long(j, "\n");

    // Hajo: once this should be removed -> it makes IMO
    // no sense to save the UI language with a game ?!
    // translator::rdwr( file );

    display_speichern( file->gib_file(), file->is_zipped());
}



// just the preliminaries, opens the file, checks the versions ...
void
karte_t::laden(const char *filename)
{
	mute_sound(true);
	display_show_load_pointer(true);

#ifndef DEMO
	loadsave_t file;

	DBG_MESSAGE("karte_t::laden", "loading game from '%s'", filename);

	if(!file.rd_open(filename)) {

		if(file.get_version() == -1) {
			create_win(-1, -1, new nachrichtenfenster_t(this, "WRONGSAVE"), w_autodelete);
		}
		else {
			create_win(-1, -1, new nachrichtenfenster_t(this, "Kann Spielstand\nnicht laden.\n"), w_autodelete);
		}
	} else if(file.get_version() < 84006) {
		// too old
		create_win(-1, -1, new nachrichtenfenster_t(this, "WRONGSAVE"), w_autodelete);
	}
	else {
		destroy_all_win();
		event_t ev;
		ev.ev_class=EVENT_NONE;
		check_pos_win(&ev);

DBG_MESSAGE("karte_t::laden()","Savegame version is %d", file.get_version());

		laden(&file);
		file.close();
		steps_bis_jetzt = steps-4;
		create_win(-1, -1, 30, new nachrichtenfenster_t(this, "Spielstand wurde\ngeladen!\n"), w_autodelete);
	}
#endif
	einstellungen->setze_filename(filename);
	display_show_load_pointer(false);
}



// handles the actual loading
void karte_t::laden(loadsave_t *file)
{
    char buf[80];
    int x,y;

    setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), Z_PLAN,  NO_SOUND, NO_SOUND );

    intr_disable();

    // powernets zum laden vorbereiten -> tabelle loeschen
    powernet_t::prepare_loading();

    // alte werte merken fuer neuen Zeiger
    const int bild = zeiger->gib_bild();
    const int yoff = zeiger->gib_yoff();

    // gibt alles frei

    // Zeiger entfernen
    zeiger->setze_pos(koord3d::invalid);     // war nicht in welt enthalten
    delete zeiger;
    zeiger = NULL;

    // Datenstrukturen loeschen
    destroy();

	// jetzt geht das laden los
	DBG_MESSAGE("karte_t::laden", "Fileversion: %d", file->get_version());
	einstellungen->rdwr(file);
	if(einstellungen->gib_allow_player_change()  &&  umgebung_t::use_timeline!=2) {
		// not locked => eventually switch off timeline settings, if explicitly stated
		einstellungen->setze_use_timeline( umgebung_t::use_timeline );
		DBG_DEBUG("karte_t::laden", "timeline: reset to %i", umgebung_t::use_timeline );
	}
	if(einstellungen->gib_beginner_mode()) {
		warenbauer_t::set_multiplier( umgebung_t::beginner_price_factor );
	}
	else {
		warenbauer_t::set_multiplier( 1000 );
	}
DBG_DEBUG("karte_t::laden", "einstellungen loaded (groesse %i,%i) timeline=%i beginner=%i",einstellungen->gib_groesse_x(),einstellungen->gib_groesse_y(),umgebung_t::use_timeline,einstellungen->gib_beginner_mode());

    // wird gecached, um den Pointerzugriff zu sparen, da
    // die groesse _sehr_ oft referenziert wird
    cached_groesse_gitter_x = einstellungen->gib_groesse_x();
    cached_groesse_gitter_y = einstellungen->gib_groesse_y();
    cached_groesse_max = max(cached_groesse_gitter_x,cached_groesse_gitter_y);
    cached_groesse_karte_x = cached_groesse_gitter_x-1;
    cached_groesse_karte_y = cached_groesse_gitter_y-1;

    // Reliefkarte an neue welt anpassen
    reliefkarte_t::gib_karte()->setze_welt(this);

    //12-Jan-02     Markus Weber added
    grundwasser = einstellungen->gib_grundwasser();
DBG_DEBUG("karte_t::laden()","grundwasser %i",grundwasser);

DBG_DEBUG("karte_t::laden", "init felder ... ");
	init_felder();
DBG_DEBUG("karte_t::laden", "init felder ok");

	hausbauer_t::neue_karte();

	file->rdwr_long(ticks, " ");
	file->rdwr_long(letzter_monat, " ");
	file->rdwr_long(letztes_jahr, "\n");
	if(file->get_version()<86006) {
		letztes_jahr += umgebung_t::starting_year;
	}
	// set the current month count
	setze_ticks_bits_per_tag(einstellungen->gib_bits_per_month());
	current_month = letzter_monat + (letztes_jahr*12);
	steps = 0;

DBG_MESSAGE("karte_t::laden()","savegame loading at tick count %i",ticks);

DBG_DEBUG("karte_t::laden()","built timeline for citycars");
	stadtauto_t::built_timeline_liste(this);

DBG_DEBUG("karte_t::laden", "init %i cities",einstellungen->gib_anzahl_staedte());
	stadt = new weighted_vector_tpl <stadt_t *> (einstellungen->gib_anzahl_staedte());
	for(int i=0; i<einstellungen->gib_anzahl_staedte(); i++) {
		stadt_t *s=new stadt_t(this, file);
		stadt->append( s, s->gib_einwohner(), 64 );
	}

	DBG_MESSAGE("karte_t::laden()","loading blocks");
	old_blockmanager_t::rdwr(this, file);

	DBG_MESSAGE("karte_t::laden()","loading tiles");
	for(y=0; y<gib_groesse_y(); y++) {
		for(x=0; x<gib_groesse_x(); x++) {
			if(file->is_eof()) {
				dbg->fatal("karte_t::laden()","Savegame file mangled (too short)!");
			}
			access(x, y)->rdwr(this, file);
		}
		display_progress(y, gib_groesse_y());
		display_flush(IMG_LEER,0, 0, 0, "", "", 0, 0);
	}

DBG_MESSAGE("karte_t::laden()","loading grid");
	for(y=0; y<=gib_groesse_y(); y++) {
		for(x=0; x<=gib_groesse_x(); x++) {
			int hgt;
			file->rdwr_long(hgt, "\n");
			setze_grid_hgt(koord(x, y), hgt);
		}
	}

	if(file->get_version()<88009) {
		DBG_MESSAGE("karte_t::laden()","loading slopes from older version");
		// Hajo: load slopes for older versions
		// now part of the grund_t structure
		for(y=0; y<gib_groesse_y(); y++) {
			for(x=0; x<gib_groesse_x(); x++) {
				sint8 slope;
				file->rdwr_byte(slope, ",");
				access(x, y)->gib_kartenboden()->setze_grund_hang(slope);
			}
		}
	}

	if(file->get_version()<=88000) {
		// because from 88.01.4 on the foundations are handled differently
		for(y=0; y<gib_groesse_y(); y++) {
			for(x=0; x<gib_groesse_x(); x++) {
				koord k(x,y);
				if(access(x,y)->gib_kartenboden()->gib_typ()==grund_t::fundament) {
					access(x,y)->gib_kartenboden()->setze_hoehe( max_hgt(k) );
					access(x,y)->gib_kartenboden()->setze_grund_hang(hang_t::flach);
				}
			}
		}
    	}

    // Reliefkarte an neue welt anpassen
    DBG_MESSAGE("karte_t::laden()", "init relief");
    win_setze_welt( this );
    reliefkarte_t::gib_karte()->setze_welt(this);

    int fabs;
    file->rdwr_long(fabs, "\n");
    DBG_MESSAGE("karte_t::laden()", "prepare for %i factories", fabs);

    for(int i = 0; i < fabs; i++) {
	// liste in gleicher reihenfolge wie vor dem speichern wieder aufbauen
	fabrik_t *fab = new fabrik_t(this, file);
	if(fab->gib_besch()) {
		fab_list.append( fab );
	}
	else {
		dbg->error("karte_t::laden()","Unknown fabrik skipped!");
	}
    }
    DBG_MESSAGE("karte_t::laden()", "clean up factories");
    if (fab_list.count() > 0) {
      gib_fab(0)->laden_abschliessen();
    }

	// crossconnect all?
	if(umgebung_t::crossconnect_factories) {
	    DBG_MESSAGE("karte_t::laden()", "crossconnecting factories");
		slist_iterator_tpl <fabrik_t *> iter (this->fab_list);
		while( iter.next() ) {
			iter.get_current()->add_all_suppliers();
		}
	}

    DBG_MESSAGE("karte_t::laden()", "%d factories loaded", fab_list.count());

	// auch die fabrikverbindungen können jetzt neu init werden
	// must be done after reliefkarte is initialized
	for(unsigned i=0; i<stadt->get_count(); i++) {
		stadt->at(i)->laden_abschliessen();
	}
	// must resort them ...
	weighted_vector_tpl<stadt_t *>  *new_weighted_stadt = new weighted_vector_tpl <stadt_t *> ( stadt->get_count()+1 );
	for(unsigned i=0; i<stadt->get_count(); i++) {
		stadt_t *s = stadt->at(i);
//		s->neuer_monat();
		new_weighted_stadt->append( s, s->gib_einwohner(), 64 );
		INT_CHECK("simworld 1278");
	}
	delete stadt;
	stadt = new_weighted_stadt;
	// ok finished
	DBG_MESSAGE("karte_t::laden()", "cities initialized");

	// load linemanagement status (and lines)
	// @author hsiegeln
	if (file->get_version() > 82003  &&  file->get_version()<88003) {
		DBG_MESSAGE("karte_t::laden()", "load linemanagement");
		gib_spieler(0)->simlinemgmt.rdwr(this, file);
		DBG_MESSAGE("karte_t::laden()", "%d lines loaded", gib_spieler(0)->simlinemgmt.count_lines());
	}
	// end load linemanagement

	DBG_MESSAGE("karte_t::laden()", "load convois");
	while( true ) {
		file->rd_obj_id(buf, 79);

		// printf("'%s'\n", buf);

		if(tstrequ(buf, "Ende Convois")) {
			break;
		}
		else {
			convoi_t *cnv = new convoi_t(this, file);
			convoi_array.append( cnv->self, 16 );

			if(cnv->in_depot()) {
				grund_t * gr = lookup(cnv->gib_pos());
				depot_t *dep = gr ? gr->gib_depot() : 0;
				if(dep) {
					dep->convoi_arrived(cnv, false);
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
	}
DBG_MESSAGE("karte_t::laden()", "%d convois/trains loaded", convoi_array.get_count());

    // reinit pointer with new pointer object and old values
    zeiger = new zeiger_t(this, koord3d(0,0,0), spieler[0]);	// Zeiger ist spezialobjekt
    zeiger->setze_bild(0, bild);
    zeiger->setze_yoff( yoff );

	// jetzt können die spieler geladen werden
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		spieler[i]->rdwr(file);
	}
	for(int i=0; i<MAX_PLAYER_COUNT-2; i++) {
		umgebung_t::automaten[i] = spieler[i+2]->is_active();
	}
DBG_MESSAGE("karte_t::laden()", "players loaded");

    // nachdem die welt jetzt geladen ist können die Blockstrecken neu
    // angelegt werden
		old_blockmanager_t::laden_abschliessen(this);
    DBG_MESSAGE("karte_t::laden()", "blocks loaded");

    file->rdwr_delim("View ");
    file->rdwr_long(mi, " ");
    file->rdwr_long(mj, "\n");
    DBG_MESSAGE("karte_t::laden()", "Setting view to %d,%d", mi,mj);
    setze_ij_off(koord(mi,mj));

	display_laden(file->gib_file(), file->is_zipped());

DBG_MESSAGE("karte_t::laden()", "%d ways loaded",weg_t::gib_alle_wege().count());
	for(y=0; y<gib_groesse_y(); y++) {
		for(x=0; x<gib_groesse_x(); x++) {
			const planquadrat_t *plan = lookup(koord(x,y));
			const int boden_count = plan->gib_boden_count();
			for(int schicht=0; schicht<boden_count; schicht++) {
				grund_t *gr = plan->gib_boden_bei(schicht);
				gr->calc_bild();
				for(int n=0; n<gr->gib_top(); n++) {
					ding_t *d = gr->obj_bei(n);
					if(d) {
						d->laden_abschliessen();
					}
				}
			}
		}
	}

	// assing lines and other stuff for convois
	for(unsigned i=0;  i<convoi_array.get_count();  i++ ) {
		convoihandle_t cnv = convoi_array.at(i);
		cnv->laden_abschliessen();
	}

    // register all line stops and change line types, if needed
	for(int i=0; i<MAX_PLAYER_COUNT ; i++) {
		spieler[i]->laden_abschliessen();
	}

	// just keep the record for the last 12 years ... to allow infite long games
	while(ticks>(288u << karte_t::ticks_bits_per_tag)) {
		ticks -= (144u << karte_t::ticks_bits_per_tag);
	}
	// get the number of year (12 is the longest history we got
	if(ticks > (144u << karte_t::ticks_bits_per_tag)) {
		basis_jahr = letztes_jahr-12;
	}
	else {
		basis_jahr =  letztes_jahr-(ticks/(12 << karte_t::ticks_bits_per_tag));
	}
	// make counter for next month
	next_month_ticks =(ticks+karte_t::ticks_per_tag) & (~(karte_t::ticks_per_tag-1));
	letzter_monat %= 12;

	DBG_MESSAGE("karte_t::laden()","savegame from %i/%i, base year %i, next month=%i, ticks=%i (per month=1<<%i)",letzter_monat,letztes_jahr,basis_jahr,next_month_ticks,ticks,karte_t::ticks_bits_per_tag);

	// change grounds to winter?
	const int current_season=(2+letzter_monat/3)&3; // summer always zero
	if(  season!=current_season  ) {
		season = current_season;
		boden_t::toggle_season( current_season );
	}
    setze_dirty();

	DBG_MESSAGE("karte_t::laden()","rebuild_destinations()");

	// rebuild destination lists
	const slist_tpl<halthandle_t> & list = haltestelle_t::gib_alle_haltestellen();
	slist_iterator_tpl <halthandle_t> iter (list);
	while( iter.next() ) {
		iter.get_current()->recalc_station_type();	// fix broken post flags
	}
	schedule_counter++;	// force check for unroutable goods and connections

	reset_timer();
     recalc_average_speed();
	intr_enable();
	mute_sound(false);
}

// Hilfsfunktionen zum Speichern

int
karte_t::sp2num(spieler_t *sp)
{
	for(int i=0; i<MAX_PLAYER_COUNT; i++) {
		if(spieler[i] == sp) {
			return i;
		}
	}
	return -1;
}

// ende karte_t methoden



/**
 * Creates a map from a heightfield
 * @param sets game settings
 * @author Hansjörg Malthaner
 */
void karte_t::load_heightfield(einstellungen_t *sets)
{
  FILE *file = fopen(sets->heightfield, "rb");

  if(file) {
    char buf [256];
    int w, h;

    read_line(buf, 255, file);

    if(strncmp(buf, "P6", 2)) {
      dbg->error("karte_t::load_heightfield()",
		 "Heightfield has wrong image type %s", buf);
      create_win(-1, -1, new nachrichtenfenster_t(this, "\nHeightfield has wrong image type.\n"), w_autodelete);
      return;
    }

    read_line(buf, 255, file);

    sscanf(buf, "%d %d", &w, &h);
    sets->setze_groesse(w,h);

    read_line(buf, 255, file);

    if(strncmp(buf, "255", 2)) {
      dbg->fatal("karte_t::load_heightfield()",
		 "Heightfield has wrong color depth %s", buf);
    }


    // create map
    init(sets);
  }
  else {
    dbg->error("karte_t::load_heightfield()",
	       "Cant open file '%s'", sets->heightfield.chars());

    create_win(-1, -1, new nachrichtenfenster_t(this, "\nCan't open heightfield file.\n"), w_autodelete);
  }
}


static void warte_auf_mausklick_oder_taste(event_t *ev)
{
    do {
	win_get_event(ev);
    }while( !(IS_LEFTRELEASE(ev) ||
	      (ev->ev_class == EVENT_KEYBOARD &&
	       (ev->ev_code >= 32 && ev->ev_code < 256))));
}



// in fast forward mode?
bool karte_t::is_fast_forward()
{
	return fast_forward;
}



void karte_t::reset_timer()
{
	DBG_MESSAGE("karte_t::reset_timer()","called");
	// Reset timers
	sync_last_time_now();
	intr_set_last_time(get_system_ms());
	last_step_time = get_current_time_millis();

	// Hajo: this actually is too conservative but the correct
	// solution is way too difficult for a simple pause function ...
	// init for a good separation
	for(int i=0; i<GRUPPEN; i++) {
		step_group_times[i] = (50*i)/GRUPPEN+1;
	}
	DBG_MESSAGE("karte_t::reset_timer()","ok");
}



// jump one year ahead
void karte_t::step_year()
{
	DBG_MESSAGE("karte_t::step_year()","called");
	ticks += 12*karte_t::ticks_per_tag;
	next_month_ticks += 12*karte_t::ticks_per_tag;
	current_month += 12;
	letztes_jahr ++;
	reset_timer();
}


void karte_t::do_pause()
{
	display_fillbox_wh(display_get_width()/2-100, display_get_height()/2-50,
	                           200,100, MN_GREY2, true);
	display_ddd_box(display_get_width()/2-100, display_get_height()/2-50,
	                            200,100, MN_GREY4, MN_GREY0);
	display_ddd_box(display_get_width()/2-92, display_get_height()/2-42,
	                            200-16,100-16, MN_GREY0, MN_GREY4);

	display_proportional(display_get_width()/2, display_get_height()/2-5,
	                                 translator::translate("GAME PAUSED"),
	                                 ALIGN_MIDDLE, COL_BLACK, false);

	// Pause: warten auf die nächste Taste
	event_t ev;
	display_flush_buffer();
	warte_auf_mausklick_oder_taste(&ev);
	reset_timer();
}


/**
 * I tried to make it more centering than the old -5,-5.
 * Looks okay, but still map height effects position.
 * @author Volker Meyer
 * @date  20.06.2003
 */
void karte_t::zentriere_auf(koord k)
{
    //setze_ij_off(k + koord(-5,-5));


    const int dp_height = display_get_height() / get_tile_raster_width();

    setze_ij_off(k - koord(dp_height - 3, dp_height - 2));
}


ding_t *
karte_t::gib_zeiger() const
{
    return zeiger;
}


void karte_t::bewege_zeiger(const event_t *ev)
{
	if(zeiger) {
		const int rw1 = get_tile_raster_width();
		const int rw2 = rw1/2;
		const int rw4 = rw1/4;

		const int scale = rw1/64;

		int i_alt=zeiger->gib_pos().x;
		int j_alt=zeiger->gib_pos().y;

		int screen_y = ev->my - y_off - 16 -rw4 - ((display_get_width()/rw1)&1)*rw4;
		//	int screen_y = ev->my - y_off - rw2 - ((display_get_width()/rw1)&1)*16;
		int screen_x = (ev->mx - x_off - rw2 - display_get_width()/2) / 2;

		if(zeiger->gib_yoff() == Z_PLAN) {
			screen_y -= rw4;
		}
		else if(zeiger->gib_yoff() == Z_LINES) {
			screen_y -= rw4/2;
		}


		// berechnung der basis feldkoordinaten in i und j

		/*  this would calculate raster i,j koordinates if there was no height
		*  die formeln stehen hier zur erinnerung wie sie in der urform aussehen

		int base_i = (screen_x+screen_y)/2;
		int base_j = (screen_y-screen_x)/2;

		int raster_base_i = (int)floor(base_i / 16.0);
		int raster_base_j = (int)floor(base_j / 16.0);

		*/

		// iterative naeherung fuer zeigerposition
		// iterative naehreung an gelaendehoehe

		int hgt = lookup_hgt(koord(i_alt, j_alt));

		const int i_off = gib_ij_off().x;
		const int j_off = gib_ij_off().y;

		for(int n = 0; n < 2; n++) {

			const int base_i = (screen_x+screen_y+height_scaling(hgt*scale) )/2;
			const int base_j = (screen_y-screen_x+height_scaling(hgt*scale) )/2;

			mi = ((int)floor(base_i/(double)rw4)) + i_off;
			mj = ((int)floor(base_j/(double)rw4)) + j_off;


			/*
			if(zeiger->gib_yoff() == Z_GRID) {
				hgt = max_hgt(koord(mi,mj))+8;
			} else */{
				const planquadrat_t *plan = lookup(koord(mi,mj));
				if(plan != NULL) {
					hgt = plan->gib_kartenboden()->gib_hoehe();
				} else {
					hgt = grundwasser;
				}
			}

			if(hgt < grundwasser) {
				hgt = grundwasser;
			}
		}

		// rueckwaerttransformation um die zielkoordinaten
		// mit den mauskoordinaten zu vergleichen
		int neu_x = ((mi-i_off) - (mj-j_off))*rw2;
		int neu_y = ((mi-i_off) + (mj-j_off))*rw4;

		neu_x += display_get_width()/2 + rw2;
		neu_y += rw1;


		// prüfe richtung d.h. welches nachbarfeld ist am naechsten
		if(ev->mx-x_off < neu_x) {
			zeiger->setze_richtung(ribi_t::west);
		}
		else {
			zeiger->setze_richtung(ribi_t::nord);
		}


		// zeiger bewegen

		if(mi >= 0 && mj >= 0 && mi<gib_groesse_x() && mj<gib_groesse_y() && (mi != i_alt || mj != j_alt)) {

			i_alt = mi;
			j_alt = mj;

			koord3d pos = lookup(koord(mi,mj))->gib_kartenboden()->gib_pos();
			if(zeiger->gib_yoff()==Z_GRID) {
				pos.z = lookup_hgt(koord(mi,mj));
				// pos.z = max_hgt(koord(mi,mj));
			}
			zeiger->setze_pos(pos);
		}
	}
}


/**
 * Creates a tooltip from tip text and money value
 * @author Hj. Malthaner
 */
static char * tool_tip_with_price(const char * tip, sint64 price)
{
  static char buf[256];

  const int n = sprintf(buf, "%s, ", tip);
  money_to_string(buf+n, (double)price/-100.0);
  return buf;
}



/* goes to next active player */
void karte_t::switch_active_player(uint8 new_player)
{
	// cheat: play as AI
	char buf[512];

	if(new_player>=MAX_PLAYER_COUNT) {
		new_player = 0;
	}

	// no cheating allowed?
	if(!einstellungen->gib_allow_player_change()) {
		active_player_nr = 0;
		active_player = spieler[0];
		if(new_player!=0) {
			sprintf(buf, translator::translate("On this map, you are not\nallowed to change player!\n"), get_active_player()->gib_name() );
			message_t::get_instance()->add_message(buf,koord(-1,-(sint16)simrand(63)),message_t::problems,get_active_player()->get_player_nr(),IMG_LEER);
		}
	}
	else {
		active_player_nr = new_player;
		active_player = spieler[new_player];
		sprintf(buf, translator::translate("Now active as %s.\n"), get_active_player()->gib_name() );
		message_t::get_instance()->add_message(buf,koord(-1,-(sint16)simrand(63)),message_t::problems,get_active_player()->get_player_nr(),IMG_LEER);
	}
	// open edit tools
	if(active_player_nr==1) {
	    werkzeug_parameter_waehler_t *wzw = new werkzeug_parameter_waehler_t(this, "EDITTOOLS");

	    wzw->setze_hilfe_datei("edittools.txt");

	    wzw->add_param_tool(&wkz_grow_city,
			  (long)100,
			  Z_PLAN,
			  -1,
			  SFX_FAILURE,
			  skinverwaltung_t::edit_werkzeug->gib_bild_nr(0),
			  skinverwaltung_t::upzeiger->gib_bild_nr(0),
			  translator::translate("Grow city") );

	    wzw->add_param_tool(&wkz_grow_city,
			  (long)-100,
			  Z_PLAN,
			  -1,
			  SFX_FAILURE,
			  skinverwaltung_t::edit_werkzeug->gib_bild_nr(1),
			  skinverwaltung_t::downzeiger->gib_bild_nr(0),
			  translator::translate("Shrink city") );

		// cityroads
		const weg_besch_t *besch = get_city_road();
		if(besch!=NULL) {
			sprintf(buf, "%s, %ld$ (%ld$), %dkm/h",
				translator::translate(besch->gib_name()),
				besch->gib_preis()/100,
				besch->gib_wartung()/100,
				besch->gib_topspeed());

		    wzw->add_param_tool(&wkz_wegebau,
				  (const void *)besch,
	  			  Z_PLAN,
				  SFX_JACKHAMMER,
				  SFX_FAILURE,
				  skinverwaltung_t::edit_werkzeug->gib_bild_nr(2),
				  wegbauer_t::gib_besch("asphalt_road")->gib_cursor()->gib_bild_nr(0),
				  buf);
		}

	    wzw->add_tool(&wkz_add_attraction,
			  Z_PLAN,
			  SFX_JACKHAMMER,
			  SFX_FAILURE,
			  skinverwaltung_t::edit_werkzeug->gib_bild_nr(3),
			  skinverwaltung_t::undoc_zeiger->gib_bild_nr(0),
			  translator::translate("Built random attraction") );

	    wzw->add_tool(wkz_build_industries_land,
			  Z_PLAN,
			  SFX_JACKHAMMER,
			  SFX_FAILURE,
			  skinverwaltung_t::special_werkzeug->gib_bild_nr(3),
			  skinverwaltung_t::undoc_zeiger->gib_bild_nr(0),
			  translator::translate("Build land consumer"));

	    wzw->add_tool(wkz_build_industries_city,
			  Z_PLAN,
			  SFX_JACKHAMMER,
			  SFX_FAILURE,
			  skinverwaltung_t::special_werkzeug->gib_bild_nr(4),
			  skinverwaltung_t::undoc_zeiger->gib_bild_nr(0),
			  translator::translate("Build city market"));

	    wzw->add_tool(wkz_lock,
			  Z_PLAN,
			  SFX_JACKHAMMER,
			  SFX_FAILURE,
			  skinverwaltung_t::edit_werkzeug->gib_bild_nr(5),
			  skinverwaltung_t::edit_werkzeug->gib_bild_nr(5),
			  translator::translate("Lock game"));

	    wzw->add_tool(wkz_step_year,
			  Z_PLAN,
			  SFX_JACKHAMMER,
			  SFX_FAILURE,
			  skinverwaltung_t::edit_werkzeug->gib_bild_nr(6),
			  skinverwaltung_t::undoc_zeiger->gib_bild_nr(0),
			  translator::translate("Step timeline one year"));

		wzw->zeige_info(magic_edittools);
	}
}



void
karte_t::interactive_event(event_t &ev)
{
	extern const weg_besch_t *default_track;
	extern const weg_besch_t *default_road;
	extern const way_obj_besch_t *default_electric;
    struct sound_info click_sound;

    click_sound.index = SFX_SELECT;
    click_sound.volume = 255;
    click_sound.pri = 0;


    if(ev.ev_class == EVENT_KEYBOARD) {
	DBG_MESSAGE("karte_t::interactive_event()","Keyboard event with code %d '%c'", ev.ev_code, ev.ev_code);

	switch(ev.ev_code) {
	case ',':
	    sound_play(click_sound);
	    set_time_multi(get_time_multi()-1);
	    break;
	case '.':
	    sound_play(click_sound);
	    set_time_multi(get_time_multi()+1);
	    break;
	case '!':
	    sound_play(click_sound);
            umgebung_t::show_names = (umgebung_t::show_names+1) & 3;
	    setze_dirty();
	    break;
	case '"':
	    sound_play(click_sound);
	    gebaeude_t::hide++;
	    if(gebaeude_t::hide>gebaeude_t::ALL_HIDDEN) {
	    	gebaeude_t::hide = gebaeude_t::NOT_HIDDEN;
	    }
	    baum_t::hide = !baum_t::hide;
	    setze_dirty();
	    break;
	case '#':
	    sound_play(click_sound);
	    boden_t::toggle_grid();
	    setze_dirty();
	    break;
	case 167:    // Hajo: '§'
	    verteile_baeume(3);
	    break;

	case 'a':
	  setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), Z_PLAN,  NO_SOUND, NO_SOUND );
	    break;
	case 'B':
	    sound_play(click_sound);
	    create_win(0, 0, new message_frame_t(this), w_info);
	    break;
	case 'c':
	    sound_play(click_sound);
	    display_snapshot();
	    create_win(-1, -1, 60, new nachrichtenfenster_t(this, "Screenshot\ngespeichert.\n"), w_autodelete);
            break;
	case 'd':
	    setze_maus_funktion(wkz_lower, skinverwaltung_t::downzeiger->gib_bild_nr(0), Z_GRID,  NO_SOUND, NO_SOUND );
	    break;
	case 'e':
	    if(default_electric==NULL) {
			default_electric = wayobj_t::wayobj_search(weg_t::schiene,weg_t::overheadlines,get_timeline_year_month());
	    }
	    setze_maus_funktion(wkz_wayobj, default_electric->gib_cursor()->gib_bild_nr(0), Z_PLAN, (long)default_electric, SFX_JACKHAMMER, SFX_FAILURE);
	    break;
	case 'f': /* OCR: Finances */
	    sound_play(click_sound);
         create_win(-1, -1, -1, get_active_player()->gib_money_frame(), w_info);
	    break;
	case 'g':
	    if(skinverwaltung_t::senke) {
		setze_maus_funktion(wkz_senke, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), Z_PLAN,  NO_SOUND, NO_SOUND );
	    }
	    break;

	case 'H':
	    if(hausbauer_t::headquarter.count()>0) {
		setze_maus_funktion(wkz_headquarter, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), Z_PLAN, SFX_JACKHAMMER, SFX_FAILURE);
	    }
	    break;

	case 'I':
	  setze_maus_funktion(wkz_build_industries_land, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), Z_PLAN,  NO_SOUND, NO_SOUND );
	    break;

	case 'k':
	    create_win(272,160, -1, new ki_kontroll_t(this), w_info, magic_ki_kontroll_t);
	    break;
	case 'l':
	    if(wegbauer_t::leitung_besch) {
			setze_maus_funktion(wkz_wegebau,
					wegbauer_t::leitung_besch->gib_cursor()->gib_bild_nr(0),
					    Z_PLAN,
					    (const void *)wegbauer_t::leitung_besch, 0, 0);
	    }
	    break;
	case 'm':
	    create_win(-1, -1, -1, new map_frame_t(this), w_info, magic_reliefmap);
	    break;
	case 'M':
	    setze_maus_funktion(wkz_marker, skinverwaltung_t::belegtzeiger->gib_bild_nr(0), Z_PLAN,  NO_SOUND, NO_SOUND );
	    break;
#ifdef USE_DRIVABLES
        case 'n':
            setze_maus_funktion(wkz_test_new_cars, skinverwaltung_t::belegtzeiger->gib_bild_nr(0), Z_PLAN,  NO_SOUND, NO_SOUND );
            break;
#endif
	case 'P':
		sound_play(click_sound);
		switch_active_player( active_player_nr+1 );
		break;
	case 'p':
	    sound_play(click_sound);
	    do_pause();
	    break;
	case 'r':
	    setze_maus_funktion(wkz_remover, skinverwaltung_t::killzeiger->gib_bild_nr(0), Z_PLAN, SFX_REMOVER, SFX_FAILURE);
	    break;
	case 's':
	    if(default_road==NULL) {
			default_road = wegbauer_t::weg_search(weg_t::strasse,90,get_timeline_year_month());
	    }
	    setze_maus_funktion(wkz_wegebau, default_road->gib_cursor()->gib_bild_nr(0), Z_PLAN, (long)default_road, SFX_JACKHAMMER, SFX_FAILURE);
	    break;
	case 'v':
	    umgebung_t::station_coverage_show = !umgebung_t::station_coverage_show;
	    setze_dirty();
	    break;
	case 't':
	    if(default_track==NULL) {
			default_track = wegbauer_t::weg_search(weg_t::schiene,100,get_timeline_year_month());
	  	}
	  	// may be NULL, if no track exists ...
	    if(default_track!=NULL) {
	    	setze_maus_funktion(wkz_wegebau, default_track->gib_cursor()->gib_bild_nr(0), Z_PLAN,	(long)default_track, SFX_JACKHAMMER, SFX_FAILURE);
	    }
	    break;
	case 'u':
	    setze_maus_funktion(wkz_raise, skinverwaltung_t::upzeiger->gib_bild_nr(0), Z_GRID,  NO_SOUND, NO_SOUND );
	    break;
	case 'W':
		fast_forward ^= 1;
		reset_timer();
		break;
	case 'w':
	    sound_play(click_sound);
	    create_win(-1, -1, -1, get_active_player()->get_line_frame(), w_info);
	    break;
	case '+':
	    display_set_light(display_get_light()+1);
	    setze_dirty();
	    break;
	case '-':
	    display_set_light(display_get_light()-1);
	    setze_dirty();
	    break;
	case 'C':
	    sound_play(click_sound);
	    setze_maus_funktion(wkz_add_city, skinverwaltung_t::stadtzeiger->gib_bild_nr(0), Z_GRID,  NO_SOUND, NO_SOUND );
	    break;

	case 'G':
	  create_win(0, 0,new goods_frame_t(this), w_autodelete);
	  break;

	case 'Q':
	case 'X':
	    sound_play(click_sound);
	    destroy_all_win();
	    //beenden();        // 02-Nov-2001  Markus Weber    Function has a new parameter
            beenden(false);
	    break;
	case 'L':
	    sound_play(click_sound);
	    create_win(new loadsave_frame_t(this, true), w_info, magic_load_t);
	    break;
#ifdef AUTOTEST
	case 'R':
	    setze_maus_funktion(tst_railtest, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), Z_PLAN,  NO_SOUND, NO_SOUND );
	    break;
#endif
	case 'S':
	    sound_play(click_sound);
	    create_win(new loadsave_frame_t(this, false), w_info, magic_save_t);
	    break;

	case 'T':
	    sound_play(click_sound);
	    create_win(0, 0, new citylist_frame_t(this), w_info);
	    break;

	case 'z':
		wkz_undo(get_active_player(),this);
		break;
	    /*
#ifdef AUTOTEST
	case 'T':
	    {
		worldtest_t tester (this, "test.log");
		tester.check_consistency();
	    }
	    break;
#endif
	    */
	    /*
#ifdef AUTOTEST
	case '%':
	  create_win(0, 0,
		     new buildings_frame_t(),
		     w_autodelete);
	    break;
#endif
		*/

	case 'V':
	    sound_play(click_sound);
	    create_win(-1, -1, -1, new convoi_frame_t(get_active_player(), this), w_autodelete, magic_convoi_t);
	    break;
		case SIM_KEY_F2:
		case SIM_KEY_F3:
		case SIM_KEY_F4:
		case SIM_KEY_F5:
		case SIM_KEY_F6:
		case SIM_KEY_F7:
		case SIM_KEY_F8:
		case SIM_KEY_F9:
		case SIM_KEY_F10:
		case SIM_KEY_F11:
		case SIM_KEY_F12:
		case SIM_KEY_F13:
		case SIM_KEY_F14:
		case SIM_KEY_F15:
		{
			int num = ev.ev_code - SIM_KEY_F2;
			if (event_get_last_control_shift() == 2) {
				DBG_MESSAGE("karte_t()","Save mouse_funk");
				if(quick_shortcuts.at(num) == NULL)
					quick_shortcuts.at(num) = new save_mouse_func;

				quick_shortcuts.at(num)->save_mouse_funk = mouse_funk;
				quick_shortcuts.at(num)->mouse_funk_param = mouse_funk_param;
				quick_shortcuts.at(num)->mouse_funk_ok_sound = mouse_funk_ok_sound;
				quick_shortcuts.at(num)->mouse_funk_ko_sound = mouse_funk_ko_sound;
				quick_shortcuts.at(num)->zeiger_versatz = zeiger->gib_yoff();
				quick_shortcuts.at(num)->zeiger_bild = zeiger->gib_bild();
			}
			else if (quick_shortcuts.at(num) != NULL) {
				DBG_MESSAGE("karte_t()","Recall mouse_funk");
				mouse_funk = quick_shortcuts.at(num)->save_mouse_funk;
				mouse_funk_param = quick_shortcuts.at(num)->mouse_funk_param;
				mouse_funk_ok_sound = quick_shortcuts.at(num)->mouse_funk_ok_sound;
				mouse_funk_ko_sound = quick_shortcuts.at(num)->mouse_funk_ko_sound;
				zeiger->setze_yoff(quick_shortcuts.at(num)->zeiger_versatz);
				zeiger->setze_bild(0,quick_shortcuts.at(num)->zeiger_bild);
				zeiger->set_flag(ding_t::dirty);
			}
		}
		break;
	case '9':
	    setze_ij_off(gib_ij_off()+koord::nord);
	    break;
	case '1':
	    setze_ij_off(gib_ij_off()+koord::sued);
	    break;
	case '7':
	    setze_ij_off(gib_ij_off()+koord::west);
	    break;
	case '3':
	    setze_ij_off(gib_ij_off()+koord::ost);
	    break;

	case '6':
	case SIM_KEY_RIGHT:
	    setze_ij_off(gib_ij_off()+koord(+1,-1));
	    break;
	case '2':
	case SIM_KEY_DOWN:
	    setze_ij_off(gib_ij_off()+koord(+1,+1));
	    break;
	case '8':
	case SIM_KEY_UP:
	    setze_ij_off(gib_ij_off()+koord(-1,-1));
	    break;
	case '4':
	case SIM_KEY_LEFT:
	    setze_ij_off(gib_ij_off()+koord(-1,+1));
	    break;


	case '>':
	  win_set_zoom_factor(get_zoom_factor() > 1 ? get_zoom_factor()-1 : 1);
	  setze_dirty();
	  break;

	case '<':
	  win_set_zoom_factor(get_zoom_factor() < 4 ? get_zoom_factor()+1 : 4);
	  setze_dirty();
	  break;

	case 27:
	case 127:
		// close topmost win
		destroy_win( win_get_top() );
		break;

	case 8:
	    sound_play(click_sound);
	    destroy_all_win();
	    break;

	case SIM_KEY_F1:
	    create_win(new help_frame_t("general.txt"), w_autodelete, magic_none);

	    break;
	case 0:
	case 13:
	    // just ignore the key
	    break;
	default:
	// ignore special keys
	if (ev.ev_code <= 255) {
	    DBG_MESSAGE("karte_t::interactive_event()",
			 "key `%c` is not bound to a function.",ev.ev_code);

	    create_win(new help_frame_t("keys.txt"), w_autodelete, magic_keyhelp);
	}
	}
    }

    if (IS_LEFTRELEASE(&ev)) {

	if(ev.my < 32) {
	    switch( ev.mx/32 ) {
	    case 0:
		sound_play(click_sound);
		create_win(240, 120, -1, new optionen_gui_t(this), w_info,
			   magic_optionen_gui_t);
		break;
	    case 1:
		sound_play(click_sound);
		create_win(-1, -1, -1, new map_frame_t(this), w_info, magic_reliefmap);
		break;
	    case 2:
		setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), Z_PLAN,  NO_SOUND, NO_SOUND );
		break;
	    case 3:
                {
		    werkzeug_parameter_waehler_t *wzw =
                        new werkzeug_parameter_waehler_t(this, "SLOPETOOLS");
		    wzw->setze_hilfe_datei("slopetools.txt");

			// evt. we have also the normal tools here ...
			if(skinverwaltung_t::hang_werkzeug->gib_bild_nr(8)!=IMG_LEER) {
			    wzw->add_tool(wkz_raise,
					  Z_GRID,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  skinverwaltung_t::hang_werkzeug->gib_bild_nr(8),
					  skinverwaltung_t::upzeiger->gib_bild_nr(0),
					  tool_tip_with_price(translator::translate("Anheben"), umgebung_t::cst_alter_land));

			    wzw->add_tool(wkz_lower,
					  Z_GRID,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  skinverwaltung_t::hang_werkzeug->gib_bild_nr(9),
					  skinverwaltung_t::downzeiger->gib_bild_nr(0),
					  tool_tip_with_price(translator::translate("Absenken"), umgebung_t::cst_alter_land));
			}

		    wzw->add_param_tool(wkz_set_slope,(long)SOUTH_SLOPE,
				  Z_PLAN,
				  SFX_JACKHAMMER,
				  SFX_FAILURE,
				  skinverwaltung_t::hang_werkzeug->gib_bild_nr(0),
				  skinverwaltung_t::slopezeiger->gib_bild_nr(0),
				  tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));

		    wzw->add_param_tool(wkz_set_slope,(long)NORTH_SLOPE,
				  Z_PLAN,
				  SFX_JACKHAMMER,
				  SFX_FAILURE,
				  skinverwaltung_t::hang_werkzeug->gib_bild_nr(1),
				  skinverwaltung_t::slopezeiger->gib_bild_nr(0),
				  tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));

		    wzw->add_param_tool(wkz_set_slope,(long)WEST_SLOPE,
				  Z_PLAN,
				  SFX_JACKHAMMER,
				  SFX_FAILURE,
				  skinverwaltung_t::hang_werkzeug->gib_bild_nr(2),
				  skinverwaltung_t::slopezeiger->gib_bild_nr(0),
				  tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));

		    wzw->add_param_tool(wkz_set_slope,(long)EAST_SLOPE,
				  Z_PLAN,
				  SFX_JACKHAMMER,
				  SFX_FAILURE,
				  skinverwaltung_t::hang_werkzeug->gib_bild_nr(3),
				  skinverwaltung_t::slopezeiger->gib_bild_nr(0),
				  tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));

		    wzw->add_param_tool(wkz_set_slope,(long)ALL_DOWN_SLOPE,
				  Z_PLAN,
				  SFX_JACKHAMMER,
				  SFX_FAILURE,
				  skinverwaltung_t::hang_werkzeug->gib_bild_nr(5),
				  skinverwaltung_t::slopezeiger->gib_bild_nr(0),
				  tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));

		    wzw->add_param_tool(wkz_set_slope,(long)ALL_UP_SLOPE,
				  Z_PLAN,
				  SFX_JACKHAMMER,
				  SFX_FAILURE,
				  skinverwaltung_t::hang_werkzeug->gib_bild_nr(6),
				  skinverwaltung_t::slopezeiger->gib_bild_nr(0),
				  tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));

/*
			// cover tile
		    wzw->add_param_tool(wkz_set_slope,(long)0,
				  Z_PLAN,
				  SFX_JACKHAMMER,
				  SFX_FAILURE,
				  skinverwaltung_t::hang_werkzeug->gib_bild_nr(4),
				  skinverwaltung_t::slopezeiger->gib_bild_nr(0),
				  tool_tip_with_price(translator::translate("Built artifical slopes"), umgebung_t::cst_set_slope));
*/

		    wzw->add_param_tool(wkz_set_slope,(long)RESTORE_SLOPE,
				  Z_PLAN,
				  SFX_JACKHAMMER,
				  SFX_FAILURE,
				  skinverwaltung_t::hang_werkzeug->gib_bild_nr(7),
				  skinverwaltung_t::slopezeiger->gib_bild_nr(0),
				  tool_tip_with_price(translator::translate("Restore natural slope"), umgebung_t::cst_alter_land));

		    sound_play(click_sound);

		    wzw->zeige_info(magic_slopetools);
                }
		break;
	    case 4:
	    		if(wegbauer_t::weg_search(weg_t::schiene,1,get_timeline_year_month())!=NULL) {
		    werkzeug_parameter_waehler_t *wzw =
                        new werkzeug_parameter_waehler_t(this, "RAILTOOLS");

		    wzw->setze_hilfe_datei("railtools.txt");

		    wegbauer_t::fill_menu(wzw,
					  weg_t::schiene,
					  wkz_wegebau,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  get_timeline_year_month()
					  );

			wayobj_t::fill_menu(wzw,
					weg_t::schiene,
					wkz_wayobj,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month()
					);

		    wzw->add_param_tool(&wkz_wayremover,
				  (const int)weg_t::schiene,
	  			  Z_PLAN,
				  SFX_REMOVER,
				  SFX_FAILURE,
				  skinverwaltung_t::edit_werkzeug->gib_bild_nr(7),
				  skinverwaltung_t::killzeiger->gib_bild_nr(0),
				  "remove tracks");

		    brueckenbauer_t::fill_menu(wzw,
					  weg_t::schiene,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
  					  get_timeline_year_month()
					  );

		    if(tunnelbauer_t::schienentunnel) {
		      wzw->add_param_tool(&tunnelbauer_t::baue,
					  weg_t::schiene,
					  Z_PLAN,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  tunnelbauer_t::schienentunnel->gib_cursor()->gib_bild_nr(1),
					  tunnelbauer_t::schienentunnel->gib_cursor()->gib_bild_nr(0),
					  tool_tip_with_price(translator::translate("Schienentunnel"), umgebung_t::cst_tunnel));
		    }

			roadsign_t::fill_menu(wzw,
					weg_t::schiene,
					wkz_roadsign,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month()
					);

		  wzw->add_param_tool(wkz_depot,
				hausbauer_t::bahn_depot_besch,
				Z_PLAN,
				SFX_GAVEL,
				SFX_FAILURE,
				hausbauer_t::bahn_depot_besch->gib_cursor()->gib_bild_nr(1),
				hausbauer_t::bahn_depot_besch->gib_cursor()->gib_bild_nr(0),
				tool_tip_with_price(translator::translate("Build train depot"), umgebung_t::cst_depot_rail));

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::bahnhof,
					  wkz_halt,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_station,
  					  get_timeline_year_month() );

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::bahnhof_geb,
					  wkz_station_building,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_post,
  					  get_timeline_year_month() );

		    sound_play(click_sound);

		    wzw->zeige_info(magic_railtools);
              }
              else {
				create_win(-1, -1, 60, new nachrichtenfenster_t(this, "Trains are not available yet!"), w_autodelete);
              }
		break;
	    case 5:
	    		if(wegbauer_t::weg_search(weg_t::monorail,1,get_timeline_year_month())!=NULL  &&  hausbauer_t::monorail_depot_besch!=NULL) {
				werkzeug_parameter_waehler_t *wzw =
					new werkzeug_parameter_waehler_t(this, "MONORAILTOOLS");

				wzw->setze_hilfe_datei("monorailtools.txt");

				wegbauer_t::fill_menu(wzw,
					weg_t::monorail,
					wkz_wegebau,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month(),
					0
				);

				wegbauer_t::fill_menu(wzw,
					weg_t::monorail,
					wkz_wegebau,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month(),
					1
				);

			wayobj_t::fill_menu(wzw,
					weg_t::monorail,
					wkz_wayobj,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month()
					);

		    wzw->add_param_tool(&wkz_wayremover,
				  (const int)weg_t::monorail,
	  			  Z_PLAN,
				  SFX_REMOVER,
				  SFX_FAILURE,
				  skinverwaltung_t::edit_werkzeug->gib_bild_nr(8),
				  skinverwaltung_t::killzeiger->gib_bild_nr(0),
				  "remove monorails");

		    brueckenbauer_t::fill_menu(wzw,
					  weg_t::monorail,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
  					  get_timeline_year_month()
					  );

			roadsign_t::fill_menu(wzw,
					weg_t::monorail,
					wkz_roadsign,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month()
					);

		      wzw->add_param_tool(wkz_depot,
				hausbauer_t::monorail_depot_besch,
				Z_PLAN,
				SFX_GAVEL,
				SFX_FAILURE,
				hausbauer_t::monorail_depot_besch->gib_cursor()->gib_bild_nr(1),
				hausbauer_t::monorail_depot_besch->gib_cursor()->gib_bild_nr(0),
				tool_tip_with_price(translator::translate("Build monotrail depot"), umgebung_t::cst_depot_rail));

		    hausbauer_t::fill_menu(wzw,
						  hausbauer_t::monorailstop,
						  wkz_halt,
						  SFX_JACKHAMMER,
						  SFX_FAILURE,
						  umgebung_t::cst_multiply_station,
						  get_timeline_year_month() );

		    hausbauer_t::fill_menu(wzw,
						  hausbauer_t::monorail_geb,
						  wkz_station_building,
						  SFX_JACKHAMMER,
						  SFX_FAILURE,
						  umgebung_t::cst_multiply_station,
						  get_timeline_year_month() );

				sound_play(click_sound);

				wzw->zeige_info(magic_monorailtools);
		    }
              else {
				create_win(-1, -1, 60, new nachrichtenfenster_t(this, "Monorails are not available yet!"), w_autodelete);
              }
			break;
	    case 6:
	    		if(wegbauer_t::weg_search(weg_t::schiene_strab,1,get_timeline_year_month())!=NULL  &&  hausbauer_t::tram_depot_besch!=NULL) {
				werkzeug_parameter_waehler_t *wzw =
					new werkzeug_parameter_waehler_t(this, "TRAMTOOLS");

				wzw->setze_hilfe_datei("tramtools.txt");

				wegbauer_t::fill_menu(wzw,
					weg_t::schiene,
					wkz_wegebau,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month(),
					7
				);

			wayobj_t::fill_menu(wzw,
					weg_t::schiene,
					wkz_wayobj,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month()
					);

				wzw->add_param_tool(&wkz_wayremover,
				  (const int)weg_t::schiene,
	  			  Z_PLAN,
				  SFX_REMOVER,
				  SFX_FAILURE,
				  skinverwaltung_t::edit_werkzeug->gib_bild_nr(7),
				  skinverwaltung_t::killzeiger->gib_bild_nr(0),
				  "remove tracks");

			roadsign_t::fill_menu(wzw,
					weg_t::schiene,
					wkz_roadsign,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month()
					);

		      wzw->add_param_tool(wkz_depot,
				hausbauer_t::tram_depot_besch,
				Z_PLAN,
				SFX_GAVEL,
				SFX_FAILURE,
				hausbauer_t::tram_depot_besch->gib_cursor()->gib_bild_nr(1),
				hausbauer_t::tram_depot_besch->gib_cursor()->gib_bild_nr(0),
				tool_tip_with_price(translator::translate("Build tram depot"), umgebung_t::cst_depot_rail));

		    hausbauer_t::fill_menu(wzw,
						  hausbauer_t::bahnhof,
						  wkz_halt,
						  SFX_JACKHAMMER,
						  SFX_FAILURE,
						  umgebung_t::cst_multiply_station,
						  get_timeline_year_month() );

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::bushalt,
					  wkz_halt,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_roadstop,
					  get_timeline_year_month() );

				sound_play(click_sound);

				wzw->zeige_info(magic_tramtools);
		    }
              else {
				create_win(-1, -1, 60, new nachrichtenfenster_t(this, "Trams are not available yet!"), w_autodelete);
              }
			break;
	    case 7:
	    		if(wegbauer_t::weg_search(weg_t::strasse,1,get_timeline_year_month())!=NULL) {
		    werkzeug_parameter_waehler_t *wzw =
                        new werkzeug_parameter_waehler_t(this, "ROADTOOLS");

		    wzw->setze_hilfe_datei("roadtools.txt");

			wegbauer_t::fill_menu(wzw,
				weg_t::strasse,
				wkz_wegebau,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				get_timeline_year_month(),
				0
			);

			wegbauer_t::fill_menu(wzw,
				weg_t::strasse,
				wkz_wegebau,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				get_timeline_year_month(),
				1
			);

			wayobj_t::fill_menu(wzw,
					weg_t::strasse,
					wkz_wayobj,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month()
					);

		    wzw->add_param_tool(&wkz_wayremover,
				  (const int)weg_t::strasse,
	  			  Z_PLAN,
				  SFX_REMOVER,
				  SFX_FAILURE,
				  skinverwaltung_t::edit_werkzeug->gib_bild_nr(9),
				  skinverwaltung_t::killzeiger->gib_bild_nr(0),
				  "remove roads");

		    brueckenbauer_t::fill_menu(wzw,
					  weg_t::strasse,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  get_timeline_year_month()
					  );

		    if(tunnelbauer_t::strassentunnel) {
		      wzw->add_param_tool(&tunnelbauer_t::baue,
					  (long)weg_t::strasse,
					  Z_PLAN,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  tunnelbauer_t::strassentunnel->gib_cursor()->gib_bild_nr(1),
					  tunnelbauer_t::strassentunnel->gib_cursor()->gib_bild_nr(0),
					  tool_tip_with_price(translator::translate("Strassentunnel"), umgebung_t::cst_tunnel));
		    }

			roadsign_t::fill_menu(wzw,
					weg_t::strasse,
					wkz_roadsign,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month()
					);

		      wzw->add_param_tool(wkz_depot,
				hausbauer_t::str_depot_besch,
				Z_PLAN,
				SFX_GAVEL,
				SFX_FAILURE,
				hausbauer_t::str_depot_besch->gib_cursor()->gib_bild_nr(1),
				hausbauer_t::str_depot_besch->gib_cursor()->gib_bild_nr(0),
				tool_tip_with_price(translator::translate("Build road depot"), umgebung_t::cst_depot_rail));

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::bushalt,
					  wkz_halt,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_roadstop,
  					  get_timeline_year_month() );

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::ladebucht,
					  wkz_halt,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_roadstop,
  					  get_timeline_year_month() );

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::bushalt_geb,
					  wkz_station_building,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_post,
  					  get_timeline_year_month() );

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::ladebucht_geb,
					  wkz_station_building,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_post,
  					  get_timeline_year_month() );

		    sound_play(click_sound);

		    wzw->zeige_info(magic_roadtools);
                }
              else {
				create_win(-1, -1, 60, new nachrichtenfenster_t(this, "Cars are not available yet!"), w_autodelete);
              }
		break;
	    case 8:
                {
		    werkzeug_parameter_waehler_t *wzw =
                        new werkzeug_parameter_waehler_t(this, "SHIPTOOLS");

		    wzw->setze_hilfe_datei("shiptools.txt");

			wegbauer_t::fill_menu(wzw,
					  weg_t::wasser,
					  wkz_wegebau,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  get_timeline_year_month()
					  );

			wayobj_t::fill_menu(wzw,
					weg_t::wasser,
					wkz_wayobj,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month()
					);

		    wzw->add_param_tool(&wkz_wayremover,
				  (const int)weg_t::wasser,
	  			  Z_PLAN,
				  SFX_REMOVER,
				  SFX_FAILURE,
				  skinverwaltung_t::edit_werkzeug->gib_bild_nr(10),
				  skinverwaltung_t::killzeiger->gib_bild_nr(0),
				  "remove channels");

		    brueckenbauer_t::fill_menu(wzw,
					  weg_t::wasser,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  get_timeline_year_month()
					  );

			roadsign_t::fill_menu(wzw,
					weg_t::wasser,
					wkz_roadsign,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month()
					);

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::binnenhafen,
					  wkz_halt,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_dock,
					  get_timeline_year_month() );

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::hafen,
					  wkz_dockbau,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_dock,
					  get_timeline_year_month() );

		      wzw->add_param_tool(wkz_depot,
				hausbauer_t::sch_depot_besch,
				Z_PLAN,
				SFX_DOCK,
				SFX_FAILURE,
				skinverwaltung_t::schiffs_werkzeug->gib_bild_nr(0),
				skinverwaltung_t::werftNSzeiger->gib_bild_nr(0),
				tool_tip_with_price(translator::translate("Build ship depot"), umgebung_t::cst_depot_ship));

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::binnenhafen_geb,
					  wkz_station_building,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_post,
					  get_timeline_year_month() );

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::hafen_geb,
					  wkz_station_building,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_post,
					  get_timeline_year_month() );

		    sound_play(click_sound);

		    wzw->zeige_info(magic_shiptools);
                }
		break;

	    case 9:
	     if(hausbauer_t::air_depot.count()>0  &&  wegbauer_t::weg_search(weg_t::luft,1,get_timeline_year_month())!=NULL) {
					// start aircraft
					werkzeug_parameter_waehler_t *wzw =
						new werkzeug_parameter_waehler_t(this, "AIRTOOLS");

					wzw->setze_hilfe_datei("airtools.txt");

			wegbauer_t::fill_menu(wzw,
				weg_t::luft,
				wkz_wegebau,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				get_timeline_year_month(),
				0
			);

			wegbauer_t::fill_menu(wzw,
				weg_t::luft,
				wkz_wegebau,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				get_timeline_year_month(),
				1
			);

			wayobj_t::fill_menu(wzw,
					weg_t::luft,
					wkz_wayobj,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month()
					);

			roadsign_t::fill_menu(wzw,
					weg_t::luft,
					wkz_roadsign,
					SFX_JACKHAMMER,
					SFX_FAILURE,
					get_timeline_year_month()
					);

/* since the route calculation is different for air, the tool will simply crash :( */
		    wzw->add_param_tool(&wkz_wayremover,
				  (const int)weg_t::luft,
	  			  Z_PLAN,
				  SFX_REMOVER,
				  SFX_FAILURE,
				  skinverwaltung_t::edit_werkzeug->gib_bild_nr(11),
				  skinverwaltung_t::killzeiger->gib_bild_nr(0),
				  "remove airstrips");

		    hausbauer_t::fill_menu(wzw,
						  hausbauer_t::air_depot,
						  wkz_depot,
						  SFX_GAVEL,
						  SFX_FAILURE,
						  umgebung_t::cst_multiply_airterminal,
						  get_timeline_year_month() );

		    hausbauer_t::fill_menu(wzw,
						  hausbauer_t::airport,
						  wkz_halt,
						  SFX_JACKHAMMER,
						  SFX_FAILURE,
						  umgebung_t::cst_multiply_airterminal,
						  get_timeline_year_month() );

		    hausbauer_t::fill_menu(wzw,
						  hausbauer_t::airport_geb,
						  wkz_station_building,
						  SFX_JACKHAMMER,
						  SFX_FAILURE,
						  umgebung_t::cst_multiply_post,
						  get_timeline_year_month() );

					wzw->zeige_info(magic_airtools);
					// end aircraft
				}
              else {
				create_win(-1, -1, 60, new nachrichtenfenster_t(this, "Planes are not available yet!"), w_autodelete);
              }
		break;
	    case 10:
          {
		    werkzeug_parameter_waehler_t *wzw =
                        new werkzeug_parameter_waehler_t(this, "SPECIALTOOLS");

		    wzw->setze_hilfe_datei("special.txt");

			wegbauer_t::fill_menu(wzw,
				weg_t::strasse,
				wkz_wegebau,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				get_timeline_year_month(),
				255
			);

			wegbauer_t::fill_menu(wzw,
				weg_t::schiene,
				wkz_wegebau,
				SFX_JACKHAMMER,
				SFX_FAILURE,
				get_timeline_year_month(),
				255
			);

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::post,
					  wkz_station_building,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_post,
					  get_timeline_year_month() );

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::wartehalle,
					  wkz_station_building,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_post,
					  get_timeline_year_month() );

		    hausbauer_t::fill_menu(wzw,
					  hausbauer_t::lagerhalle,
					  wkz_station_building,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  umgebung_t::cst_multiply_post,
					  get_timeline_year_month() );

		      wzw->add_tool(wkz_switch_player,
					  Z_PLAN,
					  -1,
					  -1,
					  skinverwaltung_t::edit_werkzeug->gib_bild_nr(4),
					  IMG_LEER,
					  translator::translate("Change player") );

		      wzw->add_tool(wkz_add_city,
					  Z_PLAN,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  skinverwaltung_t::special_werkzeug->gib_bild_nr(0),
					  skinverwaltung_t::stadtzeiger->gib_bild_nr(0),
					  tool_tip_with_price(translator::translate("Found new city"), umgebung_t::cst_found_city));

		    wzw->add_tool(wkz_pflanze_baum,
				  Z_PLAN,
				  SFX_SELECT,
				  SFX_FAILURE,
				  skinverwaltung_t::special_werkzeug->gib_bild_nr(6),
				  skinverwaltung_t::baumzeiger->gib_bild_nr(0),
				  translator::translate("Plant tree"));

	    if(wegbauer_t::leitung_besch) {
		    wzw->add_param_tool(wkz_wegebau,
				  (const void *)wegbauer_t::leitung_besch,
				  Z_PLAN,
				  SFX_JACKHAMMER,
				  SFX_FAILURE,
				  wegbauer_t::leitung_besch->gib_cursor()->gib_bild_nr(1),
				  wegbauer_t::leitung_besch->gib_cursor()->gib_bild_nr(0),
				  tool_tip_with_price(translator::translate("Build powerline"), -wegbauer_t::leitung_besch->gib_preis()));

		      wzw->add_tool(wkz_senke,
					  Z_PLAN,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  skinverwaltung_t::special_werkzeug->gib_bild_nr(2),
					  skinverwaltung_t::undoc_zeiger->gib_bild_nr(0),
					 translator::translate("Build drain"));
			}

		      wzw->add_tool(wkz_marker,
					  Z_PLAN,
					  SFX_JACKHAMMER,
					  SFX_FAILURE,
					  skinverwaltung_t::special_werkzeug->gib_bild_nr(5),
					 skinverwaltung_t::belegtzeiger->gib_bild_nr(0),
					 translator::translate("Marker"));

		    sound_play(click_sound);
		    wzw->zeige_info(magic_specialtools);
                }
		break;
	    case 11:
		setze_maus_funktion(wkz_remover, skinverwaltung_t::killzeiger->gib_bild_nr(0), Z_PLAN, SFX_REMOVER, SFX_FAILURE);
		break;
	    case 12:
	      // left empty
	    break;
	    case 13:
	      sound_play(click_sound);
	    create_win(-1, -1, -1, get_active_player()->get_line_frame(), w_info);
	      break;
	    case 14:
          {
		    werkzeug_parameter_waehler_t *wzw =
                        new werkzeug_parameter_waehler_t(this, "LISTTOOLS");

		    wzw->setze_hilfe_datei("list.txt");

		      wzw->add_tool(wkz_list_halt_tool,
					  Z_PLAN,
					  -1,
					  -1,
					  skinverwaltung_t::listen_werkzeug->gib_bild_nr(0),
					 IMG_LEER,
					 translator::translate("hl_title"));

		      wzw->add_tool(wkz_list_vehicle_tool,
					  Z_PLAN,
					  -1,
					  -1,
					  skinverwaltung_t::listen_werkzeug->gib_bild_nr(1),
					 IMG_LEER,
					 translator::translate("cl_title"));

		      wzw->add_tool(wkz_list_town_tool,
					  Z_PLAN,
					  -1,
					  -1,
					  skinverwaltung_t::listen_werkzeug->gib_bild_nr(2),
					 IMG_LEER,
					 translator::translate("tl_title"));

		      wzw->add_tool(wkz_list_good_tool,
					  Z_PLAN,
					  -1,
					  -1,
					  skinverwaltung_t::listen_werkzeug->gib_bild_nr(3),
					 IMG_LEER,
					 translator::translate("gl_title"));

		      wzw->add_tool(wkz_list_factory_tool,
					  Z_PLAN,
					  -1,
					  -1,
					  skinverwaltung_t::listen_werkzeug->gib_bild_nr(4),
					 IMG_LEER,
					 translator::translate("fl_title"));

		      wzw->add_tool(wkz_list_curiosity_tool,
					  Z_PLAN,
					  -1,
					  -1,
					  skinverwaltung_t::listen_werkzeug->gib_bild_nr(5),
					 IMG_LEER,
					 translator::translate("curlist_title"));

		    sound_play(click_sound);

		    wzw->zeige_info(magic_listtools);
         }
		break;
	    case 15:
	    	// line info
		sound_play(click_sound);
	    create_win(0, 0, new message_frame_t(this), w_info);
		break;
	    case 16:
		sound_play(click_sound);
           create_win(-1, -1, -1, get_active_player()->gib_money_frame(), w_info);
		break;
	    case 18:
		sound_play(click_sound);
                display_snapshot();
		create_win(-1, -1, 60, new nachrichtenfenster_t(this, "Screenshot\ngespeichert.\n"), w_autodelete);
		break;
				case 19:
					// Pause: warten auf die nächste Taste
					do_pause();
					setze_dirty();
					break;
				case 20:
					fast_forward ^= 1;
					reset_timer();
					break;
				case 21:
					sound_play(click_sound);
					create_win(new help_frame_t("general.txt"), w_autodelete, magic_mainhelp);
					break;
			}
		}
		else {
			if(mouse_funk != NULL) {
				koord pos (mi,mj);

DBG_MESSAGE("karte_t::interactive_event(event_t &ev)", "calling a tool");
				const int ok = mouse_funk(get_active_player(), this, pos, mouse_funk_param);
				if(ok) {
					if(mouse_funk_ok_sound!=NO_SOUND) {
						struct sound_info info = {mouse_funk_ok_sound,255,0};
//DBG_MESSAGE("karte_t::interactive_event(event_t &ev)", "play sound %i",mouse_funk_ok_sound);
						sound_play(info);
					}
				} else {
					if(mouse_funk_ko_sound!=NO_SOUND) {
						struct sound_info info = {mouse_funk_ko_sound,255,0};
//DBG_MESSAGE("karte_t::interactive_event(event_t &ev)", "play sound %i",mouse_funk_ko_sound);
						sound_play(info);
					}
				}
			 }
		}
	}

    // mouse wheel scrolled -> rezoom
    if (ev.ev_class == EVENT_CLICK) {
    	switch (ev.ev_code) {
    		case MOUSE_WHEELUP:
	  			win_set_zoom_factor(get_zoom_factor() > 1 ? get_zoom_factor()-1 : 1);
				  setze_dirty();
    		break;
    		case MOUSE_WHEELDOWN:
	  			win_set_zoom_factor(get_zoom_factor() < 4 ? get_zoom_factor()+1 : 4);
				  setze_dirty();
    		break;
    	}
  	}
    INT_CHECK("simworld 2117");
}



void
karte_t::interactive_update()
{
	event_t ev;
	bool swallowed = false;

	do {

		// get an event
		win_poll_event(&ev);

		if(ev.ev_class==EVENT_SYSTEM  &&  ev.ev_code==SYSTEM_QUIT) {
			// Beenden des Programms wenn das Fenster geschlossen wird.
			destroy_all_win();
			beenden(true);
			return;
		}

		if(ev.ev_class!=EVENT_NONE &&  ev.ev_class!=IGNORE_EVENT) {

			swallowed = check_pos_win(&ev);

			if(IS_RIGHTCLICK(&ev)) {
				display_show_pointer(false);
			} else if(IS_RIGHTRELEASE(&ev)) {
				display_show_pointer(true);
			} else if(ev.ev_class==EVENT_DRAG  &&  ev.ev_code==MOUSE_RIGHTBUTTON) {
				// unset following
				if(follow_convoi.is_bound()) {
					follow_convoi = convoihandle_t();
				}
				blick_aendern(&ev);
			}

			if(ev.button_state==0  &&  ev.ev_class==EVENT_MOVE) {
				bewege_zeiger(&ev);
			}
		}

		INT_CHECK("simworld 2630");

	} while(ev.button_state != 0);

	if (!swallowed) {
		interactive_event(ev);
	}
}



#define MIN_LOOPS 5
#define MAX_LOOPS 15
#define MIN_FPS 50
#define MAX_FPS 100

bool
karte_t::interactive()
{
	unsigned long now = get_system_ms();
	unsigned long this_step_time = 0;
	thisFPS = 0;

	active_player_nr = 0;
	active_player = spieler[0];

	last_step_time = get_current_time_millis();
	steps_bis_jetzt = steps;

	sleep_time = 5000;
	doit = true;

	// init for a good separation
	for(int i=0; i<GRUPPEN; i++) {
		step_group_times[i] = (350*i)/GRUPPEN;
	}

	do {

		if(fast_forward) {

			if(abs((long)(steps-now))>=5) {
				const long t = get_system_ms();

				last_simloops = steps-now;
				realFPS = (thisFPS*1000)/((t-this_step_time)|1);// make sure, this is always!=0!!!
				lastFPS = thisFPS;
				thisFPS = 0;
				this_step_time = t;
				now = steps;
			}

			last_step_time = ticks;

			INT_CHECK("simworld 3746");
			interactive_update();

			// check if we need to play a new midi file
			check_midi();

			INT_CHECK("simworld 3763");
			interactive_update();

			sync_step( -987 );

			step(ticks-last_step_time);
		}
		else {
			INT_CHECK("simworld 3746");
			interactive_update();

			// check if we need to play a new midi file
			check_midi();

			INT_CHECK("simworld 3763");
			interactive_update();

			// welt steppen
			// but this will not neccessarily result in a step!
			this_step_time = get_current_time_millis();
			step((long)(this_step_time-last_step_time));
			last_step_time = this_step_time;

			INT_CHECK("simworld 3772");
			interactive_update();

			const unsigned long t = get_system_ms();

			if(t > now+1000) {
				// every second is enough ...

				const long stretch_factor = get_time_multi();
				last_simloops = ( (steps - steps_bis_jetzt) * 16) / stretch_factor;
				realFPS = (thisFPS * 1000) / (t-now);
				lastFPS = (thisFPS * 16) / stretch_factor;
				thisFPS = 0;

				if(last_simloops<MIN_LOOPS) {
					sleep_time = 0;
					increase_frame_time();
				}
				else if(last_simloops>MAX_LOOPS) {
					sleep_time += 1;
				}
				else if(last_simloops<MAX_LOOPS  &&  sleep_time>=1) {
					sleep_time -= 1;
				}

				if(realFPS>MAX_FPS  ||  last_simloops<MIN_LOOPS) {
					increase_frame_time();
				} else if(realFPS<MIN_FPS  &&  last_simloops>MIN_LOOPS) {
					reduce_frame_time();
				}

				steps_bis_jetzt = steps;

				now = t;
			}

			if(sleep_time>0) {
				interactive_update();
				if(sleep_time>25) {
					sleep_time = 25;
				}
				simusleep( sleep_time );
			}
		}

	}while(doit);

	return m_quit_simutrans;      // 02-Nov-2001    Markus Weber    Added
}
