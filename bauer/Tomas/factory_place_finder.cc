/*
 * factory_place_finder.cpp
 *
 * Copyright (c) 2003        Tomas Kubes
 *                           (with help of Hansjörg Malthaner, Volker Meyer)
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*

	Factory Place Finder
	**************

  Responsible for all necessary things that are required
  to create nice and flat land for new factory

  !!IT IS NOT MENT TO BE CALLED BY USER AT ALL!!


	External Dependencies
	*********************
  depends on:
  -----------
    factory_place_finder.h (header)

	methods and types concering map information retrival
	methods and types necessary to map mofification

  modifies:
  ---------
    map, searches it, eventually makes small terrrain adjustments :-)

  methods being called outside:
  -----------------------------
    x
*/


#include "../simdebug.h"
#include "../bauer/factorybuilder.h"
#include "../simworld.h"
#include "../simfab.h"
#include "../simdisplay.h"
#include "../simgraph.h"
#include "../simtools.h"
#include "../simcity.h"
#include "../simskin.h"
#include "../simhalt.h"
#include "../simplay.h"

#include "../dings/leitung.h"

#include "../boden/grund.h"
#include "../boden/wege/dock.h"

#include "../dataobj/einstellungen.h"
#include "../dataobj/translator.h"

#include "../sucher/factory_place_finder.h"





factory_place_finder_t::factory_place_finder_t
 (const koord k, const uint8 x, const uint8 y, const karte_t *w, const uint8 p) : world(w), priority(p)
{

	place = k;
	size_x = x;
	size_y = y;
	place_checked_ok = FALSE;

}

bool factory_place_finder_t::Is_Square_Ok (const koord k) const
{
	const grund_t *gr = world->lookup(k)->gib_kartenboden();
	return ((gr->gib_grund_hang() == hang_t::flach) && (gr->ist_natur()));

}



//only basiclally checks if land is flat and free to build
bool factory_place_finder_t::Is_Place_Ok (void)
{
	uint8 x, y;

	for ( x = 0; x < size_x; x++){
		for ( y = 0; y < size_y; y++){

			if (!Is_Square_Ok ( place + koord( x, y))) return FALSE;

		}
	}

	//if we have gotten here, test was passed succesfully
	place_checked_ok = TRUE; //note it for further use
	return TRUE;
}








land_factory_place_finder_t::land_factory_place_finder_t
 (const koord k, const uint8 x, const uint8 y, const karte_t *w, const uint8 p)
  : factory_place_finder_t(k, x, y, w, p)
{
}



bool land_factory_place_finder_t::Check_Place (void)
{

	return factory_place_finder_t::Is_Place_Ok ();

}










city_factory_place_finder_t::city_factory_place_finder_t
 (const koord k, const uint8 x, const uint8 y, const karte_t *w, const uint8 p)
  : land_factory_place_finder_t(k, x, y, w, p)
{
}


bool city_factory_place_finder_t::Check_Place (void)
{
	//if pririty is high, skip searching for nearby road, by seting search result true in advance
	bool road_test = (priority <= (MAX_TRIED_CELLS/2))?FALSE:TRUE;

	if (!road_test)
	{
		//we will check if there is some road nearby (we are near the city)
		for (sint8 i = -ROAD_DISTANCE_CELLS; i < (this->Gib_Size_X()+ROAD_DISTANCE_CELLS); i ++)
		{
			for (sint8 j = -ROAD_DISTANCE_CELLS; j < (this->Gib_Size_Y()+ROAD_DISTANCE_CELLS); j++)
			{
				const koord k = (this -> Gib_Current_Pos()) + koord ( i, j);
				if ( Is_In_Map(k,world) ){
					const grund_t *gr = world->lookup( k ) -> gib_kartenboden();
					//we will test if there is some way here - city exists there
					//(this will not work properly when map will be filled with player ways)
					if ( gr -> hat_wege() == TRUE)
					{
						road_test = TRUE;
						break;
					}

				}

			}

			//if place was checked OK, stop search
			if (road_test) break;
		}

		//now we can test if there was some road in that location found
		//if not we can happily quit searching. This was wrong place
		if (road_test == FALSE) return FALSE;
	}

	//now we can test if there was some road in that location found
	//if not we can happily quit searching. This was wrong place
	//except if priority is high, then we will place factory anywhere in the cell
	if (road_test == FALSE) return FALSE;

	//now we check if we can build on that place
	return land_factory_place_finder_t::Check_Place ();
}




water_factory_place_finder_t::water_factory_place_finder_t
 (const koord k, const uint8 x, const uint8 y, const karte_t *w, const uint8 p)
  : factory_place_finder_t( k, x, y, w, p)
{
}

bool water_factory_place_finder_t::Is_Square_Ok (const koord k) const
{
	const grund_t *gr = world->lookup(k)->gib_kartenboden();
	return gr->ist_wasser();

}




bool water_factory_place_finder_t::Check_Place (void)
{

	return Is_Place_Ok ();
}
