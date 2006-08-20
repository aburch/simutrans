/*
 * hausbauer.h
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef simhausbau_h
#define simhausbau_h

class karte_t;
class tabfileobj_t;
class spieler_t;
class gebaeude_t;

#include "../dataobj/koord3d.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/inthashtable_tpl.h"
#include "../simtypes.h"

class gebaeude_t;
class haus_besch_t;
class haus_tile_besch_t;
class werkzeug_parameter_waehler_t;

/**
 * Diese Klasse übernimmt den Bau von Mehrteiligen Gebäuden.
 * Sie kennt die Beschreibung (fast) aller Gebäude was
 * Typ, Höhe, Größe, Bilder, Animationen angeht.
 * Diese Daten werden aus "gebaeude.tab" gelesen.
 *
 * Für Denkmäler wird eine Liste der ungebauten geführt.
 *
 * @author Hj. Malthaner/V. Meyer
 * @version $Revision: 1.3 $
 */
class hausbauer_t
{
    static stringhashtable_tpl<const haus_besch_t *> besch_names;
public:
    /**
     * Unbekannte Gebäude sind nochmal unterteilt
     */
    enum utyp { unbekannt=0, special=1, sehenswuerdigkeit=2, denkmal=3, fabrik=4, rathaus=5, weitere=6, firmensitz=7,
    					 bahnhof=8, bushalt=9, ladebucht=10, hafen=11, binnenhafen=12, airport=13,
    					 bahnhof_geb=16, bushalt_geb=17, ladebucht_geb=18, hafen_geb=19, binnenhafen_geb=20, airport_geb=21,
    					 wartehalle=30, post=31, lagerhalle=32 };

private:
    static slist_tpl<const haus_besch_t *> alle;
    static slist_tpl<const haus_besch_t *> sehenswuerdigkeiten;
    static slist_tpl<const haus_besch_t *> spezials;
    static slist_tpl<const haus_besch_t *> denkmaeler;
    static slist_tpl<const haus_besch_t *> ungebaute_denkmaeler;
    static slist_tpl<const haus_besch_t *> fabriken;

public:
    static slist_tpl<const haus_besch_t *> wohnhaeuser;
    static slist_tpl<const haus_besch_t *> gewerbehaeuser;
    static slist_tpl<const haus_besch_t *> industriehaeuser;

    /**
     * Gebäude, die das Programm direkt kennen muß
     */
    static const haus_besch_t *frachthof_besch;
    static const haus_besch_t *bahn_depot_besch;
    static const haus_besch_t *tram_depot_besch;
    static const haus_besch_t *str_depot_besch;
    static const haus_besch_t *sch_depot_besch;
    static const haus_besch_t *monorail_depot_besch;
    static const haus_besch_t *monorail_foundation_besch;

    // to allow for an arbitary number, we use lists
    static slist_tpl<const haus_besch_t *> hausbauer_t::station_building;
    static slist_tpl<const haus_besch_t *> hausbauer_t::air_depot;
    static slist_tpl<const haus_besch_t *> hausbauer_t::headquarter;

private:
    /**
     * Liefert den Eintrag mit passendem Level aus der Liste,
     * falls es ihn gibt.
     * Falls es ihn nicht gibt wird ein gebäude des nächstöheren vorhandenen
     * levels geliefert.
     * Falls es das auch nicht gibt wird das Gebäude mit dem höchsten
     * level aus der Liste geliefert.
     *
     * Diese Methode liefert niemals NULL!
     *
     * @author Hj. Malthaner
     */
    static const haus_besch_t * gib_aus_liste(slist_tpl<const haus_besch_t *> &liste, int level, uint16);

    /**
     * Liefert einen zufälligen Eintrag aus der Liste.
     * @author V. Meyer
     */
    static const haus_besch_t *waehle_aus_liste(slist_tpl<const haus_besch_t *> &liste,uint16 time);

    /**
     * Sucht einen Eintrag aus einer Liste passend zum Namen ("Name=..." aus gebaeude.tab).
     * @author V. Meyer
     */
    static const haus_besch_t *finde_in_liste(slist_tpl<const haus_besch_t *>  &liste, utyp utype, const char *name);

    /**
     * Sucht eine Eintrag aus der Spezialgebäudeliste mit der passenden Bevölkerung.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_special_intern(int bev, enum utyp utype, uint16 time);

    static void insert_sorted(slist_tpl<const haus_besch_t *> &liste, const haus_besch_t *besch);

public:
    static const haus_besch_t *gib_random_station(const enum utyp utype,const uint16 time,const uint8 enables);

    static const haus_tile_besch_t *find_tile(const char *name, int idx);

    static bool register_besch(const haus_besch_t *besch);
    static bool alles_geladen();

    /* Fill menu with icons of given stops from the list
     * @author prissi
     */
    static void fill_menu(werkzeug_parameter_waehler_t *wzw,
    		slist_tpl<const haus_besch_t *>&stops,
    		int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
    		const int sound_ok, const int sound_ko,const int cost, const uint16 time);

    /* Fill menu with icons of given stops of a given type
     * @author prissi
     */
    static void fill_menu(werkzeug_parameter_waehler_t *wzw,
    		hausbauer_t::utyp,
    		int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
    		const int sound_ok, const int sound_ko,const int cost, const uint16 time);

    /**
     * Sucht ein Gebäude, welches bei der gegebenen Bevölkerungszahl gebaut
     * werden soll.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_special(int bev,uint16 time=0) { return gib_special_intern(bev, special,time);  };

    /**
     * Sucht ein Rathaus, welches bei der gegebenen Bevölkerungszahl gebaut
     * werden soll.
     * @author V. Meyer
     */
   static const haus_besch_t *gib_rathaus(int bev, uint16 time) { return gib_special_intern(bev, rathaus, time);  }

    /**
     * Gewerbegebäude passend zum Level liefern. Zur Zeit sind die Einträge
     * eindeutig aufsteigend.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_gewerbe(int level, uint16 time) { return gib_aus_liste(gewerbehaeuser, level, time); };

    /**
     * Industriegebäude passend zum Level liefern. Zur Zeit sind die Einträge
     * eindeutig aufsteigend.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_industrie(int level,uint16 time) { return gib_aus_liste(industriehaeuser, level,time); };

    /**
     * Wohnhaus passend zum Level liefern. Zur Zeit sind die Einträge
     * eindeutig aufsteigend.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_wohnhaus(int level,uint16 time) { return gib_aus_liste(wohnhaeuser, level,time); };

    /**
     * Fabrikbeschreibung anhand des Namens raussuchen.
     * eindeutig aufsteigend.
     * @author V. Meyer
     */
    static const haus_besch_t *finde_fabrik(const char *name) { return finde_in_liste(fabriken, fabrik, name); };

    /**
     * Denkmal anhand des Namens raussuchen.
     * @author V. Meyer
     */
    static const haus_besch_t *finde_denkmal(const char *name) { return finde_in_liste(denkmaeler, denkmal, name ); };

    /**
     * Liefert per Zufall die Beschreibung eines Sehenswuerdigkeit,
     * die bei Kartenerstellung gebaut werden kann.
     * @author V. Meyer
     */
    static const haus_besch_t *waehle_sehenswuerdigkeit(uint16 time) { return waehle_aus_liste(sehenswuerdigkeiten,time); };

    /**
     * Liefert per Zufall die Beschreibung eines ungebauten Denkmals.
     * @author V. Meyer
     */
    static const haus_besch_t *waehle_denkmal(uint16 time=0) { return waehle_aus_liste(ungebaute_denkmaeler,time); };

    /**
     * Diese Funktion setzt intern alle Denkmäler auf ungebaut. Damit wir
     * keines doppelt bauen.
     * @author V. Meyer
     */
    static void neue_karte();

    /**
     * Dem Hausbauer Bescheid sagen, dass ein bestimmtes Denkmal gebaut wurde.
     * @author V. Meyer
     */
    static void denkmal_gebaut(const haus_besch_t *besch) { ungebaute_denkmaeler.remove(besch); };


    /*
     * Baut Häuser um!
     * @author V. Meyer
     */
    static void umbauen(karte_t *welt,
                        gebaeude_t *gb,
			const haus_besch_t *besch);

    /**
     * Baut alles was in gebaeude.tab beschrieben ist.
     * Es werden immer gebaeude_t-Objekte erzeugt.
     * @author V. Meyer
     */
    static void baue(karte_t *welt,
		     spieler_t *sp,
		     koord3d pos,
		     int layout,
		     const haus_besch_t *besch,
		     bool clear = true,
		     void *param = NULL);


    static gebaeude_t *neues_gebaeude( karte_t *welt,
			    spieler_t *sp,
			    koord3d pos,
			    int layout,
			    const haus_besch_t *besch,
			    void *param = NULL);
};

#endif
