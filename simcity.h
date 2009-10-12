/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef simcity_h
#define simcity_h

#include "simdings.h"
#include "dings/gebaeude.h"

#include "tpl/vector_tpl.h"
#include "tpl/weighted_vector_tpl.h"
#include "tpl/sparse_tpl.h"

class karte_t;
class spieler_t;
class cbuffer_t;
class cstring_t;

class rule_t;


#define MAX_CITY_HISTORY_YEARS  (12) // number of years to keep history
#define MAX_CITY_HISTORY_MONTHS (12) // number of months to keep history

#define PAX_DESTINATIONS_SIZE (128) // size of the minimap.

enum city_cost {
	HIST_CITICENS=0,// total people
	HIST_GROWTH,	// growth (just for convenience)
	HIST_BUILDING,	// number of buildings
	HIST_CITYCARS,	// number of citycars generated
	HIST_PAS_TRANSPORTED, // number of passengers who could start their journey
	HIST_PAS_GENERATED,	// total number generated
	HIST_MAIL_TRANSPORTED,	// letters that could be sended
	HIST_MAIL_GENERATED,	// all letters generated
	HIST_GOODS_RECIEVED,	// times all storages were not empty
	HIST_GOODS_NEEDED,	// times sotrages checked
	HIST_POWER_RECIEVED,	// power consumption (not used at the moment!)
	MAX_CITY_HISTORY	// Total number of items in array
};

/**
 * Die Objecte der Klasse stadt_t bilden die Staedte in Simu. Sie
 * wachsen automatisch.
 * @author Hj. Malthaner
 */
class stadt_t
{
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

		koord get_pos() const { return best_pos;}
	// sint32 get_wert() const { return best_wert; }
	};

public:
	/**
	 * Reads city configuration data
	 * @author Hj. Malthaner
	 */
	static bool cityrules_init(cstring_t objpathname);

	static uint32 get_industry_increase();
	static void set_industry_increase(uint32 ind_increase);
	static uint32 get_minimum_city_distance();
	static void set_minimum_city_distance(uint32 s);

private:
	static karte_t *welt;
	spieler_t *besitzer_p;
	const char *name;

	weighted_vector_tpl <gebaeude_t *> buildings;

	sparse_tpl<uint8> pax_destinations_old;
	sparse_tpl<uint8> pax_destinations_new;

	// this counter will increment by one for every change => dialogs can question, if they need to update map
	unsigned long pax_destinations_new_change;

	koord pos;			// Gruendungsplanquadrat der Stadt
	koord lo, ur;		// max size of housing area
	bool  has_low_density;	// in this case extend borders by two

	bool allow_citygrowth;	// town can be static and will grow (true by default)

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

	/* updates the city history
	* @author prissi
	*/
	void roll_history(void);

public:
	/**
	 * Returns pointer to history for city
	 * @author hsiegeln
	 */
	sint64* get_city_history_year() { return *city_history_year; }
	sint64* get_city_history_month() { return *city_history_month; }

	// just needed by stadt_info.cc
	static inline karte_t* get_welt() { return welt; }

	uint32 stadtinfo_options;

	/* end of histroy related thingies */
private:
	sint32 best_haus_wert;
	sint32 best_strasse_wert;

	best_t best_haus;
	best_t best_strasse;

	/**
	 * Arbeitsplätze der Einwohner
	 * @author Hj. Malthaner
	 */
	weighted_vector_tpl<fabrik_t *> arbeiterziele;

	/**
	 * Initialization of pax_destinations_old/new
	 * @author Hj. Malthaner
	 */
	void init_pax_destinations();

	// recalcs city borders (after loading and deletion)
	void recalc_city_size();

	// calculates the growth rate for next step_bau using all the different indicators
	void calc_growth();

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
	void merke_passagier_ziel(koord ziel, uint8 color);

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
	void renoviere_gebaeude(gebaeude_t *gb);

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
	bool bewerte_loc(koord pos, rule_t &regel, int rotation);

	/*
	 * evaluates the location, tests again all rules, and caches the result
	 */
	uint16 bewerte_loc_cache(const koord pos, bool force=false);

	/**
	 * Check rule in all transformations at given position
	 * @author Hj. Malthaner
	 */
	sint32 bewerte_pos(koord pos, rule_t &regel);

	void bewerte_strasse(koord pos, sint32 rd, rule_t &regel);
	void bewerte_haus(koord pos, sint32 rd, rule_t &regel);

	void pruefe_grenzen(koord pos);

public:
	/**
	 * sucht arbeitsplätze für die Einwohner
	 * @author Hj. Malthaner
	 */
	void verbinde_fabriken();

	/* returns all factories connected to this city ...
	 * @author: prissi
	 */
	const weighted_vector_tpl<fabrik_t*>& get_arbeiterziele() const { return arbeiterziele; }
	void remove_arbeiterziel(fabrik_t *fab) { arbeiterziele.remove(fab); }

	// this function removes houses from the city house list
	// (called when removed by player, or by town)
	void remove_gebaeude_from_stadt(gebaeude_t *gb);

	// this function adds houses to the city house list
	void add_gebaeude_to_stadt(const gebaeude_t *gb);

	// changes the weight; must be called if there is a new definition (tile) for that house
	void update_gebaeude_from_stadt(gebaeude_t *gb);

	/**
	* Returns the finance history for cities
	* @author hsiegeln
	*/
	sint64 get_finance_history_year(int year, int type) { return city_history_year[year][type]; }
	sint64 get_finance_history_month(int month, int type) { return city_history_month[month][type]; }

	// growth number (smoothed!)
	sint32 get_wachstum() const {return ((sint32)city_history_month[0][HIST_GROWTH]*5) + (sint32)(city_history_month[1][HIST_GROWTH]*4) + (sint32)city_history_month[2][HIST_GROWTH]; }

	/**
	 * ermittelt die Einwohnerzahl der Stadt
	 * @author Hj. Malthaner
	 */
	sint32 get_einwohner() const {return (buildings.get_sum_weight()*6)+((2*bev-arb-won)>>1);}

	uint32 get_buildings()  const { return buildings.get_count(); }
	sint32 get_unemployed() const { return bev - arb; }
	sint32 get_homeless()   const { return bev - won; }

	/**
	 * Gibt den Namen der Stadt zurück.
	 * @author Hj. Malthaner
	 */
	const char *get_name() const { return name; }

	/**
	 * Ermöglicht Zugriff auf Namesnarray
	 * @author Hj. Malthaner
	 */
	void set_name( const char *name );

	/**
	 * gibt einen zufällingen gleichverteilten Punkt innerhalb der
	 * Stadtgrenzen zurück
	 * @author Hj. Malthaner
	 */
	koord get_zufallspunkt() const;

	/**
	 * gibt das pax-statistik-array für letzten monat zurück
	 * @author Hj. Malthaner
	 */
	const sparse_tpl<unsigned char>* get_pax_destinations_old() const { return &pax_destinations_old; }

	/**
	 * gibt das pax-statistik-array für den aktuellen monat zurück
	 * @author Hj. Malthaner
	 */
	const sparse_tpl<unsigned char>* get_pax_destinations_new() const { return &pax_destinations_new; }

	/* this counter will increment by one for every change
	 * => dialogs can question, if they need to update map
	 * @author prissi
	 */
	unsigned long get_pax_destinations_new_change() const { return pax_destinations_new_change; }

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

	void rotate90( const sint16 y_size );

	/* change size of city
	* @author prissi */
	void change_size( long delta_citicens );

	// when ng is false, no town growth any more
	void set_citygrowth_yesno( bool ng ) { allow_citygrowth = ng; }
	bool get_citygrowth() const { return allow_citygrowth; }

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
	inline koord get_pos() const {return pos;}

	inline koord get_linksoben() const { return lo;}
	inline koord get_rechtsunten() const { return ur;}

	/**
	 * Erzeugt ein Array zufaelliger Startkoordinaten,
	 * die fuer eine Stadtgruendung geeignet sind.
	 * @param wl Die Karte auf der die Stadt gegruendet werden soll.
	 * @param anzahl die Anzahl der zu liefernden Koordinaten
	 * @author Hj. Malthaner
	 * @param old_x, old_y: Generate no cities in (0,0) - (old_x, old_y)
	 * @author Gerd Wachsmuth
	 */
	static vector_tpl<koord> *random_place(const karte_t *wl, sint32 anzahl, sint16 old_x, sint16 old_y);
	// geeigneten platz zur Stadtgruendung durch Zufall ermitteln

	void zeige_info(void);

	void add_factory_arbeiterziel(fabrik_t *fab);
};

#endif
