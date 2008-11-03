#include "umgebung.h"
#include "../simconst.h"
#include "../simtypes.h"
#include "../simcolor.h"
#include "../utils/cstring_t.h"


// Hajo: hier Standardwerte belegen.

char umgebung_t::program_dir[1024];
const char *umgebung_t::user_dir = 0;

cstring_t umgebung_t::objfilename;	// will start empty ...

uint16 umgebung_t::max_convoihandles = 8192;
uint16 umgebung_t::max_linehandles = 2048;
uint16 umgebung_t::max_halthandles = 8192;

bool umgebung_t::testlauf = false;

bool umgebung_t::night_shift = false;

bool umgebung_t::freeplay = false;

bool umgebung_t::hide_with_transparency = true;
bool umgebung_t::hide_trees = false;
uint8 umgebung_t::hide_buildings = umgebung_t::NOT_HIDE;

/* station stuff */
bool umgebung_t::use_transparency_station_coverage = true;
uint8 umgebung_t::station_coverage_show = NOT_SHOWN_COVERAGE;
int umgebung_t::station_coverage_size = 2;
int umgebung_t::set_max_hops = 1000;
int umgebung_t::max_transfers = 9;

// passenger manipulation factor
uint32 umgebung_t::passenger_factor=16;

int  umgebung_t::show_names = 3;

bool umgebung_t::automaten[6] = {0,0,0,0,1,0};

int umgebung_t::message_flags[4] =  { 0x017F, 0x0108, 0x0080, 0 };

bool umgebung_t::fussgaenger = true;
uint32 umgebung_t::water_animation = 250; // 250ms per wave stage
uint32 umgebung_t::ground_object_probability = 10; // every n-th tile
uint32 umgebung_t::moving_object_probability = 1000; // every n-th tile

long umgebung_t::stadtauto_duration = 120;	// ten years

bool umgebung_t::verkehrsteilnehmer_info = false;
bool umgebung_t::tree_info = true;
bool umgebung_t::ground_info = false;
bool umgebung_t::townhall_info = false;
bool umgebung_t::single_info = false;

bool umgebung_t::window_buttons_right = false;
bool umgebung_t::window_frame_active = false;

// debug level (0: only fatal, 1: error, 2: warning, 3: alles
uint8 umgebung_t::verbose_debug = 0;

sint64 umgebung_t::starting_money = 20000000;

sint32 umgebung_t::maint_building = 5000;

uint8 umgebung_t::default_sortmode = 1;	// sort by amount
sint8 umgebung_t::default_mapmode = 0;	// show cities

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
cstring_t * umgebung_t::intercity_road_type = new cstring_t( "cobblestone_road" );

/**
 * Typ (Name) initiale Stadtstrassen
 *
 * @author Hj. Malthaner
 */
cstring_t * umgebung_t::city_road_type = new cstring_t( "city_road" );

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
uint8 umgebung_t::show_month = 0;

/**
 * Starting year of the game
 *
 * @author Hj. Malthaner
 */
sint16 umgebung_t::starting_year = 1930;

/* prissi: 1<<bits_per_day is the duration of a month in ms */
sint16 umgebung_t::bits_per_month=18;


/* prissi: maximum number of steps for breath search */
int umgebung_t::max_route_steps = 1000000;

/* prissi: autosave every x months (0=off) */
int umgebung_t::autosave = 0;

#ifdef OTTD_LIKE
/* prissi: crossconnect all factories (like OTTD and similar games) */
bool umgebung_t::crossconnect_factories=true;
sint16 umgebung_t::crossconnect_factor=100;
#else
/* prissi: crossconnect a certain number */
bool umgebung_t::crossconnect_factories=false;
sint16 umgebung_t::crossconnect_factor=33;
#endif

/* minimum spacing between two factories */
sint16 umgebung_t::factory_spacing = 6;

/* prissi: do not distribute goods to overflowing factories */
bool umgebung_t::just_in_time=true;

/* prissi: drive on the left side of the road */
bool umgebung_t::drive_on_left=false;

/* the big cost section */
sint64 umgebung_t::cst_multiply_dock=-50000;
sint64 umgebung_t::cst_multiply_station=-60000;
sint64 umgebung_t::cst_multiply_roadstop=-40000;
sint64 umgebung_t::cst_multiply_airterminal=-300000;
sint64 umgebung_t::cst_multiply_post=-30000;
sint64 umgebung_t::cst_multiply_headquarter=-100000;
sint64 umgebung_t::cst_depot_rail=-100000;
sint64 umgebung_t::cst_depot_road=-130000;
sint64 umgebung_t::cst_depot_ship=-250000;
sint64 umgebung_t::cst_depot_air=-500000;
sint64 umgebung_t::cst_signal=-50000;
sint64 umgebung_t::cst_tunnel=-1000000;
sint64 umgebung_t::cst_third_rail=-8000;
// alter landscape
sint64 umgebung_t::cst_buy_land=-10000;
sint64 umgebung_t::cst_alter_land=-100000;
sint64 umgebung_t::cst_set_slope=-250000;
sint64 umgebung_t::cst_found_city=-500000000;
sint64 umgebung_t::cst_multiply_found_industry=-2000000;
sint64 umgebung_t::cst_remove_tree=-10000;
sint64 umgebung_t::cst_multiply_remove_haus=-100000;
sint64 umgebung_t::cst_multiply_remove_field=-500000;
// cost for transformers
sint64 umgebung_t::cst_transformer=-250000;
sint64 umgebung_t::cst_maintain_transformer=-2000;

// costs for the way searcher
sint32 umgebung_t::way_count_straight=1;
sint32 umgebung_t::way_count_curve=2;
sint32 umgebung_t::way_count_double_curve=6;
sint32 umgebung_t::way_count_90_curve=50;
sint32 umgebung_t::way_count_slope=10;
sint32 umgebung_t::way_count_tunnel=8;
uint32 umgebung_t::way_max_bridge_len=15;
sint32 umgebung_t::way_count_leaving_road=25;

// easier prices for beginner
uint32 umgebung_t::beginner_price_factor=1500;

// easier prices for beginner
bool umgebung_t::beginner_mode_first=false;

// default climate zones
sint16 umgebung_t::climate_borders[MAX_CLIMATES] = { 0, 0, 0, 3, 6, 8, 10, 10 };
sint16 umgebung_t::winter_snowline = 7;	// not mediterran

// default: make 25 frames per second (if possible)
uint32 umgebung_t::fps=25;

// maximum speedup set to 1000 (effectively no limit)
sint16 umgebung_t::max_acceleration=1000;

bool umgebung_t::quit_simutrans = false;

sint32 	umgebung_t::default_electric_promille = 330;

bool umgebung_t::no_tree=false;

bool umgebung_t::show_tooltips = true;
uint8 umgebung_t::tooltip_color = 4;
uint8 umgebung_t::tooltip_textcolor = COL_BLACK;

/* multiplier for steps on diagonal:
 * 1024: TT-like, faktor 2, vehicle will be too long and too fast
 * 724: correct one, faktor sqrt(2)
 */
uint16 umgebung_t::pak_diagonal_multiplier = 724;

sint8 umgebung_t::daynight_level = 0;
