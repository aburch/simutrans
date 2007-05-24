/* umgebung.h
 *
 * Hier werden die Kommandozeilenparametr in für das Spiel
 * nutzbarer Form gespeichert.
 *
 * von Hansjörg Malthaner, November 2000
 */

#ifndef dataobj_umgebung_h
#define dataobj_umgebung_h

#include "../simtypes.h"

class cstring_t;

/**
 * Diese Klasse bildet eine Abstraktion der Kommandozeilenparameter.
 * Alle Attribute sind statisch, damit sie überall zugänglich sind.
 * Das ist kein Problem, denn sie existieren garantiert nur einmal!
 *
 * @author Hj. Malthaner
 */
class umgebung_t
{
public:
	// points to the current simutrans data directory
	static char program_dir[1024];

	//points to the current directory user for loading and saving
	static const char *user_dir;

	static cstring_t objfilename;

	// maximum number of handles
	static uint16 max_convoihandles;
	static uint16 max_linehandles;
	static uint16 max_halthandles;

	/**
	* bei Testläufen wird sofort eine standardkarte erzeugt
	*
	* @author Hj. Malthaner
	*/
	static bool testlauf;

	/**
	* im Freispiel gibt es keine Limits wie im echten Spiel, z.B.
	* kann das Konto unbegernzt überzogen werden
	*
	* @author Hj. Malthaner
	*/
	static bool freeplay;

	/**
	* tag-nacht wechsel zeigen ?
	*
	* @author Hj. Malthaner
	*/
	static bool night_shift;

	/**
	* Stationsabdeckung zeigen
	* @author prissi
	*/
	static bool use_transparency_station_coverage;
	static uint8 station_coverage_show;
	enum { NOT_SHOWN_COVERAGE=0, SHOW_MY_COVERAGE, SHOW_ALL_COVERAGE };
	static int station_coverage_size;

	// use transparency to hide buildings and trees
	static bool hide_with_transparency;

	/**
	 * three states:
	 */
	enum { NOT_HIDE=0, SOME_HIDDEN_BUIDLING, ALL_HIDDEN_BUIDLING };
	static uint8 hide_buildings;

	/**
	 * Set to true to hide all trees. "Hiding" is implemented by showing the
	 * first pic which should be very small.
	 * @author Volker Meyer
	 * @date  10.06.2003
	 */
	static bool hide_trees;

	/**
	* Namen (Städte, Haltestellen) anzeigen? (0 .. 3)
	*
	* @author Hj. Malthaner
	*/
	static int show_names;

	/**
	* Welche KIs sollen bei neuen Spielen aktiviert werden?
	*
	* @author V. Meyer
	*/
	static bool automaten[6];

	/**
	* which messages to display where?
	*
	* @author prissi
	*/
	static int message_flags[4];

	/**
	* Zufällig Fussgänger in den Städten erzeugen?
	*
	* @author Hj. Malthaner
	*/
	static bool fussgaenger;

	/**
	* Info-Fenster für Fussgänger und Privatfahrzeuge
	*
	* @author Hj. Malthaner
	*/
	static bool verkehrsteilnehmer_info;

	/* How many tiles can a simutrans car go, before it forever break ...
	* @author prissi
	*/
	static long stadtauto_duration;

	/**
	* Info-Fenster für Bäume
	* @author prissi
	*/
	static bool tree_info;

	/**
	* Info-Fenster for all grounds
	* @author prissi
	*/
	static bool ground_info;

	/**
	* Info-Fenster für Townhall
	* @author prissi
	*/
	static bool townhall_info;

	/**
	* Only one info window
	* @author prissi
	*/
	static bool single_info;

	/**
	* Only one info window
	* @author prissi
	*/
	static bool window_buttons_right;

	/**
	* Produce more debug info ?
	*
	* @author Hj. Malthaner
	*/
	static bool verbose_debug;


	/**
	* Startkapital für Spieler
	*
	* @author Hj. Malthaner
	*/
	static sint64 starting_money;


	/**
	* Wartungskosten für Gebäude
	*
	* @author Hj. Malthaner
	*/
	static sint32 maint_building;


	/**
	* Wartungskosten für Wege
	*
	* @author Hj. Malthaner
	*/
	static sint32 maint_way;

	/**
	* Use numbering for stations?
	*
	* @author Hj. Malthaner
	*/
	static bool numbered_stations;


	/**
	* Max. Länge für initiale Stadtverbindungen
	*
	* @author Hj. Malthaner
	*/
	static int intercity_road_length;


	/**
	* Typ (Name) initiale Stadtverbindungen
	*
	* @author Hj. Malthaner
	*/
	static cstring_t * intercity_road_type;

	/**
	 * Typ (Name) initiale Stadtstrassen
	 *
	 * @author Hj. Malthaner
	 */
	static cstring_t * city_road_type;

	/**
	* Should the timeline be activated?
	*
	* @author Hj. Malthaner
	*/
	static char use_timeline;

	/**
	* show month in date?
	*
	* @author hsiegeln
	*/
	static uint8 show_month;

	/**
	* Starting year of the game
	*
	* @author Hj. Malthaner
	*/
	static sint16 starting_year;

	/**
	* 1<<bits_per_month is the duration of a day in ms
	*
	* @author Hj. Malthaner
	*/
	static sint16 bits_per_month;


	/* prissi: maximum number of steps for breath search */
	static int max_route_steps;

	// max steps for good routing
	static int set_max_hops;

	/* prissi: maximum number of steps for breath search */
	static int max_transfers;

	/* prissi: do autosave every month? */
	static int autosave;

	/* prissi: crossconnect all factories (like OTTD and similar games) */
	static bool crossconnect_factories;

	/* prissi: crossconnect all factories (like OTTD and similar games) */
	static sint16 crossconnect_factor;

	/* prissi: do not distribute goods to overflowing factories */
	static bool just_in_time;

	/* prissi: drive on the left side of the road */
	static bool drive_on_left;

	/* the big cost section */
	static sint64 cst_multiply_dock;
	static sint64 cst_multiply_station;
	static sint64 cst_multiply_roadstop;
	static sint64 cst_multiply_airterminal;
	static sint64 cst_multiply_post;
	static sint64 cst_multiply_headquarter;
	static sint64 cst_depot_rail;
	static sint64 cst_depot_road;
	static sint64 cst_depot_ship;
//	static sint64 cst_depot_air;	// unused
	static sint64 cst_signal;
	static sint64 cst_tunnel;
	static sint64 cst_third_rail;
	// alter landscape
	static sint64 cst_buy_land;
	static sint64 cst_alter_land;
	static sint64 cst_set_slope;
	static sint64 cst_found_city;
	static sint64 cst_multiply_found_industry;
	static sint64 cst_remove_tree;
	static sint64 cst_multiply_remove_haus;
	static sint64 cst_multiply_remove_field;
	static sint64 cst_transformer;
	static sint64 cst_maintain_transformer;

	// costs for the way searcher
	static sint32 way_count_straight;
	static sint32 way_count_curve;
	static sint32 way_count_double_curve;
	static sint32 way_count_90_curve;
	static sint32 way_count_slope;
	static sint32 way_count_tunnel;
	static uint32 way_max_bridge_len;
	static sint32 way_count_leaving_road;

	// passenger manipulation factor (=16 about old value)
	static uint32 passenger_factor;

	// changing the prices of all goods
	static uint32 beginner_price_factor;

	// default beginner mode in a new map
	static bool beginner_mode_first;

	// current climate borders
	static sint16 climate_borders[MAX_CLIMATES];
	static sint16 winter_snowline;	// summer snowline is obviously just the artic climate ...

	// set the frame rate for the display
	static sint16 fps;

	// maximum acceleration with fast forward
	static sint16 max_acceleration;

	// false to quit the programs
	static bool quit_simutrans;
};

#endif
