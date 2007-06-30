/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef HAUSBAUER_H
#define HAUSBAUER_H

#include "../dataobj/koord3d.h"
#include "../simtypes.h"
#include "../tpl/slist_tpl.h"

class gebaeude_t;
class haus_besch_t;
class haus_tile_besch_t;
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
	public:
		/**
		 * Unbekannte Gebäude sind nochmal unterteilt
		 */
		enum utyp
		{
			unbekannt         =  0,
			special           =  1,
			sehenswuerdigkeit =  2,
			denkmal           =  3,
			fabrik            =  4,
			rathaus           =  5,
			weitere           =  6,
			firmensitz        =  7,
			bahnhof           =  8,
			bushalt           =  9,
			ladebucht         = 10,
			hafen             = 11,
			binnenhafen       = 12,
			airport           = 13,
			monorailstop      = 14,
			bahnhof_geb       = 16,
			bushalt_geb       = 17,
			ladebucht_geb     = 18,
			hafen_geb         = 19,
			binnenhafen_geb   = 20,
			airport_geb       = 21,
			monorail_geb      = 22,
			wartehalle        = 30,
			post              = 31,
			lagerhalle        = 32
		};

	private:
		static slist_tpl<const haus_besch_t*> sehenswuerdigkeiten_land;
		static slist_tpl<const haus_besch_t*> sehenswuerdigkeiten_city;
		static slist_tpl<const haus_besch_t*> rathaeuser;
		static slist_tpl<const haus_besch_t*> denkmaeler;
		static slist_tpl<const haus_besch_t*> ungebaute_denkmaeler;
		static slist_tpl<const haus_besch_t*> fabriken;

	public:
		static slist_tpl<const haus_besch_t*> wohnhaeuser;
		static slist_tpl<const haus_besch_t*> gewerbehaeuser;
		static slist_tpl<const haus_besch_t*> industriehaeuser;

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
		static slist_tpl<const haus_besch_t*> station_building;
		static slist_tpl<const haus_besch_t*> air_depot;
		static slist_tpl<const haus_besch_t*> headquarter;

	private:
		/**
		 * Liefert den Eintrag mit passendem Level aus der Liste, falls es ihn gibt.
		 * Falls es ihn nicht gibt, wird ein Gebäude des nächsthöheren vorhandenen
		 * levels geliefert.  Falls es das auch nicht gibt wird das Gebäude mit dem
		 * höchsten level aus der Liste geliefert.
		 * Diese Methode liefert niemals NULL!
		 * @author Hj. Malthaner
		 */
		static const haus_besch_t* gib_aus_liste(slist_tpl<const haus_besch_t*>& liste, int level, uint16, climate cl);

		/**
		 * Liefert einen zufälligen Eintrag aus der Liste.
		 * @author V. Meyer
		 */
		static const haus_besch_t* waehle_aus_liste(slist_tpl<const haus_besch_t*>& liste, uint16 time, climate cl);

		static void insert_sorted(slist_tpl<const haus_besch_t*>& liste, const haus_besch_t* besch);

	public:
		static const haus_besch_t* gib_random_station(const enum utyp utype, const uint16 time, const uint8 enables);

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
		static void fill_menu(werkzeug_parameter_waehler_t* wzw, hausbauer_t::utyp, tool_func werkzeug, const int sound_ok, const int sound_ko,const sint64 cost, const karte_t* welt);

		/**
		 * Gewerbegebäude passend zum Level liefern. Zur Zeit sind die Einträge
		 * eindeutig aufsteigend.
		 * @author V. Meyer
		 */
		static const haus_besch_t* gib_gewerbe(int level, uint16 time, climate cl)
		{
			return gib_aus_liste(gewerbehaeuser, level, time,cl);
		}

		/**
		 * Industriegebäude passend zum Level liefern. Zur Zeit sind die Einträge
		 * eindeutig aufsteigend.
		 * @author V. Meyer
		 */
		static const haus_besch_t* gib_industrie(int level, uint16 time, climate cl)
		{
			return gib_aus_liste(industriehaeuser, level, time, cl);
		}

		/**
		 * Wohnhaus passend zum Level liefern. Zur Zeit sind die Einträge
		 * eindeutig aufsteigend.
		 * @author V. Meyer
		 */
		static const haus_besch_t* gib_wohnhaus(int level, uint16 time, climate cl)
		{
			return gib_aus_liste(wohnhaeuser, level, time, cl);
		}

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
		static const haus_besch_t* gib_special(int bev, enum utyp utype, uint16 time, climate cl);

		static gebaeude_t* baue(karte_t* welt, spieler_t* sp, koord3d pos, int layout, const haus_besch_t* besch, bool clear = true, void* param = NULL);

	/* build all kind of stop, station building, and depots
	 * may change the layout of neighbouring buildings, if layout>4 and station
	 */
		static gebaeude_t* neues_gebaeude(karte_t* welt, spieler_t* sp, koord3d pos, int layout, const haus_besch_t* besch, void* param = NULL);
};

#endif
