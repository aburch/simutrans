#include "umgebung.h"
#include "../simtypes.h"


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

bool umgebung_t::ground_info = false;

bool umgebung_t::townhall_info = false;

bool umgebung_t::single_info = false;

bool umgebung_t::window_buttons_right = false;

bool umgebung_t::verbose_debug = false;

int umgebung_t::starting_money = 15000000;

int umgebung_t::maint_building;

int umgebung_t::maint_way;


/**
 * Wartungskosten für Oberleitungen
 * (not used since 89.02.2)
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
 * show month in date?
 *
 * @author hsiegeln
 */
sint8 umgebung_t::show_month = 0;

/**
 * Starting year of the game
 *
 * @author Hj. Malthaner
 */
sint16 umgebung_t::starting_year;

/* prissi: 1<<bits_per_day is the duration of a month in ms */
sint16 umgebung_t::bits_per_month=18;


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

/* the big cost section */
sint64 umgebung_t::cst_multiply_dock=-200000;
sint64 umgebung_t::cst_multiply_station=-240000;
sint64 umgebung_t::cst_multiply_roadstop=-150000;
sint64 umgebung_t::cst_multiply_airterminal=-1500000;
sint64 umgebung_t::cst_multiply_post=-60000;
sint64 umgebung_t::cst_multiply_headquarter=-1000000;
sint64 umgebung_t::cst_depot_rail=-100000;
sint64 umgebung_t::cst_depot_road=-130000;
sint64 umgebung_t::cst_depot_ship=-250000;
sint64 umgebung_t::cst_signal=-50000;
sint64 umgebung_t::cst_tunnel=-1000000;
sint64 umgebung_t::cst_third_rail=-8000;
// alter landscape
sint64 umgebung_t::cst_alter_land=-50000;
sint64 umgebung_t::cst_set_slope=-250000;
sint64 umgebung_t::cst_found_city=-500000000;
sint64 umgebung_t::cst_multiply_found_industry=-100000000;
sint64 umgebung_t::cst_remove_tree=-10000;
sint64 umgebung_t::cst_multiply_remove_haus=-100000;

// costs for the way searcher
sint32 umgebung_t::way_count_curve=10;
sint32 umgebung_t::way_count_double_curve=40;
sint32 umgebung_t::way_count_90_curve=2000;
sint32 umgebung_t::way_count_slope=80;
sint32 umgebung_t::way_count_tunnel=8;
uint32 umgebung_t::way_max_bridge_len=15;

// passenger manipulation factor
uint32 umgebung_t::passenger_factor=16;

// easier prices for beginner
uint32 umgebung_t::beginner_price_factor=1500;

// easier prices for beginner
bool umgebung_t::beginner_mode_first=false;

// default climate zones
sint16 umgebung_t::climate_borders[MAX_CLIMATES] = { 0, 0, 0, 3, 6, 8, 10, 10 };
sint16 umgebung_t::winter_snowline = 7;	// not mediterran
