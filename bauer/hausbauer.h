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

class gebaeude_t;
class haus_besch_t;
class haus_tile_besch_t;

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
    enum utyp { unbekannt, special, sehenswuerdigkeit, denkmal, fabrik, rathaus, weitere };

private:
    static slist_tpl<const haus_besch_t *> alle;
    static slist_tpl<const haus_besch_t *> sehenswuerdigkeiten;
    static slist_tpl<const haus_besch_t *> spezials;
    static slist_tpl<const haus_besch_t *> fabriken;
    static slist_tpl<const haus_besch_t *> denkmaeler;
    static slist_tpl<const haus_besch_t *> ungebaute_denkmaeler;
    //static slist_tpl<const haus_besch_t *> hausbauer_t::train_stops;

public:

    static slist_tpl<const haus_besch_t *> wohnhaeuser;
    static slist_tpl<const haus_besch_t *> gewerbehaeuser;
    static slist_tpl<const haus_besch_t *> industriehaeuser;


    /**
     * Gebäude, die das Programm direkt kennen muß
     */
    static const haus_besch_t *bahnhof_besch;
    static const haus_besch_t *gueterbahnhof_besch;
    static const haus_besch_t *frachthof_besch;
    static const haus_besch_t *bushalt_besch;
    static const haus_besch_t *dock_besch;
    static const haus_besch_t *bahn_depot_besch;
    static const haus_besch_t *str_depot_besch;
    static const haus_besch_t *sch_depot_besch;
    static const haus_besch_t *post_besch;
    static const haus_besch_t *muehle_besch;

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
    static const haus_besch_t * gib_aus_liste(slist_tpl<const haus_besch_t *> &liste, int level);


    /**
     * Liefert einen zufälligen Eintrag aus der Liste.
     * @author V. Meyer
     */
    static const haus_besch_t *waehle_aus_liste(slist_tpl<const haus_besch_t *> &liste);

    /**
     * Sucht einen Eintrag aus einer Liste passend zum Namen ("Name=..." aus gebaeude.tab).
     * @author V. Meyer
     */
    static const haus_besch_t *finde_in_liste(slist_tpl<const haus_besch_t *>  &liste, utyp utype, const char *name);

    /**
     * Sucht eine Eintrag aus der Spezialgebäudeliste mit der passenden Bevölkerung.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_special_intern(int bev, enum utyp utype);

    static void insert_sorted(slist_tpl<const haus_besch_t *> &liste, const haus_besch_t *besch);
public:

    static const haus_tile_besch_t *find_tile(const char *name, int idx);

    static bool register_besch(const haus_besch_t *besch);
    static bool alles_geladen();
    /**
     * Sucht ein Gebäude, welches bei der gegebenen Bevölkerungszahl gebaut
     * werden soll.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_special(int bev)
    { return gib_special_intern(bev, special);  }

    /**
     * Sucht ein Rathaus, welches bei der gegebenen Bevölkerungszahl gebaut
     * werden soll.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_rathaus(int bev)
    { return gib_special_intern(bev, rathaus);  }

    /**
     * Gewerbegebäude passend zum Level liefern. Zur Zeit sind die Einträge
     * eindeutig aufsteigend.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_gewerbe(int level)
    { return gib_aus_liste(gewerbehaeuser, level); }

    /**
     * Industriegebäude passend zum Level liefern. Zur Zeit sind die Einträge
     * eindeutig aufsteigend.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_industrie(int level)
    { return gib_aus_liste(industriehaeuser, level); }

    /**
     * Wohnhaus passend zum Level liefern. Zur Zeit sind die Einträge
     * eindeutig aufsteigend.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_wohnhaus(int level)
    { return gib_aus_liste(wohnhaeuser, level); }

    /**
     * Fabrikbeschreibung anhand des Namens raussuchen.
     * eindeutig aufsteigend.
     * @author V. Meyer
     */
    static const haus_besch_t *finde_fabrik(const char *name)
    { return finde_in_liste(fabriken, fabrik, name); }

    /**
     * Denkmal anhand des Namens raussuchen.
     * @author V. Meyer
     */
    static const haus_besch_t *finde_denkmal(const char *name)
    { return finde_in_liste(denkmaeler, denkmal, name ); }

    /**
     * Liefert per Zufall die Beschreibung eines Sehenswuerdigkeit,
     * die bei Kartenerstellung gebaut werden kann.
     * @author V. Meyer
     */
    static const haus_besch_t *waehle_sehenswuerdigkeit()
    { return waehle_aus_liste(sehenswuerdigkeiten); }

    /**
     * Liefert per Zufall die Beschreibung eines ungebauten Denkmals.
     * @author V. Meyer
     */
    static const haus_besch_t *waehle_denkmal()
    { return waehle_aus_liste(ungebaute_denkmaeler); }

    /**
     * Diese Funktion setzt intern alle Denkmäler auf ungebaut. Damit wir
     * keines doppelt bauen.
     * @author V. Meyer
     */
    static void neue_karte();

    /**
     * Dem Hausbauer Bescheid sagen, dasß ein bestimmtes Denkmal gebaut wurde.
     * @author V. Meyer
     */
    static void denkmal_gebaut(const haus_besch_t *besch)
    {    ungebaute_denkmaeler.remove(besch); }


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
