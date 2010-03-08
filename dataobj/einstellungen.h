#ifndef dataobj_einstellungen_h
#define dataobj_einstellungen_h

#include "../utils/cstring_t.h"
#include "../simtypes.h"
#include "../simconst.h"

/**
 * Spieleinstellungen
 *
 * Hj. Malthaner
 *
 * April 2000
 */

class loadsave_t;
class tabfile_t;

class einstellungen_t
{
private:
	sint32 groesse_x, groesse_y;
	sint32 nummer;

	/* new setting since version 0.85.01
	 * @author prissi
	 * not used any more:    sint32 industrie_dichte;
	 */
	sint32 land_industry_chains;
	sint32 electric_promille;
	sint32 tourist_attractions;

	sint32 anzahl_staedte;
	sint32 mittlere_einwohnerzahl;

	// town growth factors
	sint32 passenger_multiplier;
	sint32 mail_multiplier;
	sint32 goods_multiplier;
	sint32 electricity_multiplier;

	// Also there are size dependen factors (0=no growth)
	sint32 growthfactor_small;
	sint32 growthfactor_medium;
	sint32 growthfactor_large;

	// percentage of routing
	sint16 factory_worker_percentage;
	sint16 tourist_percentage;
	sint16 factory_worker_radius;

	uint16 station_coverage_size;

	/**
	 * ab welchem level erzeugen gebaeude verkehr ?
	 */
	sint32 verkehr_level;

	/**
	 * sollen Fussgaenger angezeigt werden ?
	 */
	sint32 show_pax;

	 /**
	 * waterlevel, climate borders, lowest snow in winter
	 */

	sint16 grundwasser;
	sint16 climate_borders[MAX_CLIMATES];
	sint16 winter_snowline;

	double max_mountain_height;                  //01-Dec-01        Markus Weber    Added
	double map_roughness;                        //01-Dec-01        Markus Weber    Added

	// river stuff
	sint16 river_number;
	sint16 min_river_length;
	sint16 max_river_length;

	uint8 allow_player_change;
	uint8 use_timeline;
	sint16 starting_year;
	sint16 starting_month;
	sint16 bits_per_month;

	cstring_t filename;

	bool beginner_mode;
	sint32 beginner_price_factor;

	bool just_in_time;

	// default 0, will be incremented after each 90 degree rotation until 4
	uint8 rotation;

	sint16 origin_x, origin_y;

	sint32 passenger_factor;

	sint16 factory_spacing;

	/* prissi: crossconnect all factories (like OTTD and similar games) */
	bool crossconnect_factories;

	/* prissi: crossconnect all factories (like OTTD and similar games) */
	sint16 crossconnect_factor;

	/**
	* Zufällig Fussgänger in den Städten erzeugen?
	*
	* @author Hj. Malthaner
	*/
	bool fussgaenger;

	sint32 stadtauto_duration;

	bool freeplay;

	sint64 starting_money;

	typedef struct {
		sint16 year;
		sint64 money;
		bool interpol;
	} yearmoney;

	yearmoney startingmoneyperyear[10];

	/**
	 * Use numbering for stations?
	 *
	 * @author Hj. Malthaner
	 */
	bool numbered_stations;

	/**
	 * Typ (Name) initiale Stadtstrassen
	 *
	 * @author Hj. Malthaner
	 */
	char city_road_type[256];

	/* prissi: maximum number of steps for breath search */
	sint32 max_route_steps;

	// max steps for good routing
	sint32 max_hops;

	/* prissi: maximum number of steps for breath search */
	sint32 max_transfers;

	/* multiplier for steps on diagonal:
	 * 1024: TT-like, faktor 2, vehicle will be too long and too fast
	 * 724: correct one, faktor sqrt(2)
	 */
	uint16 pak_diagonal_multiplier;

	// names of the stations ...
	char language_code_names[4];

	// true, if the different caacities (passengers/mail/freight) are counted seperately
	bool seperate_halt_capacities;

	/**
	 * payment is only for the distance that got shorter between target and start
	 * three modes:
	 * 0 = pay for travelled manhattan distance
	 * 1 = pay for distance difference to next transfer stop
	 * 2 = pay for distance to destination
	 * 0 allows chaeting, but the income with 1 or two are much lower
	 */
	uint8 pay_for_total_distance;

	/* if set, goods will avoid being routed over overcrowded stops */
	bool avoid_overcrowding;

	/* if set, goods will not routed over overcroded stations but rather try detours (if possible) */
	bool no_routing_over_overcrowding;

	// true, if this pak should be used with extensions (default)
	bool with_private_paks;

	// if true, you can buy obsolete stuff
	bool allow_buying_obsolete_vehicles;

	uint32 random_counter;
	uint32 frames_per_second;	// only used in network mode ...
	uint32 frames_per_step;

public:
	/* the big cost section */
	sint32 maint_building;	// normal building

	sint64 cst_multiply_dock;
	sint64 cst_multiply_station;
	sint64 cst_multiply_roadstop;
	sint64 cst_multiply_airterminal;
	sint64 cst_multiply_post;
	sint64 cst_multiply_headquarter;
	sint64 cst_depot_rail;
	sint64 cst_depot_road;
	sint64 cst_depot_ship;
	sint64 cst_depot_air;

	// alter landscape
	sint64 cst_buy_land;
	sint64 cst_alter_land;
	sint64 cst_set_slope;
	sint64 cst_found_city;
	sint64 cst_multiply_found_industry;
	sint64 cst_remove_tree;
	sint64 cst_multiply_remove_haus;
	sint64 cst_multiply_remove_field;
	sint64 cst_transformer;
	sint64 cst_maintain_transformer;

	// costs for the way searcher
	sint32 way_count_straight;
	sint32 way_count_curve;
	sint32 way_count_double_curve;
	sint32 way_count_90_curve;
	sint32 way_count_slope;
	sint32 way_count_tunnel;
	uint32 way_max_bridge_len;
	sint32 way_count_leaving_road;

	// true if active
	bool automaten[MAX_PLAYER_COUNT];
	// 0 = emtpy, otherwise some vaule from simplay
	uint8 spieler_type[MAX_PLAYER_COUNT];

public:
	/**
	 * If map is read from a heightfield, this is the name of the heightfield.
	 * Set to empty string in order to avoid loading.
	 * @author Hj. Malthaner
	 */
	cstring_t heightfield;

	einstellungen_t();

	void rdwr(loadsave_t *file);

	// init form this file ...
	void parse_simuconf( tabfile_t &simuconf, sint16 &disp_width, sint16 &disp_height, sint16 &fullscreen, cstring_t &objfilename );

	void set_groesse_x(sint32 g) {groesse_x=g;}
	void set_groesse_y(sint32 g) {groesse_y=g;}
	void set_groesse(sint32 w,sint32 h) {groesse_x=w;groesse_y=h;}
	sint32 get_groesse_x() const {return groesse_x;}
	sint32 get_groesse_y() const {return groesse_y;}

	void set_karte_nummer(sint32 n) {nummer=n;}
	sint32 get_karte_nummer() const {return nummer;}

	void set_land_industry_chains(sint32 d) {land_industry_chains=d;}
	sint32 get_land_industry_chains() const {return land_industry_chains;}

	void set_electric_promille(sint32 d) { electric_promille=d;}
	sint32 get_electric_promille() const {return electric_promille;}

	void set_tourist_attractions(sint32 d) {tourist_attractions=d;}
	sint32 get_tourist_attractions() const {return tourist_attractions;}

	void set_anzahl_staedte(sint32 n) {anzahl_staedte=n;}
	sint32 get_anzahl_staedte() const {return anzahl_staedte;}

	void set_mittlere_einwohnerzahl(sint32 n) {mittlere_einwohnerzahl=n;}
	sint32 get_mittlere_einwohnerzahl() const {return mittlere_einwohnerzahl;}

	void set_verkehr_level(sint32 l) {verkehr_level=l;}
	sint32 get_verkehr_level() const {return verkehr_level;}

	void set_show_pax(bool yesno) {show_pax=yesno;}
	bool get_show_pax() const {return show_pax != 0;}

	void set_grundwasser(sint32 n) {grundwasser=n;}
	sint32 get_grundwasser() const {return grundwasser;}

	void set_max_mountain_height(double n) {max_mountain_height=n;}          //01-Dec-01        Markus Weber    Added
	double get_max_mountain_height() const {return max_mountain_height;}

	void set_map_roughness(double n) {map_roughness=n;}                      //01-Dec-01        Markus Weber    Added
	double get_map_roughness() const {return map_roughness;}

	void set_station_coverage(unsigned short n) {station_coverage_size=n;}	// prissi, May-2005
	uint16 get_station_coverage() const {return station_coverage_size;}

	void set_allow_player_change(char n) {allow_player_change=n;}	// prissi, Oct-2005
	uint8 get_allow_player_change() const {return allow_player_change;}

	void set_use_timeline(char n) {use_timeline=n;}	// prissi, Oct-2005
	uint8 get_use_timeline() const {return use_timeline;}

	void set_starting_year(short n) {starting_year=n;}	// prissi, Oct-2005
	sint16 get_starting_year() const {return starting_year;}

	void set_starting_month(short n) {starting_month=n;}
	sint16 get_starting_month() const {return starting_month;}

	void set_bits_per_month(short n) {bits_per_month=n;}	// prissi, Oct-2005
	sint16 get_bits_per_month() const {return bits_per_month;}

	void set_filename(const char *n) {filename=n;}	// prissi, Jun-06
	const char* get_filename() const { return filename; }

	void set_beginner_mode(bool yesno) {beginner_mode=yesno;}	// prissi, Aug-06
	bool get_beginner_mode() const {return beginner_mode;}

	void set_just_in_time(bool yesno) {just_in_time=yesno;}	// prissi, Aug-06
	bool get_just_in_time() const {return just_in_time;}

	void set_default_climates();	// will reanble the new borders assigned below to the array
	const sint16 *get_climate_borders() { return climate_borders; }

	void set_winter_snowline(sint16 sl) { winter_snowline = sl; }
	sint16 get_winter_snowline() const {return winter_snowline;}

	void rotate90() { rotation = (rotation+1)&3; set_groesse( groesse_y, groesse_x ); }
	uint8 get_rotation() const { return rotation; }

	void set_origin_x(sint16 x) { origin_x = x; }
	void set_origin_y(sint16 y) { origin_y = y; }
	sint16 get_origin_x() const { return origin_x; }
	sint16 get_origin_y() const { return origin_y; }

	bool is_freeplay() const { return freeplay; }
	void set_freeplay( bool f ) { freeplay = f; }

	sint32 get_max_route_steps() const { return max_route_steps; }
	void set_max_route_steps(sint32 m) { max_route_steps=m; }
	sint32 get_max_hops() const { return max_hops; }
	void set_max_hops(sint32 m) { max_hops=m; }
	sint32 get_max_transfers() const { return max_transfers; }
	void set_max_transfers(sint32 m) { max_transfers=m; }

	sint64 get_starting_money(sint16 year) const;
	void set_starting_money(sint64 s) { starting_money = s; }

	bool get_random_pedestrians() const { return fussgaenger; }
	void set_random_pedestrians( bool f ) { fussgaenger = f; }

	sint16 get_factory_spacing() const { return factory_spacing; }
	void set_factory_spacing(sint16 s) { factory_spacing = s; }
	sint16 get_crossconnect_factor() const { return crossconnect_factor; }
	void set_crossconnect_factor(sint16 s) { crossconnect_factor = s; }
	bool is_crossconnect_factories() const { return crossconnect_factories; }
	void set_crossconnect_factories( bool f ) { crossconnect_factories = f; }

	bool get_numbered_stations() const { return numbered_stations; }
	void set_numbered_stations(bool b) { numbered_stations = b; }

	sint32 get_stadtauto_duration() const { return stadtauto_duration; }
	void set_stadtauto_duration(sint32 d) { stadtauto_duration = d; }

	sint32 get_beginner_price_factor() const { return beginner_price_factor; }
	void set_beginner_price_factor(sint32 s) { beginner_price_factor = s; }

	const char *get_city_road_type() const { return city_road_type; }

	uint16 get_pak_diagonal_multiplier() const { return pak_diagonal_multiplier; }
	void set_pak_diagonal_multiplier( uint16 pdm ) { pak_diagonal_multiplier = pdm; }

	const char *get_name_language_iso() const { return language_code_names; }
	void set_name_language_iso( const char *iso ) {
		language_code_names[0] = iso[0];
		language_code_names[1] = iso[1];
		language_code_names[2] = 0;
	}

	void set_player_active(uint8 i, bool b) { automaten[i] = b; }

	uint8 get_player_type(uint8 i) const { return spieler_type[i]; }
	void set_player_type(uint8 i, uint8 t) { spieler_type[i] = t; }

	bool is_seperate_halt_capacities() const { return seperate_halt_capacities ; }
	void set_seperate_halt_capacities( bool b ) { seperate_halt_capacities = b; }

	// allowed modes are 0,1,2
	enum { TO_PREVIOUS=0, TO_TRANSFER, TO_DESTINATION };
	uint8 get_pay_for_total_distance_mode() const { return pay_for_total_distance ; }
	void set_pay_for_total_distance_mode( uint8 b ) { pay_for_total_distance = b < 2 ? b : 0; }

	// do not take people to overcrowded destinations
	bool is_avoid_overcrowding() const { return avoid_overcrowding; }
	void set_avoid_overcrowding( bool b ) { avoid_overcrowding = b; }

	// do not allow routes over overcrowded destinations
	bool is_no_routing_over_overcrowding() const { return no_routing_over_overcrowding; }
	void set_no_routing_over_overcrowding( bool b ) { no_routing_over_overcrowding = b; }

	sint16 get_river_number() const { return river_number; }
	void set_river_number( sint16 n ) { river_number=n; }
	sint16 get_min_river_length() const { return min_river_length; }
	void set_min_river_length( sint16 n ) { min_river_length=n; }
	sint16 get_max_river_length() const { return max_river_length; }
	void set_max_river_length( sint16 n ) { max_river_length=n; }

	// true, if this pak should be used with extensions (default)
	bool get_with_private_paks() const { return with_private_paks; }
	void set_with_private_paks(bool b) { with_private_paks = b; }

	sint32 get_passenger_factor() const { return passenger_factor; }
	void set_passenger_factor(sint32 n) { passenger_factor = n; }

	// town growth stuff
	sint32 get_passenger_multiplier() const { return passenger_multiplier; }
	void set_passenger_multiplier(sint32 n) { passenger_multiplier = n; }
	sint32 get_mail_multiplier() const { return mail_multiplier; }
	void set_mail_multiplier(sint32 n) { mail_multiplier = n; }
	sint32 get_goods_multiplier() const { return goods_multiplier; }
	void set_goods_multiplier(sint32 n) { goods_multiplier = n; }
	sint32 get_electricity_multiplier() const { return electricity_multiplier; }
	void set_electricity_multiplier(sint32 n) { electricity_multiplier = n; }

	// Also there are size dependen factors (0=no growth)
	sint32 get_growthfactor_small() const { return growthfactor_small; }
	void set_growthfactor_small(sint32 n) { growthfactor_small = n; }
	sint32 get_growthfactor_medium() const { return growthfactor_medium; }
	void set_growthfactor_medium(sint32 n) { growthfactor_medium = n; }
	sint32 get_growthfactor_large() const { return growthfactor_large; }
	void set_growthfactor_large(sint32 n) { growthfactor_large = n; }

	// amount of different destinations
	sint32 get_factory_worker_percentage() const { return factory_worker_percentage; }
	void set_factory_worker_percentage(sint32 n) { factory_worker_percentage = n; }
	sint32 get_tourist_percentage() const { return tourist_percentage; }
	void set_tourist_percentage(sint32 n) { tourist_percentage = n; }

	// radius within factories belog to towns (usually set to 77 but 1/8 of map size may be meaningful too)
	sint32 get_factory_worker_radius() const { return factory_worker_radius; }
	void set_factory_worker_radius(sint32 n) { factory_worker_radius = n; }

	// disallow using obsolete vehicles in depot
	bool get_allow_buying_obsolete_vehicles() const { return allow_buying_obsolete_vehicles; }
	void set_allow_buying_obsolete_vehicles(bool n) { allow_buying_obsolete_vehicles = n; }

	// usually only used in network mode => no need to set them!
	uint32 get_random_counter() const { return random_counter; }
	uint32 get_frames_per_second() const { return frames_per_second; }
	uint32 get_frames_per_step() const { return frames_per_step; }
};

#endif
