/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef HAUSBAUER_H
#define HAUSBAUER_H

#include "../besch/haus_besch.h"
#include "../dataobj/koord3d.h"
#include "../simtypes.h"
#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"

class gebaeude_t;
class karte_t;
class spieler_t;
class werkzeug_parameter_waehler_t;

/**
 * Diese Klasse übernimmt den Bau von mehrteiligen Gebäuden. Sie kennt die
 * Beschreibung (fast) aller Gebäude was Typ, Höhe, Größe, Bilder, Animationen
 * angeht. Diese Daten werden aus "gebaeude.tab" gelesen. Für Denkmäler wird
 * eine Liste der ungebauten geführt.
 * @author Hj. Malthaner/V. Meyer
 */
class hausbauer_t
{

	private:
		static slist_tpl<const haus_besch_t*> sehenswuerdigkeiten_land;
		static slist_tpl<const haus_besch_t*> sehenswuerdigkeiten_city;
		static slist_tpl<const haus_besch_t*> rathaeuser;
		static slist_tpl<const haus_besch_t*> denkmaeler;
		static slist_tpl<const haus_besch_t*> ungebaute_denkmaeler;

	public:
		/**
		 * Gebäude, die das Programm direkt kennen muß
		 */
		static const haus_besch_t* bahn_depot_besch;
		static const haus_besch_t* tram_depot_besch;
		static const haus_besch_t* str_depot_besch;
		static const haus_besch_t* sch_depot_besch;
		static const haus_besch_t* monorail_depot_besch;
		static const haus_besch_t* monorail_foundation_besch;

		// to allow for an arbitary number, we use lists
		static vector_tpl<const haus_besch_t*> station_building;
		static vector_tpl<const haus_besch_t*> headquarter;
		static slist_tpl<const haus_besch_t*> air_depot;

	private:
		/**
		 * Liefert einen zufälligen Eintrag aus der Liste.
		 * @author V. Meyer
		 */
		static const haus_besch_t* waehle_aus_liste(slist_tpl<const haus_besch_t*>& liste, uint16 time, climate cl);

	public:
		/* finds a station building, which enables pas/mail/goods for the AI
		 * for time==0 the timeline will be ignored
		 */
		static const haus_besch_t* gib_random_station(const haus_besch_t::utyp utype, const uint16 time, const uint8 enables);

		static const haus_tile_besch_t* find_tile(const char* name, int idx);

		static bool register_besch(const haus_besch_t *besch);
		static bool alles_geladen();

		typedef int (*tool_func)(spieler_t*, karte_t*, koord, value_t);

		/* Fill menu with icons of buildings from the list
		 * @author prissi
		 */
		static void fill_menu(werkzeug_parameter_waehler_t* wzw, slist_tpl<const haus_besch_t*>& stops, tool_func werkzeug, const int sound_ok, const int sound_ko, const sint64 cost, const karte_t* welt);

		/* Fill menu with icons of buildings of a given type
		 * @author prissi
		 */
		static void fill_menu(werkzeug_parameter_waehler_t* wzw, haus_besch_t::utyp, tool_func werkzeug, const int sound_ok, const int sound_ko,const sint64 cost, const karte_t* welt);

		/**
		 * Gewerbegebäude passend zum Level liefern. Zur Zeit sind die Einträge
		 * eindeutig aufsteigend.
		 * @author V. Meyer
		 */
		static const haus_besch_t* gib_gewerbe(int level, uint16 time, climate cl);

		/**
		 * Industriegebäude passend zum Level liefern. Zur Zeit sind die Einträge
		 * eindeutig aufsteigend.
		 * @author V. Meyer
		 */
		static const haus_besch_t* gib_industrie(int level, uint16 time, climate cl);

		/**
		 * Wohnhaus passend zum Level liefern. Zur Zeit sind die Einträge
		 * eindeutig aufsteigend.
		 * @author V. Meyer
		 */
		static const haus_besch_t* gib_wohnhaus(int level, uint16 time, climate cl);

		/**
		 * Liefert per Zufall die Beschreibung eines Sehenswuerdigkeit,
		 * die bei Kartenerstellung gebaut werden kann.
		 * @author V. Meyer
		 */
		static const haus_besch_t* waehle_sehenswuerdigkeit(uint16 time, climate cl)
		{
			return waehle_aus_liste(sehenswuerdigkeiten_land, time, cl);
		}

		/**
		 * Liefert per Zufall die Beschreibung eines ungebauten Denkmals.
		 * @author V. Meyer
		 */
		static const haus_besch_t* waehle_denkmal(uint16 time = 0)
		{
			return waehle_aus_liste(ungebaute_denkmaeler, time, MAX_CLIMATES);
		}

		/**
		 * Teilt dem Hausbauer mit, dass eine neue Karte geladen oder generiert wird.
		 * In diesem Fall müssen wir die Liste der ungebauten Denkmäler wieder füllen.
		 * @author V. Meyer
		 */
		static void neue_karte();

		/**
		 * Dem Hausbauer Bescheid sagen, dass ein bestimmtes Denkmal gebaut wurde.
		 * @author V. Meyer
		 */
		static void denkmal_gebaut(const haus_besch_t* besch) { ungebaute_denkmaeler.remove(besch); }

		/* called for an attraction or a townhall with a certain number of inhabitants (bev)
		 * bev==-1 will search for an attraction outside of cities.
		 */
		static const haus_besch_t* gib_special(int bev, haus_besch_t::utyp utype, uint16 time, climate cl);

		/* use this to remove an arbitary building
		 * it will also take care of factories and foundations
		 */
		static void remove( karte_t *welt, spieler_t *sp, gebaeude_t *gb );

		/* Main function for all non-traffic buildings, including factories
		 * building size can be larger than 1x1
		 * Also the underlying ground will be changed to foundation.
		 * @return The first built part of the building. Usually at pos, if this
		 *         part is not empty.
		 * @author V. Meyer
		 */
		static gebaeude_t* baue(karte_t* welt, spieler_t* sp, koord3d pos, int layout, const haus_besch_t* besch, bool clear = true, void* param = NULL);

		/* build all kind of stops and depots
		 * The building size must be 1x1
		 * may change the layout of neighbouring buildings, if layout>4 and station
		 */
		static gebaeude_t* neues_gebaeude(karte_t* welt, spieler_t* sp, koord3d pos, int layout, const haus_besch_t* besch, void* param = NULL);
};

#endif
