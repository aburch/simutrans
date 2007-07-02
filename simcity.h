/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simcity_h
#define simcity_h

#include "simdings.h"
#include "dings/gebaeude.h"

#include "tpl/vector_tpl.h"
#include "tpl/weighted_vector_tpl.h"
#include "tpl/array2d_tpl.h"

class karte_t;
class spieler_t;
class cbuffer_t;
class stadt_info_t;

// part of passengers going to factories or toursit attractions (100% mx)
#define FACTORY_PAX 33	// workers
#define TOURIST_PAX 16		// tourists
// other long-distance travellers


#define MAX_CITY_HISTORY_YEARS  12 // number of years to keep history
#define MAX_CITY_HISTORY_MONTHS  12 // number of months to keep history
#define MAX_CITY_HISTORY 4      // Total number of items in array

#define HIST_CITICENS 0	// total people
#define HIST_GROWTH 1 // growth (just for convenience)
#define HIST_TRANSPORTED 2
#define HIST_GENERATED 3



class cstring_t;

/**
 * Die Objecte der Klasse stadt_t bilden die Staedte in Simu. Sie
 * wachsen automatisch.
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
		sint32 best_wert;
		koord best_pos;
	public:
		void reset(koord pos) { best_wert = 0; best_pos = pos; }

		void check(koord pos, sint32 wert) {
			if(wert > best_wert) {
				best_wert = wert;
				best_pos = pos;
			}
		}

		bool found() const { return best_wert > 0; }

		koord gib_pos() const { return best_pos;}
	// sint32 gib_wert() const { return best_wert; }
	};

public:
	/**
	 * Reads city configuration data
	 * @author Hj. Malthaner
	 */
	static bool cityrules_init(cstring_t objpathname);

private:
	static karte_t *welt;
	spieler_t *besitzer_p;
	char *name;

	weighted_vector_tpl <const gebaeude_t *> buildings;

	array2d_tpl<unsigned char> pax_ziele_alt;
	array2d_tpl<unsigned char> pax_ziele_neu;

	koord pos;			// Gruendungsplanquadrat der Stadt
	koord lo, ur;	// max size of housing area

	// this counter indicate which building will be processed next
	uint32 step_count;

	/**
	 * step so every house is asked once per month
	 * i.e. 262144/(number of houses) per step
	 * @author Hj. Malthaner
	 */
	uint32 step_interval;

	/**
	 * next passenger generation timer
	 * @author Hj. Malthaner
	 */
	uint32 next_step;

	/**
	 * in this fixed interval, construction will happen
	 */
	static const uint32 step_bau_interval;

	/**
	 * next construction
	 * @author Hj. Malthaner
	 */
	uint32 next_bau_step;

	// attribute fuer die Bevoelkerung
	sint32 bev;	// Bevoelkerung gesamt
	sint32 arb;	// davon mit Arbeit
	sint32 won;	// davon mit Wohnung

	/**
	 * Counter: possible passengers in this month
	 * transient data, not saved
	 * @author Hj. Malthaner
	 */
	sint32 pax_erzeugt;

	/**
	 * Counter: transported passengers in this month
	 * transient data, not saved
	 * @author Hj. Malthaner
	 */
	sint32 pax_transport;

	/**
	 * Modifier for city growth
	 * transient data, not saved
	 * @author Hj. Malthaner
	 */
	sint32 wachstum;

	/**
	* City history
	* @author prissi
	*/
	sint64 city_history_year[MAX_CITY_HISTORY_YEARS][MAX_CITY_HISTORY];
	sint64 city_history_month[MAX_CITY_HISTORY_MONTHS][MAX_CITY_HISTORY];

	sint32 last_month_bev;
	sint32 last_year_bev;
	sint32	this_year_pax, this_year_transported;

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
	sint64* get_city_history_year() { return *city_history_year; }
	sint64* get_city_history_month() { return *city_history_month; }

	/* returns the money dialoge of a city
	 * @author prissi
	 */
	stadt_info_t *gib_stadt_info();

	// just needed by stadt_info.cc
	static inline karte_t* get_welt() { return welt; }

	/* end of histroy related thingies */
private:
	sint32 best_haus_wert;
	sint32 best_strasse_wert;

	best_t best_haus;
	best_t best_strasse;

	koord strasse_anfang_pos;
	koord strasse_best_anfang_pos;

	/**
	 * Arbeitsplätze der Einwohner
	 * @author Hj. Malthaner
	 */
	weighted_vector_tpl<fabrik_t *> arbeiterziele;

	/**
	 * allokiert pax_ziele_alt/neu und init. die werte
	 * @author Hj. Malthaner
	 */
	void init_pax_ziele();

	// recalculate house informations (used for target selection)
	void recount_houses();

	// recalcs city borders (after loading and deletion)
	void recalc_city_size();

	/**
	 * plant das bauen von Gebaeuden
	 * @author Hj. Malthaner
	 */
	void step_bau();

	enum pax_zieltyp { no_return, factoy_return, tourist_return, town_return };

	/**
	 * verteilt die Passagiere auf die Haltestellen
	 * @author Hj. Malthaner
	 */
	void step_passagiere();

	/**
	 * ein Passagierziel in die Zielkarte eintragen
	 * @author Hj. Malthaner
	 */
	void merke_passagier_ziel(koord ziel, sint32 color);

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

	// find out, what building matches best
	void bewerte_res_com_ind(const koord pos, int &ind, int &com, int &res);

	/**
	 * baut ein Gebaeude auf Planquadrat x,y
	 */
	void baue_gebaeude(koord pos);
	void erzeuge_verkehrsteilnehmer(koord pos, sint32 level,koord target);
	void renoviere_gebaeude(koord pos);

	/**
	 * baut ein Stueck Strasse
	 *
	 * @param k         Bauposition
	 *
	 * @author Hj. Malthaner, V. Meyer
	 */
	bool baue_strasse(const koord k, spieler_t *sp, bool forced);

	void baue();

	/**
	 * @param pos position to check
	 * @param regel the rule to evaluate
	 * @return true on match, false otherwise
	 * @author Hj. Malthaner
	 */
	bool bewerte_loc(koord pos, const char *regel, int rotation);

	/**
	 * Check rule in all transformations at given position
	 * @author Hj. Malthaner
	 */
	sint32 bewerte_pos(koord pos, const char *regel);

	void bewerte_strasse(koord pos, sint32 rd, const char* regel);
	void bewerte_haus(koord pos, sint32 rd, const char* regel);

	void pruefe_grenzen(koord pos);

	// fuer die haltestellennamen
	sint32 zentrum_namen_cnt, aussen_namen_cnt;

public:
	/**
	 * sucht arbeitsplätze für die Einwohner
	 * @author Hj. Malthaner
	 */
	void verbinde_fabriken();

	/* returns all factories connected to this city ...
	 * @author: prissi
	 */
	const weighted_vector_tpl<fabrik_t*>& gib_arbeiterziele() const { return arbeiterziele; }

	// this function removes houses from the city house list
	// (called when removed by player, or by town)
	void remove_gebaeude_from_stadt(const gebaeude_t *gb);

	// this function adds houses to the city house list
	void add_gebaeude_to_stadt(const gebaeude_t *gb);

	sint32 gib_pax_erzeugt() const {return pax_erzeugt;}
	sint32 gib_pax_transport() const {return pax_transport;}
	sint32 gib_wachstum() const {return wachstum;}

	/**
	 * ermittelt die Einwohnerzahl der Stadt
	 * @author Hj. Malthaner
	 */
	sint32 gib_einwohner() const {return (buildings.get_sum_weight()*6)+((2*bev-arb-won)>>1);}

	/**
	 * Gibt den Namen der Stadt zurück.
	 * @author Hj. Malthaner
	 */
	const char* gib_name() const { return name; }

	/**
	 * Ermöglicht Zugriff auf Namesnarray
	 * @author Hj. Malthaner
	 */
	char* access_name() { return name; }

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
	const array2d_tpl<unsigned char>* gib_pax_ziele_alt() const { return &pax_ziele_alt; }

	/**
	 * gibt das pax-statistik-array für den aktuellen monat zurück
	 * @author Hj. Malthaner
	 */
	const array2d_tpl<unsigned char>* gib_pax_ziele_neu() const { return &pax_ziele_neu; }

	/**
	 * Erzeugt eine neue Stadt auf Planquadrat (x,y) die dem Spieler sp
	 * gehoert.
	 * @param sp Der Besitzer der Stadt.
	 * @param x x-Planquadratkoordinate
	 * @param y y-Planquadratkoordinate
	 * @param number of citizens
	 * @author Hj. Malthaner
	 */
	stadt_t(spieler_t* sp, koord pos, sint32 citizens);

	/**
	 * Erzeugt eine neue Stadt nach Angaben aus der Datei file.
	 * @param welt Die Karte zu der die Stadt gehoeren soll.
	 * @param file Zeiger auf die Datei mit den Stadtbaudaten.
	 * @see stadt_t::speichern()
	 * @author Hj. Malthaner
	 */
	stadt_t(karte_t *welt, loadsave_t *file);

	// closes window and that stuff
	~stadt_t();

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

	void step(long delta_t);

	void neuer_monat();

	/**
	 * such ein (zufälliges) ziel für einen Passagier
	 * @author Hj. Malthaner
	 */
	koord finde_passagier_ziel(pax_zieltyp *will_return);

	/**
	 * Gibt die Gruendungsposition der Stadt zurueck.
	 * @return die Koordinaten des Gruendungsplanquadrates
	 * @author Hj. Malthaner
	 */
	inline koord gib_pos() const {return pos;}

	inline koord get_linksoben() const { return lo;}
	inline koord get_rechtsunten() const { return ur;}

	/**
	 * Creates a station name
	 * @param number if >= 0, then a number is added to the name
	 * @author Hj. Malthaner
	 */
	char* haltestellenname(koord pos, const char* typ, sint32 number);

	/**
	 * Erzeugt ein Array zufaelliger Startkoordinaten,
	 * die fuer eine Stadtgruendung geeignet sind.
	 * @param wl Die Karte auf der die Stadt gegruendet werden soll.
	 * @param anzahl die Anzahl der zu liefernden Koordinaten
	 * @author Hj. Malthaner
	 */
	static vector_tpl<koord> *random_place(const karte_t *wl, sint32 anzahl);	// geeigneten platz zur Stadtgruendung durch Zufall ermitteln

	/**
	 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 * @see simwin
	 */
	char *info(char *buf) const;

	void add_factory_arbeiterziel(fabrik_t *fab);
};

#endif
