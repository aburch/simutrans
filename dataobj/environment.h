/*
/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_ENVIRONMENT_H
#define DATAOBJ_ENVIRONMENT_H


#include <string>
#include "../simtypes.h"
#include "../simconst.h"
#include "../simcolor.h"
#include "settings.h"
#include "../display/scr_coord.h"

#include "../tpl/vector_tpl.h"
#include "../utils/plainstring.h"
#include "../utils/log.h"


#define TILE_HEIGHT_STEP (env_t::pak_tile_height_step)

enum { MENU_LEFT, MENU_TOP, MENU_RIGHT, MENU_BOTTOM };

/**
 * Class to save all environment parameters, ie everything that changes
 * the look and feel of the game. Most of them can be changed by command-line
 * parameters or simuconf.tab files.
 */
class env_t
{
public:
	/// Points to the current simutrans data directory. Usually this is the same directory
	/// where the executable is located, unless -use_workdir is specified.
	static char data_dir[PATH_MAX];

	static sint16 menupos;

	static sint16 fullscreen;

	/// Controls size of the virtual display
	static sint16 display_scale_percent;

	static bool reselect_closes_tool;

	/// points to the current user directory for loading and saving
	static const char *user_dir;

	/// version for which the savegames should be created
	static const char *savegame_version_str;

	/// name of the directory to the pak-set
	static std::string objfilename;

	/// this the the preferred GUI theme at startup
	static plainstring default_theme;

	/**
	 * @name Network-related settings
	 */
	/// @{
	/// true, if we are in networkmode
	static bool networkmode;

	/// number of simulation frames server runs ahead of clients
	static sint32 server_frames_ahead;

	/// additional number of frames client is behind server
	static sint32 additional_client_frames_behind;

	/// number of sync_steps before one step
	/// @see karte_t::interactive()
	static sint32 network_frames_per_step;

	/// server sends information to clients for checking synchronization
	/// after this number of sync_steps
	/// @see karte_t::interactive()
	static uint32 server_sync_steps_between_checks;

	/// when true, restore the windows from a savegame
	static bool restore_UI;

	/// if we are the server, we are at this port
	/// @see network_init_server()
	static const uint16 &server;

	/// enable/disable server announcement
	static uint32 server_announce;

	/// number of seconds between announcements
	static sint32 server_announce_interval;

	static uint8 chat_window_transparency;

	/// if true a kill event will save the  game under recovery#portnr#.sve
	static bool server_save_game_on_quit;

	/// if true save game under autosave-#paksetname#.sve and reload it upon startup
	static bool reload_and_save_on_quit;

	static uint8 network_heavy_mode;
	/// @} end of Network-related settings


	/**
	 * @name Information about server which is send to list-server
	 */
	/// @{
	/// If set, we are in easy server mode, assuming the IP can change any moment and thus query it before announce)
	static bool easy_server;
	/// Default port to start a new server
	static int server_port;
	/// DNS name or IP address clients should use to connect to server
	static std::string server_dns;
	/// second DNS name or more liekly IP address (for a dualstack machine) to connect to our server
	static std::string server_alt_dns;
	/// Name of server for display on list server
	static std::string server_name;
	/// Comments about server for display on list server
	static std::string server_comments;
	/// Email address of server maintainer
	static std::string server_email;
	/// Download location for pakset needed to play on server
	static std::string server_pakurl;
	/// Link to further information about server
	static std::string server_infurl;
	/// Text to be show on startup; can be formatted like helpfiles
	static std::string server_motd_filename;
	/// @} end of Information about server


	/**
	 * @name Network-related settings
	 */
	/// @{
	/// Server admin password (for use with nettool)
	static std::string server_admin_pw;

	/// IP addresses to listen on/send announcements on
	static vector_tpl<std::string> listen;

	/// pause server if no client connected
	static bool pause_server_no_clients;

	/// nickname of player
	static std::string nickname;

	/// @} end of Network-related settings


	/**
	 * @name GUI settings and windows behavior
	 */
	/// @{

	/// current language
	static const char *language_iso;

	/// controls scrolling speed and scrolling direction
	static sint16 scroll_multi;

	/// enables infinite scrolling with trackball or mouse, by may fail with sytlus
	static bool scroll_infinite;

	/// scrolling with general tool (like building stops or setting halts) after dragging a threshold
	static uint16 scroll_threshold;

	/// converts numpad keys to arrows no matter of numlock state
	static bool numpad_always_moves_map;

	/// open info windows for pedestrian and private cars
	static bool road_user_info;

	/// open info windows for trees
	static bool tree_info;

	/// open info windows for ground tiles
	static bool ground_info;

	/// open info windows for townhalls
	static bool townhall_info;

	/// open only one info window per click on a map-square
	static bool single_info;

	///  linelist enforcing single line GUI
	static bool single_line_gui;

	/// for schedules with rails hide the back ticket button
	static bool hide_rail_return_ticket;

	/// show/hide delete buttons in savegame frame
	static bool show_delete_buttons;

	/// how to sort destination of goods
	/// @see freight_list_sorter_t::sort_mode_t
	static uint8 default_sortmode;

	/// default behavior of the map-window
	static uint32 default_mapmode;

	/// cut through the map when following convois?
	static uint8 follow_convoi_underground;

	///which messages to display where?
	/**
	 * message_flags[i] is bitfield, where bit is set if message should be show at location i,
	 * where 0 = show message in ticker, 1 = open auto-close window, 2 = open persistent window, 3 = ignore message
	 * @see message_option_t
	 */
	static sint32 message_flags[4];

	static bool left_to_right_graphs;

	/**
	 * window button at right corner (like Windows)
	 */
	static bool window_buttons_right;

	static bool second_open_closes_win;

	static bool remember_window_positions;

	static sint16 window_snap_distance;

	static scr_size iconsize;

	/// customize your tooltips
	static bool show_tooltips;
	static uint32 tooltip_color_rgb;
	static PIXVAL tooltip_color;
	static uint32 tooltip_textcolor_rgb;
	static PIXVAL tooltip_textcolor;
	static uint32 tooltip_delay;
	static uint32 tooltip_duration;

	/// limit width and height of menu toolbars
	static sint8 toolbar_max_width;
	static sint8 toolbar_max_height;

	// how to highlight topped (untopped windows)
	static bool window_frame_active;
	static uint32 front_window_text_color_rgb;
	static PIXVAL front_window_text_color;
	static uint32 bottom_window_text_color_rgb;
	static PIXVAL bottom_window_text_color;
	static uint32 default_window_title_color_rgb;
	static PIXVAL default_window_title_color;
	static uint8 bottom_window_darkness;

	static uint8 gui_player_color_dark;
	static uint8 gui_player_color_bright;

	// default font name and -size
	static std::string fontname;
	static uint8 fontsize;

	// display compass
	static uint16 compass_map_position;
	static uint16 compass_screen_position;

	// what finances are shown? (default bank balance)
	static bool player_finance_display_account;

	/// @} end of GUI settings

	/**
	 * @name Settings to control display of game world
	 */
	/// @{

	/// show day-night cycle
	static bool night_shift;

	/// fixed day/night view level
	static sint8 daynight_level;

	/// show error/info tooltips over the vehicles
	static uint8 show_vehicle_states;

	/// show station coverage indicators
	static uint8 station_coverage_show;

	/// display station coverage by transparent overlay
	/// (otherwise by colored squares)
	static bool use_transparency_station_coverage;

	/// use transparency to hide buildings and trees
	static bool hide_with_transparency;

	/// which is the deafult economy?
	static uint8 just_in_time;

	/// Three states to control hiding of building
	enum hide_buildings_states {
		NOT_HIDE = 0,         ///< show all buildings
		SOME_HIDDEN_BUILDING, ///< hide buildings near cursor
		ALL_HIDDEN_BUILDING   ///< hide all buildings
	};

	/// hide buildings if this is not NOT_HIDE
	static uint8 hide_buildings;

	/**
	 * Set to true to hide all trees. "Hiding" is implemented by showing the
	 * first pic which should be very small.
	 */
	static bool hide_trees;

	/// If hide_under_cursor is true then
	/// buildings and trees near mouse cursor will be hidden.
	static bool hide_under_cursor;

	/// Hide buildings and trees within range of mouse cursor
	static uint16 cursor_hide_range;

	/// color used for cursor overlay blending
	static uint32 cursor_overlay_color_rgb;
	static PIXVAL cursor_overlay_color;

	static sint8 show_money_message;

	/// color used for solid background draw
	static uint32 background_color_rgb;
	static PIXVAL background_color;

	/// true if the border shut be shown as cut through the earth
	static bool draw_earth_border;

	/// true if the outside tiles should be shown
	static bool draw_outside_tile;

	/**
	 * Show labels (city and station names, ...)
	 * and waiting indicator bar for stations
	 * @see grund_t::display_overlay
	 */
	static sint32 show_names;

	/// Show factory storage bar
	static uint8 show_factory_storage_bar;

	/// if a schedule is open, show tiles which are used by it
	static bool visualize_schedule;

	/// time per water animation frame (0=off)
	static uint32 water_animation;

	/// how many internal pixel per height step (default 16)
	static sint8 pak_tile_height_step;

	/// new height for old slopes after conversion - 1=single height, 2=double height
	/// Only use during loading of old games!
	static sint8 pak_height_conversion_factor;

	enum height_conversion_mode
	{
		HEIGHT_CONV_LEGACY_SMALL, ///< Old (fixed) height conversion, small height difference
		HEIGHT_CONV_LEGACY_LARGE, ///< Old (fixed) height conversion, larger height difference
		HEIGHT_CONV_LINEAR,       ///< linear interpolation between min_/max_allowed_height
		HEIGHT_CONV_CLAMP,        ///< Use 1 height level per 1 greyscale level, clamp to allowed height (cut off mountains)
		NUM_HEIGHT_CONV_MODES
	};

	static height_conversion_mode height_conv_mode;

	/// use the faster drawing routine (and allow for clipping errors)
	static bool simple_drawing;

	/// if tile-size is less than this value (will be updated automatically)
	static sint16 simple_drawing_normal;

	/// if tile-size is less than this value (set by simuconf.tab)
	static sint16 simple_drawing_default;

	/// always use fast drawing in fast forward
	static bool simple_drawing_fast_forward;

	/// format in which date is shown
	enum date_fmt {
		DATE_FMT_SEASON             = 0,
		DATE_FMT_MONTH              = 1,
		DATE_FMT_JAPANESE           = 2,
		DATE_FMT_US                 = 3,
		DATE_FMT_GERMAN             = 4,
		DATE_FMT_JAPANESE_NO_SEASON = 5,
		DATE_FMT_US_NO_SEASON       = 6,
		DATE_FMT_GERMAN_NO_SEASON   = 7
	};

	/**
	 * show month in date?
	 */
	static uint8 show_month;

	/// @} end of Settings to control display of game world


	/**
	 * @name Settings to control the simulation (to some extent)
	 */
	/// @{

	static uint32 fps;                ///< target frame rate
	static uint32 ff_fps;             ///< target fps during fast forward
	static const uint32 min_fps = 5;  ///< minimum target fps (actual fps may be lower for large zoom out on slow machines)
	static const uint32 max_fps = 100;

	/// maximum acceleration with fast forward
	static sint16 max_acceleration;

	/// number of threads to use (if MULTI_THREAD defined)
	static uint8 num_threads;

	/// false to quit the programs
	static bool quit_simutrans;

	/// @} end of Settings to control the simulation


	/**
	 * @name Settings used at world creation
	 */
	/// @{

	/// probability for ground objects (if exists)
	static uint32 ground_object_probability;

	/// probability for moving objects (if there)
	static uint32 moving_object_probability;

	/// maximum length of city connections
	static sint32 intercity_road_length;

	// AI construction speed for new games (default 8000)
	static uint32 default_ai_construction_speed;

	/**
	 * Name of rivers; first the river with the lowest number
	 */
	static plainstring river_type[10];

	/// number of different river types
	static uint8 river_types;
	/// @}


	/// Produce more debug info:
	/// can be set by command-line switch '-debug'
	static log_t::level_t verbose_debug;

	/// do autosave every month?
	static sint32 autosave;


	/**
	 * @name Midi/sound options
	 */
	/// @{

	static sint16 global_volume, midi_volume;
	static bool mute_midi, shuffle_midi;
	static bool global_mute_sound;
	static uint16 specific_volume[MAX_SOUND_TYPES];

	/// how dast are distant sounds fading (1: very fast 25: very little)
	static uint32 sound_distance_scaling;

	// FluidSynth MIDI parameters
	static std::string soundfont_filename;

	/// @}

	/// if true this will show a softkeyboard only when editing text
	/// default is off
	static bool hide_keyboard;

	/// default settings
	/// read in simmain.cc from various tab files
	/// @see simmain.cc
	static settings_t default_settings;

	/// construct always straight ways
	/// as if ctrl-key permanently pressed
	/// cannot be used in network mode
	static bool straight_way_without_control;

	/// initialize with default values
	static void init();

	/**
	 * load/saving these from file (settings.xml)
	 * @see simmain.cc
	 */
	static void rdwr(loadsave_t *file);
};

#endif
