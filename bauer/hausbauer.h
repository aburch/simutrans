/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef HAUSBAUER_H
#define HAUSBAUER_H

#include "../besch/haus_besch.h"
#include "../dataobj/koord3d.h"
#include "../simtypes.h"
#include "../tpl/vector_tpl.h"

class gebaeude_t;
class karte_t;
class spieler_t;
class werkzeug_waehler_t;

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
	static vector_tpl<const haus_besch_t*> sehenswuerdigkeiten_land;
	static vector_tpl<const haus_besch_t*> sehenswuerdigkeiten_city;
	static vector_tpl<const haus_besch_t*> rathaeuser;
	static vector_tpl<const haus_besch_t*> denkmaeler;
	static vector_tpl<const haus_besch_t*> ungebaute_denkmaeler;

public:
	/**
	 * Gebäude, die das Programm direkt kennen muß
	 */
	static const haus_besch_t* elevated_foundation_besch;

	// to allow for an arbitary number, we use lists
	static vector_tpl<const haus_besch_t*> station_building;
private:
	static vector_tpl<const haus_besch_t*> headquarter;

	/**
	 * Liefert einen zufälligen Eintrag aus der Liste.
	 * @author V. Meyer
	 */
	static const haus_besch_t* waehle_aus_liste(vector_tpl<const haus_besch_t*>& liste, uint16 time, bool ignore_retire, climate cl);

public:
	/**
	 * Finds a station building, which enables pas/mail/goods for the AI.
	 * If time==0 the timeline will be ignored.
	 * Returns station that can be built above ground.
	 */
	static const haus_besch_t* get_random_station(const haus_besch_t::utyp utype, const waytype_t wt, const uint16 time, const uint8 enables);

	static const haus_tile_besch_t* find_tile(const char* name, int idx);

	static bool register_besch(haus_besch_t *besch);
	static bool alles_geladen();

	/* Fill menu with icons of buildings of a given type
	 * this is only needed for stations and depots => use waytype too!
	 * @author prissi
	 */
	static void fill_menu(werkzeug_waehler_t* wzw, haus_besch_t::utyp, waytype_t wt, sint16 sound_ok, const karte_t* welt);

	/**
	 * Gewerbegebäude passend zum Level liefern. Zur Zeit sind die Einträge
	 * eindeutig aufsteigend.
	 * @author V. Meyer
	 */
	static const haus_besch_t* get_gewerbe(int level, uint16 time, climate cl);

	/**
	 * Industriegebäude passend zum Level liefern. Zur Zeit sind die Einträge
	 * eindeutig aufsteigend.
	 * @author V. Meyer
	 */
	static const haus_besch_t* get_industrie(int level, uint16 time, climate cl);

	/**
	 * Wohnhaus passend zum Level liefern. Zur Zeit sind die Einträge
	 * eindeutig aufsteigend.
	 * @author V. Meyer
	 */
	static const haus_besch_t* get_wohnhaus(int level, uint16 time, climate cl);

	/**
	 * Returns Headquarter with Level level
	 * (takes the first matching one)
	 * @author Dwachs
	 */
	static const haus_besch_t* get_headquarter(int level, uint16 time);

	/**
	 * Liefert per Zufall die Beschreibung eines Sehenswuerdigkeit,
	 * die bei Kartenerstellung gebaut werden kann.
	 * @author V. Meyer
	 */
	static const haus_besch_t* waehle_sehenswuerdigkeit(uint16 time, bool ignore_retire, climate cl)
	{
		return waehle_aus_liste(sehenswuerdigkeiten_land, time, ignore_retire, cl);
	}

	/**
	 * Liefert per Zufall die Beschreibung eines ungebauten Denkmals.
	 * @author V. Meyer
	 */
	static const haus_besch_t* waehle_denkmal(uint16 time = 0)
	{
		return waehle_aus_liste(ungebaute_denkmaeler, time, false, MAX_CLIMATES);
	}

	/**
	 * Teilt dem Hausbauer mit, dass eine neue Karte geladen oder generiert wird.
	 * In diesem Fall müssen wir die Liste der ungebauten Denkmäler wieder füllen.
	 * @author V. Meyer
	 */
	static void neue_karte();

	/**
	 * True, if this is still valid ...
	 * @author V. Meyer
	 */
	static bool is_valid_denkmal(const haus_besch_t* besch) { return ungebaute_denkmaeler.is_contained(besch); }

	/**
	 * Dem Hausbauer Bescheid sagen, dass ein bestimmtes Denkmal gebaut wurde.
	 * @author V. Meyer
	 */
	static void denkmal_gebaut(const haus_besch_t* besch) { ungebaute_denkmaeler.remove(besch); }

	/**
	 * Called for a city attraction or a townhall with a certain number of inhabitants (bev).
	 */
	static const haus_besch_t* get_special(uint32 bev, haus_besch_t::utyp utype, uint16 time, bool ignore_retire, climate cl);

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
	static gebaeude_t* baue(karte_t* welt, spieler_t* sp, koord3d pos, int layout, const haus_besch_t* besch, void* param = NULL);

	/* build all kind of stops and depots
	 * The building size must be 1x1
	 * may change the layout of neighbouring buildings, if layout>4 and station
	 */
	static gebaeude_t* neues_gebaeude(karte_t* welt, spieler_t* sp, koord3d pos, int layout, const haus_besch_t* besch, void* param = NULL);

	// currently only used for edit menu
	static const vector_tpl<const haus_besch_t *> *get_list( haus_besch_t::utyp typ );
	static const vector_tpl<const haus_besch_t *> *get_citybuilding_list( gebaeude_t::typ typ );

};

#endif
