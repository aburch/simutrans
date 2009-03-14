/*
 * dialog for setting the climate border and other map related parameters
 *
 * prissi
 *
 * August 2006
 */

#include "climates.h"
#include "karte.h"
#include "welt.h"

#include "../besch/grund_besch.h"

#include "../simdebug.h"
#include "../simworld.h"
#include "../simwin.h"

#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"

// just for their structure size ...
#include "../simgraph.h"

#include "../utils/simstring.h"
#include "components/list_button.h"

#define START_HEIGHT (28)

#define LEFT_ARROW (130)
#define RIGHT_ARROW (170)
#define TEXT_RIGHT (155) // 10 are offset in routine ..

#include <sys/stat.h>
#include <time.h>

/**
 * set the climate borders
 * @author prissi
 */

climate_gui_t::climate_gui_t(einstellungen_t* sets) :
	gui_frame_t("Climate Control")
{
DBG_MESSAGE("","sizeof(stat)=%d, sizeof(tm)=%d",sizeof(struct stat),sizeof(struct tm) );
	this->sets = sets;

	// select map stuff ..
	int intTopOfButton = 4;

	// mountian/water stuff
	water_level[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_ARROW,intTopOfButton) );
	water_level[0].add_listener( this );
	add_komponente( water_level+0 );
	water_level[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_ARROW,intTopOfButton) );
	water_level[1].add_listener( this );
	add_komponente( water_level+1 );
	intTopOfButton += 12;

	mountain_height[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_ARROW,intTopOfButton) );
	mountain_height[0].add_listener( this );
	add_komponente( mountain_height+0 );
	mountain_height[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_ARROW,intTopOfButton) );
	mountain_height[1].add_listener( this );
	add_komponente( mountain_height+1 );
	intTopOfButton += 12;

	mountain_roughness[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_ARROW,intTopOfButton) );
	mountain_roughness[0].add_listener( this );
	add_komponente( mountain_roughness+0 );
	mountain_roughness[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_ARROW,intTopOfButton) );
	mountain_roughness[1].add_listener( this );
	add_komponente( mountain_roughness+1 );
	intTopOfButton += 12+5;

	// summer snowline alsway startig above highest climate
	intTopOfButton += 12;

	snowline_winter[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_ARROW,intTopOfButton) );
	snowline_winter[0].add_listener( this );
	add_komponente( snowline_winter+0 );
	snowline_winter[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_ARROW,intTopOfButton) );
	snowline_winter[1].add_listener( this );
	add_komponente( snowline_winter+1 );
	intTopOfButton += 12+5;

	// other climate borders ...
	end_desert[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_ARROW,intTopOfButton) );
	end_desert[0].add_listener( this );
	add_komponente( end_desert+0 );
	end_desert[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_ARROW,intTopOfButton) );
	end_desert[1].add_listener( this );
	add_komponente( end_desert+1 );
	intTopOfButton += 12;

	end_tropic[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_ARROW,intTopOfButton) );
	end_tropic[0].add_listener( this );
	add_komponente( end_tropic+0 );
	end_tropic[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_ARROW,intTopOfButton) );
	end_tropic[1].add_listener( this );
	add_komponente( end_tropic+1 );
	intTopOfButton += 12;

	end_mediterran[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_ARROW,intTopOfButton) );
	end_mediterran[0].add_listener( this );
	add_komponente( end_mediterran+0 );
	end_mediterran[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_ARROW,intTopOfButton) );
	end_mediterran[1].add_listener( this );
	add_komponente( end_mediterran+1 );
	intTopOfButton += 12;

	end_temperate[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_ARROW,intTopOfButton) );
	end_temperate[0].add_listener( this );
	add_komponente( end_temperate+0 );
	end_temperate[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_ARROW,intTopOfButton) );
	end_temperate[1].add_listener( this );
	add_komponente( end_temperate+1 );
	intTopOfButton += 12;

	end_tundra[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_ARROW,intTopOfButton) );
	end_tundra[0].add_listener( this );
	add_komponente( end_tundra+0 );
	end_tundra[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_ARROW,intTopOfButton) );
	end_tundra[1].add_listener( this );
	add_komponente( end_tundra+1 );
	intTopOfButton += 12;

	end_rocky[0].init( button_t::repeatarrowleft, NULL, koord(LEFT_ARROW,intTopOfButton) );
	end_rocky[0].add_listener( this );
	add_komponente( end_rocky+0 );
	end_rocky[1].init( button_t::repeatarrowright, NULL, koord(RIGHT_ARROW,intTopOfButton) );
	end_rocky[1].add_listener( this );
	add_komponente( end_rocky+1 );
	intTopOfButton += 12;

	// the rocky will be alway below the snow line; no need to set this explicitely
	intTopOfButton += 4;

	no_tree.init( button_t::square, "no tree", koord(10,intTopOfButton), koord(BUTTON_WIDTH,BUTTON_HEIGHT)); // right align
	no_tree.pressed=umgebung_t::no_tree;
	no_tree.add_listener( this );
	add_komponente( &no_tree );
	intTopOfButton += 12+4;

	river_n.set_pos(koord(LEFT_ARROW,intTopOfButton) );
	river_n.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
	river_n.add_listener(this);
	river_n.set_limits(0,1024);
	river_n.set_value( sets->get_river_number() );
	river_n.wrap_mode( false );
	add_komponente( &river_n );
	intTopOfButton += 12;

	river_min.set_pos(koord(LEFT_ARROW,intTopOfButton) );
	river_min.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
	river_min.add_listener(this);
	river_min.set_limits(0,max(16,sets->get_max_river_length())-16);
	river_min.set_value( sets->get_min_river_length() );
	river_min.wrap_mode( false );
	add_komponente( &river_min );
	intTopOfButton += 12;

	river_max.set_pos(koord(LEFT_ARROW,intTopOfButton) );
	river_max.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
	river_max.add_listener(this);
	river_max.set_limits(sets->get_min_river_length()+16,1024);
	river_max.set_value( sets->get_max_river_length() );
	river_max.wrap_mode( false );
	add_komponente( &river_max );
	intTopOfButton += 12;

	set_fenstergroesse( koord(RIGHT_ARROW+16, intTopOfButton+4+16) );
}





/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool
climate_gui_t::action_triggered( gui_action_creator_t *komp, value_t v)
{
	welt_gui_t *welt_gui = dynamic_cast<welt_gui_t *>(win_get_magic( magic_welt_gui_t ));
	if(komp==&no_tree) {
		umgebung_t::no_tree ^= 1;
		no_tree.pressed ^= 1;
		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
	}
	else if(komp==water_level+0) {
		if(sets->get_grundwasser() > -10*Z_TILE_STEP ) {
			sets->set_grundwasser( sets->get_grundwasser() - Z_TILE_STEP );
			if(  welt_gui  ) {
				welt_gui->update_preview();
			}
		}
	}
	else if(komp==water_level+1) {
		if(sets->get_grundwasser() < 0 ) {
			sets->set_grundwasser( sets->get_grundwasser() + Z_TILE_STEP );
			if(  welt_gui  ) {
				welt_gui->update_preview();
			}
		}
	}
	else if(komp==mountain_height+0) {
		if(sets->get_max_mountain_height() > 0.0 ) {
			sets->set_max_mountain_height( sets->get_max_mountain_height() - 10 );
			if(  welt_gui  ) {
				welt_gui->update_preview();
			}
		}
	}
	else if(komp==mountain_height+1) {
		if(sets->get_max_mountain_height() < 320.0 ) {
			sets->set_max_mountain_height( sets->get_max_mountain_height() + 10 );
			if(  welt_gui  ) {
				welt_gui->update_preview();
			}
		}
	}
	else if(komp==mountain_roughness+0) {
		if(sets->get_map_roughness() > 0.4 ) {
			sets->set_map_roughness( sets->get_map_roughness() - 0.05 );
			if(  welt_gui  ) {
				welt_gui->update_preview();
			}
		}
	}
	else if(komp==mountain_roughness+1) {
#ifndef DOUBLE_GROUNDS
		if(sets->get_map_roughness() < 0.7 ) {
#else
		if(sets->get_map_roughness() < 1.00) {
#endif
			sets->set_map_roughness( sets->get_map_roughness() + 0.05 );
			if(  welt_gui  ) {
				welt_gui->update_preview();
			}
		}
	}
	else if(komp==&river_n) {
		sets->set_river_number( v.i );
	}
	else if(komp==&river_min) {
		sets->set_min_river_length( v.i );
		river_max.set_limits(v.i+16,1024);
	}
	else if(komp==&river_max) {
		sets->set_max_river_length( v.i );
		river_min.set_limits(0,max(16,v.i)-16);
	}
	else {
		// all climate borders from here on
		sint16 *climate_borders = (sint16 *)sets->get_climate_borders();

		if(komp==snowline_winter+0) {
			if(sets->get_winter_snowline()>0) {
				sets->set_winter_snowline( sets->get_winter_snowline()-1 );
			}
		}
		else if(komp==snowline_winter+1) {
			sets->set_winter_snowline( sets->get_winter_snowline()+1 );
		}
		else if(komp==end_desert+0) {
			if(climate_borders[desert_climate]>0) {
				climate_borders[desert_climate]--;
			}
		}
		else if(komp==end_desert+1) {
			climate_borders[desert_climate]++;
		}
		else if(komp==end_tropic+0) {
			if(climate_borders[tropic_climate]>0) {
				climate_borders[tropic_climate]--;
			}
		}
		else if(komp==end_tropic+1) {
			climate_borders[tropic_climate]++;
		}
		else if(komp==end_mediterran+0) {
			if(climate_borders[mediterran_climate]>0) {
				climate_borders[mediterran_climate]--;
			}
		}
		else if(komp==end_mediterran+1) {
			climate_borders[mediterran_climate]++;
		}
		else if(komp==end_temperate+0) {
			if(climate_borders[temperate_climate]>0) {
				climate_borders[temperate_climate]--;
			}
		}
		else if(komp==end_temperate+1) {
			climate_borders[temperate_climate]++;
		}
		else if(komp==end_tundra+0) {
			if(climate_borders[tundra_climate]>0) {
				climate_borders[tundra_climate]--;
			}
		}
		else if(komp==end_tundra+1) {
			climate_borders[tundra_climate]++;
		}
		else if(komp==end_rocky+0) {
			if(climate_borders[rocky_climate]>0) {
				climate_borders[rocky_climate]--;
			}
		}
		else if(komp==end_rocky+1) {
			climate_borders[rocky_climate]++;
		}

		// artic starts at maximum end of climate
		sint16 arctic = 0;
		for(  int i=0;  i<MAX_CLIMATES-1;  i++  ) {
			if(climate_borders[i]>arctic) {
				arctic = climate_borders[i];
			}
		}
		climate_borders[arctic_climate] = arctic;

		// correct summer snowline too
		if(arctic<sets->get_winter_snowline()) {
			sets->set_winter_snowline( arctic );
		}
	}
	return true;
}




void climate_gui_t::zeichnen(koord pos, koord gr)
{
	no_tree.pressed = umgebung_t::no_tree;

	no_tree.set_text( "no tree" );

	gui_frame_t::zeichnen(pos, gr);

	const int x = pos.x+10;
	int y = pos.y+16+4;

	const sint16 water_level = (sets->get_grundwasser()/Z_TILE_STEP)+6;
	const sint16 *climate_borders = sets->get_climate_borders();

	// water level       18-Nov-01       Markus W. Added
	display_proportional_clip(x, y, translator::translate("Water level"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(water_level,0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Mountain height"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos((int)(sets->get_max_mountain_height()), 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Map roughness"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos((int)(sets->get_map_roughness()*20.0 + 0.5)-8 , 0) , ALIGN_RIGHT, COL_WHITE, true);     // x = round(roughness * 10)-4  // 0.6 * 10 - 4 = 2    //29-Nov-01     Markus W. Added
	y += 12+5;

	// season stuff
	display_proportional_clip(x, y, translator::translate("Summer snowline"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(climate_borders[arctic_climate], 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Winter snowline"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(sets->get_winter_snowline(),0),ALIGN_RIGHT, COL_WHITE, true);
	y += 12+5;

	// climate borders
	display_proportional_clip(x, y, translator::translate(grund_besch_t::get_climate_name_from_bit(desert_climate)), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(climate_borders[desert_climate], 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate(grund_besch_t::get_climate_name_from_bit(tropic_climate)), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(climate_borders[tropic_climate], 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate(grund_besch_t::get_climate_name_from_bit(mediterran_climate)), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(climate_borders[mediterran_climate], 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate(grund_besch_t::get_climate_name_from_bit(temperate_climate)), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(climate_borders[temperate_climate], 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate(grund_besch_t::get_climate_name_from_bit(tundra_climate)), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(climate_borders[tundra_climate], 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate(grund_besch_t::get_climate_name_from_bit(rocky_climate)), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(climate_borders[rocky_climate], 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;

	y += 12+4+4;	// no tree

	display_proportional_clip(x, y, translator::translate("Number of rivers"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("minimum length of rivers"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("maximum length of rivers"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
}
