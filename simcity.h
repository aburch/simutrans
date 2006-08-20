/*
 * simcity.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simcity_h
#define simcity_h

#include "simdings.h"
#include "dings/gebaeude.h"

#include "tpl/slist_tpl.h"
#include "tpl/vector_tpl.h"
#include "tpl/array_tpl.h"
#include "tpl/array2d_tpl.h"

class karte_t;
class spieler_t;
class haltestelle_t;
class cstring_t;
class cbuffer_t;
class stadt_info_t;

struct hausbesch;


#define MAX_CITY_HISTORY_YEARS  12 // number of years to keep history
#define MAX_CITY_HISTORY_MONTHS  12 // number of months to keep history
#define MAX_CITY_HISTORY      4 // Total number of items in array

#define HIST_CITICENS 0	// total people
#define HIST_GROWTH 1 // growth (just for convenience)
#define HIST_TRANSPORTED 2
#define HIST_GENERATED 3





/**
 * Die Objecte der Klasse stadt_t bilden die Staedte in Simu. Sie
 * wachsen automatisch.
 * @version 0.7
 * @author Hj. Malthaner
 */

class stadt_t {

    /**
     * best_t:
     *
     * Kleine Hilfsklasse - speichert die beste Bewertung einer Position.
     *
     * @author V. Meyer
     */
    class best_t {
	int best_wert;
	koord best_pos;
    public:
	void reset(koord pos) { best_wert = 0; best_pos = pos; }

	void check(koord pos, int wert) {
	    if(wert > best_wert) {
		best_wert = wert;
		best_pos = pos;
	    }
	}
	bool found() const { return best_wert > 0; }

	koord gib_pos() const { return best_pos;}
	// int gib_wert() const { return best_wert; }
    };


private:

    /**
     * Ein Step alle step_intervall/1000 Sekunden.
     * @author Hj. Malthaner
     */
    static const int step_interval;

public:

    /**
     * Reads city configuration data
     * @author Hj. Malthaner
     */
    static bool init();

private:
    karte_t *welt;
    spieler_t *besitzer_p;
    char name[64];

    array2d_tpl<unsigned char> pax_ziele_alt;
    array2d_tpl<unsigned char> pax_ziele_neu;

    koord pos;			// Gruendungsplanquadrat der Stadt
    int ob, un, li, re;		// aktuelle Ausdehnung

    // das wird bei jedem laden wieder von 0 angefangen
    // step wird nicht gespeichert!!!
    int step_count;


    /**
     * Echtzeitpunkt für nächsten step.
     * @author Hj. Malthaner
     */
    unsigned long next_step;


    // attribute fuer die Bevoelkerung

    int bev;	// Bevoelkerung gesamt
    int arb;	// davon mit Arbeit
    int won;	// davon mit Wohnung


    /**
     * Counter: possible passengers in this month
     * transient data, not saved
     * @author Hj. Malthaner
     */
    int pax_erzeugt;


    /**
     * Counter: transported passengers in this month
     * transient data, not saved
     * @author Hj. Malthaner
     */
    int pax_transport;


    /**
     * Modifier for city growth
     * transient data, not saved
     * @author Hj. Malthaner
     */
    int wachstum;

    /**
    * City history
    * @author prissi
    */
    sint64 city_history_year[MAX_CITY_HISTORY_YEARS][MAX_CITY_HISTORY];
    sint64 city_history_month[MAX_CITY_HISTORY_MONTHS][MAX_CITY_HISTORY];

    int last_month_bev;
    int last_year_bev;
    int	this_year_pax, this_year_transported;

	/* updates the city history
	 * @author prissi
	 */
	void roll_history(void);

	stadt_info_t *stadt_info;

public:
	/**
	 * Returns pointer to history for city
	 * @author hsiegeln
	 */
	sint64* get_city_history_year() {return *city_history_year;};
	sint64* get_city_history_month() {return *city_history_month;};

	/* returns the money dialoge of a city
	 * @author prissi
	 */
	stadt_info_t *gib_stadt_info();

	/* end of histroy related thingies */
private:
    int best_haus_wert;
    int best_strasse_wert;

    best_t best_haus;
    best_t best_strasse;

    koord strasse_anfang_pos;
    koord strasse_best_anfang_pos;

    /**
     * Arbeitsplätze der Einwohner
     * @author Hj. Malthaner
     */
    slist_tpl<fabrik_t *> arbeiterziele;
    int max_pax_arbeiterziele;

    /**
     * allokiert pax_ziele_alt/neu und init. die werte
     * @author Hj. Malthaner
     */
    void init_pax_ziele();


    /**
     * plant das bauen von Gebaeuden
     * @author Hj. Malthaner
     */
    void step_bau();


    /**
     * verteilt die Passagiere auf die Haltestellen
     * @author Hj. Malthaner
     */
    void step_passagiere();


    /**
     * ein Passagierziel in die Zielkarte eintragen
     * @author Hj. Malthaner
     */
    void merke_passagier_ziel(koord ziel, int color);


    /**
     * baut Spezialgebaeude, z.B Stadion
     * @author Hj. Malthaner
     */
    void check_bau_spezial(bool);

    /**
     * baut ein angemessenes Rathaus
     * @author V. Meyer
     */
    void check_bau_rathaus(bool);

    /**
     * constructs a new consumer
     * @author prissi
     */
    void check_bau_factory(bool);

    void bewerte();

    // bewertungsfunktionen fuer den Hauserbau
    // wie gut passt so ein Gebaeudetyp an diese Stelle ?

    gebaeude_t::typ was_ist_an(koord pos) const;

    int bewerte_industrie(koord pos);
    int bewerte_gewerbe(koord pos);
    int bewerte_wohnung(koord pos);


    /**
     * baut ein Gebaeude auf Planquadrat x,y
     */
    void baue_gebaeude(koord pos);
    void erzeuge_verkehrsteilnehmer(koord pos, int level);
    void renoviere_gebaeude(koord pos);


    /**
     * baut ein Stueck Strasse
     *
     * @param k         Bauposition
     *
     * @author Hj. Malthaner, V. Meyer
     */
    bool baue_strasse(koord k, spieler_t *sp, bool forced);

    void baue();


    /**
     * Symbols in rules:
     * S = darf keine Strasse sein
     * s = muss Strasse sein
     * n = muss Natur sein
     * . = beliebig
     *
     * @param pos position to check
     * @param regel the rule to evaluate
     * @return true on match, false otherwise
     * @author Hj. Malthaner
     */
    bool bewerte_loc(koord pos, const char *regel);


    /**
     * Check rule in all transformations at given position
     * @author Hj. Malthaner
     */
    int bewerte_pos(koord pos, const char *regel);

    void bewerte_strasse(koord pos, int rd, char *regel);
    void bewerte_haus(koord pos, int rd, char *regel);

    void trans_y(const char *regel, char *tr);
    void trans_l(const char *regel, char *tr);
    void trans_r(const char *regel, char *tr);

    void pruefe_grenzen(koord pos);

    // fuer die haltestellennamen
    int zentrum_namen_cnt, aussen_namen_cnt;


public:
    /**
     * sucht arbeitsplätze für die Einwohner
     * @author Hj. Malthaner
     */
    void verbinde_fabriken();

    /* returns all factories connected to this city ...
     * @author: prissi
     */
    const slist_tpl <fabrik_t *> & gib_arbeiterziele() const {return arbeiterziele;};

    int gib_pax_erzeugt() const {return pax_erzeugt;};
    int gib_pax_transport() const {return pax_transport;};
    int gib_wachstum() const {return wachstum;};

    /**
     * ermittelt die Einwohnerzahl der Stadt
     * @author Hj. Malthaner
     */
    int gib_einwohner() const {return bev;};


    /**
     * Gibt den Namen der Stadt zurück.
     * @author Hj. Malthaner
     */
    const char * gib_name() const {return name;};


    /**
     * Ermöglicht Zugriff auf Namesnarray
     * @author Hj. Malthaner
     */
    char * access_name() {return name;};


    /**
     * gibt einen zufällingen gleichverteilten Punkt innerhalb der
     * Stadtgrenzen zurück
     * @author Hj. Malthaner
     */
    koord gib_zufallspunkt() const;


    /**
     * gibt das pax-statistik-array für letzten monat zurück
     * @author Hj. Malthaner
     */
    const array2d_tpl<unsigned char> * gib_pax_ziele_alt() const {return &pax_ziele_alt;};
//    const array2d_tpl<int> * gib_pax_ziele_alt() const {return &pax_ziele_alt;};


    /**
     * gibt das pax-statistik-array für den aktuellen monat zurück
     * @author Hj. Malthaner
     */
    const array2d_tpl<unsigned char> * gib_pax_ziele_neu() const {return &pax_ziele_neu;};
//    const array2d_tpl<int> * gib_pax_ziele_neu() const {return &pax_ziele_neu;};

	karte_t* gib_welt() const { return welt; };

    /**
     * Erzeugt eine neue Stadt auf Planquadrat (x,y) die dem Spieler sp
     * gehoert.
     * @param welt Die Karte zu der die Stadt gehoeren soll.
     * @param sp Der Besitzer der Stadt.
     * @param x x-Planquadratkoordinate
     * @param y y-Planquadratkoordinate
     * @param number of citizens
     * @author Hj. Malthaner
     */
    stadt_t(karte_t *welt, spieler_t *sp, koord pos,int citizens);


    /**
     * Erzeugt eine neue Stadt nach Angaben aus der Datei file.
     * @param welt Die Karte zu der die Stadt gehoeren soll.
     * @param file Zeiger auf die Datei mit den Stadtbaudaten.
     * @see stadt_t::speichern()
     * @author Hj. Malthaner
     */
    stadt_t(karte_t *welt, loadsave_t *file);


    /**
     * Speichert die Daten der Stadt in der Datei file so, dass daraus
     * die Stadt wieder erzeugt werden kann. Die Gebaude und strassen der
     * Stadt werden nicht mit der Stadt gespeichert sondern mit den
     * Planquadraten auf denen sie stehen.
     * @see stadt_t::stadt_t()
     * @see planquadrat_t
     * @author Hj. Malthaner
     */
    void rdwr(loadsave_t *file);

    /**
     * Wird am Ende der LAderoutine aufgerufen, wenn die Welt geladen ist
     * und nur noch die Datenstrukturenneu verknüpft werden müssen.
     * @author Hj. Malthaner
     */
    void laden_abschliessen();

	/* change size of city
	 * @author prissi */
	void change_size( long delta_citicens );

    void step();

    void neuer_monat();


    /**
     * such ein (zufälliges) ziel für einen Passagier
     * @author Hj. Malthaner
     */
    koord finde_passagier_ziel(bool *will_return);


    /**
     * Gibt die Gruendungsposition der Stadt zurueck.
     * @return die Koordinaten des Gruendungsplanquadrates
     * @author Hj. Malthaner
     */
    inline koord gib_pos() const {return pos;};

    inline koord get_linksoben() const { return koord(li, ob);};
    inline koord get_rechtsunten() const { return koord(re, un);};


    /**
     * Gibt den Namen der Stadt zurueck.
     * @return den Namen der Stadt
     * @author Hj. Malthaner
     */
    inline const char * get_name() const {return name;};


    /**
     * Creates a station name
     * @param number if >= 0, then a number is added to the name
     * @author Hj. Malthaner
     */
    const char * haltestellenname(koord pos, const char *typ, int number);


    /**
     * Erzeugt ein Array zufaelliger Startkoordinaten,
     * die fuer eine Stadtgruendung geeignet sind.
     * @param wl Die Karte auf der die Stadt gegruendet werden soll.
     * @param anzahl die Anzahl der zu liefernden Koordinaten
     * @author Hj. Malthaner
     */
    static vector_tpl<koord> *random_place(const karte_t *wl, int anzahl);	// geeigneten platz zur Stadtgruendung durch Zufall ermitteln


    /**
     * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird.
     * @author Hj. Malthaner
     * @see simwin
     */
    char *info(char *buf) const;


    /**
     * A short info about the city stats
     * @author Hj. Malthaner
     */
    void get_short_info(cbuffer_t & buf) const;

    void add_factory_arbeiterziel(fabrik_t *fab);
};


#endif
