/**
 * Header Datei für dir convoi_t Klasse für Fahrzeugverbände
 * von Hansjörg Malthaner
 */

#ifndef simconvoi_h
#define simconvoi_h

#include "simtypes.h"
#include "linehandle_t.h"

#include "ifc/sync_steppable.h"

#include "dataobj/route.h"
#include "vehicle/overtaker.h"
#include "tpl/array_tpl.h"
#include "tpl/minivec_tpl.h"

#include "convoihandle_t.h"
#include "halthandle_t.h"

#define MAX_CONVOI_COST   6 // Total number of cost items
#define MAX_MONTHS     12 // Max history
#define MAX_CONVOI_NON_MONEY_TYPES 2 // number of non money types in convoi's financial statistic
#define CONVOI_CAPACITY   0 // the amount of ware that could be transported, theoretically
#define CONVOI_TRANSPORTED_GOODS 1 // the amount of ware that has been transported
#define CONVOI_REVENUE		2 // the income this CONVOI generated
#define CONVOI_OPERATIONS         3 // the cost of operations this CONVOI generated
#define CONVOI_PROFIT             4 // total profit of this convoi
#define CONVOI_DISTANCE           5 // total distance traveld this month

class depot_t;
class karte_t;
class spieler_t;
class convoi_info_t;
class vehikel_t;
class vehikel_besch_t;
class schedule_t;
class cbuffer_t;

/**
 * Basisklasse für alle Fahrzeugverbände. Convois könnnen über Zeiger
 * oder Handles angesprochen werden. Zeiger sind viel schneller, dafür
 * können Handles geprüft werden, ob das Ziel noch vorhanden ist.
 *
 * @author Hj. Malthaner
 */
class convoi_t : public sync_steppable, public overtaker_t
{
public:
	/* Konstanten
	* @author prissi
	*/
	enum { max_vehicle=4, max_rail_vehicle = 24 };

	enum states {INITIAL,
		FAHRPLANEINGABE,
		ROUTING_1,
		DUMMY4,
		DUMMY5,
		NO_ROUTE,
		DRIVING,
		LOADING,
		WAITING_FOR_CLEARANCE,
		WAITING_FOR_CLEARANCE_ONE_MONTH,
		CAN_START,
		CAN_START_ONE_MONTH,
		SELF_DESTRUCT,
		WAITING_FOR_CLEARANCE_TWO_MONTHS,
		CAN_START_TWO_MONTHS,
		LEAVING_DEPOT,
		ENTERING_DEPOT,
		MAX_STATES
	};

private:
	/**
	* Route of this convoi - a sequence of coordinates. Actually
	* the path of the first vehicle
	* @author Hj. Malthaner
	*/
	route_t route;

	/**
	* assigned line
	* @author hsiegeln
	*/
	linehandle_t line;
	uint16 line_id;

	/**
	* holds id of line with pendig update
	* -1 if no pending update
	* @author hsiegeln
	*/
	linehandle_t line_update_pending;

	/**
	* Name of the convoi.
	* @see set_name
	* @author V. Meyer
	*/
	uint8 name_offset;
	char name_and_id[128];

	/**
	* Information window for ourselves.
	* @author Hj. Malthaner
	*/
	convoi_info_t *convoi_info;

	/**
	* Alle vehikel-fahrplanzeiger zeigen hierauf
	* @author Hj. Malthaner
	*/
	schedule_t *fpl;

	/**
	* loading_level was ladegrad before. Actual percentage loaded for loadable vehicles (station length!).
	* needed as int, since used by the gui
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	sint32 loading_level;

	/**
	* At which loading level is the train allowed to start? 0 during driving.
	* needed as int, since used by the gui
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	sint32 loading_limit;

	/**
	* The vehicles of this convoi
	*
	* @author Hj. Malthaner
	*/
	array_tpl<vehikel_t*> fahr;

	/*
	 * a list of all catg_index, which can be transported by this convoy.
	 */
	minivec_tpl<uint8> goods_catg_index;

	/**
	* Convoi owner
	* @author Hj. Malthaner
	*/
	spieler_t *besitzer_p;

	/**
	* Current map
	* @author Hj. Malthaner
	*/
	static karte_t *welt;

 	/**
	* the convoi is being withdrawn from service
	* @author kierongreen
	*/
	bool withdraw;

 	/**
	* nothing will be loaded onto this convoi
	* @author kierongreen
	*/
	bool no_load;

	/**
	* the convoi caches its freight info; it is only recalculation after loading or resorting
	* @author prissi
	*/
	bool freight_info_resort;

	// true, if at least one vehicle of a convoi is obsolete
	bool has_obsolete;

	// ture, if there is at least one engine that requires catenary
	bool is_electric;

	/**
	* the convoi caches its freight info; it is only recalculation after loading or resorting
	* @author prissi
	*/
	uint8 freight_info_order;

	/**
	* Number of vehicles in this convoi.
	* @author Hj. Malthaner
	*/
	uint8 anz_vehikel;

	/* Number of steps the current convoi did already
	 * (only needed for leaving/entering depot)
	 */
	sint16 steps_driven;

	/**
	* Gesamtleistung. Wird nicht gespeichert, sondern aus den Einzelleistungen
	* errechnet.
	* @author Hj. Malthaner
	*/
	uint32 sum_leistung;

	/**
	* Gesamtleistung mit Gear. Wird nicht gespeichert, sondern aus den Einzelleistungen
	* errechnet.
	* @author prissi
	*/
	sint32 sum_gear_und_leistung;

	/* sum_gewicht: leergewichte aller vehicles *
	* sum_gesamtgewicht: gesamtgewichte aller vehicles *
	* Werden nicht gespeichert, sondern aus den Einzelgewichten
	* errechnet beim beladen/fahren.
	* @author Hj. Malthaner, prissi
	*/
	sint32 sum_gewicht;
	sint32 sum_gesamtgewicht;

	// cached values
	// will be recalculated if
	// recalc_data is true
	bool recalc_data;
	sint32 sum_friction_weight;
	uint32 speed_limit;


	/**
	* Lowest top speed of all vehicles. Doesn't get saved, but calculated
	* from the vehicles data
	* @author Hj. Malthaner
	*/
	sint32 min_top_speed;

	/**
	* this give the index of the next signal or the end of the route
	* convois will slow down before it, if this is not a waypoint or the cannot pass
	* The slowdown ist done by the vehicle routines
	* @author prissi
	*/
	uint16 next_stop_index;

	/**
	* manchmal muss eine bestimmte Zeit gewartet werden.
	* wait_lock bestimmt wie lange gewartet wird (in ms).
	* @author Hanjsörg Malthaner
	*/
	sint32 wait_lock;

	/**
	* time, when a convoi waiting for full load will drive on
	* @author prissi
	*/
	uint32 go_on_ticks;

	/**
	* akkumulierter gewinn über ein jahr hinweg
	* @author Hanjsörg Malthaner
	*/
	sint64 jahresgewinn;

	/* the odometer */
	sint64 total_distance_traveled;

	/**
	* Set, when there was a income calculation (avoids some cheats)
	* Since 99.15 it will stored directly in the vehikel_t
	* @author prissi
	*/
	koord3d last_stop_pos;

	// things for the world record
	sint32 max_record_speed; // current convois fastest speed ever
	koord record_pos;

	// needed for speed control/calculation
	sint32 akt_speed_soll;    // target speed
	sint32 akt_speed;	        // current speed
	sint32 sp_soll;           // steps to go
	sint32 previous_delta_v;  // Stores the previous delta_v value; otherwise these digits are lost during calculation and vehicle do not accelrate

	uint32 next_wolke;	// time to next smoke

	states state;

	ribi_t::ribi alte_richtung;

	/**
	* Initialize all variables with default values.
	* Each constructor must call this method first!
	* @author Hj. Malthaner
	*/
	void init(karte_t *welt, spieler_t *sp);

	/**
	* Berechne route von Start- zu Zielkoordinate
	* @author Hanjsörg Malthaner
	*/
	bool drive_to();

	/**
	* Setup vehicles for moving in same direction than before
	* if the direction is the same as before
	* @author Hanjsörg Malthaner
	*/
	bool can_go_alte_richtung();

	/**
	* Mark first and last vehicle.
	* @author Hanjsörg Malthaner
	*/
	void set_erstes_letztes();

	// returns the index of the vehikel at position length (16=1 tile)
	int get_vehicle_at_length(uint16);

	/**
	* calculate income for last hop
	* only used for entering depot or recalculating routes when a schedule window is opened
	* @author Hj. Malthaner
	*/
	void calc_gewinn();

	/**
	* Recalculates loading level and limit.
	* While driving loading_limit will be set to 0.
	* @author Volker Meyer
	* @date  20.06.2003
	*/
	void calc_loading();

	/* Calculates (and sets) akt_speed
	 * needed for driving, entering and leaving a depot)
	 */
	void calc_acceleration(long delta_t);

	/**
	* Convoi haelt an Haltestelle und setzt quote fuer Fracht
	* @author Hj. Malthaner
	*/
	void hat_gehalten(koord k, halthandle_t halt);

	/*
	* struct holds new financial history for convoi
	* @author hsiegeln
	*/
	sint64 financial_history[MAX_MONTHS][MAX_CONVOI_COST];

	/**
	* initialize the financial history
	* @author hsiegeln
	*/
	void init_financial_history();

	/**
	* the koordinate of the home depot of this convoi
	* the last depot visited is considered beeing the home depot
	* @author hsiegeln
	*/
	koord3d home_depot;

	/**
	* unset line -> remove cnv from line
	* @author hsiegeln
	*/
	void unset_line();

	void check_pending_updates();

public:
	route_t* get_route() { return &route; }

	/**
	* Checks if this convoi has a driveable route
	* @author Hanjsörg Malthaner
	*/
	bool hat_keine_route() const { return (state==NO_ROUTE); }

	/**
	* get line
	* @author hsiegeln
	*/
	linehandle_t get_line() const {return line;}

	/* true, if electrification needed for this convoi */
	bool needs_electrification() const { return is_electric; }

	/**
	* set line
	* @author hsiegeln
	*/
	void set_line(linehandle_t );

	/**
	* registers the convoy with a line, but does not apply the line's fahrplan!
	* used only during convoi restoration from savegame!
	* @author hsiegeln
	*/
	void register_with_line(uint16 line_id);

	/* changes the state of a convoi via werkzeug_t; mandatory for networkmode! *
	 * for list of commands and parameter see werkzeug_t::wkz_change_convoi_t
	 */
	void call_convoi_tool( const char function, const char *extra ) const;

	/**
	* get state
	* @author hsiegeln
	*/
	int get_state() { return state; }

	/**
	* true if in waiting state (maybe also due to starting)
	* @author hsiegeln
	*/
	bool is_waiting() { return (state>=WAITING_FOR_CLEARANCE  &&  state<=CAN_START_ONE_MONTH)  ||  state==WAITING_FOR_CLEARANCE_TWO_MONTHS  ||  state==CAN_START_TWO_MONTHS;}

	/**
	* reset state to no error message
	* @author prissi
	*/
	void reset_waiting() { state=WAITING_FOR_CLEARANCE; }

	/**
	* Das Handle für uns selbst. In Anlehnung an 'this' aber mit
	* allen checks beim Zugriff.
	* @author Hanjsörg Malthaner
	*/
	convoihandle_t self;

	/**
	 * Der Gewinn in diesem Jahr
	 * @author Hanjsörg Malthaner
	 */
	const sint64 & get_jahresgewinn() const {return jahresgewinn;}

	const sint64 & get_total_distance_traveled() const { return total_distance_traveled; }

	/**
	 * returns the total running cost for all vehicles in convoi
	 * @author hsiegeln
	 */
	sint32 get_running_cost() const;

	/**
	* Constructor for loading from file,
	* @author Hj. Malthaner
	*/
	convoi_t(karte_t *welt, loadsave_t *file);

	convoi_t(spieler_t* sp);

	virtual ~convoi_t();

	/**
	* Load or save this convoi data
	* @author Hj. Malthaner
	*/
	void rdwr(loadsave_t *file);

	void laden_abschliessen();

	void rotate90( const sint16 y_size );

	/**
	* Called if a vehicle enters a depot
	* @author Hanjsörg Malthaner
	*/
	void betrete_depot(depot_t *dep);

	/**
	* @return Current map.
	* @author Hj. Malthaner
	*/
	karte_t* get_welt() { return welt; }

	/**
	* Gibt Namen des Convois zurück.
	* @return Name des Convois
	* @author Hj. Malthaner
	*/
	const char *get_internal_name() const {return name_and_id+name_offset;}

	/**
	* Allows editing ...
	* @return Name des Convois
	* @author Hj. Malthaner
	*/
	char *access_internal_name() {return name_and_id+name_offset;}

	/**
	* Gibt Namen des Convois zurück.
	* @return Name des Convois
	* @author Hj. Malthaner
	*/
	const char *get_name() const {return name_and_id;}

	/**
	* Sets the name. Copies name into this->name and translates it.
	* @author V. Meyer
	*/
	void set_name(const char *name);

	/**
	 * Gibt die Position des Convois zurück.
	 * @return Position des Convois
	 * @author Hj. Malthaner
	 */
	koord3d get_pos() const;

	/**
	 * @return current speed, this might be different from topspeed
	 *         actual currently set speed.
	 * @author Hj. Malthaner
	 */
	const sint32& get_akt_speed() const { return akt_speed; }

	/**
	 * @return total power of this convoi
	 * @author Hj. Malthaner
	 */
	const uint32 & get_sum_leistung() const {return sum_leistung;}
	const sint32 & get_min_top_speed() const {return min_top_speed;}
	const sint32 & get_sum_gewicht() const {return sum_gewicht;}
	const sint32 & get_sum_gesamtgewicht() const {return sum_gesamtgewicht;}

	uint32 get_length() const;

	/**
	 * Add the costs for traveling one tile
	 * @author Hj. Malthaner
	 */
	void add_running_cost();

	/**
	 * moving the veicles of a convoi and acceleration/deacceleration
	 * all other stuff => convoi_t::step()
	 * @author Hj. Malthaner
	 */
	bool sync_step(long delta_t);

	/**
	 * All things like route search or laoding, that may take a little
	 * @author Hj. Malthaner
	 */
	void step();

	/**
	 * Calculates total weight of freight in KG
	 * @author Hj. Malthaner
	 */
	int calc_freight_weight() const;

	/**
	* setzt einen neuen convoi in fahrt
	* @author Hj. Malthaner
	*/
	void start();

	void ziel_erreicht(); ///< Called, when the first vehicle reaches the target

	/**
	* Ein Fahrzeug hat ein Problem erkannt und erzwingt die
	* Berechnung einer neuen Route
	* @author Hanjsörg Malthaner
	*/
	void suche_neue_route();

	/**
	* Wartet bis Fahrzeug 0 freie Fahrt meldet
	* will be called during a hop_check, if the road/track is blocked
	* @author Hj. Malthaner
	*/
	void warten_bis_weg_frei(int restart_speed);

	/**
	* @return Vehicle count
	* @author Hj. Malthaner
	*/
	uint8 get_vehikel_anzahl() const { return anz_vehikel; }

	/**
	 * @return Vehicle at position i
	 */
	vehikel_t* get_vehikel(uint16 i) const { return fahr[i]; }

	/**
	* Adds a vehicel at the start or end of the convoi.
	* @author Hj. Malthaner
	*/
	bool add_vehikel(vehikel_t *v, bool infront = false);

	/**
	* Removes vehicles at position i
	* @author Hj. Malthaner
	*/
	vehikel_t * remove_vehikel_bei(unsigned short i);

	const minivec_tpl<uint8> &get_goods_catg_index() const { return goods_catg_index; }

	// recalculates the good transported by this convoy and (in case of changes) will start schedule recalculation
	void recalc_catg_index();

	/**
	* Sets a schedule
	* @author Hj. Malthaner
	*/
	bool set_schedule(schedule_t *f);

	/**
	* @return Current schedule
	* @author Hj. Malthaner
	*/
	schedule_t* get_schedule() const { return fpl; }

	/**
	* Creates a new schedule if there isn't one already.
	* @return Current schedule
	* @author Hj. Malthaner
	*/
	schedule_t * create_schedule();

	/**
	* @return Owner of this convoi
	* @author Hj. Malthaner
	*/
	spieler_t * get_besitzer() const { return besitzer_p; }

	/**
	* Opens an information window
	* @author Hj. Malthaner
	* @see simwin
	*/
	void zeige_info();

	/**
	* @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	* Beobachtungsfenster angezeigt wird.
	* @author Hj. Malthaner
	* @see simwin
	*/
	void info(cbuffer_t & buf) const;

	/**
	* @param buf the buffer to fill
	* @return Freight dscription text (buf)
	* @author Hj. Malthaner
	*/
	void get_freight_info(cbuffer_t & buf);
	void set_sortby(uint8 order);
	uint8 get_sortby() const { return freight_info_order; }

	/**
	* Opens the schedule window
	* @author Hj. Malthaner
	* @see simwin
	*/
	void open_schedule_window( bool show );

	static bool pruefe_vorgaenger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter);
	static bool pruefe_nachfolger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter);

	/**
	* pruefe ob Beschraenkungen fuer alle Fahrzeuge erfuellt sind
	* @author Hj. Malthaner
	*/
	bool pruefe_alle();

	/**
	* Kontrolliert Be- und Entladen.
	* V.Meyer: returns nothing
	* @author Hj. Malthaner
	*/
	void laden();

	/**
	* Setup vehicles before starting to move
	* @author Hanjsörg Malthaner
	*/
	void vorfahren();

	/**
	* Calculate the total value of the convoi as the sum of all vehicle values.
	* @author Volker Meyer
	* @date  09.06.2003
	*/
	sint64 calc_restwert() const;

	/**
	* Check if this convoi has entered a depot.
	* @author Volker Meyer
	* @date  09.06.2003
	*/
	bool in_depot() const { return state == INITIAL; }

	/**
	* loading_level was ladegrad before. Actual percentage loaded of loadable
	* vehicles.
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	const sint32 &get_loading_level() const { return loading_level; }

	/**
	* At which loading level is the train allowed to start? 0 during driving.
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	const sint32 &get_loading_limit() const { return loading_limit; }

	/**
	* Schedule convoid for self destruction. Will be executed
	* upon next sync step
	* @author Hj. Malthaner
	*/
	void self_destruct();

	/**
	* Helper method to remove convois from the map that cannot
	* removed normally (i.e. by sending to a depot) anymore.
	* This is a workaround for bugs in the game.
	* @author Hj. Malthaner
	* @date  12-Jul-03
	*/
	void destroy();

	/**
	* Debug info nach stderr
	* @author Hj. Malthaner
	* @date 04-Sep-03
	*/
	void dump() const;

	/**
	* book a certain amount into the convois financial history
	* is called from vehicle during un/load
	* @author hsiegeln
	*/
	void book(sint64 amount, int cost_type);

	/**
	* return a pointer to the financial history
	* @author hsiegeln
	*/
	sint64* get_finance_history() { return *financial_history; }

	/**
	* return a specified element from the financial history
	* @author hsiegeln
	*/
	sint64 get_finance_history(int month, int cost_type) { return financial_history[month][cost_type]; }

	/**
	* only purpose currently is to roll financial history
	* @author hsiegeln
	*/
	void new_month();

	/**
	 * Methode fuer jaehrliche aktionen
	 * @author Hj. Malthaner
	 */
	void neues_jahr();

	void set_update_line(linehandle_t l) { line_update_pending = l; }

	void set_home_depot(koord3d hd) { home_depot = hd; }

	koord3d get_home_depot() { return home_depot; }

	/**
	* this give the index of the next signal or the end of the route
	* convois will slow down before it, if this is not a waypoint or the cannot pass
	* The slowdown ist done by the vehicle routines
	* @author prissi
	*/
	uint16 get_next_stop_index() const {return next_stop_index;}
	void set_next_stop_index(uint16 n) {next_stop_index=n;}

	/* the current state of the convoi */
	uint8 get_status_color() const;

	// returns tiles needed for this convoi
	uint16 get_tile_length() const;

	bool has_obsolete_vehicles() const { return has_obsolete; }

	bool get_withdraw() const { return withdraw; }

	void set_withdraw(bool new_withdraw);

	bool get_no_load() const { return no_load; }

	void set_no_load(bool new_no_load) { no_load = new_no_load; }

	void must_recalc_data() { recalc_data = true; }

	// Overtaking for convois
	virtual bool can_overtake(overtaker_t *other_overtaker, int other_speed, int steps_other, int diagonal_length);
};

#endif
