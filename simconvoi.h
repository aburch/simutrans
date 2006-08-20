/**
 * simconvoi.h
 *
 * Header Datei für dir convoi_t Klasse für Fahrzeugverbände
 * von Hansjörg Malthaner
 */

#ifndef simconvoi_h
#define simconvoi_h

#ifndef simtypes_h
#include "simtypes.h"
#endif

#ifndef sync_steppable_h
#include "ifc/sync_steppable.h"
#endif

#ifndef route_h
#include "dataobj/route.h"
#endif

#ifndef tpl_array_tpl_h
#include "tpl/array_tpl.h"
#endif

#ifndef tpl_id_handle_tpl
#include "tpl/id_handle_tpl.h"
#endif

#ifndef convoihandle_t_h
#include "convoihandle_t.h"
#endif

#ifndef halthandle_t_h
#include "halthandle_t.h"
#endif


#define MAX_CONVOI_COST   6 // Total number of cost items
#define MAX_MONTHS     12 // Max history
#define MAX_CONVOI_NON_MONEY_TYPES 3 // number of non money types in convoi's financial statistic
#define CONVOI_CAPACITY   0 // the amount of ware that could be transported, theoretically
#define CONVOI_TRANSPORTED_GOODS 1 // the amount of ware that has been transported
#define CONVOI_VEHICLES		2 // the amount of vehicles that build this convoi
#define CONVOI_REVENUE		3 // the income this CONVOI generated
#define CONVOI_OPERATIONS         4 // the cost of operations this CONVOI generated
#define CONVOI_PROFIT             5 // total profit of this convoi


class depot_t;
class karte_t;
class spieler_t;
class haltestelle_t;
class convoi_info_t;
class vehikel_t;
class vehikel_besch_t;
class simline_t;
class fahrplan_t;

struct event_t;


/**
 * Basisklasse für alle Fahrzeugverbände. Convois könnnen über Zeiger
 * oder Handles angesprochen werden. Zeiger sind viel schneller, dafür
 * können Handles geprüft werden, ob das Ziel noch vorhanden ist.
 *
 * @author Hj. Malthaner
 */
class convoi_t : public sync_steppable
{
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
    simline_t * line;
    int line_id;


    /**
     * Name of the convoi.
     * @see setze_name
     * @author V. Meyer
     */
    char name[128];


    /**
     * Information window for ourselves.
     * @author Hj. Malthaner
     */
    convoi_info_t *convoi_info;


    /**
     * Alle vehikel-fahrplanzeiger zeigen hierauf
     * @author Hj. Malthaner
     */
    fahrplan_t *fpl;


    /**
     * loading_level was ladegrad before. Actual percentage loaded for loadable
     * vehicles (station length!).
     * Cached value calculated by calc_loading().
     * @author Volker Meyer
     * @date  12.06.2003
     */
    int loading_level;


    /**
     * At which loading level is the train allowed to start? 0 during driving.
     * Cached value calculated by calc_loading().
     * @author Volker Meyer
     * @date  12.06.2003
     */
    int loading_limit;


    /**
     * The vehicles of this convoi
     *
     * @author Hj. Malthaner
     */
    array_tpl <vehikel_t *> *fahr;


    /**
     * Während des Routings müssen zeitweise die pos gespeichert werden
     * @author Hj. Malthaner
     */
    array_tpl <koord3d> tmp_pos;


    /**
     * Convoi owner
     * @author Hj. Malthaner
     */
    spieler_t *besitzer_p;


    /**
     * Current map
     * @author Hj. Malthaner
     */
    karte_t   *welt;


    /**
     * Wird von anhalten und weiterfahren benutzt
     * @author Hj. Malthaner
     */
    bool ist_fahrend;


    /**
     * Number of vehicles in this convoi.
     * @author Hj. Malthaner
     */
    int anz_vehikel;


    /**
     * Gesamtleistung. Wird nicht gespeichert, sondern aus den Einzelleistungen
     * errechnet.
     * @author Hj. Malthaner
     */
    int sum_leistung;


    /**
     * Gesamtleistung mit Gear. Wird nicht gespeichert, sondern aus den Einzelleistungen
     * errechnet.
     * @author prissi
     */
    int sum_gear_und_leistung;


    /* sum_gewicht: leergewichte aller vehicles *
     * sum_gesamtgewicht: gesamtgewichte aller vehicles *
     * Werden nicht gespeichert, sondern aus den Einzelgewichten
     * errechnet beim beladen/fahren.
     * @author Hj. Malthaner, prissi
     */
    int sum_gewicht;
    int sum_gesamtgewicht;

    /**
     * Stores the previous delta_v value; otherwise these digits are lost during calculation and vehicle do not accelrate
     * @author prissi
     */
	int previous_delta_v;

    /**
     * Lowest top speed of all vehicles. Doesn't get saved, but calculated
     * from the vehicles data
     * @author Hj. Malthaner
     */
    int min_top_speed;


    /**
     * returns the total running cost for all vehicles in convoi
     * @author hsiegeln
     */
    int get_running_cost() const;


    /**
     * inititailisiert die Buttons fuers Fenster.
     * Muss aus jedem Kinstruktor aufgerufen werden
     * @author Hj. Malthaner
     */
    void init_buttons();


    /**
     * manchmal muss eine bestimmte Zeit gewartet werden.
     * wait_lock bestimmt wie lange gewartet wird (in ms).
     * @author Hanjsörg Malthaner
     */
    long wait_lock;


    /**
     * akkumulierter gewinn über ein jahr hinweg
     * @author Hanjsörg Malthaner
     */
    long jahresgewinn;


    // zur Geschwindigkeitssteuerung des convois

    int akt_speed_soll;            // Sollgeschwindigkeit

    int akt_speed;                 // momentane Geschwindigkeit

    int sp_soll;                   // Sollstrecke

    unsigned long next_wolke;                // zeit fuer naechste wolke

    int anz_ready;


    enum states {INITIAL,
	         FAHRPLANEINGABE,
                 ROUTING_1, ROUTING_2,
		 ROUTING_4, ROUTING_5,
		 DRIVING,
		 LOADING,
                 WAITING_FOR_CLEARANCE,
                 SELF_DESTRUCT};

    enum states state;


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
    int  drive_to(koord3d s, koord3d z);


    /**
     * Berechne route zum nächsten Halt
     * @author Hanjsörg Malthaner
     */
    void drive_to_next_stop();


    /**
     * Setup vehicles for moving in other direction than before
     * @author Hanjsörg Malthaner
     */
    void go_neue_richtung();


    /**
     * Setup vehicles for moving in same direction than before
     * @author Hanjsörg Malthaner
     */
    void go_alte_richtung();


    /**
     * Called if a vehicle enters a depot
     * @author Hanjsörg Malthaner
     */
    void betrete_depot(depot_t *dep);


    /**
     * Mark first and last vehicle.
     * @author Hanjsörg Malthaner
     */
    void setze_erstes_letztes();


    /**
     * calculate income for last hop
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
		 * holds id of line with pendig update
		 * -1 if no pending update
		 * @author hsiegeln
		 */
		int line_update_pending;

		/**
		 * the koordinate of the home depot of this convoi
		 * the last depot visited is considered beeing the home depot
		 * @author hsiegeln
		 */
		koord3d home_depot;

public:


		route_t * get_route() { return &route; };

    /**
     * Checks if this convoi has a driveable route
     * @author Hanjsörg Malthaner
     */
    bool hat_keine_route() const;


    /**
     * get line
     * @author hsiegeln
     */
    simline_t * get_line() const;


    /**
     * has line
     * returns true if convoi is member of a line, false otherwise
     * @author hsiegeln
     */
    bool has_line() { return (get_line() != NULL); };


    /**
     * set line
     * @author hsiegeln
     */
    void set_line(simline_t *);


    /**
     * registers the convoy with a line, but does not apply
     * the line's fahrplan!
     * used only during convoi restoration from savegame!
     * @author hsiegeln
     */
    void register_with_line(int line_id);


    /**
     * unset line -> remove cnv from line
     * @author hsiegeln
     */
    void unset_line();


    /**
     * get state
     * @author hsiegeln
     */
    int get_state() { return state; }


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
    const long & gib_jahresgewinn() const {return jahresgewinn;};


    /**
     * Constructor for loading from file,
     * @author Hj. Malthaner
     */
    convoi_t(karte_t *welt, loadsave_t *file);


    /**
     * Basic constructor.
     * @author Hj. Malthaner
     */
    convoi_t(karte_t *welt, spieler_t *sp);


    /**
     * Destructor.
     * @author Hj. Malthaner
     */
    virtual ~convoi_t();


    /**
     * @return Current map.
     * @author Hj. Malthaner
     */
    karte_t * gib_welt() {return welt;};


    /**
     * Gibt Namen des Convois zurück.
     * @return Name des Convois
     * @author Hj. Malthaner
     */
    const char *gib_name() const {return name;};


    /**
     * Erlaubt Änderung des Namens
     * @return Name des Convois
     * @author Hj. Malthaner
     */
    char *access_name() {return name;};

    /**
     * Sets the name. Copies name into this->name and translates it.
     * @author V. Meyer
     */
    void setze_name(const char *name);

    /**
     * Gibt die Position des Convois zurück.
     * @return Position des Convois
     * @author Hj. Malthaner
     */
    koord3d gib_pos() const;


    /**
     * wird vom ersten Fahrzeug des Convois aufgerufen, um Aenderungen
     * der Grundgeschwindigkeit zu melden. Berechnet (Brems-) Beschleunigung
     * @author Hj. Malthaner
     */
    void setze_akt_speed_soll(int akt_speed);


    /**
     * @return current speed, this might be different from topspeed and
     *         actual currently set speed.
     * @author Hj. Malthaner
     */
    const int &gib_akt_speed() const {return akt_speed;};


    int gib_min_top_speed() const {return min_top_speed;};


    /**
     * Vehicles of the convoi add their running cost by using this
     * method
     * @author Hj. Malthaner
     */
    void add_running_cost(int cost);


    /**
     * Vorbereitungsmethode für Echtzeitfunktionen eines Objekts.
     * @author Hj. Malthaner
     */
    void sync_prepare();


    /**
     * Methode für Echtzeitfunktionen eines Objekts.
     * @author Hj. Malthaner
     */
    bool sync_step(long delta_t);


    /**
     * Asynchrone step methode des Convois
     * @author Hj. Malthaner
     */
    void step();


    /**
     * Methode fuer jaehrliche aktionen
     * @author Hj. Malthaner
     */
    void neues_jahr();


    /**
     * Calculates total weight of freight in KG
     * @author Hj. Malthaner
     */
    int calc_freight_weight() const;


    /**
     * Calculates accelleration of this convoi
     * @param mini minimal accelleration cap
     * @return accelleration
     * @@author Hj. Malthaner
     */
    int calc_accelleration(int mini) const;


    /**
     * @return total power of this convoi
     * @author Hj. Malthaner
     */
    int gib_sum_leistung() const {return sum_leistung;};


    /**
     * setzt einen neuen convoi in fahrt
     * @author Hj. Malthaner
     */
    void start();


    /**
     * Bereit-Meldung entgegennehmen
     * @author Hj. Malthaner
     */
    void ziel_erreicht(vehikel_t *v);


    /**
     * Ein Fahrzeug hat ein Problem erkannt und erzwingt die
     * Berechnung einer neuen Route
     * @author Hanjsörg Malthaner
     */
    void suche_neue_route();


    /**
     * Advance route by one step.
     * @return next position on route or koord3d(-1,-1,-1) if route has ended
     * @author Hanjsörg Malthaner
     */
    koord3d advance_route(int n) const;


    /**
     * Wartet bis Fahrzeug 0 freie Fahrt meldet
     * @author Hj. Malthaner
     */
    void warten_bis_weg_frei(int restart_speed);


    /**
     * Convoi anhalten
     * @author Hj. Malthaner
     */
    void anhalten(int restart_speed);


    /**
     * Convoi wieder losfahren lassen
     * @author Hj. Malthaner
     */
    void weiterfahren();


    /**
     * @return Vehicle count
     * @author Hj. Malthaner
     */
    const int & gib_vehikel_anzahl() const {return anz_vehikel;};


    /**
     * @return Vehicle at position i or NULL if the is not vehicle at i
     * @author Hj. Malthaner
     */
    vehikel_t * gib_vehikel(int i) const {
	if(i>=0 && i<16) return fahr->at(i); else return NULL;
    };


    /**
     * Adds a vehicel at the start or end of the convoi.
     * @author Hj. Malthaner
     */
    bool add_vehikel(vehikel_t *v, bool infront = false);


    /**
     * Removes vehicles at position i
     * @author Hj. Malthaner
     */
    vehikel_t * remove_vehikel_bei(int i);


    /**
     * Sets a schedule
     * @author Hj. Malthaner
     */
    bool setze_fahrplan(fahrplan_t *f);


    /**
     * @return Current schedule
     * @author Hj. Malthaner
     */
    fahrplan_t * gib_fahrplan() const {return fpl;};


    /**
     * Creates a new schedule if there isn't one already.
     * @return Current schedule
     * @author Hj. Malthaner
     */
    fahrplan_t * erzeuge_fahrplan();


    /**
     * @return Owner of this convoi
     * @author Hj. Malthaner
     */
    spieler_t * gib_besitzer() { return besitzer_p; }


    /**
     * Load or save this convoi data
     * @author Hj. Malthaner
     */
    virtual void rdwr(loadsave_t *file);



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


    /**
     * Opens the schedule window
     * @author Hj. Malthaner
     * @see simwin
     */
    void open_schedule_window();

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
    int calc_restwert() const;

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
    const int &get_loading_level() const { return loading_level; }

    /**
     * At which loading level is the train allowed to start? 0 during driving.
     * @author Volker Meyer
     * @date  12.06.2003
     */
    const int &get_loading_limit() const { return loading_limit; }


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
     * prepares the convoi to receive a new schedule
     * @author hsiegeln
     */
    void convoi_t::prepare_for_new_schedule (fahrplan_t *);

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
    sint64* get_finance_history() { return *financial_history; };

    /**
     * return a specified element from the financial history
     * @author hsiegeln
     */
    sint64 get_finance_history(int month, int cost_type) { return financial_history[month][cost_type]; };

    /**
     * only purpose currently is to roll financial history
     * @author hsiegeln
     */
    void new_month();

    int get_line_update_pending() { return line_update_pending; };

    void set_line_update_pending(int id) { line_update_pending = id; };

		void check_pending_updates();

		void set_home_depot(koord3d hd) { home_depot = hd; };

		koord3d get_home_depot() { return home_depot; };

};


#endif
