#include "umgebung.h"
#include "loadsave.h"
#include "../simconst.h"
#include "../simtypes.h"
#include "../simcolor.h"
#include "../utils/cstring_t.h"


// Hajo: hier Standardwerte belegen.

char umgebung_t::program_dir[1024];
const char *umgebung_t::user_dir = 0;

cstring_t umgebung_t::objfilename;	// will start empty ...

bool umgebung_t::night_shift = false;

bool umgebung_t::hide_with_transparency = true;
bool umgebung_t::hide_trees = false;
uint8 umgebung_t::hide_buildings = umgebung_t::NOT_HIDE;

/* station stuff */
bool umgebung_t::use_transparency_station_coverage = true;
uint8 umgebung_t::station_coverage_show = NOT_SHOWN_COVERAGE;

sint32  umgebung_t::show_names = 3;
sint16 umgebung_t::scroll_multi = 1;

sint32 umgebung_t::message_flags[4] =  { 0x017F, 0x0108, 0x0080, 0 };

bool umgebung_t::no_tree = false;
uint32 umgebung_t::water_animation = 250; // 250ms per wave stage
uint32 umgebung_t::ground_object_probability = 10; // every n-th tile
uint32 umgebung_t::moving_object_probability = 1000; // every n-th tile

bool umgebung_t::verkehrsteilnehmer_info = false;
bool umgebung_t::tree_info = true;
bool umgebung_t::ground_info = false;
bool umgebung_t::townhall_info = false;
bool umgebung_t::single_info = false;

bool umgebung_t::window_buttons_right = false;
bool umgebung_t::window_frame_active = false;

// debug level (0: only fatal, 1: error, 2: warning, 3: alles
uint8 umgebung_t::verbose_debug = 0;

uint8 umgebung_t::default_sortmode = 1;	// sort by amount
sint8 umgebung_t::default_mapmode = 0;	// show cities

/**
 * show month in date?
 *
 * @author hsiegeln
 */
uint8 umgebung_t::show_month = 0;

/**
 * Max. Länge für initiale Stadtverbindungen
 *
 * @author Hj. Malthaner
 */
sint32 umgebung_t::intercity_road_length = 200;

/**
 * Typ (Name) initiale Stadtverbindungen
 *
 * @author Hj. Malthaner
 */
const char *umgebung_t::intercity_road_type = NULL;

const char *umgebung_t::river_type[10] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
};
uint8 umgebung_t::river_types = 0;


/* prissi: autosave every x months (0=off) */
sint32 umgebung_t::autosave = 0;

/* prissi: drive on the left side of the road */
bool umgebung_t::drive_on_left=false;

// default: make 25 frames per second (if possible)
uint32 umgebung_t::fps=25;

// maximum speedup set to 1000 (effectively no limit)
sint16 umgebung_t::max_acceleration=1000;

bool umgebung_t::quit_simutrans = false;

bool umgebung_t::show_tooltips = true;
uint8 umgebung_t::tooltip_color = 4;
uint8 umgebung_t::tooltip_textcolor = COL_BLACK;

uint8 umgebung_t::cursor_overlay_color = COL_ORANGE;

uint8 umgebung_t::show_vehicle_states = 1;

sint8 umgebung_t::daynight_level = 0;

const char *umgebung_t::language_iso = "en";

// midi/sound option
sint16 umgebung_t::global_volume = 127;
sint16 umgebung_t::midi_volume = 127;
bool umgebung_t::mute_sound = false;
bool umgebung_t::mute_midi = false;
bool umgebung_t::shuffle_midi = true;

bool umgebung_t::hilly = false;

bool umgebung_t::finance_ltr_graphs = true;
bool umgebung_t::other_ltr_graphs = true;


// default settings for new games
einstellungen_t umgebung_t::default_einstellungen;


// save/restore environment
void umgebung_t::rdwr(loadsave_t *file)
{
	xml_tag_t u( file, "umgebung_t" );

	file->rdwr_short( scroll_multi, "" );
	file->rdwr_bool( night_shift, "" );
	file->rdwr_byte( daynight_level, "" );
	file->rdwr_long( water_animation, "" );
	file->rdwr_bool( drive_on_left, "" );

	file->rdwr_byte( show_month, "" );

	file->rdwr_bool( use_transparency_station_coverage, "" );
	file->rdwr_byte( station_coverage_show, "" );
	file->rdwr_long( show_names, "" );

	file->rdwr_bool( hide_with_transparency, "" );
	file->rdwr_byte( hide_buildings, "" );
	file->rdwr_bool( hide_trees, "" );

	file->rdwr_long( message_flags[0], "" );
	file->rdwr_long( message_flags[1], "" );
	file->rdwr_long( message_flags[2], "" );
	file->rdwr_long( message_flags[3], "" );

	file->rdwr_bool( show_tooltips, "" );
	file->rdwr_byte( tooltip_color, "" );
	file->rdwr_byte( tooltip_textcolor, "" );

	file->rdwr_long( autosave, "" );
	file->rdwr_long( fps, "" );
	file->rdwr_short( max_acceleration, "" );

	file->rdwr_bool( verkehrsteilnehmer_info , "" );
	file->rdwr_bool( tree_info, "" );
	file->rdwr_bool( ground_info , "" );
	file->rdwr_bool( townhall_info , "" );
	file->rdwr_bool( single_info , "" );

	file->rdwr_byte( default_sortmode, "" );
	file->rdwr_byte( default_mapmode, "" );

	file->rdwr_bool( window_buttons_right , "" );
	file->rdwr_bool( window_frame_active , "" );

	file->rdwr_byte( verbose_debug, "" );

	file->rdwr_long( intercity_road_length, "" );
	file->rdwr_bool( no_tree, "" );
	file->rdwr_long( ground_object_probability, "" );
	file->rdwr_long( moving_object_probability, "" );

	if(  file->is_loading()  ) {
		// these three bytes will be lost ...
		const char *c = NULL;
		file->rdwr_str( c );
		language_iso = c;
	}
	else {
		file->rdwr_str( language_iso );
	}

	file->rdwr_short( global_volume, "" );
	file->rdwr_short( midi_volume, "" );
	file->rdwr_bool( mute_sound, "" );
	file->rdwr_bool( mute_midi, "" );
	file->rdwr_bool( shuffle_midi, "" );

	if(  file->get_version()>102001  ) {
		file->rdwr_byte( show_vehicle_states, "" );
		file->rdwr_bool(finance_ltr_graphs, "");
		file->rdwr_bool(other_ltr_graphs, "");
	}
}

