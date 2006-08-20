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

bool umgebung_t::tree_info = false;

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
 * Should the timeline be activated?
 *
 * @author Hj. Malthaner
 */
bool umgebung_t::use_timeline = 0;


/**
 * Starting year of the game
 *
 * @author Hj. Malthaner
 */
int umgebung_t::starting_year;
