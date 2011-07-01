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
#include "tpl/array2d_tpl.h"
#include "tpl/slist_tpl.h"
#include "tpl/koordhashtable_tpl.h"

#include "vehicle/simverkehr.h"
#include "tpl/sparse_tpl.h"

#include <string>

class karte_t;
class spieler_t;
class fabrik_t;

class rule_t;

// part of passengers going to factories or toursit attractions (100% mx)
#define FACTORY_PAX (33)	// workers
#define TOURIST_PAX (16)		// tourists

#define MAX_CITY_HISTORY_YEARS  (12) // number of years to keep history
#define MAX_CITY_HISTORY_MONTHS (12) // number of months to keep history

#define PAX_DESTINATIONS_SIZE (128) // size of the minimap in the city window.

enum city_cost {
	HIST_CITICENS=0,// total people
	HIST_GROWTH,	// growth (just for convenience)
	HIST_BUILDING,	// number of buildings
	HIST_CITYCARS,	// Amount of private traffic produced by the city
	HIST_PAS_TRANSPORTED, // number of passengers who could start their journey
	HIST_PAS_GENERATED,	// total number generated
	HIST_MAIL_TRANSPORTED,	// letters that could be sent
	HIST_MAIL_GENERATED,	// all letters generated
	HIST_GOODS_RECIEVED,	// times all storages were not empty
	HIST_GOODS_NEEDED,	// times sotrages checked
	HIST_POWER_RECIEVED,	// power consumption 
	HIST_POWER_NEEDED,		// Power demand by the city.
	HIST_CONGESTION,	// Level of congestion in the city, expressed in percent.
	HIST_CAR_OWNERSHIP,	// Proportion of total population who have access to cars.
	MAX_CITY_HISTORY	// Total number of items in array
};

enum route_status 
{
	no_route = 0,
	too_slow = 1,
	good = 2,
	private_car_only = 3,
	can_walk = 4
};

class road_destination_finder_t : public fahrer_t
{
private:
	automobil_t *master;
	karte_t* welt;
	koord3d dest;

public:
	road_destination_finder_t(karte_t *w, automobil_t* m) 
	{ 
		welt = w;
		dest = koord3d::invalid;
		master = m;
	};

	virtual void set_destination(koord3d d) { dest = d; }
	
	virtual waytype_t get_waytype() const { return road_wt; };
	virtual bool ist_befahrbar( const grund_t* gr ) const;

	virtual bool ist_ziel( const grund_t* gr, const grund_t* ) const;

	virtual ribi_t::ribi get_ribi( const grund_t* gr) const;

	virtual int get_kosten( const grund_t* gr, const sint32 max_speed, koord from_pos) const;

	~road_destination_finder_t()
	{
		delete master;
	}
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
	* "Small helper class - saves the best assessment of a position." (Google)
	*
	* @author V. Meyer
	*/
	class best_t {
		sint8 best_wert;
		koord best_pos;
	public:
		void reset(koord pos) { best_wert = 0; best_pos = pos; }

		void check(koord pos, sint8 wert) {
			if(wert > best_wert) {
				best_wert = wert;
				best_pos = pos;
			}
		}

		bool found() const { return best_wert > 0; }

		koord get_pos() const { return best_pos;}
	// sint8 get_wert() const { return best_wert; }
	};

public:
	/**
	 * Reads city configuration data from config/cityrules.tab
	 * @author Hj. Malthaner
	 */
	static bool cityrules_init(const std::string &objpathname);
	static void privatecar_init(const std::string &objfilename);
	sint16 get_private_car_ownership(sint32 monthyear);
	uint16 get_electricity_consumption(sint32 monthyear) const;
	static void electricity_consumption_init(const std::string &objfilename);

	/**
	 * Reads/writes city configuration data from/to a savegame
	 * called from settings_t::rdwr
	 * only written for networkgames
	 * @author Dwachs
	 */
	static void cityrules_rdwr(loadsave_t *file);
	static void privatecar_rdwr(loadsave_t *file);
	static void electricity_consumption_rdwr(loadsave_t *file);

private:
	static karte_t *welt;
	spieler_t *besitzer_p;
	const char *name;

	weighted_vector_tpl <gebaeude_t *> buildings;

	sparse_tpl<uint8> pax_destinations_old;
	sparse_tpl<uint8> pax_destinations_new;

	// this counter will increment by one for every change => dialogs can question, if they need to update map
	unsigned long pax_destinations_new_change;

	koord pos;			// Gruendungsplanquadrat der Stadt ("founding grid square" - Google)
	koord townhall_road; // road in front of townhall
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

	// population statistics
	sint32 bev; // total population
	sint32 arb; // amount with jobs
	sint32 won; // amount with homes

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

	void set_private_car_trip(int passengers, stadt_t* destination_town);

	// This is needed to prevent double counting of incoming traffic.
	sint32 incoming_private_cars;
	
	//This is needed because outgoing cars are disregarded when calculating growth.
	sint32 outgoing_private_cars;

	slist_tpl<stadtauto_t *> current_cars;

	// The factories that are *inside* the city limits.
	// Needed for power consumption of such factories.
	vector_tpl<fabrik_t *> city_factories;

	enum journey_distance_type { local, midrange, longdistance };

	// Hashtable of all cities/attractions/industries connected by road from this city.
	// Key: city (etc.) location
	// Value: journey time per tile (equiv. straight line distance)
	// (in 10ths of minutes); 65535 = unreachable.
	// @author: jamespetts, April 2010, modified December 2010 to koords rather than poiners
	// so as to be network safe
	koordhashtable_tpl<koord, uint16> connected_cities;
	koordhashtable_tpl<koord, uint16> connected_industries;
	koordhashtable_tpl<koord, uint16> connected_attractions;

	road_destination_finder_t *finder;
	route_t *private_car_route;

	vector_tpl<senke_t*> substations;

	// The month in which this city will update its private car routes
	// if an update is needed. This spreads the computational load over
	// a year instead of forcing it all into a month, thus improving 
	// performance.
	// @author: jamespetts, February 2011
	uint8 private_car_update_month;

	sint32 number_of_cars;

public:
	/**
	 * Returns pointer to history for city
	 * @author hsiegeln
	 */
	sint64* get_city_history_year() { return *city_history_year; }
	sint64* get_city_history_month() { return *city_history_month; }

	sint32 get_outstanding_cars();

	// just needed by stadt_info.cc
	static inline karte_t* get_welt() { return welt; }

	uint32 stadtinfo_options;

	void set_private_car_trips(uint16 number) 
	{
		city_history_month[0][HIST_CITYCARS] += number;
		city_history_year[0][HIST_CITYCARS] += number;
		incoming_private_cars += number;
	}

	inline void add_transported_passengers(uint16 passengers)
	{
		city_history_year[0][HIST_PAS_TRANSPORTED] += passengers;
		city_history_month[0][HIST_PAS_TRANSPORTED] += passengers;
	}

	inline void add_transported_mail(uint16 mail)
	{
		city_history_year[0][HIST_MAIL_TRANSPORTED] += mail;
		city_history_month[0][HIST_MAIL_TRANSPORTED] += mail;
	}

	//@author: jamespetts
	void add_power(uint32 p) { city_history_month[0][HIST_POWER_RECIEVED] += p; city_history_year[0][HIST_POWER_RECIEVED] += p; }

	void add_power_demand(uint32 p) { city_history_month[0][HIST_POWER_NEEDED] += p; city_history_year[0][HIST_POWER_NEEDED] += p; }

	//@ 9th of February 2011. 
	/*
	 * Used for recording congestion in cases where there is a traffic jam specifically noted.
	 */
	void add_congestion(uint32 c) { city_history_month[0][HIST_CONGESTION] += c; city_history_year[0][HIST_CONGESTION] += c; }

	/* end of history related thingies */
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

	// recalcs city borders (after loading and deletion)
	void recalc_city_size();

	// calculates the growth rate for next step_bau using all the different indicators
	void calc_growth();

	/**
	 * plant das bauen von Gebaeuden
	 * @author Hj. Malthaner
	 */
	void step_bau();

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
	 * baut ein Gebaeude auf Planquadrat x,y
	 */
	void baue_gebaeude(koord pos, bool new_town);
	void erzeuge_verkehrsteilnehmer(koord pos, sint32 level,koord target);
	bool renoviere_gebaeude(gebaeude_t *gb);

	/**
	 * baut ein Stueck Strasse
	 *
	 * @param k         Bauposition
	 *
	 * @author Hj. Malthaner, V. Meyer
	 */
	bool baue_strasse(const koord k, spieler_t *sp, bool forced);

	void baue(bool new_town);

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

	void calc_internal_passengers();

	uint16 adjusted_passenger_routing_local_chance;

	// Checks to see whether this town is connected
	// by road to each other town.
	// @author: jamespetts, April 2010
	uint16 check_road_connexion_to(stadt_t* city);
	uint16 check_road_connexion_to(const fabrik_t* industry);
	uint16 check_road_connexion_to(const gebaeude_t* attraction);
	uint16 check_road_connexion(koord3d destination);

	// Adds a connexion back from a city when a route has been calculated.
	void add_road_connexion(uint16 journey_time_per_tile, stadt_t* origin_city);
	void set_no_connexion_to_industry(const fabrik_t* unconnected_industry);
	void set_no_connexion_to_attraction(const gebaeude_t* unconnected_attraction);

	bool check_road_connexions;

	inline void register_factory_passenger_generation(int* pax_left_to_do, const ware_besch_t *const wtyp, factory_set_t &target_factories, factory_entry_t* &factory_entry);

public:
	/**
	 * sucht arbeitsplätze für die Einwohner
	 * "looking jobs for residents" (Google)
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
	 * "determines the population of the city"
	 * @author Hj. Malthaner
	 */
	sint32 get_einwohner() const {return (buildings.get_sum_weight()*6)+((2*bev-arb-won)>>1);}

	uint32 get_buildings()  const { return buildings.get_count(); }
	sint32 get_unemployed() const { return bev - arb; }
	sint32 get_homeless()   const { return bev - won; }

	/**
	 * Gibt den Namen der Stadt zurück.
	 * "Specifies the name of the town." (Google)
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
	koord get_zufallspunkt(uint32 min_distance = 0, uint32 max_distance = 16384, koord origin = koord::invalid) const;

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

	void neuer_monat(bool check);

	//@author: jamespetts
	union destination_object
	{
		stadt_t* town;
		const fabrik_t* industry;
		const gebaeude_t* attraction;
	};

	struct destination
	{
		koord location;
		uint16 type; //1 = town; others as #define above.
		destination_object object; 
		factory_entry_t* factory_entry;
		destination() { factory_entry = NULL; }
	};


private:
	/**
	 * A weighted list of distances
	 * @author Knightly
	 */
	static weighted_vector_tpl<uint32> distances;

	/**
	 * Record of a target city
	 * @author Knightly
	 */
	struct target_city_t
	{
		stadt_t *city;
		uint32 distance;

		target_city_t() : city(NULL), distance(0) { }
		target_city_t(stadt_t *const _city, const uint32 _distance) : city(_city), distance(_distance) { }

		bool operator == (const target_city_t &other) const { return city == other.city; }

		static bool less_than_or_equal(const target_city_t &a, const target_city_t &b) { return a.distance <= b.distance; }
		static bool less_than(const target_city_t &a, const target_city_t &b) { return a.distance < b.distance; }
	};

	/**
	 * List of target cities weighted by both city size and distance
	 * @author Knightly
	 */
	weighted_vector_tpl<target_city_t> target_cities;

	/**
	 * List of target attractions weighted by both passenger level and distance
	 * @author Knightly
	 */
	weighted_vector_tpl<gebaeude_t *> target_attractions;

public:

	/**
	 * Initialise the weighted list of distances
	 * @author Knightly
	 */
	static void init_distances(const uint32 max_distance);

	/**
	 * Functions for manipulating the list of target cities
	 * @author Knightly
	 */
	void add_target_city(stadt_t *const city);
	void remove_target_city(stadt_t *const city) { target_cities.remove( target_city_t(city, 0) ); }
	void update_target_city(stadt_t *const city);
	void update_target_cities();
	void recalc_target_cities();

	/**
	 * Functions for manipulating the list of target attractions
	 * @author Knightly
	 */
	void add_target_attraction(gebaeude_t *const attraction);
	void remove_target_attraction(gebaeude_t *const attraction) { target_attractions.remove(attraction); }
	void recalc_target_attractions();

	/**
	 * such ein (zufälliges) ziel für einen Passagier
	 * @author Hj. Malthaner
	 */

	destination find_destination(factory_set_t &target_factories, 
		const sint64 generated, 
		pax_return_type* will_return, 
		uint32 min_distance = 0, 
		uint32 max_distance = 16384,
		koord origin = koord::invalid);

	/**
	 * Gibt die Gruendungsposition der Stadt zurueck.
	 * @return die Koordinaten des Gruendungsplanquadrates
	 * "eturn the coordinates of the establishment grid square" (Babelfish)
	 * @author Hj. Malthaner
	 */
	inline koord get_pos() const {return pos;}
	inline koord get_townhall_road() const {return townhall_road;}

	inline koord get_linksoben() const { return lo;} // "Top left" (Google)
	inline koord get_rechtsunten() const { return ur;} // "Bottom right" (Google)

	// Checks whether any given postition is within the city limits.
	bool is_within_city_limits(koord k) const;

	/**
	 * Erzeugt ein Array zufaelliger Startkoordinaten,
	 * die fuer eine Stadtgruendung geeignet sind.
	 * @param wl Die Karte auf der die Stadt gegruendet werden soll.
	 * @param anzahl die Anzahl der zu liefernden Koordinaten
	 * @author Hj. Malthaner
	 * @param old_x, old_y: Generate no cities in (0,0) - (old_x, old_y)
	 * @author Gerd Wachsmuth
	 */
	static vector_tpl<koord> *random_place(const karte_t *wl, const vector_tpl<sint32> *sizes_list, sint16 old_x, sint16 old_y);
	// geeigneten platz zur Stadtgruendung durch Zufall ermitteln

	void zeige_info(void);

	void add_factory_arbeiterziel(fabrik_t *fab);

	uint8 get_congestion() { return (uint8) city_history_month[0][HIST_CONGESTION]; }

	void add_city_factory(fabrik_t *fab) { city_factories.append(fab); }
	void remove_city_factory(fabrik_t *fab) { city_factories.remove(fab); }
	const vector_tpl<fabrik_t*>& get_city_factories() const { return city_factories; }

	uint32 get_power_demand() const;

	void add_substation(senke_t* substation);
	void remove_substation(senke_t* substation);
	vector_tpl<senke_t*>* get_substations() { return &substations; }

	/**
	* These methods are used for removing a connected city (etc.)
	* from the list when these objects are deleted, to prevent 
	* acces violations.
	*/
	void remove_connected_city(stadt_t* city);
	void remove_connected_industry(fabrik_t* fab);
	void remove_connected_attraction(gebaeude_t* attraction);

	// @author: jamespetts
	// September 2010
	uint16 get_max_dimension();

	/*@author: jamespetts
	 * February 2011
	 */
	void add_car(stadtauto_t* car);

	slist_tpl<stadtauto_t *> * get_current_cars() { return &current_cars; }

};

#endif
