#include "umgebung.h"


// Hajo: hier Standardwerte belegen.


bool umgebung_t::testlauf = false;

bool umgebung_t::night_shift = false;

bool umgebung_t::freeplay = false;

bool umgebung_t::station_coverage_show = true;
int umgebung_t::station_coverage_size = 2;

int  umgebung_t::show_names = 3;

bool umgebung_t::automaten[6] = {0,0,0,0,1,0};

int umgebung_t::message_flags[4] =  { 0xFF7F, 0x0108, 0x0080, 0 };

bool umgebung_t::bodenanimation = true;

bool umgebung_t::fussgaenger = false;

bool umgebung_t::verkehrsteilnehmer_info = false;

long umgebung_t::stadtauto_duration = 100000;

bool umgebung_t::tree_info = false;

bool umgebung_t::townhall_info = false;

bool umgebung_t::single_info = false;

bool umgebung_t::window_buttons_right = false;

bool umgebung_t::verbose_debug = false;

int umgebung_t::starting_money = 15000000;

int umgebung_t::maint_building;

int umgebung_t::maint_way;


/**
 * Wartungskosten für Oberleitungen
 *
 * @author Hj. Malthaner
 */
int umgebung_t::maint_overhead = 200;


/**
 * Use numbering for stations?
 *
 * @author Hj. Malthaner
 */
bool umgebung_t::numbered_stations = false;

/**
 * show month in date?
 *
 * @author hsiegeln
 */
bool umgebung_t::show_month = false;

/**
 * Max. Länge für initiale Stadtverbindungen
 *
 * @author Hj. Malthaner
 */
int umgebung_t::intercity_road_length = 8000;


/**
 * Typ (Name) initiale Stadtverbindungen
 *
 * @author Hj. Malthaner
 */
cstring_t * umgebung_t::intercity_road_type = 0;


/**
 * Typ (Name) initiale Stadtstrassen
 *
 * @author Hj. Malthaner
 */
cstring_t * umgebung_t::city_road_type = 0;


/**
 * Should the timeline be activated?
 *
 * @author Hj. Malthaner
 */
char umgebung_t::use_timeline = 2;	// do not care


/**
 * Starting year of the game
 *
 * @author Hj. Malthaner
 */
int umgebung_t::starting_year;


/* prissi: maximum number of steps for breath search */
int umgebung_t::max_route_steps = 10000;

/* prissi: maximum number of steps for breath search */
int umgebung_t::max_transfers = 7;

/* prissi: autosave every x months */
int umgebung_t::autosave = 3;

/* prissi: crossconnect all factories (like OTTD and similar games) */
bool umgebung_t::crossconnect_factories=false;

/* prissi: do not distribute goods to overflowing factories */
bool umgebung_t::just_in_time=true;

/* prissi: drive on the left side of the road */
bool umgebung_t::drive_on_left=false;
