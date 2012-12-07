#ifndef dataobj_umgebung_h
#define dataobj_umgebung_h

#include <string>
#include "../simtypes.h"
#include "../simconst.h"
#include "einstellungen.h"
#include "koord.h"

#include "../tpl/vector_tpl.h"
#include "../utils/plainstring.h"

#define TILE_HEIGHT_STEP (umgebung_t::pak_tile_height_step)


/**
 * Class to save all environment parameters, ie everything that changes
 * the look and feel of the game. Most of them can be changed by command-line
 * parameters or simuconf.tab files.
 *
 * @author Hj. Malthaner
 */
class umgebung_t
{
public:
	/// points to the current simutrans data directory
	static char program_dir[1024];

	/// points to the current user directory for loading and saving
	static const char *user_dir;

	/// version for which the savegames should be created
	static const char *savegame_version_str;

	/// name of the directory to the pak-set
	static std::string objfilename;


	/**
	 * @name Network-related settings
	 */
	/// @{
	/// true, if we are in networkmode
	static bool networkmode;

	/// number of simulation frames server runs ahead of clients
	static long server_frames_ahead;

	/// additional number of frames client is behind server
	static long additional_client_frames_behind;

	/// number of sync_steps before one step
	/// @see karte_t::interactive()
	static long network_frames_per_step;

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

	/// @} end of Network-related settings


	/**
	 * @name Information about server which is send to list-server
	 */
	/// @{
	/// DNS name or IP address clients should use to connect to server
	static std::string server_dns;
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

	/// open info windows for pedestrian and private cars
	static bool verkehrsteilnehmer_info;

	/// open info windows for trees
	static bool tree_info;

	/// open info windows for ground tiles
	static bool ground_info;

	/// open info windows for townhalls
	static bool townhall_info;

	/// open only one info window per click on a map-square
	static bool single_info;

	/// for schedules with rails hide the back ticket button
	static bool hide_rail_return_ticket;

	/// how to sort destination of goods
	/// @see freight_list_sorter_t::sort_mode_t
	static uint8 default_sortmode;

	/// default behavior of the map-window
	static uint32 default_mapmode;

	///which messages to display where?
	/**
	 * message_flags[i] is bitfield, where bit is set if message should be show at location i,
	 * where 0 = show message in ticker, 1 = open auto-close window, 2 = open persistent window, 3 = ignore message
	 * @see message_option_t
	 * @author prissi
	 */
	static sint32 message_flags[4];

	static bool left_to_right_graphs;

	/**
	 * window button at right corner (like Windows)
	 * @author prissi
	 */
	static bool window_buttons_right;

	static sint16 window_snap_distance;

	static koord iconsize;

	/// customize your tooltips
	static bool show_tooltips;
	static uint8 tooltip_color;
	static uint8 tooltip_textcolor;
	static uint32 tooltip_delay;
	static uint32 tooltip_duration;

	/// limit width and height of menu toolbars
	static uint8 toolbar_max_width;
	static uint8 toolbar_max_height;

	// how to highlight topped (untopped windows)
	static bool window_frame_active;
	static uint8 front_window_bar_color;
	static uint8 front_window_text_color;
	static uint8 bottom_window_bar_color;
	static uint8 bottom_window_text_color;

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

	/// Three states to control hiding of building
	enum hide_buildings_states {
		NOT_HIDE=0,           ///< show all buildings
		SOME_HIDDEN_BUIDLING, ///< hide buildings near cursor
		ALL_HIDDEN_BUIDLING   ///< hide all buildings
	};

	/// hide buildings if this is not NOT_HIDE
	static uint8 hide_buildings;

	/**
	 * Set to true to hide all trees. "Hiding" is implemented by showing the
	 * first pic which should be very small.
	 * @author Volker Meyer
	 * @date  10.06.2003
	 */
	static bool hide_trees;

	/// If hide_under_cursor is true then
	/// buildings and trees near mouse cursor will be hidden.
	static bool hide_under_cursor;


	/// Hide buildings and trees within range of mouse cursor
	static uint16 cursor_hide_range;


	/// color used for cursor overlay blending
	static uint8 cursor_overlay_color;

	/**
	 * Show labels (city and station names, ...)
	 * and waiting indicator bar for stations
	 * @see grund_t::display_overlay
	 */
	static sint32 show_names;

	/// if a schedule is open, show tiles which are used by it
	static bool visualize_schedule;

	/// time per water animation frame (0=off)
	static uint32 water_animation;


	/// how many internal pixel per height step (default 16)
	static sint8 pak_tile_height_step;

	/// use the faster drawing routine (and allow for clipping errors)
	static bool simple_drawing;

	/// if tile-size is less than this value (will be updated automatically)
	static sint16 simple_drawing_normal;

	/// if tile-size is less than this value (set by simuconf.tab)
	static sint16 simple_drawing_default;

	/// always use fast drwing in fast forward
	static bool simple_drawing_fast_forward;

	/// format in which date is shown
	enum date_fmt {
		DATE_FMT_SEASON   = 0,
		DATE_FMT_MONTH    = 1,
		DATE_FMT_JAPANESE = 2,
		DATE_FMT_US       = 3,
		DATE_FMT_GERMAN   = 4,
		DATE_FMT_JAPANESE_NO_SEASON = 5,
		DATE_FMT_US_NO_SEASON       = 6,
		DATE_FMT_GERMAN_NO_SEASON   = 7
	};

	/**
	 * show month in date?
	 *
	 * @author hsiegeln
	 */
	static uint8 show_month;

	/// @} end of Settings to control display of game world


	/**
	 * @name Settings to control the simulation (to some extent)
	 */
	/// @{

	/// set the frame rate for the display
	static uint32 fps;

	/// maximum acceleration with fast forward
	static sint16 max_acceleration;

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

	/**
	 * Name of rivers; first the river with the lowest number
	 * @author prissi
	 */
	static plainstring river_type[10];

	/// number of different river types
	static uint8 river_types;
	/// @}


	/**
	* Produce more debug info:
	* can be set by command-line switch '-debug'
	* @author Hj. Malthaner
	*/
	static uint8 verbose_debug;


	/// do autosave every month?
	/// @author prissi
	static sint32 autosave;


	/**
	 * @name Midi/sound options
	 */
	/// @{

	static sint16 global_volume, midi_volume;
	static bool mute_sound, mute_midi, shuffle_midi;

	/// @}


	/// default settings
	/// read in simmain.cc from various tab files
	/// @see simmain.cc
	static settings_t default_einstellungen;

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
