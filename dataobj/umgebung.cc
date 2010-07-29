#include <string>
#include "umgebung.h"
#include "loadsave.h"
#include "../simconst.h"
#include "../simtypes.h"
#include "../simcolor.h"

// since this is used at load time and not to be changed afterwards => extra init!
bool umgebung_t::drive_on_left = false;
char umgebung_t::program_dir[1024];
const char *umgebung_t::user_dir = 0;
bool umgebung_t::networkmode = false;
bool umgebung_t::server = false;
long umgebung_t::server_frames_ahead = 1;
long umgebung_t::server_ms_ahead = 250;
long umgebung_t::network_frames_per_step = 4;
uint32 umgebung_t::server_sync_steps_between_checks = 256;

// this is explicitely and interactively set by user => we do not touch it in init
const char *umgebung_t::language_iso = "en";
sint16 umgebung_t::scroll_multi = 1;
sint16 umgebung_t::global_volume = 127;
sint16 umgebung_t::midi_volume = 127;
bool umgebung_t::mute_sound = false;
bool umgebung_t::mute_midi = false;
bool umgebung_t::shuffle_midi = true;

// only used internally => do not touch further
bool umgebung_t::quit_simutrans = false;

// default settings for new games
einstellungen_t umgebung_t::default_einstellungen;


// the following initialisation is not important; set values in init()!
std::string umgebung_t::objfilename;
bool umgebung_t::night_shift;
bool umgebung_t::hide_with_transparency;
bool umgebung_t::hide_trees;
uint8 umgebung_t::hide_buildings;
bool umgebung_t::use_transparency_station_coverage;
uint8 umgebung_t::station_coverage_show;
sint32  umgebung_t::show_names;
sint32 umgebung_t::message_flags[4];
uint32 umgebung_t::water_animation;
uint32 umgebung_t::ground_object_probability;
uint32 umgebung_t::moving_object_probability;
bool umgebung_t::verkehrsteilnehmer_info;
bool umgebung_t::tree_info;
bool umgebung_t::ground_info;
bool umgebung_t::townhall_info;
bool umgebung_t::single_info;
bool umgebung_t::window_buttons_right;
bool umgebung_t::window_frame_active;
uint8 umgebung_t::verbose_debug;
uint8 umgebung_t::default_sortmode;
sint8 umgebung_t::default_mapmode;
uint8 umgebung_t::show_month;
sint32 umgebung_t::intercity_road_length;
const char *umgebung_t::river_type[10] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
};
uint8 umgebung_t::river_types;
sint32 umgebung_t::autosave;
uint32 umgebung_t::fps;
sint16 umgebung_t::max_acceleration;
bool umgebung_t::show_tooltips;
uint8 umgebung_t::tooltip_color;
uint8 umgebung_t::tooltip_textcolor;
uint8 umgebung_t::toolbar_max_width;
uint8 umgebung_t::toolbar_max_height;
uint8 umgebung_t::cursor_overlay_color;
uint8 umgebung_t::show_vehicle_states;
sint8 umgebung_t::daynight_level;
bool umgebung_t::hilly = false;
bool umgebung_t::cities_ignore_height = false;

// constraints:
// number_of_big_cities <= anzahl_staedte
// number_of_big_cities == 0 if anzahl_staedte == 0
// number_of_big_cities >= 1 if anzahl_staedte !=0
uint32 umgebung_t::number_of_big_cities = 1;
//constraints:
// 0<= number_of_clusters <= anzahl_staedts/4
uint32 umgebung_t::number_of_clusters = 0;
uint32 umgebung_t::cluster_size = 200;
uint8 umgebung_t::cities_like_water = 60;
bool umgebung_t::left_to_right_graphs = true;
uint32 umgebung_t::tooltip_delay;
uint32 umgebung_t::tooltip_duration;



// Hajo: hier Standardwerte belegen.
void umgebung_t::init()
{
	// settings for messages
	message_flags[0] = 0x017F;
	message_flags[1] = 0x0108;
	message_flags[2] = 0x0080;
	message_flags[3] = 0;

	night_shift = false;

	hide_with_transparency = true;
	hide_trees = false;
	hide_buildings = umgebung_t::NOT_HIDE;

	/* station stuff */
	use_transparency_station_coverage = true;
	station_coverage_show = NOT_SHOWN_COVERAGE;

	show_names = 3;

	water_animation = 250; // 250ms per wave stage
	ground_object_probability = 10; // every n-th tile
	moving_object_probability = 1000; // every n-th tile

	verkehrsteilnehmer_info = false;
	tree_info = true;
	ground_info = false;
	townhall_info = false;
	single_info = true;

	window_buttons_right = false;
	window_frame_active = false;

	// debug level (0: only fatal, 1: error, 2: warning, 3: alles
	verbose_debug = 0;

	default_sortmode = 1;	// sort by amount
	default_mapmode = 0;	// show cities

	/**
	 * show month in date?
	 * @author hsiegeln
	 */
	show_month = DATE_FMT_US;

	/**
	 * Max. Länge für initiale Stadtverbindungen
	 * @author Hj. Malthaner
	 */
	intercity_road_length = 200;

	river_types = 0;


	/* prissi: autosave every x months (0=off) */
	autosave = 0;

	// default: make 25 frames per second (if possible)
	fps=25;

	// maximum speedup set to 1000 (effectively no limit)
	max_acceleration=50;

	show_tooltips = true;
	tooltip_color = 4;
	tooltip_textcolor = COL_BLACK;

	toolbar_max_width = 0;
	toolbar_max_height = 0;

	cursor_overlay_color = COL_ORANGE;

	show_vehicle_states = 1;

	daynight_level = 0;

	// midi/sound option
	global_volume = 127;
	midi_volume = 127;
	mute_sound = false;
	mute_midi = false;
	shuffle_midi = true;

	left_to_right_graphs = false;

	tooltip_delay = 500;
	tooltip_duration = 5000;
}

// save/restore environment
void umgebung_t::rdwr(loadsave_t *file)
{
	xml_tag_t u( file, "umgebung_t" );

	file->rdwr_short( scroll_multi);
	file->rdwr_bool( night_shift);
	file->rdwr_byte( daynight_level);
	file->rdwr_long( water_animation);
	file->rdwr_bool( drive_on_left);

	file->rdwr_byte( show_month);

	file->rdwr_bool( use_transparency_station_coverage);
	file->rdwr_byte( station_coverage_show);
	file->rdwr_long( show_names);

	file->rdwr_bool( hide_with_transparency);
	file->rdwr_byte( hide_buildings);
	file->rdwr_bool( hide_trees);

	file->rdwr_long( message_flags[0]);
	file->rdwr_long( message_flags[1]);
	file->rdwr_long( message_flags[2]);
	file->rdwr_long( message_flags[3]);

	file->rdwr_bool( show_tooltips);
	file->rdwr_byte( tooltip_color);
	file->rdwr_byte( tooltip_textcolor);

	file->rdwr_long( autosave);
	file->rdwr_long( fps);
	file->rdwr_short( max_acceleration);

	file->rdwr_bool( verkehrsteilnehmer_info );
	file->rdwr_bool( tree_info);
	file->rdwr_bool( ground_info );
	file->rdwr_bool( townhall_info );
	file->rdwr_bool( single_info );

	file->rdwr_byte( default_sortmode);
	file->rdwr_byte( default_mapmode);

	file->rdwr_bool( window_buttons_right );
	file->rdwr_bool( window_frame_active );

	file->rdwr_byte( verbose_debug);

	file->rdwr_long( intercity_road_length);
	if(  file->get_version()<=102002  ) {
		bool no_tree = false;
		file->rdwr_bool( no_tree);
	}
	file->rdwr_long( ground_object_probability);
	file->rdwr_long( moving_object_probability);

	if(  file->is_loading()  ) {
		// these three bytes will be lost ...
		const char *c = NULL;
		file->rdwr_str( c );
		language_iso = c;
	}
	else {
		file->rdwr_str( language_iso );
	}

	file->rdwr_short( global_volume);
	file->rdwr_short( midi_volume);
	file->rdwr_bool( mute_sound);
	file->rdwr_bool( mute_midi);
	file->rdwr_bool( shuffle_midi);

	if(  file->get_version()>102001  ) {
		file->rdwr_byte( show_vehicle_states );
		bool dummy;
		file->rdwr_bool(dummy);
		file->rdwr_bool(left_to_right_graphs);
	}

	if(file->get_experimental_version() >= 6)
	{
		file->rdwr_bool(hilly);
		file->rdwr_bool(cities_ignore_height);
	}

	if(  file->get_experimental_version() >= 9 ) 
	{
		file->rdwr_long( tooltip_delay);
		file->rdwr_long( tooltip_duration);

	}
}
