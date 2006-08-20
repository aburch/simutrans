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
	*
	* @author prissi
	*/
	static bool station_coverage_show;
	static int station_coverage_size;


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
	* Wasser (Boden) animieren?
	*
	* @author Hj. Malthaner
	*/
	static bool bodenanimation;


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
	static int starting_money;


	/**
	* Wartungskosten für Gebäude
	*
	* @author Hj. Malthaner
	*/
	static int maint_building;


	/**
	* Wartungskosten für Wege
	*
	* @author Hj. Malthaner
	*/
	static int maint_way;


	/**
	* Wartungskosten für Oberleitungen
	*
	* @author Hj. Malthaner
	*/
	static int maint_overhead;


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
	static sint8 show_month;

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


	/* prissi: maximum number of steps for breath search */
	static int max_transfers;


	/* prissi: do autosave every month? */
	static int autosave;

	/* prissi: crossconnect all factories (like OTTD and similar games) */
	static bool crossconnect_factories;

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
	static sint64 cst_alter_land;
	static sint64 cst_set_slope;
	static sint64 cst_found_city;
	static sint64 cst_multiply_found_industry;
	static sint64 cst_remove_tree;
	static sint64 cst_multiply_remove_haus;

	// costs for the way searcher
	static sint32 way_count_curve;
	static sint32 way_count_double_curve;
	static sint32 way_count_90_curve;
	static sint32 way_count_slope;
	static sint32 way_count_tunnel;
	static uint32 way_max_bridge_len;

	// passenger manipulation factor (=16 about old value)
	static uint32 passenger_factor;
};

#endif
