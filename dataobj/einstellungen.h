#ifndef dataobj_einstellungen_h
#define dataobj_einstellungen_h

#include "../utils/cstring_t.h"
#include "../simtypes.h"
#include "../simconst.h"
#include "tabfile.h"

/**
 * Spieleinstellungen
 *
 * Hj. Malthaner
 *
 * April 2000
 */

class loadsave_t;

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

	sint32 scroll_multi;

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

	unsigned char allow_player_change;
	unsigned char use_timeline;
	sint16 starting_year;
	sint16 bits_per_month;

	cstring_t filename;

	bool beginner_mode;
	sint32 beginner_price_factor;

	bool just_in_time;

	// default 0, will be incremented after each 90 degree rotation until 4
	uint8 rotation;

	sint16 origin_x, origin_y;

	bool no_trees;

	sint32 passenger_factor;

	sint32 default_electric_promille;

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
	const char *city_road_type;

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
	const char *language_code_names;

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
	sint64 cst_signal;
	sint64 cst_tunnel;
	sint64 cst_third_rail;

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
	// NULL if not password
	const char *password[MAX_PLAYER_COUNT];

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

	void setze_groesse_x(sint32 g) {groesse_x=g;}
	void setze_groesse_y(sint32 g) {groesse_y=g;}
	void setze_groesse(sint32 w,sint32 h) {groesse_x=w;groesse_y=h;}
	sint32 gib_groesse_x() const {return groesse_x;}
	sint32 gib_groesse_y() const {return groesse_y;}

	void setze_karte_nummer(sint32 n) {nummer=n;}
	sint32 gib_karte_nummer() const {return nummer;}

	void setze_land_industry_chains(sint32 d) {land_industry_chains=d;}
	sint32 gib_land_industry_chains() const {return land_industry_chains;}

	void setze_electric_promille(sint32 d) {electric_promille=d;}
	sint32 gib_electric_promille() const {return electric_promille;}

	void setze_tourist_attractions(sint32 d) {tourist_attractions=d;}
	sint32 gib_tourist_attractions() const {return tourist_attractions;}

	void setze_anzahl_staedte(sint32 n) {anzahl_staedte=n;}
	sint32 gib_anzahl_staedte() const {return anzahl_staedte;}

	void setze_mittlere_einwohnerzahl(sint32 n) {mittlere_einwohnerzahl=n;}
	sint32 gib_mittlere_einwohnerzahl() const {return mittlere_einwohnerzahl;}

	void setze_verkehr_level(sint32 l) {verkehr_level=l;}
	sint32 gib_verkehr_level() const {return verkehr_level;}

	void setze_show_pax(bool yesno) {show_pax=yesno;}
	bool gib_show_pax() const {return show_pax != 0;}

	void setze_grundwasser(sint32 n) {grundwasser=n;}
	sint32 gib_grundwasser() const {return grundwasser;}

	void setze_max_mountain_height(double n) {max_mountain_height=n;}          //01-Dec-01        Markus Weber    Added
	double gib_max_mountain_height() const {return max_mountain_height;}

	void setze_map_roughness(double n) {map_roughness=n;}                      //01-Dec-01        Markus Weber    Added
	double gib_map_roughness() const {return map_roughness;}

	void setze_station_coverage(unsigned short n) {station_coverage_size=n;}	// prissi, May-2005
	unsigned short gib_station_coverage() const {return station_coverage_size;}

	void setze_allow_player_change(char n) {allow_player_change=n;}	// prissi, Oct-2005
	unsigned char gib_allow_player_change() const {return allow_player_change;}

	void setze_use_timeline(char n) {use_timeline=n;}	// prissi, Oct-2005
	unsigned char gib_use_timeline() const {return use_timeline;}

	void setze_starting_year(short n) {starting_year=n;}	// prissi, Oct-2005
	short gib_starting_year() const {return starting_year;}

	void setze_bits_per_month(short n) {bits_per_month=n;}	// prissi, Oct-2005
	short gib_bits_per_month() const {return bits_per_month;}

	void setze_filename(const char *n) {filename=n;}	// prissi, Jun-06
	const char* gib_filename() const { return filename; }

	void setze_beginner_mode(bool yesno) {beginner_mode=yesno;}	// prissi, Aug-06
	bool gib_beginner_mode() const {return beginner_mode;}

	void setze_just_in_time(bool yesno) {just_in_time=yesno;}	// prissi, Aug-06
	bool gib_just_in_time() const {return just_in_time;}

	void setze_climate_border(sint16 *);	// prissi, Aug-06
	const sint16 *gib_climate_borders() const {return climate_borders;}

	void setze_winter_snowline(sint16 sl) { winter_snowline = sl; }
	sint16 gib_winter_snowline() const {return winter_snowline;}

	void rotate90() { rotation = (rotation+1)&3; }
	uint8 get_rotation() const { return rotation; }

	void setze_origin_x(sint16 x) { origin_x = x; }
	void setze_origin_y(sint16 y) { origin_y = y; }
	sint16 get_origin_x() const { return origin_x; }
	sint16 get_origin_y() const { return origin_y; }

	bool is_freeplay() const { return freeplay; }
	void setze_freeplay( bool f ) { freeplay = f; }

	sint32 gib_max_route_steps() const { return max_route_steps; }
	sint32 gib_max_hops() const { return max_hops; }
	sint32 gib_max_transfers() const { return max_transfers; }

	sint64 gib_starting_money() const { return starting_money; }

	bool gib_random_pedestrians() const { return fussgaenger; }
	void setze_random_pedestrians( bool f ) { fussgaenger = f; }	// NETWORK!

	sint16 gib_factory_spacing() const { return factory_spacing; }
	sint16 gib_crossconnect_factor() const { return crossconnect_factor; }
	bool is_crossconnect_factories() const { return crossconnect_factories; }

	sint32 gib_passenger_factor() const { return passenger_factor; }

	sint32 gib_numbered_stations() const { return numbered_stations; }

	sint32 gib_stadtauto_duration() const { return stadtauto_duration; }

	sint32 gib_beginner_price_factor() const { return beginner_price_factor; }

	const char *gib_city_road_type() const { return city_road_type; }

	uint16 gib_pak_diagonal_multiplier() const { return pak_diagonal_multiplier; }
	void setze_pak_diagonal_multiplier( uint16 pdm ) { pak_diagonal_multiplier = pdm; }

	const char *gib_name_language_iso() const { return language_code_names; }
	// will fail horribly with NULL pointer for iso ...
	void setze_name_language_iso( const char *iso ) { delete language_code_names; language_code_names = strdup(iso); }

	void set_player_active(uint8 i, bool b) { automaten[i] = b; }
	void set_player_type(uint8 i, uint8 t) { spieler_type[i] = t; }
};

#endif
