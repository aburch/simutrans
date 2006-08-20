/*
 * factory_place_finder.h
 *
 * Copyright (c) 1997 - 2003 Tomas Kubes
 *                           Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*

  For dependencies see: factory_plce_finder.cpp

*/

#ifndef factory_place_finder_h
#define factory_place_finder_h

#include "../tpl/vector_tpl.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/array_tpl.h"
#include "../dataobj/koord3d.h"
#include "../simtypes.h"

//this is here to maintain consistency of "enum platzierung", there can be tmep definition here, but then it will be hell to track changes which will cause finder to malfunction
//#include "../dataobj/fabesch.h"


//temporary definitions
class karte_t;
class spieler_t;
class fabrik_t;


//constant definition, it is used both in place finder and factory builder
//be careful, this value is also used with priority
//settign it too low might cause that many factories wont be placed
#define MAX_TRIED_CELLS 6
//this is used in city placefinder
#define ROAD_DISTANCE_CELLS 2


class factory_place_finder_t
{
	koord place;
	uint8 size_x, size_y;


	factory_place_finder_t(); //disabled default constructor

   protected:
	const karte_t *world;  //some functions in childeren need to acces it, but it is read only, so it can be accessed freely without any harm
	bool place_checked_ok;

	//priority is used to determine if we can stick to a bad place because many tries to find good one already failed
	//its range is from 0 to MAX_TRIED_CELLS
	const uint8 priority;

	factory_place_finder_t (const koord k, const uint8 x, const uint8 y, const karte_t *w, const uint8 p);


	virtual bool Is_Square_Ok (const koord k) const;
	bool Is_Place_Ok (void); //this function is purely responsible for checking, that area is flat and clear an in map - same in all objects


   public:

	enum finder_types {WATER, LAND, CITY};
	  //  enum platzierung {Land, Wasser, Stadt};


	//help function to test if koordinates are in buildable map boundaries
	inline static bool Is_In_Map (const koord k, const karte_t *w) {return ( (k.x > 0) && (k.y > 0) && (k.x < w->gib_groesse()) && ( k.y < w->gib_groesse()) );}

/*	//this will determine finder type that will be used for the factory
	inline static finder_types Determine_Finder_Type (fabesch_t::platzierung factory_place_type)


//	inline static factory_place_finder_t::finder_types factory_place_finder_t::Determine_Finder_Type (const enum factory_place_type)
{

//	enum finder_types {WATER, LAND, CITY};
	  //  enum platzierung {Land, Wasser, Stadt};

	switch (factory_place_type)
	{
		case (fabesch_t::Wasser): return WATER;
		case (fabesch_t::Stadt): return CITY;

	}
	return LAND;
}*/

	//returns value of place
	inline koord Gib_Current_Pos (void) const {return place;}

	//return values of size
	inline uint8 Gib_Size_X (void) const { return size_x;}
	inline uint8 Gib_Size_Y (void) const { return size_y;}


	inline koord Get_Place (void) const {return (place_checked_ok ? place : (koord::invalid));} //returns found coordinates, or invalid if not found
	virtual bool Check_Place (void) = 0; //responsible for the ground checking and corecting procedure

};







class land_factory_place_finder_t : public factory_place_finder_t
{

public:

	land_factory_place_finder_t (const koord k, const uint8 x, const uint8 y, const karte_t *w, const uint8 p);
	virtual bool Check_Place (void);

};





class city_factory_place_finder_t : public land_factory_place_finder_t
{


public:

	city_factory_place_finder_t (const koord k, const uint8 x, const uint8 y, const karte_t *w, const uint8 p);
	virtual bool Check_Place (void);


};





class water_factory_place_finder_t : public factory_place_finder_t
{

	virtual bool Is_Square_Ok (const koord k) const;

public:

	water_factory_place_finder_t (const koord k, const uint8 x, const uint8 y, const karte_t *w, const uint8 p);

	virtual bool Check_Place (void);

};























#endif factory_place_finder_h
