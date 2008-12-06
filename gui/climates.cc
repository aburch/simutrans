/*
 * dialog for setting the climate border and other map related parameters
 *
 * prissi
 *
 * August 2006
 */

#include "climates.h"
#include "karte.h"

#include "../besch/grund_besch.h"

#include "../simdebug.h"
#include "../simworld.h"

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

climate_gui_t::climate_gui_t(welt_gui_t* welt_gui, einstellungen_t* sets) :
	gui_frame_t("Climate Control")
{
DBG_MESSAGE("","sizeof(stat)=%d, sizeof(tm)=%d",sizeof(struct stat),sizeof(struct tm) );
	this->sets = sets;
	this->welt_gui = welt_gui;

	// select map stuff ..
	int intTopOfButton = 16;

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

	setze_fenstergroesse( koord(RIGHT_ARROW+16, intTopOfButton+14+8+16) );

	no_tree.init( button_t::square, "no tree", koord(14,intTopOfButton+7), koord(BUTTON_WIDTH,BUTTON_HEIGHT)); // right align
	no_tree.pressed=umgebung_t::no_tree;
	no_tree.add_listener( this );
	add_komponente( &no_tree );
	intTopOfButton += 12;
}





/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool
climate_gui_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp==&no_tree) {
		umgebung_t::no_tree ^= 1;
		no_tree.pressed ^= 1;
		welt_gui->update_preview();
		}
	else if(komp==water_level+0) {
		if(sets->gib_grundwasser() > -10*Z_TILE_STEP ) {
			sets->setze_grundwasser( sets->gib_grundwasser() - 2*Z_TILE_STEP );
			welt_gui->update_preview();
		}
	}
	else if(komp==water_level+1) {
		if(sets->gib_grundwasser() < 0 ) {
			sets->setze_grundwasser( sets->gib_grundwasser() + 2*Z_TILE_STEP );
			welt_gui->update_preview();
		}
	}
	else if(komp==mountain_height+0) {
		if(sets->gib_max_mountain_height() > 0.0 ) {
			sets->setze_max_mountain_height( sets->gib_max_mountain_height() - 10 );
			welt_gui->update_preview();
		}
	}
	else if(komp==mountain_height+1) {
		if(sets->gib_max_mountain_height() < 320.0 ) {
			sets->setze_max_mountain_height( sets->gib_max_mountain_height() + 10 );
			welt_gui->update_preview();
		}
	}
	else if(komp==mountain_roughness+0) {
		if(sets->gib_map_roughness() > 0.4 ) {
			sets->setze_map_roughness( sets->gib_map_roughness() - 0.05 );
			welt_gui->update_preview();
		}
	}
	else if(komp==mountain_roughness+1) {
#ifndef DOUBLE_GROUNDS
		if(sets->gib_map_roughness() < 0.7 ) {
#else
		if(sets->gib_map_roughness() < 1.00) {
#endif
			sets->setze_map_roughness( sets->gib_map_roughness() + 0.05 );
			welt_gui->update_preview();
		}
	}

	else {
		// all climate borders from here on
		sint16 *climate_borders = (sint16 *)sets->gib_climate_borders();

		if(komp==snowline_winter+0) {
			if(sets->gib_winter_snowline()>0) {
				sets->setze_winter_snowline( sets->gib_winter_snowline()-1 );
			}
		}
		else if(komp==snowline_winter+1) {
			sets->setze_winter_snowline( sets->gib_winter_snowline()+1 );
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
		if(arctic<sets->gib_winter_snowline()) {
			sets->setze_winter_snowline( arctic );
		}
	}
	return true;
}




void climate_gui_t::zeichnen(koord pos, koord gr)
{
	no_tree.pressed=umgebung_t::no_tree;

	no_tree.setze_text( "no tree" );

	gui_frame_t::zeichnen(pos, gr);

	const int x = pos.x+10;
	int y = pos.y+16+16;

	const sint16 water_level = sets->gib_grundwasser()/(Z_TILE_STEP*2)+5;
	const sint16 *climate_borders = sets->gib_climate_borders();

	// water level       18-Nov-01       Markus W. Added
	display_proportional_clip(x, y, translator::translate("Water level"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(water_level,0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Mountain height"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos((int)(sets->gib_max_mountain_height()), 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Map roughness"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos((int)(sets->gib_map_roughness()*20.0 + 0.5)-8 , 0) , ALIGN_RIGHT, COL_WHITE, true);     // x = round(roughness * 10)-4  // 0.6 * 10 - 4 = 2    //29-Nov-01     Markus W. Added
	y += 12+5;

	// season stuff
	display_proportional_clip(x, y, translator::translate("Summer snowline"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(climate_borders[arctic_climate], 0), ALIGN_RIGHT, COL_WHITE, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Winter snowline"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos(sets->gib_winter_snowline(),0),ALIGN_RIGHT, COL_WHITE, true);
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
}
