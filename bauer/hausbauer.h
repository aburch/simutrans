/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef HAUSBAUER_H
#define HAUSBAUER_H

#include "../descriptor/building_desc.h"
#include "../dataobj/koord3d.h"
#include "../simtypes.h"
#include "../tpl/vector_tpl.h"

class gebaeude_t;
class karte_ptr_t;
class player_t;
class tool_selector_t;

/**
 * Diese Klasse übernimmt den Bau von mehrteiligen Gebäuden. Sie kennt die
 * Description (fast) aller Gebäude was Typ, Höhe, Größe, Bilder, Animationen
 * angeht. Diese Daten werden aus "gebaeude.tab" gelesen. Für Denkmäler wird
 * eine Liste der ungebauten geführt.
 * @author Hj. Malthaner/V. Meyer
 */
class hausbauer_t
{

private:
	static vector_tpl<const building_desc_t*> attractions_land; // Land attractions
	static vector_tpl<const building_desc_t*> attractions_city; // City attractions
	static vector_tpl<const building_desc_t*> townhalls; // Town halls
	static vector_tpl<const building_desc_t*> monuments; // Monuments
	static vector_tpl<const building_desc_t*> unbuilt_monuments; // Unbuilt monuments

	static karte_ptr_t welt;
public:
	/**
	 * Gebäude, die das Programm direkt kennen muß
	 */
	static const building_desc_t* elevated_foundation_desc;

	// to allow for an arbitrary number, we use lists
	static vector_tpl<const building_desc_t*> station_building;
	static vector_tpl<building_desc_t*> modifiable_station_buildings;

private:

	static vector_tpl<const building_desc_t*> headquarters;

	/**
	 * Liefert einen zufälligen Eintrag aus der Liste.
	 * @author V. Meyer
	 */
	static const building_desc_t* get_random_desc(vector_tpl<const building_desc_t*>& list, uint16 time, bool ignore_retire, climate cl);

public:
	/**
	 * Finds a station building, which enables pas/mail/goods for the AI.
	 * If time==0 the timeline will be ignored.
	 * Returns station that can be built above ground.
	 */
	static const building_desc_t* get_random_station(const building_desc_t::btype utype, const waytype_t wt, const uint16 time, const uint16 enables);

	static const building_tile_desc_t* find_tile(const char* name, int idx);

	static const building_desc_t* get_desc(const char *name);

	static bool register_desc(building_desc_t *desc);
	static bool successfully_loaded();

	/* Fill menu with icons of buildings of a given type
	 * this is only needed for stations and depots => use waytype too!
	 * @author prissi
	 */
	static void fill_menu(tool_selector_t* tool_selector, building_desc_t::btype, waytype_t wt, sint16 sound_ok);

	/**
	 * Gewerbegebäude passend zum Level liefern. Zur Zeit sind die Einträge
	 * eindeutig aufsteigend.
	 * @author V. Meyer
	 */
	static const building_desc_t* get_commercial(int level, uint16 time, climate cl, bool allow_earlier = false, uint32 clusters = 0l);

	/**
	 * Industriegebäude passend zum Level liefern. Zur Zeit sind die Einträge
	 * eindeutig aufsteigend.
	 * @author V. Meyer
	 */
	static const building_desc_t* get_industrial(int level, uint16 time, climate cl, bool allow_earlier = false, uint32 clusters = 0l);

	/**
	 * Wohnhaus passend zum Level liefern. Zur Zeit sind die Einträge
	 * eindeutig aufsteigend.
	 * @author V. Meyer
	 */
	static const building_desc_t* get_residential(int level, uint16 time, climate cl, bool allow_earlier = false, uint32 clusters = 0l);

	/**
	 * Returns Headquarters with Level level
	 * (takes the first matching one)
	 * @author Dwachs
	 */
	static const building_desc_t* get_headquarter(int level, uint16 time);

	/**
	 * Liefert per Zufall die Description eines Sehenswuerdigkeit,
	 * die bei Kartenerstellung gebaut werden kann.
	 * @author V. Meyer
	 */
	static const building_desc_t* get_random_attraction(uint16 time, bool ignore_retire, climate cl)
	{
		return get_random_desc(attractions_land, time, ignore_retire, cl);
	}

	/**
	 * Liefert per Zufall die Description eines ungebauten Denkmals.
	 * @author V. Meyer
	 */
	static const building_desc_t* get_random_monument(uint16 time = 0)
	{
		return get_random_desc(unbuilt_monuments, time, false, MAX_CLIMATES);
	}

	/**
	 * Teilt dem Hausbauer mit, dass eine neue Karte geladen oder generiert wird.
	 * In diesem Fall müssen wir die Liste der ungebauten Denkmäler wieder füllen.
	 * @author V. Meyer
	 */
	static void new_world();

	/**
	 * True, if this is still valid ...
	 * @author V. Meyer
	 */
	static bool is_valid_monument(const building_desc_t* desc) { return unbuilt_monuments.is_contained(desc); }

	/**
	 * Dem Hausbauer Bescheid sagen, dass ein bestimmtes Denkmal gebaut wurde.
	 * @author V. Meyer
	 */
	static void monument_erected(const building_desc_t* desc) { unbuilt_monuments.remove(desc); }

	/**
	 * Called for a city attraction or a townhall with a certain number of inhabitants (bev).
	 */
	static const building_desc_t* get_special(uint32 bev, building_desc_t::btype btype, uint16 time, bool ignore_retire, climate cl);

	/* use this to remove an arbitrary building
	 * it will also take care of factories and foundations
	 */
	static void remove( player_t *player, gebaeude_t *gb, bool map_generation );

	/* Main function for all non-traffic buildings, including factories and signalboxes
	 * building size can be larger than 1x1
	 * Also the underlying ground will be changed to foundation.
	 * @return The first built part of the building. Usually at pos, if this
	 *         part is not empty.
	 * @author V. Meyer
	 */
	static gebaeude_t* build(player_t* player, koord3d pos, int layout, const building_desc_t* desc, void* param = NULL);

	/* build all kind of stops and depots
	 * The building size must be 1x1
	 * may change the layout of neighbouring buildings, if layout>4 and station
	 */
	static gebaeude_t* build_station_extension_depot(player_t* player, koord3d pos, int layout, const building_desc_t* desc, void* param = NULL);

	/// @returns house list of type @p typ
	static const vector_tpl<const building_desc_t *> *get_list(building_desc_t::btype typ);

	/// @returns city building list of type @p typ (res/com/ind)
	static const vector_tpl<const building_desc_t *> *get_citybuilding_list(building_desc_t::btype typ);

	static void new_month();
};

#endif
