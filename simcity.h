/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef simcity_h
#define simcity_h

#include "simobj.h"
#include "obj/gebaeude.h"

#include "tpl/vector_tpl.h"
#include "tpl/weighted_vector_tpl.h"
#include "tpl/sparse_tpl.h"
#include "utils/plainstring.h"

#include <string>

class karte_ptr_t;
class spieler_t;
class rule_t;


#define MAX_CITY_HISTORY_YEARS  (12) // number of years to keep history
#define MAX_CITY_HISTORY_MONTHS (12) // number of months to keep history

#define PAX_DESTINATIONS_SIZE (256) // size of the minimap (sparse array)

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
	 * Reads city configuration data from config/cityrules.tab
	 * @author Hj. Malthaner
	 */
	static bool cityrules_init(const std::string &objpathname);
	/**
	 * Reads/writes city configuration data from/to a savegame
	 * called from settings_t::rdwr
	 * only written for networkgames
	 * @author Dwachs
	 */
	static void cityrules_rdwr(loadsave_t *file);

	static uint32 get_industry_increase();
	static void set_industry_increase(uint32 ind_increase);
	static uint32 get_minimum_city_distance();
	static void set_minimum_city_distance(uint32 s);
	static void set_cluster_factor( uint32 factor ) { stadt_t::cluster_factor = factor; }
	static uint32 get_cluster_factor() { return stadt_t::cluster_factor; }

private:
	static karte_ptr_t welt;
	spieler_t *besitzer_p;
	plainstring name;

	weighted_vector_tpl <gebaeude_t *> buildings;

	sparse_tpl<uint8> pax_destinations_old;
	sparse_tpl<uint8> pax_destinations_new;

	// this counter will increment by one for every change => dialogs can question, if they need to update map
	unsigned long pax_destinations_new_change;

	koord pos;			// Gruendungsplanquadrat der Stadt
	koord townhall_road; // road in front of townhall
	koord lo, ur;		// max size of housing area
	koord last_center;
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
	static const uint32 city_growth_step;

	/**
	 * When to do growth next
	 * @author Hj. Malthaner
	 */
	uint32 next_growth_step;

	static uint32 cluster_factor;

	// attribute fuer die Bevoelkerung
	sint32 bev;	// Bevoelkerung gesamt
	sint32 arb;	// davon mit Arbeit
	sint32 won;	// davon mit Wohnung

	/**
	 * Unsupplied city growth needs
	 * A value of 2^32 means 1 new resident
	 * @author Nathanael Nerode (neroden)
	 */
	sint64 unsupplied_city_growth;

	/**
	* City history
	* @author prissi
	*/
	sint64 city_history_year[MAX_CITY_HISTORY_YEARS][MAX_CITY_HISTORY];
	sint64 city_history_month[MAX_CITY_HISTORY_MONTHS][MAX_CITY_HISTORY];

	/* updates the city history
	* @author prissi
	*/
	void roll_history();

public:
	/**
	 * Returns pointer to history for city
	 * @author hsiegeln
	 */
	sint64* get_city_history_year() { return *city_history_year; }
	sint64* get_city_history_month() { return *city_history_month; }

	uint32 stadtinfo_options;

	/* end of histroy related thingies */
private:
	sint32 best_haus_wert;
	sint32 best_strasse_wert;

	best_t best_haus;
	best_t best_strasse;

public:
	/**
	 * Classes for storing and manipulating target factories and their data
	 * @author Knightly
	 */
	struct factory_entry_t
	{
		union
		{
			fabrik_t *factory;
			struct
			{
				sint16 factory_pos_x;
				sint16 factory_pos_y;
			};
		};
		sint32 demand;		// amount demanded by the factory; shifted by DEMAND_BITS
		sint32 supply;		// amount that the city can supply
		sint32 remaining;	// portion of supply which has not realised yet; remaining <= supply

		factory_entry_t() : factory(NULL), demand(0), supply(0), remaining(0) { }
		factory_entry_t(fabrik_t *_factory) : factory(_factory), demand(0), supply(0), remaining(0) { }
		factory_entry_t(fabrik_t *_factory, sint32 _demand) : factory(_factory), demand(_demand), supply(0), remaining(0) { }

		bool operator == (const factory_entry_t &other) const { return ( this->factory==other.factory ); }
		void new_month() { supply = 0; remaining = 0; }
		void rdwr(loadsave_t *file);
		void resolve_factory();
	};
	#define RATIO_BITS (25)
	struct factory_set_t
	{
		vector_tpl<factory_entry_t> entries;
		sint32 total_demand;		// shifted by DEMAND_BITS
		sint32 total_remaining;
		sint32 total_generated;
		uint32 generation_ratio;
		bool ratio_stale;

		factory_set_t() : total_demand(0), total_remaining(0), total_generated(0), generation_ratio(0), ratio_stale(true) { }

		const vector_tpl<factory_entry_t>& get_entries() const { return entries; }
		const factory_entry_t* get_entry(const fabrik_t *const factory) const;
		factory_entry_t* get_random_entry();
		void update_factory(fabrik_t *const factory, const sint32 demand);
		void remove_factory(fabrik_t *const factory);
		void recalc_generation_ratio(const sint32 default_percent, const sint64 *city_stats, const int stats_count, const int stat_type);
		void new_month();
		void rdwr(loadsave_t *file);
		void resolve_factories();
	};

private:
	/**
	 * Data of target factories for pax/mail
	 * @author Knightly
	 */
	factory_set_t target_factories_pax;
	factory_set_t target_factories_mail;

	/**
	 * Initialization of pax_destinations_old/new
	 * @author Hj. Malthaner
	 */
	void init_pax_destinations();

	/**
	 * Recalculates city borders (after loading and deletion).
	 * @warning Do not call this during multithreaded loading!
	 */
	void recalc_city_size();

	// calculates the growth rate for next growth_interval using all the different indicators
	void calc_growth();

	/**
	 * Build new buildings when growing city
	 * @author Hj. Malthaner
	 */
	void step_grow_city(bool new_town = false);

	enum pax_return_type { no_return, factory_return, tourist_return, city_return };

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
	 * Build/renovates a city building at Planquadrat x,y
	 */
	void build_city_building(koord pos);
	void renovate_city_building(gebaeude_t *gb);

	void erzeuge_verkehrsteilnehmer(koord pos, sint32 level,koord target);

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
	static bool bewerte_loc(koord pos, const rule_t &regel, int rotation);

	/**
	 * Check rule in all transformations at given position
	 * @author Hj. Malthaner
	 */
	static sint32 bewerte_pos(koord pos, const rule_t &regel);

	void bewerte_strasse(koord pos, sint32 rd, const rule_t &regel);
	void bewerte_haus(koord pos, sint32 rd, const rule_t &regel);

	/**
	 * Updates city limits: tile at @p pos belongs to city.
	 * @warning Do not call this during multithreaded loading!
	 */
	void pruefe_grenzen(koord pos);

public:
	/**
	 * sucht arbeitsplätze für die Einwohner
	 * @author Hj. Malthaner
	 */
	void verbinde_fabriken();

	/**
	 * Returns the data set associated with the pax/mail target factories
	 * @author: prissi
	 */
	const factory_set_t& get_target_factories_for_pax() const { return target_factories_pax; }
	const factory_set_t& get_target_factories_for_mail() const { return target_factories_mail; }
	factory_set_t& access_target_factories_for_pax() { return target_factories_pax; }
	factory_set_t& access_target_factories_for_mail() { return target_factories_mail; }

	// this function removes houses from the city house list
	// (called when removed by player, or by town)
	void remove_gebaeude_from_stadt(gebaeude_t *gb);

	/**
	 * This function adds houses to the city house list.
	 * @param ordered true for multithreaded loading, will insert buidings ordered, will not update city limits
	 */
	void add_gebaeude_to_stadt(const gebaeude_t *gb, bool ordered=false);

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
	stadt_t(loadsave_t *file);

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
	void change_size( sint64 delta_citizens, bool new_town = false );

	// when ng is false, no town growth any more
	void set_citygrowth_yesno( bool ng ) { allow_citygrowth = ng; }
	bool get_citygrowth() const { return allow_citygrowth; }

	void step(uint32 delta_t);

	void neuer_monat( bool recalc_destinations );

private:
	/**
	 * List of target cities weighted by both city size and distance
	 * @author Knightly
	 */
	weighted_vector_tpl<stadt_t *> target_cities;

	/**
	 * List of target attractions weighted by both passenger level and distance
	 * @author Knightly
	 */
	weighted_vector_tpl<gebaeude_t *> target_attractions;

public:
	/**
	 * Functions for manipulating the list of target cities
	 * @author Knightly
	 */
	void add_target_city(stadt_t *const city);
	void remove_target_city(stadt_t *const city) { target_cities.remove( city ); }
	void recalc_target_cities();

	/**
	 * Functions for manipulating the list of target attractions
	 * @author Knightly
	 */
	void add_target_attraction(gebaeude_t *const attraction);
	void remove_target_attraction(gebaeude_t *const attraction) { target_attractions.remove( attraction ); }
	void recalc_target_attractions();

	/**
	 * such ein (zufälliges) ziel für einen Passagier
	 * @author Hj. Malthaner
	 */
	koord find_destination(factory_set_t &target_factories, const sint64 generated, pax_return_type* will_return, factory_entry_t* &factory_entry);

	/**
	 * Gibt die Gruendungsposition der Stadt zurueck.
	 * @return die Koordinaten des Gruendungsplanquadrates
	 * @author Hj. Malthaner
	 */
	inline koord get_pos() const {return pos;}
	inline koord get_townhall_road() const {return townhall_road;}

	inline koord get_linksoben() const { return lo;}
	inline koord get_rechtsunten() const { return ur;}

	koord get_center() const { return (lo+ur)/2; }

	/**
	 * Erzeugt ein Array zufaelliger Startkoordinaten,
	 * die fuer eine Stadtgruendung geeignet sind.
	 * @param wl Die Karte auf der die Stadt gegruendet werden soll.
	 * @param anzahl die Anzahl der zu liefernden Koordinaten
	 * @author Hj. Malthaner
	 * @param old_x, old_y: Generate no cities in (0,0) - (old_x, old_y)
	 * @author Gerd Wachsmuth
	 */
	static vector_tpl<koord> *random_place(sint32 anzahl, sint16 old_x, sint16 old_y);
	// geeigneten platz zur Stadtgruendung durch Zufall ermitteln

	void zeige_info();
};

#endif
