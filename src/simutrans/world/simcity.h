/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMCITY_H
#define SIMCITY_H


#include "../obj/simobj.h"
#include "../obj/gebaeude.h"

#include "../tpl/vector_tpl.h"
#include "../tpl/weighted_vector_tpl.h"
#include "../tpl/sparse_tpl.h"
#include "../utils/plainstring.h"

#include <string>

class building_desc_t;
class karte_ptr_t;
class player_t;
class rule_t;


#define MAX_CITY_HISTORY_YEARS  (12) // number of years to keep history
#define MAX_CITY_HISTORY_MONTHS (12) // number of months to keep history

#define PAX_DESTINATIONS_SIZE (256) // size of the minimap (sparse array)

enum city_cost {
	HIST_CITIZENS = 0,     // total people
	HIST_GROWTH,           // growth (just for convenience)
	HIST_BUILDING,         // number of buildings
	HIST_CITYCARS,         // number of citycars generated
	HIST_PAS_TRANSPORTED,  // number of passengers who could start their journey
	HIST_PAS_WALKED,       // direct transfer
	HIST_PAS_GENERATED,    // total number generated
	HIST_MAIL_TRANSPORTED, // letters that could be sent
	HIST_MAIL_WALKED,      // direct handover
	HIST_MAIL_GENERATED,   // all letters generated
	HIST_GOODS_RECEIVED,   // times all storages were not empty
	HIST_GOODS_NEEDED,     // times storages checked
	HIST_POWER_RECEIVED,   // power consumption (not used at the moment!)
	MAX_CITY_HISTORY       // Total number of items in array
};

// The base offset for passenger statistics.
static const uint32 HIST_BASE_PASS = HIST_PAS_TRANSPORTED;

// The base offset for mail statistics.
static const uint32 HIST_BASE_MAIL = HIST_MAIL_TRANSPORTED;

// The offset for transported statistic for passengers and mail.
static const uint32 HIST_OFFSET_TRANSPORTED = 0;

// The offset for walked statistic for passengers and mail.
static const uint32 HIST_OFFSET_WALKED = 1;

// The offset for generated statistic for passengers and mail.
static const uint32 HIST_OFFSET_GENERATED = 2;

// The number of growth factors kept track of.
static const uint32 GROWTH_FACTOR_NUMBER = 3;

// Mail return multiplier used by mail producers (attractions and factories).
static const uint32 MAIL_RETURN_MULTIPLIER_PRODUCERS = 3;

// Passenger generation ratio. This many parts to mail generation ratio parts.
static const uint32 GENERATE_RATIO_PASS = 3;

// Mail generation ratio. This many parts to passenger generation ratio parts.
static const uint32 GENERATE_RATIO_MAIL = 1;

/**
 * Die Objecte der Klasse stadt_t bilden die Staedte in Simu. Sie
 * wachsen automatisch.
 */
class stadt_t
{
	/**
	* Kleine Hilfsklasse - speichert die beste Bewertung einer Position.
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
	 */
	static bool cityrules_init(const std::string &objpathname);
	/**
	 * Reads/writes city configuration data from/to a savegame
	 * called from settings_t::rdwr
	 * only written for networkgames
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
	player_t *owner;
	plainstring name;

	weighted_vector_tpl <gebaeude_t *> buildings;

	sparse_tpl<PIXVAL> pax_destinations_old;
	sparse_tpl<PIXVAL> pax_destinations_new;

	// this counter will increment by one for every change => dialogs can question, if they need to update map
	uint32 pax_destinations_new_change;

	koord pos;             // Gruendungsplanquadrat der City
	koord townhall_road;   // road in front of townhall
	koord lo, ur;          // max size of housing area
	koord last_center;
	bool  has_low_density; // in this case extend borders by two

	bool allow_citygrowth; // town can be static and will grow (true by default)

	bool has_townhall;
	// this counter indicate which building will be processed next
	uint32 step_count;

	/**
	 * step so every house is asked once per month
	 * i.e. 262144/(number of houses) per step
	 */
	uint32 step_interval;

	/**
	 * next passenger generation timer
	 */
	uint32 next_step;

	/**
	 * in this fixed interval, construction will happen
	 */
	static const uint32 city_growth_step;

	/**
	 * When to do growth next
	 */
	uint32 next_growth_step;

	static uint32 cluster_factor;

	// attribute for the population (Bevoelkerung)
	sint32 bev; // total population (bevoelkerung)
	sint32 arb; // with a job (arbeit)
	sint32 won; // with a residence (wohnung)

	/**
	 * Un-supplied city growth needs
	 * A value of 2^32 means 1 new resident
	 */
	sint64 unsupplied_city_growth;

	/**
	* City history
	* Current month stats are not appropiate to determine satisfaction for growth.
	*/
	sint64 city_history_year[MAX_CITY_HISTORY_YEARS][MAX_CITY_HISTORY];
	sint64 city_history_month[MAX_CITY_HISTORY_MONTHS][MAX_CITY_HISTORY];

	/* updates the city history
	*/
	void roll_history();

	/* Members used to determine satisfaction for growth rate.
	 * Satisfaction of this month cannot be used as it is an averaging filter for the entire month up to the present.
	 * Instead the average over a number of growth ticks is used, defaulting to last month average if nothing is available.
	 */
private:
	// The growth factor type in form of the amount demanded and what was received.
	struct city_growth_factor_t {
		// The wanted value.
		sint64 demand;
		// The received value.
		sint64 supplied;

		city_growth_factor_t() : demand(0), supplied(0){}
	};

	// The previous values of the growth factors. Used to get delta between ticks and must be saved for determinism.
	city_growth_factor_t city_growth_factor_previous[GROWTH_FACTOR_NUMBER];

	/* Method to generate comparable growth factor data.
	 * This allows one to alter the logic which computes growth.
	 * @param factors factor array.
	 * @param month the month which is to be used for the growth factors.
	 */
	void city_growth_get_factors(city_growth_factor_t (&factors)[GROWTH_FACTOR_NUMBER], uint32 const month) const;

	/* Method to compute base growth using growth factors.
	 * Logs differences in growth factors as well.
	 * rprec : The returned fractional precision (out of sint32).
	 * cprec : The computation fractional precision (out of sint32).
	 */
	sint32 city_growth_base(uint32 const rprec = 6, uint32 const cprec = 16);

	/* Method to roll previous growth factors at end of month, called before history rolls over.
	 * Needed to prevent loss of data (not set to 0) and while keeping reasonable (no insane values).
	 * month : The month index of what is now the "last month".
	 */
	void city_growth_monthly(uint32 const month);

public:
	/**
	 * Returns pointer to history for city
	 */
	sint64* get_city_history_year() { return *city_history_year; }
	sint64* get_city_history_month() { return *city_history_month; }

	uint32 stadtinfo_options;

	/* end of history related things */
private:
	sint32 best_haus_wert;
	sint32 best_strasse_wert;

	best_t best_haus;
	best_t best_strasse;

public:
	/**
	 * Classes for storing and manipulating target factories and their data
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
		sint32 demand;    // amount demanded by the factory; shifted by DEMAND_BITS
		sint32 supply;    // amount that the city can supply
		sint32 remaining; // portion of supply which has not realised yet; remaining <= supply

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
		sint32 total_demand;    // shifted by DEMAND_BITS
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
	 */
	factory_set_t target_factories_pax;
	factory_set_t target_factories_mail;

	/**
	 * Initialization of pax_destinations_old/new
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
	 */
	void step_grow_city(bool new_town = false);

	enum pax_return_type {
		no_return,
		factory_return,
		tourist_return,
		city_return
	};

	/**
	 * verteilt die Passagiere auf die Haltestellen
	 */
	void step_passagiere();

	/**
	 * ein Passagierziel in die Zielkarte eintragen
	 */
	void merke_passagier_ziel(koord ziel, PIXVAL color);

	/**
	 * baut Spezialgebaeude, z.B Stadion
	 */
	void check_bau_spezial(bool);

	/**
	 * baut ein angemessenes Rathaus
	 */
	void check_bau_townhall(bool);

	/**
	 * constructs a new consumer
	 */
	void check_bau_factory(bool);


	// find out, what building matches best
	void bewerte_res_com_ind(const koord pos, int &ind, int &com, int &res);

	/**
	 * Build/renovates a city building at Planquadrat (tile) x,y
	 */
	void build_city_building(koord pos);
	void renovate_city_building(gebaeude_t *gb);

#ifdef DESTINATION_CITYCARS
	sint16 number_of_cars; // allowed number of cars to spawn per month
	void generate_private_cars(koord pos, koord target);
#endif

protected:
	/**
	 * baut ein Stueck Strasse
	 * @param k Bauposition
	 */
	bool build_road(const koord k, player_t *player_, bool forced);

private:
	void build();

	/**
	 * @param pos position to check
	 * @param regel the rule to evaluate
	 * @return true on match, false otherwise
	 */
	static bool bewerte_loc(koord pos, const rule_t &regel, int rotation);

	/**
	 * Check rule in all transformations at given position
	 */
	static sint32 bewerte_pos(koord pos, const rule_t &regel);

	void bewerte_strasse(koord pos, sint32 rd, const rule_t &regel);
	void bewerte_haus(koord pos, sint32 rd, const rule_t &regel);

	/**
	 * Updates city limits: tile at @p pos belongs to city.
	 * @warning Do not call this during multithreaded loading!
	 */
	void pruefe_grenzen(koord pos, koord extend);

public:
	bool is_within_players_network( const player_t* player ) const;

	/// Connects factories to this city.
	void verbinde_fabriken();

	/**
	 * Returns the data set associated with the pax/mail target factories
	 */
	const factory_set_t& get_target_factories_for_pax() const { return target_factories_pax; }
	const factory_set_t& get_target_factories_for_mail() const { return target_factories_mail; }
	factory_set_t& access_target_factories_for_pax() { return target_factories_pax; }
	factory_set_t& access_target_factories_for_mail() { return target_factories_mail; }

	// calculated the "best" orietation of city buildings, also used by editor, thus public
	static int orient_city_building(const koord k, const building_desc_t *h, koord maxarea );

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
	*/
	sint64 get_finance_history_year(int year, int type) { return city_history_year[year][type]; }
	sint64 get_finance_history_month(int month, int type) { return city_history_month[month][type]; }

	// growth number (smoothed!)
	sint32 get_wachstum() const {return ((sint32)city_history_month[0][HIST_GROWTH]*5) + (sint32)(city_history_month[1][HIST_GROWTH]*4) + (sint32)city_history_month[2][HIST_GROWTH]; }

	/**
	 * ermittelt die Einwohnerzahl der City
	 */
	sint32 get_einwohner() const {return (buildings.get_sum_weight()*6)+((2*bev-arb-won)>>1);}

	uint32 get_buildings()  const { return buildings.get_count(); }
	sint32 get_unemployed() const { return bev - arb; }
	sint32 get_homeless()   const { return bev - won; }

	const char *get_name() const { return name; }
	void set_name( const char *name );

	/// @returns a random point within city borders.
	koord get_zufallspunkt() const;

	/// @returns passenger destination statistics for the last month
	const sparse_tpl<PIXVAL>* get_pax_destinations_old() const { return &pax_destinations_old; }

	/// @returns passenger destination statistics for the current month
	const sparse_tpl<PIXVAL>* get_pax_destinations_new() const { return &pax_destinations_new; }

	/* this counter will increment by one for every change
	 * => dialogs can question, if they need to update map
	 */
	uint32 get_pax_destinations_new_change() const { return pax_destinations_new_change; }

	/**
	 * Erzeugt eine neue City auf Planquadrat (x,y) die dem Spieler sp
	 * gehoert.
	 * @param player The owner of the city
	 * @param pos Planquadratkoordinate
	 * @param citizens number of citizens
	 */
	stadt_t(player_t* player, koord pos, sint32 citizens);

	/**
	 * Erzeugt eine neue City nach Angaben aus der Datei file.
	 * @param file Zeiger auf die Datei mit den Citybaudaten.
	 * @see stadt_t::speichern()
	 */
	stadt_t(loadsave_t *file);

	// closes window and that stuff
	~stadt_t();

	/**
	 * Speichert die Daten der City in der Datei file so, dass daraus
	 * die City wieder erzeugt werden kann. Die Gebaude und strassen der
	 * City werden nicht mit der City gespeichert sondern mit den
	 * Planquadraten auf denen sie stehen.
	 * @see stadt_t::stadt_t()
	 * @see planquadrat_t
	 */
	void rdwr(loadsave_t *file);

	/**
	 * Called when loading of savegame is finished to correctly init data.
	 */
	void finish_rd();

	void rotate90( const sint16 y_size );

	/* change size of city */
	void change_size( sint64 delta_citizens, bool new_town = false );

	// when ng is false, no town growth any more
	void set_citygrowth_yesno( bool ng ) { allow_citygrowth = ng; }
	bool get_citygrowth() const { return allow_citygrowth; }

	void step(uint32 delta_t);

	void new_month( bool recalc_destinations );

private:
	/**
	 * List of target cities weighted by both city size and distance
	 */
	weighted_vector_tpl<stadt_t *> target_cities;

	/**
	 * List of target attractions weighted by both passenger level and distance
	 */
	weighted_vector_tpl<gebaeude_t *> target_attractions;

public:
	/**
	 * Functions for manipulating the list of target cities
	 */
	void add_target_city(stadt_t *const city);
	void remove_target_city(stadt_t *const city) { target_cities.remove( city ); }
	void recalc_target_cities();

	/**
	 * Functions for manipulating the list of target attractions
	 */
	void add_target_attraction(gebaeude_t *const attraction);
	void remove_target_attraction(gebaeude_t *const attraction) { target_attractions.remove( attraction ); }
	void recalc_target_attractions();

	/**
	 * Search for a possible Passenger or Mail destination.
	 * @param target_factories the factory set to use (eg passenger or mail).
	 * @param generated number of passengers already generated, used to fairly distribute to factories.
	 * @param will_return set to the jounrey return type on return.
	 * @param factory_entry set to the destination factory, if any.
	 * @param dest_city set to the destination city for return flow use.
	 */
	koord find_destination(factory_set_t &target_factories, const sint64 generated, pax_return_type* will_return, factory_entry_t* &factory_entry, stadt_t* &dest_city);

	/**
	 * Gibt die Gruendungsposition der City zurueck.
	 * @return die Koordinaten des Gruendungsplanquadrates
	 */
	inline koord get_pos() const {return pos;}
	inline koord get_townhall_road() const {return townhall_road;}

	inline koord get_linksoben() const { return lo;}
	inline koord get_rechtsunten() const { return ur;}

	koord get_center() const { return lo/2 + ur/2; }

	/**
	 * Generates an array of random coordinates suitable for creating cities.
	 * Do not consider coordinates in (0,0) - (old_x, old_y)
	 * (leave @p old_x and @p old_y 0 to generate cities on the whole map).
	 * @param count how many cities to generate
	 */
	static vector_tpl<koord> *random_place(sint32 count, sint16 old_x, sint16 old_y);

	void open_info_window();
};

#endif
