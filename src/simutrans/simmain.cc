/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#if defined(_M_X64)  ||  defined(__x86_64__)
#if   __GNUC__
#warning "Simutrans is preferably compiled as 32 bit binary!"
#endif
#endif

#if defined(_MSC_VER)  &&  defined(DEBUG)
// Console window on MSVC debug builds
#pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#endif

#include <stdio.h>
#include <string>
#include <new>

#include "pathes.h"

#include "simmain.h"
#include "world/simworld.h"
#include "simware.h"
#include "display/simview.h"
#include "gui/simwin.h"
#include "gui/gui_theme.h"
#include "gui/messagebox.h"
#include "simhalt.h"
#include "display/simimg.h"
#include "simcolor.h"
#include "simskin.h"
#include "simconst.h"
#include "ground/boden.h"
#include "ground/wasser.h"
#include "world/simcity.h"
#include "player/simplay.h"
#include "simsound.h"
#include "simintr.h"
#include "simloadingscreen.h"
#include "simticker.h"
#include "simmesg.h"
#include "tool/simmenu.h"
#include "siminteraction.h"
#include "simtypes.h"

#include "sys/simsys.h"
#include "display/simgraph.h"
#include "simevent.h"

#include "simversion.h"

#include "gui/banner.h"
#include "gui/pakselector.h"
#include "gui/pakinstaller.h"
#include "gui/welt.h"
#include "gui/help_frame.h"
#include "gui/sprachen.h"
#include "gui/climates.h"
#include "gui/messagebox.h"
#include "gui/loadsave_frame.h"
#include "gui/load_relief_frame.h"
#include "gui/scenario_frame.h"

#include "obj/baum.h"
#include "obj/wolke.h"

#include "utils/simstring.h"
#include "utils/searchfolder.h"
#include "io/rdwr/compare_file_rd_stream.h"

#include "network/network.h" // must be before any "windows.h" is included via bzlib2.h ...
#include "dataobj/loadsave.h"
#include "dataobj/environment.h"
#include "dataobj/tabfile.h"
#include "dataobj/scenario.h"
#include "dataobj/settings.h"
#include "dataobj/translator.h"
#include "network/pakset_info.h"

#include "descriptor/reader/obj_reader.h"
#include "descriptor/sound_desc.h"
#include "descriptor/ground_desc.h"

#include "music/music.h"
#include "sound/sound.h"

#include "utils/cbuffer.h"
#include "utils/simrandom.h"

#include "builder/vehikelbauer.h"
#include "script/script_tool_manager.h"

#include "vehicle/vehicle.h"
#include "vehicle/simroadtraffic.h"


using std::string;


#ifdef DEBUG
/* diagnostic routine:
 * show the size of several internal structures
 */
static void show_sizes()
{
	log_t::level_t old_level = env_t::verbose_debug;
	env_t::verbose_debug = log_t::LEVEL_MSG;

	DBG_MESSAGE("Debug", "size of structures");
	DBG_MESSAGE("sizes", "size of pointer %i", sizeof(void*));

	DBG_MESSAGE("sizes", "koord: %d", sizeof(koord));
	DBG_MESSAGE("sizes", "koord3d: %d", sizeof(koord3d));
	DBG_MESSAGE("sizes", "ribi_t::ribi: %d", sizeof(ribi_t::ribi));
	DBG_MESSAGE("sizes", "halthandle_t: %d\n", sizeof(halthandle_t));

	DBG_MESSAGE("sizes", "obj_t: %d", sizeof(obj_t));
	DBG_MESSAGE("sizes", "gebaeude_t: %d", sizeof(gebaeude_t));
	DBG_MESSAGE("sizes", "baum_t: %d", sizeof(baum_t));
	DBG_MESSAGE("sizes", "wolke_t: %d", sizeof(wolke_t));
	DBG_MESSAGE("sizes", "weg_t: %d", sizeof(weg_t));
	DBG_MESSAGE("sizes", "private_car_t: %d\n", sizeof(private_car_t));

	DBG_MESSAGE("sizes", "grund_t: %d", sizeof(grund_t));
	DBG_MESSAGE("sizes", "boden_t: %d", sizeof(boden_t));
	DBG_MESSAGE("sizes", "wasser_t: %d", sizeof(wasser_t));
	DBG_MESSAGE("sizes", "planquadrat_t: %d\n", sizeof(planquadrat_t));

	DBG_MESSAGE("sizes", "ware_t: %d", sizeof(ware_t));
	DBG_MESSAGE("sizes", "vehicle_t: %d", sizeof(vehicle_t));
	DBG_MESSAGE("sizes", "haltestelle_t: %d\n", sizeof(haltestelle_t));

	DBG_MESSAGE("sizes", "karte_t: %d", sizeof(karte_t));
	DBG_MESSAGE("sizes", "player_t: %d\n", sizeof(player_t));
	env_t::verbose_debug = old_level;
}
#endif


#if defined DEBUG || defined PROFILE
// render tests ...
static void show_times(karte_t *welt, main_view_t *view)
{
	intr_set_view(view);
	welt->set_fast_forward(true);
	intr_disable();

	dbg->message( "show_times()", "simple profiling of drawing routines" );
	int i;

	image_id img = ground_desc_t::outside->get_image(0,0);

	uint32 ms = dr_time();
	for (i = 0;  i < 6000000;  i++) {
		display_img_aux( img, 50, 50, 1, 0, true  CLIP_NUM_DEFAULT);
	}
	dbg->message( "display_img()", "%i iterations took %li ms", i, dr_time() - ms );

	image_id player_img = skinverwaltung_t::color_options->get_image_id(0);
	ms = dr_time();
	for (i = 0;  i < 1000000;  i++) {
		display_color_img( player_img, 120, 100, i%15, 0, 1);
	}
	dbg->message( "display_color_img() with recolor", "%i iterations took %li ms", i, dr_time() - ms );

	ms = dr_time();
	for (i = 0;  i < 1000000;  i++) {
		display_color_img( img, 120, 100, 0, 1, 1);
		display_color_img( player_img, 160, 150, 16, 1, 1);
	}
	dbg->message( "display_color_img()", "3x %i iterations took %li ms", i, dr_time() - ms );

	ms = dr_time();
	for (i = 0;  i < 600000;  i++) {
		dr_prepare_flush();
		dr_flush();
	}
	dbg->message( "display_flush_buffer()", "%i iterations took %li ms", i, dr_time() - ms );

	ms = dr_time();
	for (i = 0;  i < 300000;  i++) {
		display_text_proportional_len_clip_rgb(100, 120, "Dies ist ein kurzer Textetxt ...", 0, 0, false, -1);
	}
	dbg->message( "display_text_proportional_len_clip_rgb()", "%i iterations took %li ms", i, dr_time() - ms );

	ms = dr_time();
	for (i = 0;  i < 300000;  i++) {
		display_fillbox_wh_rgb(100, 120, 300, 50, 0, false);
	}
	dbg->message( "display_fillbox_wh_rgb()", "%i iterations took %li ms", i, dr_time() - ms );

	ms = dr_time();
	for (i = 0; i < 2000; i++) {
		view->display(true);
	}
	dbg->message( "view->display(true)", "%i iterations took %li ms", i, dr_time() - ms );

	ms = dr_time();
	for (i = 0; i < 2000; i++) {
		view->display(true);
		win_display_flush(0.0);
	}
	dbg->message( "view->display(true) and flush", "%i iterations took %li ms", i, dr_time() - ms );

	ms = dr_time();
	for (i = 0; i < 40000000/(int)weg_t::get_alle_wege().get_count(); i++) {
		for(weg_t * const w : weg_t::get_alle_wege() ) {
			grund_t *dummy;
			welt->lookup( w->get_pos() )->get_neighbour( dummy, invalid_wt, ribi_t::north );
		}
	}
	dbg->message( "grund_t::get_neighbour()", "%i iterations took %li ms", i*weg_t::get_alle_wege().get_count(), dr_time() - ms );

	ms = dr_time();
	for (i = 0; i < 2000; i++) {
		welt->sync_step(200);
		welt->display(200);
		welt->step();
	}
	dbg->message( "welt->sync_step(200)/display(200)/step", "%i iterations took %li ms", i, dr_time() - ms );
}
#endif


// some routines for the modal display
static bool never_quit() { return false; }
static bool no_language() { return translator::get_language()!=-1; }
#if COLOUR_DEPTH != 0
static bool empty_objfilename() { return !env_t::pak_name.empty() ||  pakinstaller_t::finish_install; }
static bool finish_install() { return pakinstaller_t::finish_install; }
#endif


#if COLOUR_DEPTH != 0
/**
 * Show pak installer
 */
static void install_objfilename()
{
	DBG_DEBUG("", "env_t::reload_and_save_on_quit=%d", env_t::reload_and_save_on_quit);
#if !defined(USE_OWN_PAKINSTALL)  &&  defined(_WIN32)
	dr_download_pakset( env_t::base_dir, env_t::base_dir == env_t::user_dir );  // windows
#else
	pakinstaller_t* sel = new pakinstaller_t();
	// notify gui to load list of paksets
	event_t ev;
	ev.ev_class = INFOWIN;
	ev.ev_code = WIN_OPEN;
	sel->infowin_event(&ev);

	destroy_all_win(true); // since eventually the successful load message is still there ....
	modal_dialogue(sel, magic_none, NULL, finish_install);
#endif
}


/**
* Show pak selector
*/
static sint16 ask_objfilename()
{
	pakselector_t* sel = new pakselector_t();
	// notify gui to load list of paksets
	event_t ev;
	ev.ev_class = INFOWIN;
	ev.ev_code  = WIN_OPEN;
	sel->infowin_event(&ev);
	sint16 entries = sel->get_entries_count();

	if(sel->has_pak()) {
		destroy_all_win(true); // since eventually the successful load message is still there ....
		modal_dialogue( sel, magic_none, NULL, empty_objfilename );
	}
	else {
		delete sel;
	}

	return entries;
}
#endif


/**
 * Show language selector
 */
static void ask_language()
{
	if(  display_get_width()==0  ) {
		// only console available ... => choose english for the moment
		dbg->warning( "ask_language", "No language selected, will use english!" );
		translator::set_language( "en" );
	}
	else {
		sprachengui_t* sel = new sprachengui_t();
		destroy_all_win(true); // since eventually the successful load message is still there ....
		modal_dialogue( sel, magic_none, NULL, no_language );
		destroy_win( sel );
	}
}


/**
 * This function will be set in the main function as the handler the runtime environment will
 * call in the case it lacks memory for new()
 */
NORETURN static void sim_new_handler()
{
	dbg->fatal("sim_new_handler()", "OUT OF MEMORY");
}


/// Helper class for retrieving command line arguments
struct args_t
{
public:
	args_t(int argc, char **argv) : argc(argc), argv(argv) {}

public:
	const char *gimme_arg(const char *arg, int off) const
	{
		for(  int i = 1;  i < argc;  i++  ) {
			if(strcmp(argv[i], arg) == 0  &&  i < argc - off  ) {
				return argv[i + off];
			}
		}
		return NULL;
	}

	bool has_arg(const char *arg) const { return gimme_arg(arg, 0) != NULL; }

private:
	int argc;
	char **argv;
};


void print_help()
{
	printf(
		"\n"
		"---------------------------------------\n"
		"  Simutrans " VERSION_NUMBER "\n"
		"  released " VERSION_DATE "\n"
		"  developed\n"
		"  by the Simutrans team.\n"
		"\n"
		"  Send feedback and questions to:\n"
		"  <markus@pristovsek.de>\n"
		"\n"
		"  Based on Simutrans 0.84.21.2\n"
		"  by Hansjörg Malthaner et. al.\n"
		"---------------------------------------\n"
		"command line parameters available: \n"
		" -addons             loads also addons (with -objects)\n"
		" -async              asynchronous images, only for SDL\n"
		" -borderless         emulate fullscreen as borderless window\n"
		" -use_hw             hardware double buffering, only for SDL\n"
		" -debug NUM          enables debugging (1..5)\n"
		" -easyserver         set up every for server (query own IP, port forwarding)\n"
		" -freeplay           play with endless money\n"
		" -fullscreen         starts simutrans in fullscreen mode\n"
		" -fps COUNT          framerate (from 5 to 100)\n"
		" -h | -help | --help displays this help\n"
		" -lang CODE          starts with specified language\n"
		" -load NAME          loads savegame with name 'NAME' from Simutrans 'save' directory\n"
		" -log                enables logging to file 'simu.log'\n"
#ifdef SYSLOG
		" -syslog             enable logging to syslog\n"
		"                     mutually exclusive with -log\n"
		" -tag TAG            sets syslog tag (default 'simutrans')\n"
#endif
		" -mute               mute all sounds\n"
		" -noaddons           does not load any addon (default)\n"
		" -nomidi             turns off background music\n"
		" -nosound            turns off ambient sounds\n"
		" -objects DIR_NAME/  load the pakset in specified directory\n"
		" -pause              starts game with paused after loading\n"
		"                     a server will pause if there are no clients\n"
		" -res N              starts in specified resolution: \n"
		"                      1=640x480, 2=800x600, 3=1024x768, 4=1280x1024\n"
		" -scenario NAME      Load scenario NAME\n"
		" -screensize WxH     set screensize to width W and height H\n"
		" -server [PORT]      starts program as server (for network game)\n"
		"                     without port specified uses 13353\n"
		" -announce           Enable server announcements\n"
		" -autodpi            Automatic screen scaling for high DPI screens\n"
		" -screen_scale N     Manual screen scaling to N percent (0=off)\n"
		"                     Ignored when -autodpi is specified\n"
		" -server_dns FQDN/IP FQDN or IP address of server for announcements\n"
		" -server_name NAME   Name of server for announcements\n"
		" -server_admin_pw PW password for server administration\n"
		" -heavy NUM          enables heavy-mode debugging for network games. VERY SLOW!\n"
		" -set_basedir WD     Use WD as directory containing all constant data.\n"
		" -set_installdir WD  Use WD as directory for pakset download.\n"
		" -set_userdir WD     Use WD as directory for local user data.\n"
		" -singleuser         Save everything in data directory (portable version)\n"
#ifdef DEBUG
		" -sizes              Show current size of some structures\n"
#endif
		" -startyear N        start in year N\n"
		" -theme N            user directory containing theme files\n"
#ifdef MULTI_THREAD
		" -threads N          use N threads if possible\n"
#endif
		" -timeline           enables timeline\n"
#if defined DEBUG || defined PROFILE
		" -times              does some simple profiling\n"
#endif
		" -until YEAR.MONTH   quits when MONTH of YEAR starts\n"
	);
}


void setup_logging(const args_t &args)
{
	const char *version = "Simutrans version " VERSION_NUMBER " from " VERSION_DATE
#ifdef REVISION
		" r" QUOTEME(REVISION)
#endif
#ifdef GIT_HASH
		" hash " QUOTEME(GIT_HASH)
#endif
		"\n";

#ifdef SYSLOG
	bool cli_syslog_enabled = args.has_arg("-syslog");
	const char* cli_syslog_tag = args.gimme_arg("-tag", 1);
#else
	bool cli_syslog_enabled = false;
	const char* cli_syslog_tag = NULL;
#endif

	env_t::verbose_debug = log_t::LEVEL_FATAL;

	if(  args.has_arg("-debug")  ) {
		const char *s = args.gimme_arg("-debug", 1);
		log_t::level_t level = log_t::LEVEL_DEBUG;
		if(s!=NULL  &&  s[0]>='0'  &&  s[0]<='9'  ) {
			level = (log_t::level_t)clamp(atoi(s), (int)log_t::LEVEL_FATAL, (int)log_t::LEVEL_DEBUG);
		}
		env_t::verbose_debug = level;
	}

	if (  cli_syslog_enabled  ) {
		printf("syslog enabled\n");
		if (  cli_syslog_tag  ) {
			printf("Init logging with syslog tag: %s\n", cli_syslog_tag);
			init_logging( "syslog", true, true, version, cli_syslog_tag );
		}
		else {
			printf("Init logging with default syslog tag\n");
			init_logging( "syslog", true, true, version, "simutrans" );
		}
	}
	else if (args.has_arg("-log")) {
		dr_chdir( env_t::user_dir );
		char temp_log_name[256];
		const char *logname = "simu.log";
		if(  args.has_arg("-server")  ) {
			const char *p = args.gimme_arg("-server", 1);
			int portadress = p ? atoi( p ) : 13353;
			sprintf( temp_log_name, "simu-server%d.log", portadress==0 ? 13353 : portadress );
			logname = temp_log_name;
		}
		init_logging( logname, true, true, version, NULL );
	}
	else if (args.has_arg("-debug")) {
		init_logging( "stderr", true, true, version, NULL );
	}
	else {
		init_logging(NULL, false, false, version, NULL);
	}
}

// find if there is an commandline option or environment variable with a valid path
static bool set_predefined_dir( const char *p, const char *opt, char *result, const char *testfile=NULL)
{
	if(  p  &&  *p  ) {
		bool ok = !dr_chdir(p);
		FILE * testf = NULL;
		ok &=  ok  &&  testfile  &&  (testf = fopen(testfile,"r"));
		if(!ok) {
			printf("WARNING: Objects not found in %s \"%s\"!\n",  opt, p);
		}
		else {
			fclose(testf);
			dr_getcwd( result, PATH_MAX-1 );
			strcat( result, PATH_SEPARATOR );
			return true;
		}
	}
	return false;
}

// search for this in all possible pakset locations for the directory chkdir and take the first match
static bool set_pakdir( const char *chkdir )
{
	if(  !chkdir  ||  !*chkdir  ) {
		return false;
	}

	char tmp[PATH_MAX];
	dr_chdir( env_t::base_dir );
	if( !dr_chdir( chkdir ) ) {
		dr_getcwd(tmp, lengthof(tmp));
		env_t::pak_dir = tmp;
		env_t::pak_dir += PATH_SEPARATOR;
		return true;
	}
	dr_chdir( env_t::install_dir );
	if( !dr_chdir( chkdir ) ) {
		dr_getcwd(tmp, lengthof(tmp));
		env_t::pak_dir = tmp;
		env_t::pak_dir += PATH_SEPARATOR;
		return true;
	}
	dr_chdir( env_t::user_dir );
	if(!dr_chdir( USER_PAK_PATH ) ) {
		if( !dr_chdir( chkdir ) ) {
			dr_getcwd(tmp, lengthof(tmp));
			env_t::pak_dir = tmp;
			env_t::pak_dir += PATH_SEPARATOR;
			return true;
		}
	}
	return false;
}



int simu_main(int argc, char** argv)
{
	std::set_new_handler(sim_new_handler);

	args_t args(argc, argv);
	env_t::init();

	// you really want help with this?
	if (args.has_arg("-h") ||
			args.has_arg("-?") ||
			args.has_arg("-help") ||
			args.has_arg("--help")) {
		print_help();
		return EXIT_SUCCESS;
	}

	// only the specified pak conf should override this!
	uint16 pak_diagonal_multiplier = env_t::default_settings.get_pak_diagonal_multiplier();
	sint8 pak_tile_height = TILE_HEIGHT_STEP;
	sint8 pak_height_conversion_factor = env_t::pak_height_conversion_factor;

	/* simutrans has three directories:
	* a global writable (install_dir)
	* a user writable (user_dir)
	* a base directory with default data (may be write protected) (base_dir)
	*
	* The directory will be determined in the following order
	* -set_XXXdir Path (command line option)
	* SIMUTRANS_XXXDIR (environment variable)
	* (in case of base_dir current path, then executable path)
	* (for user and install: machine dependent default directories)
	*
	* The pak_dir contains the complete path to the current pak, since it could be in different locations
	*
	*/
	if( !set_predefined_dir( args.gimme_arg( "-set_basedir", 1 ), "-set_basedir", env_t::base_dir, "config/simuconf.tab")) {
		if( !set_predefined_dir( getenv("SIMUTRANS_BASEDIR"), "SIMUTRANS_BASEDIR", env_t::base_dir, "config/simuconf.tab") ) {
			bool found_basedir = false;
			dr_getcwd(env_t::base_dir, lengthof(env_t::base_dir));
			strcat( env_t::base_dir, PATH_SEPARATOR );
			// test if base installation
			if (FILE* f = fopen("config/simuconf.tab", "r")) {
				fclose(f);
				found_basedir = true;
			}
			else {
				// argv[0] contain the full path to the excutable already
				char testpath[PATH_MAX];
				tstrncpy(testpath, argv[0], lengthof(testpath));
				char* c = strrchr(testpath, *PATH_SEPARATOR);
				if(c) {
					*c = 0; // remove program name
					found_basedir = set_predefined_dir(testpath, "program dir", env_t::base_dir, "config/simuconf.tab");
					if(!found_basedir) {
#ifdef __APPLE__
						// Detect if the binary is started inside an application bundle
						// Change working dir from MacOS to Resources dir
						strcpy(env_t::base_dir, testpath);
						if( strstr(env_t::base_dir, ".app/Contents/MacOS") != NULL ) {
							while (env_t::base_dir[strlen(env_t::base_dir) - 1] != 's') {
								env_t::base_dir[strlen(env_t::base_dir) - 1] = 0;
							}
							strcat(env_t::base_dir, "/Resources/simutrans/");
							found_basedir = true;
						}
#else
						// Detect if simutrans has been installed by the system and try to locate the installation relative to the binary location
						char *c = strrchr(testpath, *PATH_SEPARATOR);
						if(  c  &&  strcmp(c+1,"bin")==0  ) {
							// replace bin with other paths
							strcpy( c+1, "share/simutrans/" );
							found_basedir = set_predefined_dir(testpath, "program dir", env_t::base_dir, "config/simuconf.tab");
							if (!found_basedir) {
								strcpy( c+1 , "share/games/simutrans/" );
								found_basedir = set_predefined_dir(testpath, "share/games/simutrans", env_t::base_dir, "config/simuconf.tab");
							}
						}
#endif
					}
				}
			}
			if (!found_basedir) {
				// try the installer dir next
				if (!set_predefined_dir(dr_query_installdir(), "install_dir", env_t::base_dir, "config/simuconf.tab")) {
					dr_fatal_notify("Absolutely no base installation found.\nPlease download/install the complete base set!");
					return EXIT_FAILURE;
				}
			}
		}
	}
	dr_chdir( env_t::base_dir );
	
	// parsing config/simuconf.tab to find out single user install
	bool found_settings = false;
	bool found_simuconf = false;
	bool not_portable = !args.has_arg("-singleuser");

	// only parse global config, if not force to portable by switch
	tabfile_t simuconf;
	char path_to_simuconf[24];
	// was  config/simuconf.tab
	sprintf( path_to_simuconf, "config%ssimuconf.tab", PATH_SEPARATOR );
	if(  not_portable  &&  simuconf.open( path_to_simuconf )  ) {
		tabfileobj_t contents;
		simuconf.read( contents );
		// use different save directories
		not_portable = !(contents.get_int( "singleuser_install", !not_portable )==1  ||  !not_portable);
		found_simuconf = true;
		simuconf.close();
	}

	if( !set_predefined_dir( args.gimme_arg( "-set_installdir", 1 ), "-set_installdir", env_t::install_dir ) ) {
		if( !set_predefined_dir( getenv( "SIMUTRANS_INSTALLDIR" ), "SIMUTRANS_INSTALLDIR", env_t::install_dir ) ) {
			if( !not_portable ) {
				// portable installation
				strcpy( env_t::install_dir, env_t::base_dir );
			}
			else {
				strcpy( env_t::install_dir, dr_query_installdir() );
			}
		}
	}

	if( !set_predefined_dir( args.gimme_arg( "-set_userdir", 1 ), "-set_userdir", env_t::user_dir ) ) {
		if( !set_predefined_dir( getenv( "SIMUTRANS_USERDIR" ), "SIMUTRANS_USERDIR", env_t::user_dir ) ) {
			if( !not_portable ) {
				strcpy( env_t::user_dir, env_t::base_dir );
			}
			else {
				strcpy( env_t::user_dir, dr_query_homedir() );
			}
		}
	}
	// set portable, if base dir and user dir were same
	not_portable &= strcmp( env_t::user_dir, env_t::base_dir )!=0;

	dr_chdir( env_t::user_dir );
	dr_mkdir("maps");
	dr_mkdir(SAVE_PATH);
	dr_mkdir(SCREENSHOT_PATH);

	setup_logging(args);

	// now read last setting (might be overwritten by the tab-files)
	{
		loadsave_t settings_file;
		if(  settings_file.rd_open("settings.xml") == loadsave_t::FILE_STATUS_OK  )  {
			if(  settings_file.get_version_int()>loadsave_t::int_version(SAVEGAME_VER_NR, NULL )  ) {
				// too new => remove it
				settings_file.close();
			}
			else {
				found_settings = true;
				env_t::rdwr(&settings_file);
				env_t::default_settings.rdwr(&settings_file);
				settings_file.close();
				// reset to false (otherwise these settings will persist)
				env_t::default_settings.set_freeplay( false );
				env_t::default_settings.set_allow_player_change( true );
				env_t::server_announce = 0;
			}
		}
	}

	sint16 disp_width = 0;
	sint16 disp_height = 0;
	sint16 fullscreen = WINDOWED;

	// continue parsing
	dr_chdir( env_t::base_dir );
	if(  found_simuconf  ) {
		if(simuconf.open(path_to_simuconf)) {
			// we do not allow to change the global font name
			std::string old_fontname = env_t::fontname;
			std::string old_soundfont_filename = env_t::soundfont_filename;

			dbg->message("simu_main()", "Parsing %s%s", env_t::base_dir, path_to_simuconf);
			env_t::default_settings.parse_simuconf( simuconf, disp_width, disp_height, fullscreen, env_t::pak_name );
			simuconf.close();

			if(  (old_soundfont_filename.length() > 0)  &&  (strcmp( old_soundfont_filename.c_str(), "Error" ) != 0)  ) {
				// We had a valid soundfont saved by the user, let's restore it
				env_t::soundfont_filename = old_soundfont_filename;
			}
			env_t::fontname = old_fontname;
		}
	}

	// a portable installation could have a personal simuconf.tab in the main dir of simutrans
	// otherwise it is in ~/simutrans/simuconf.tab
	env_t::pak_dir.clear();
	env_t::pak_name.clear();
	string obj_conf = string(env_t::user_dir) + "simuconf.tab";
	if (simuconf.open(obj_conf.c_str())) {
		dbg->message("simu_main()", "Parsing %s", obj_conf.c_str());
		env_t::default_settings.parse_simuconf( simuconf, disp_width, disp_height, fullscreen, env_t::pak_name );
		if(  set_pakdir( env_t::pak_name.c_str() )  ) {
			env_t::pak_name += PATH_SEPARATOR;
		}
		simuconf.close();
	}

	// env: override previous settings
	if(  args.has_arg("-freeplay")  ) {
		env_t::default_settings.set_freeplay( true );
	}

#ifdef __ANDROID__
	// always save and reload on Android
	env_t::reload_and_save_on_quit = true;
#endif

	// now set the desired objectfilename (override all previous settings)
	if(  const char *fn = args.gimme_arg("-set_pakdir", 1)  ) {
		if(!set_pakdir(fn)) {
			// try as absolute path
			char tmp[PATH_MAX];
			if( set_predefined_dir( fn, "-set_pakdir", tmp, "config/menuconf.tab" ) ) {
				env_t::pak_dir = tmp;
				// to be done => extrat pak name!!!
				const char *last_pathsep=0;
				for( char *c=tmp; c[0]>0; c++ ) {
					if( *c==*PATH_SEPARATOR ) {
						if( c[1]==0 ) {
							c[0] = 0;
							break;
						}
						last_pathsep = c+1;
					}
				}
				if( last_pathsep ) {
					env_t::pak_name = last_pathsep;
				}
			}
		}
		else {
			env_t::pak_name = fn;
			env_t::pak_name += PATH_SEPARATOR;
		}
	}

	if(  env_t::pak_dir.empty()  ) {
		// old style (deprecated)
		if( const char* pak = args.gimme_arg( "-objects", 1 ) ) {
			if( set_pakdir( pak ) ) {
				env_t::pak_name = pak;
				env_t::pak_name += PATH_SEPARATOR;
			}
		}
	}

	if(  env_t::pak_dir.empty()  ) {
		if(  const char *filename = args.gimme_arg("-load", 1)  ) {
			// try to get a pak file path from a savegame file
			// read pak_extension from file
			loadsave_t test;
			std::string fn = env_t::user_dir;
			fn += "save/";
			fn += filename;
			if(  test.rd_open(fn.c_str()) == loadsave_t::FILE_STATUS_OK  ) {
				// add pak extension
				const char *pak = test.get_pak_extension();
				if(  !STRICMP(pak,"(unknown)")  ) {
					if( set_pakdir( pak ) ) {
						env_t::pak_name = pak;
						env_t::pak_name += PATH_SEPARATOR;
					}
				}
			}
		}
	}
	dr_chdir( env_t::base_dir );

	// starting a server?
	if(  args.has_arg("-easyserver")  ) {
		const char *p = args.gimme_arg("-easyserver", 1);
		int portadress = p ? atoi( p ) : 13353;
		if(  portadress!=0  ) {
			env_t::server_port = portadress;
		}
		// will fail fatal on the opening routine ...
		dbg->message( "simu_main()", "Server started on port %i", env_t::server_port );
		env_t::networkmode = network_init_server( env_t::server_port, env_t::listen );
		// query IP and try to open ports on router
		char IP[256], altIP[256];
		altIP[0] = 0;
		if(  prepare_for_server( IP, altIP, env_t::server_port )  ) {
			// we have forwarded a port in router, so we can continue
			env_t::server_dns = IP;
			if(  env_t::server_name.empty()  ) {
				env_t::server_name = std::string("Server at ")+IP;
			}
			env_t::server_announce = 1;
			env_t::easy_server = 1;
			env_t::server_dns = IP;
			env_t::server_alt_dns = altIP;
		}
	}

	// starting a server?
	if(  !env_t::server  ) {
		if(  args.has_arg("-server")  ) {
			const char *p = args.gimme_arg("-server", 1);
			int portadress = p ? atoi( p ) : 13353;
			if(  portadress!=0  ) {
				env_t::server_port = portadress;
			}
			// will fail fatal on the opening routine ...
			dbg->message( "simu_main()", "Server started on port %i", env_t::server_port );
			env_t::networkmode = network_init_server( env_t::server_port, env_t::listen );
		}
		else {
			// no announce for clients ...
			env_t::server_announce = 0;
		}
	}
	// enabling heavy-mode for debugging network games
	env_t::network_heavy_mode = 0;
	if(  args.has_arg("-heavy")  ) {
		int heavy = atoi(args.gimme_arg("-heavy", 1));
		env_t::network_heavy_mode = clamp(heavy, 0, 2);
	}

	DBG_MESSAGE("simu_main()", "Version:     " VERSION_NUMBER "  Date: " VERSION_DATE);
	DBG_MESSAGE("simu_main()", "Debuglevel:  %i", env_t::verbose_debug);
	DBG_MESSAGE("simu_main()", "base_dir:    %s", env_t::base_dir);
	DBG_MESSAGE("simu_main()", "install_dir: %s", env_t::install_dir);
	DBG_MESSAGE("simu_main()", "user_dir:    %s", env_t::user_dir);
	DBG_MESSAGE("simu_main()", "locale:      %s", dr_get_locale_string());

#ifdef DEBUG
	if (args.has_arg("-sizes")) {
		// show the size of some structures ...
		show_sizes();
	}
#endif

	static const sint16 resolutions[][2] = {
		{  640,  480 },
		{  800,  600 },
		{ 1024,  768 },
		{ 1280, 1024 },
		{  704,  560 } // try to force window mode with allegro
	};

	// likely only the program without graphics was downloaded
	if (args.has_arg("-res")) {
		const char* res_str = args.gimme_arg("-res", 1);
		const int res = *res_str - '1';

		switch (res) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
				fullscreen = (res<=4);
				disp_width  = resolutions[res][0];
				disp_height = resolutions[res][1];
				break;

			default:
				fprintf(stderr,
					"Invalid resolution, argument must be in 1..5\n"
					"1=640x480, 2=800x600, 3=1024x768, 4=1280x1024, 5=windowed\n"
				);
				return EXIT_FAILURE;
		}
	}

	if ( !fullscreen ) {
		fullscreen = args.has_arg("-fullscreen") ? FULLSCREEN : WINDOWED;
	}
	if ( !fullscreen ) {
		fullscreen = args.has_arg("-borderless") ? BORDERLESS : WINDOWED;
	}
	if ( !fullscreen ) {
		fullscreen = env_t::fullscreen;
	}

	if(args.has_arg("-screensize")) {
		const char* res_str = args.gimme_arg("-screensize", 1);
		int n = 0;

		if (res_str != NULL) {
			n = sscanf(res_str, "%hdx%hd", &disp_width, &disp_height);
		}

		if (n != 2) {
			fprintf(stderr,
				"Invalid argument for -screensize option\n"
				"Argument must be of format like 800x600\n"
			);
			return EXIT_FAILURE;
		}
	}

	int parameter[2];
	parameter[0] = args.has_arg("-async");
	parameter[1] = args.has_arg("-use_hw");

	if (!dr_os_init(parameter)) {
		dr_fatal_notify("Failed to initialize backend.\n");
		return EXIT_FAILURE;
	}

	if (args.has_arg("-autodpi")) {
		dr_set_screen_scale(-1);
	}
	else if (const char* scaling = args.gimme_arg("-screen_scale", 1)) {
		if (scaling[0] >= '0' && scaling[0] <= '9') {
			dr_set_screen_scale(atoi(scaling));
		}
	}

	// Get optimal resolution.
	if (disp_width == 0 || disp_height == 0) {
		resolution const res = dr_query_screen_resolution();
		if (fullscreen != WINDOWED) {
			disp_width  = res.w;
			disp_height = res.h;
		}
		else {
			disp_width  = min(704, res.w);
			disp_height = min(560, res.h);
		}
	}

	DBG_MESSAGE("simu_main()", "simgraph_init disp_width=%d, disp_height=%d, fullscreen=%d", disp_width, disp_height, fullscreen);
	if (!simgraph_init(scr_size(disp_width, disp_height), fullscreen)) {
		dbg->error("simu_main()", "Failed to initialize graphics system.");
		return EXIT_FAILURE;
	}
	DBG_MESSAGE("simu_main()", ".. results in disp_width=%d, disp_height=%d", display_get_width(), display_get_height());
	// now that the graphics system has already started
	// the saved colours can be converted to the system format
	env_t_rgb_to_system_colors();

	// parse colours now that the graphics system has started
	// default simuconf.tab
	if(  found_simuconf  ) {
		if(simuconf.open(path_to_simuconf)) {
			dbg->message("simu_main()", "Loading colours from %sconfig/simuconf.tab", env_t::base_dir);

			// we do not allow to change the global font name also from the pakset ...
			std::string old_fontname = env_t::fontname;

			env_t::default_settings.parse_colours( simuconf );
			simuconf.close();

			env_t::fontname = old_fontname;
		}
	}

	// a portable installation could have a personal simuconf.tab in the main dir of simutrans
	// otherwise it is in ~/simutrans/simuconf.tab
	obj_conf = string(env_t::user_dir) + "simuconf.tab";
	if (simuconf.open(obj_conf.c_str())) {
		dbg->message("simu_main()", "Parsing %s", obj_conf.c_str() );
		env_t::default_settings.parse_colours( simuconf );
		simuconf.close();
	}

	// prepare skins first
	bool themes_ok = false;
	if(  const char *themestr = args.gimme_arg("-theme", 1)  ) {
		dr_chdir( env_t::user_dir );
		dr_chdir( "themes" );
		themes_ok = gui_theme_t::themes_init(themestr, true, false);
		if(  !themes_ok  ) {
			dr_chdir( env_t::base_dir );
			dr_chdir( "themes" );
			themes_ok = gui_theme_t::themes_init(themestr, true, false);
		}
	}
	// next try the last used theme
	if(  !themes_ok  &&  env_t::default_theme.c_str()!=NULL  ) {
		dr_chdir( env_t::user_dir );
		dr_chdir( "themes" );
		themes_ok = gui_theme_t::themes_init( env_t::default_theme, true, false );
		if(  !themes_ok  ) {
			dr_chdir( env_t::base_dir );
			dr_chdir( "themes" );
			themes_ok = gui_theme_t::themes_init( env_t::default_theme, true, false );
		}
	}
	// specified themes not found => try default themes
#if COLOUR_DEPTH != 0
	if(  !themes_ok  ) {
		dr_chdir( env_t::base_dir );
		dr_chdir( "themes" );
#ifndef __ANDROID__
		themes_ok = gui_theme_t::themes_init("themes.tab",true,false);
#else
		themes_ok = gui_theme_t::themes_init("theme-large.tab", true, false);
#endif
	}
	if(  !themes_ok  ) {
		dbg->fatal( "simu_main()", "No GUI themes found! Please re-install!" );
	}
	dr_chdir( env_t::base_dir );

	// The loading screen needs to be initialized
	display_show_pointer(1);

	// if no object files given, we ask the user
	int retries=0;
	while(   env_t::pak_name.empty()  &&  retries < 2  ) {
		if(  ask_objfilename() == 0  ) {
			retries++;
		}
		if(  env_t::quit_simutrans  ) {
			simgraph_exit();
			return EXIT_SUCCESS;
		}
		else if (env_t::pak_name.empty()) {
			// try to download missing paks
			install_objfilename(); // all other
		}
	}
#else
	// headless server
	dr_chdir( env_t::base_dir );
	if(  env_t::pak_name.empty()  ) {
		dr_fatal_notify(
			"*** No pak set found ***\n"
			"\n"
			"Please install a pak set and select it using the '-objects'\n"
			"command line parameter or the 'pak_file_path' simuconf.tab entry.");
		simgraph_exit();
		return EXIT_FAILURE;
	}
#endif

	// check for valid pak path
	{
		FILE* const f = dr_fopen((env_t::pak_dir+"ground.Outside.pak").c_str(), "r");
		if(  !f  ) {
			cbuffer_t errmsg;
			errmsg.printf(
				"The file 'ground.Outside.pak' was not found in\n"
				"'%s'.\n"
				"This file is required for a valid pak set (graphics).\n"
				"Please install and select a valid pak set.",
				env_t::pak_dir.c_str());

			dr_fatal_notify(errmsg);
			simgraph_exit();
			return EXIT_FAILURE;
		}
		fclose(f);
	}

	// now find the pak specific tab file ...
	obj_conf = env_t::pak_dir + "config" + PATH_SEPARATOR + "simuconf.tab";
	if (simuconf.open(obj_conf.c_str())) {
		env_t::default_settings.set_way_height_clearance( 0 );
		uint8 show_month = env_t::env_t::show_month;

		dbg->message("simu_main()", "Parsing %s", obj_conf.c_str());
		env_t::default_settings.parse_simuconf( simuconf );
		env_t::default_settings.parse_colours( simuconf );

		if (show_month != env_t::show_month) {
			dbg->warning("Parsing simuconf.tab", "Pakset %s defines show_month will be ignored!", env_t::pak_name.c_str());
		}
		env_t::show_month = show_month;

		pak_diagonal_multiplier = env_t::default_settings.get_pak_diagonal_multiplier();
		pak_height_conversion_factor = env_t::pak_height_conversion_factor;
		pak_tile_height = TILE_HEIGHT_STEP;

		if(  env_t::default_settings.get_way_height_clearance() == 0  ) {
			// ok, set default as conversion factor
			env_t::default_settings.set_way_height_clearance( pak_height_conversion_factor );
		}
		simuconf.close();
	}

	// and parse again the user settings
	obj_conf = string(env_t::user_dir) + "simuconf.tab";
	if (simuconf.open(obj_conf.c_str())) {

		dbg->message("simu_main()", "Parsing %s", obj_conf.c_str());
		env_t::default_settings.parse_simuconf( simuconf );
		env_t::default_settings.parse_colours( simuconf );

		simuconf.close();
	}

	// load with private addons (now in addons/pak-name either in simutrans main dir or in userdir)
	if(  args.gimme_arg("-objects", 1)  ||  args.gimme_arg("-set_pakdir", 1)  ) {
		if(args.has_arg("-addons")) {
			env_t::default_settings.set_with_private_paks( true );
		}
		if(args.has_arg("-noaddons")) {
			env_t::default_settings.set_with_private_paks( false );
		}
	}

	// parse ~/simutrans/pakxyz/config.tab"
	if(  env_t::default_settings.get_with_private_paks()  ) {
		obj_conf = string(env_t::user_dir) + "addons/" + env_t::pak_name + "config/simuconf.tab";

		if (simuconf.open(obj_conf.c_str())) {
			dbg->message("simu_main()", "Parsing %s", obj_conf.c_str());
			env_t::default_settings.parse_simuconf( simuconf );
			env_t::default_settings.parse_colours( simuconf );
			simuconf.close();
		}

		// and parse user settings again ...
		obj_conf = string(env_t::user_dir) + "simuconf.tab";
		if (simuconf.open(obj_conf.c_str())) {
			dbg->message("simu_main()", "Parsing %s", obj_conf.c_str());
			env_t::default_settings.parse_simuconf( simuconf );
			env_t::default_settings.parse_colours( simuconf );
			simuconf.close();
		}
	}

	// now (re)set the correct length and other pak set only settings
	env_t::default_settings.set_pak_diagonal_multiplier( pak_diagonal_multiplier );
	vehicle_base_t::set_diagonal_multiplier( pak_diagonal_multiplier, pak_diagonal_multiplier );
	env_t::pak_height_conversion_factor = pak_height_conversion_factor;
	TILE_HEIGHT_STEP = pak_tile_height;

	convoihandle_t::init( 1024 );
	linehandle_t::init( 1024 );
	halthandle_t::init( 1024 );

#ifdef MULTI_THREAD
	// set number of threads
	if(  const char *ref_str = args.gimme_arg("-threads", 1)  ) {
		uint8 want_threads = atoi(ref_str);
		env_t::num_threads = clamp( want_threads, (uint8)1u, dr_get_max_threads() );
		env_t::num_threads = min( env_t::num_threads, MAX_THREADS );
		dbg->message("simu_main()","Requested %d threads.", env_t::num_threads );
	}
#else
	if(  env_t::num_threads > 1  ) {
		env_t::num_threads = 1;
		dbg->message("simu_main()","Multithreading not enabled: threads = %d ignored.", env_t::num_threads );
	}
#endif

	// just check before loading objects
	if(  !args.has_arg("-nosound")  &&  dr_init_sound()  ) {
		dbg->message("simu_main()","Reading compatibility sound data ...");
		sound_desc_t::init();
	}
	else {
		sound_set_mute(true);
	}

	// Adam - Moved away loading from simu_main() and placed into translator for better modularization
	if(  !translator::load()  ) {
		// installation error: likely only program started
		dbg->fatal("simu_main()",
			"Unable to load any language files\n"
			"*** PLEASE INSTALL PROPER BASE FILES ***\n"
			"\n"
			"either run ./get_lang_files.sh\n"
			"\n"
			"or\n"
			"\n"
			"download a complete simutrans archive and put the text/ folder here."
		);
	}

	// use requested language (if available)
	if(  args.gimme_arg("-lang", 1)  ) {
		const char *iso = args.gimme_arg("-lang", 1);
		if(  strlen(iso)>=2  ) {
			translator::set_language( iso );
		}
		if(  translator::get_language()==-1  ) {
			dbg->fatal("simu_main()", "Illegal language definition \"%s\"", iso );
		}
		env_t::language_iso = translator::get_lang()->iso_base;
	}
	else if(  found_settings  ) {
		translator::set_language( env_t::language_iso );
	}

	// simgraph_init loads default fonts, now we need to load (if not set otherwise)
//	sprachengui_t::init_font_from_lang( strcmp(env_t::fontname.c_str(), FONT_PATH_X "prop.fnt")==0 );
	dr_chdir(env_t::base_dir);

	dbg->message("simu_main()","Reading city configuration ...");
	stadt_t::cityrules_init();

	dbg->message("simu_main()","Reading speedbonus configuration ...");
	vehicle_builder_t::speedbonus_init();

	dbg->message("simu_main()","Reading menu configuration ...");
	tool_t::init_menu();

	// loading all objects in the pak
	pakset_manager_t::load_pakset(env_t::default_settings.get_with_private_paks());

	// load tool scripts
	dbg->message("simu_main()","Reading tool scripts ...");
	dr_chdir( env_t::base_dir );
	script_tool_manager_t::load_scripts((env_t::pak_dir + "tool/").c_str());
	if(  env_t::default_settings.get_with_private_paks()  ) {
		dr_chdir( env_t::user_dir );
		script_tool_manager_t::load_scripts(("addons/" + env_t::pak_name + "tool/").c_str());
	}

	dbg->message("simu_main()","Reading menu configuration ...");
	dr_chdir( env_t::base_dir );
	if (!tool_t::read_menu(env_t::pak_dir + "config/menuconf.tab")) {
		// Fatal error while reading menuconf.tab, we cannot continue!
		dbg->fatal(
			"Could not read %sconfig/menuconf.tab.\n"
			"This file is required for a valid pak set (graphics).\n"
			"Please install and select a valid pak set.",
			env_t::pak_dir.c_str());
	}

#if COLOUR_DEPTH != 0
	// reread theme
	dr_chdir( env_t::user_dir );
	dr_chdir( "themes" );

	themes_ok = gui_theme_t::themes_init( env_t::default_theme, true, false );
	if(  !themes_ok  ) {
		dr_chdir( env_t::base_dir );
		dr_chdir( "themes" );
		themes_ok = gui_theme_t::themes_init( env_t::default_theme, true, false );
	}
#endif
	dr_chdir( env_t::base_dir );

	if(  translator::get_language()==-1  ) {
		// try current language
		const char* loc = dr_get_locale();
		if(  loc==NULL  ||  !translator::set_language( loc )  ) {
			loc = dr_get_locale_string();
			if(  loc==NULL  ||  !translator::set_language( loc )  ) {
				ask_language();
			}
			else {
				sprachengui_t::init_font_from_lang();
			}
		}
		else {
			sprachengui_t::init_font_from_lang();
		}
	}

	bool new_world = true;
	std::string loadgame;

	bool pause_after_load = false;
	if(  args.has_arg("-pause")  ) {
		if( env_t::server ) {
			env_t::pause_server_no_clients = true;
		}
		else {
			pause_after_load = true;
		}
	}

	if(  args.has_arg("-load")  ) {
		cbuffer_t buf;
		dr_chdir( env_t::user_dir );
		/**
		 * Added automatic adding of extension
		 */
		const char *name = args.gimme_arg("-load", 1);
		if (strstart(name, "net:")) {
			buf.append( name );
		}
		else {
			buf.printf( SAVE_PATH_X "%s", searchfolder_t::complete(name, "sve").c_str() );
		}
		dbg->message("simu_main()", "Loading savegame \"%s\"", name );
		loadgame = buf;
		new_world = false;
	}

	// compare two savegames
	if (!new_world  &&  strstart(loadgame.c_str(), "net:")==NULL  &&  args.has_arg("-compare")) {
		cbuffer_t buf;
		dr_chdir( env_t::user_dir );

		const char *name = args.gimme_arg("-compare", 1);
		buf.printf( SAVE_PATH_X "%s", searchfolder_t::complete(name, "sve").c_str() );
		// open both files
		loadsave_t file1, file2;
		if (file1.rd_open(loadgame.c_str()) == loadsave_t::FILE_STATUS_OK  &&  file2.rd_open(buf) == loadsave_t::FILE_STATUS_OK) {
			karte_t *welt = new karte_t();

			compare_loadsave_t compare(&file1, &file2);
			welt->load(&compare);

			delete welt;
			file1.close();
			file2.close();
		}
	}

	// recover last server game
	if(  new_world  &&  env_t::server  ) {
		dr_chdir( env_t::user_dir );
		loadsave_t file;
		static char servername[128];
		sprintf( servername, "server%d-network.sve", env_t::server );
		// try recover with the latest savegame
		if(  file.rd_open(servername) == loadsave_t::FILE_STATUS_OK  ) {
			// compare pakset (objfilename has trailing path separator, pak_extension not)
			if (strstart(env_t::pak_name.c_str(), file.get_pak_extension())) {
				// same pak directory - load this
				loadgame = servername;
				new_world = false;
			}
			file.close();
		}
	}

	bool old_restore_UI = env_t::restore_UI;
	if(  new_world  &&  env_t::reload_and_save_on_quit  ) {
		// construct from pak name an autosave if requested
		loadsave_t file;

		std::string pak_name( "autosave-" );
		pak_name.append( env_t::pak_name );
		pak_name.erase( pak_name.length()-1 );
		pak_name.append( ".sve" );
		dr_chdir( env_t::user_dir );
		unlink( "temp-load.sve" );
		if( dr_rename(pak_name.c_str(), "temp-load.sve") == 0 ) {
			loadgame = "temp-load.sve";
			new_world = false;
		}
		env_t::restore_UI = true;
	}

	// still nothing to be loaded => search for demo games
	if(  new_world  ) {
		dr_chdir( env_t::base_dir );

		const std::string path = env_t::pak_dir + "demo.sve";

		// access did not work!
		if(  FILE *const f = dr_fopen(path.c_str(), "rb")  ) {
			// there is a demo game to load
			loadgame = path;
			fclose(f);
			DBG_MESSAGE("simu_main()","loadgame file found at %s",path.c_str());
		}
	}

	if(  args.has_arg("-timeline")  ) {
		const char* ref_str = args.gimme_arg("-timeline", 1);
		if(  ref_str != NULL  ) {
			env_t::default_settings.set_use_timeline( atoi(ref_str) );
		}
	}

	if(  args.has_arg("-startyear")  ) {
		const char * ref_str = args.gimme_arg("-startyear", 1); //1930
		if(  ref_str != NULL  ) {
			env_t::default_settings.set_starting_year( clamp(atoi(ref_str),1,2999) );
		}
	}

	// now always writing in user dir (which points to the data dir in multiuser mode)
	dr_chdir( env_t::user_dir );

	// init midi before loading sounds
	if(  dr_init_midi()  ) {
		dbg->message("simu_main()","Reading midi data ...");
		if(  !midi_init( env_t::pak_dir.c_str() )  &&  !midi_init( env_t::user_dir )  &&  !midi_init( env_t::base_dir )  ) {
			midi_set_mute(true);
			dbg->message("simu_main()","Midi disabled ...");
		}
		if(args.has_arg("-nomidi")) {
			midi_set_mute(true);
		}
#ifdef USE_FLUIDSYNTH_MIDI
		// Audio is ok, but we failed to find a soundfont
		if(  strcmp( env_t::soundfont_filename.c_str(), "Error" ) == 0  ) {
			midi_set_mute(true);
		}
#endif
	}
	else {
		dbg->message("simu_main()","Midi disabled ...");
		midi_set_mute(true);
	}

	if(  args.has_arg("-mute")  ) {
		sound_set_mute(true);
		midi_set_mute(true);
	}

	// restore previous sound settings ...
	sound_set_mute(  env_t::global_mute_sound  ||  sound_get_mute() );
	midi_set_mute(  env_t::mute_midi  ||  midi_get_mute() );
	sound_set_shuffle_midi( env_t::shuffle_midi!=0 );
	sound_set_global_volume( env_t::global_volume );
	sound_set_midi_volume( env_t::midi_volume );
	if(  !midi_get_mute()  ) {
		// not muted => play random song
		midi_play( env_t::shuffle_midi ? -1 : 0 );
		// reset volume after first play call else no/low sound or music with win32 and sdl
		sound_set_midi_volume( env_t::midi_volume );
	}

	karte_t *welt = new karte_t();
	main_view_t *view = new main_view_t(welt);
	welt->set_view( view );

	interaction_t *eventmanager = new interaction_t(welt->get_viewport());
	welt->set_eventmanager( eventmanager );

	// some messages about old vehicle may appear ...
	welt->get_message()->set_message_flags(0, 0, 0, 0);

	// set the frame per second
	if(  const char *ref_str = args.gimme_arg("-fps", 1)  ) {
		const int want_refresh = atoi(ref_str);
		env_t::fps = clamp(want_refresh, (int)env_t::min_fps, (int)env_t::max_fps);
	}

	// query server stuff
	// Enable server announcements
	if(  args.has_arg("-announce")  ) {
		env_t::server_announce = 1;
		DBG_DEBUG( "simu_main()", "Server will be announced." );
	}

	if(  const char *ref_str = args.gimme_arg("-server_dns", 1)  ) {
		env_t::server_dns = ref_str;
		DBG_DEBUG( "simu_main()", "Server IP set to '%s'.", ref_str );
	}

	if(  const char *ref_str = args.gimme_arg("-server_altdns", 1)  ) {
		env_t::server_alt_dns = ref_str;
		DBG_DEBUG( "simu_main()", "Server IP set to '%s'.", ref_str );
	}

	if(  const char *ref_str = args.gimme_arg("-server_name", 1)  ) {
		env_t::server_name = ref_str;
		DBG_DEBUG( "simu_main()", "Server name set to '%s'.", ref_str );
	}

	if(  const char *ref_str = args.gimme_arg("-server_admin_pw", 1)  ) {
		env_t::server_admin_pw = ref_str;
	}

	if(  env_t::server_dns.empty()  &&  !env_t::server_alt_dns.empty()  ) {
		dbg->warning( "simu_main()", "server_altdns but not server_dns set. Please use server_dns first!" );
		env_t::server_dns = env_t::server_alt_dns;
		env_t::server_alt_dns.clear();
	}

	dr_chdir(env_t::user_dir);

	// reset random counter to true randomness
	setsimrand(dr_time(), dr_time());
	clear_random_mode( 7 ); // allow all

	scenario_t *scen = NULL;
	if(  const char *scen_name = args.gimme_arg("-scenario", 1)  ) {
		scen = new scenario_t(welt);

		intr_set_view(view);
		win_set_world(welt);

		const char *err = "";
		if (env_t::default_settings.get_with_private_paks()) {
			// try addon directory first
			err = scen->init(("addons/" + env_t::pak_name + "scenario/").c_str(), scen_name, welt);
		}
		if (err) {
			// no addon scenario, look in pakset
			err = scen->init((env_t::pak_dir + "scenario/").c_str(), scen_name, welt);
		}
		if(  err  ) {
			dbg->error("simu_main()", "Could not load scenario %s%s: %s", env_t::pak_name.c_str(), scen_name, err);
			delete scen;
			scen = NULL;
		}
		else {
			new_world = false;
		}
	}

	if(  scen == NULL && (loadgame==""  ||  !welt->load(loadgame.c_str()))  ) {
		// create a default map
		DBG_MESSAGE("simu_main()", "Init with default map (failing will be a pak error!)");

		// no autosave on initial map during the first six months
		loadgame = "";
		new_world = true;

		sint32 old_autosave = env_t::autosave;
		env_t::autosave = false;

		settings_t sets;
		sets.copy_city_road( env_t::default_settings );
		sets.set_default_climates();
		sets.set_use_timeline( 1 );
		sets.set_size(64,64);
		sets.set_city_count(1);
		sets.set_factory_count(3);
		sets.set_tourist_attractions(1);
		sets.set_traffic_level(7);
		welt->init(&sets,0);

		//  start in June ...
		intr_set_view(view);
		win_set_world(welt);

		tool_t::toolbar_tool[0]->init(welt->get_active_player());

		welt->set_fast_forward(true);
		welt->sync_step(5000);
		welt->step_month(5);
		welt->step();
		welt->step();

		env_t::autosave = old_autosave;
	}
	else {
		// override freeplay setting when provided on command line
		if(  (args.has_arg("-freeplay"))  ) {
			welt->get_settings().set_freeplay( true );
		}

		if (scen == NULL) {
			// just init view (world was loaded from file)
			intr_set_view(view);
			win_set_world(welt);
		}

		tool_t::toolbar_tool[0]->init(welt->get_active_player());
	}

	welt->set_fast_forward(false);
	baum_t::recalc_outline_color();

	uint32 quit_month = 0x7FFFFFFFu;

#if defined DEBUG || defined PROFILE
	// do a render test?
	if (args.has_arg("-times")) {
		show_times(welt, view);
	}
#endif

	// finish after a certain month? (must be entered decimal, i.e. 12*year+month
	if(  args.has_arg("-until")  ) {
		const char *until = args.gimme_arg("-until", 1);
		int year = -1, month = -1;
		if ( sscanf(until, "%i.%i", &year, &month) == 2) {
			quit_month = month+year*12-1;
		}
		else {
			quit_month = atoi(until);
		}
		welt->set_fast_forward(true);
	}

	welt->reset_timer();
	if(  !env_t::networkmode  &&  !env_t::server  ) {
#ifdef display_in_main
		view->display(true);
		intr_refresh_display(true);
#endif
		intr_enable();
	}
	else {
		intr_disable();
	}


#ifdef USE_SOFTPOINTER
	// give user a mouse to work with
	if (skinverwaltung_t::mouse_cursor != NULL) {
		// we must use our softpointer (only Allegro!)
		display_set_pointer(skinverwaltung_t::mouse_cursor->get_image_id(0));
	}
#endif
	display_show_pointer(true);
	display_show_load_pointer(0);

	welt->set_dirty();

	// simgraph_init loads default fonts, now we need to load
	// the real fonts for the current language, if not set otherwise
	sprachengui_t::init_font_from_lang();

	if(   !(env_t::reload_and_save_on_quit  &&  !new_world)  ) {
		destroy_all_win(true);
	}
	env_t::restore_UI = old_restore_UI;

	if(  !env_t::networkmode  &&  !env_t::server  &&  new_world  ) {
		welt->get_message()->clear();
	}
#ifdef USE_FLUIDSYNTH_MIDI
	if(  strcmp( env_t::soundfont_filename.c_str(), "Error" ) == 0  ) {
		create_win( 0,0, new news_img("No soundfont found!\n\nMusic won't play until you load a soundfont from the sound options menu."), w_info, magic_none );
	}
#endif
	while(  !env_t::quit_simutrans  ) {
		// play next tune?
		check_midi();

		if(  !env_t::networkmode  &&  new_world  ) {
			dbg->message("simu_main()", "Show banner ... " );
			ticker::add_msg("Welcome to Simutrans", koord3d::invalid, PLAYER_FLAG | color_idx_to_rgb(COL_SOFT_BLUE));
			modal_dialogue( new banner_t(), magic_none, welt, never_quit );
			// only show new world, if no other dialogue is active ...
			new_world = win_get_open_count()==0;
		}
		if(  env_t::quit_simutrans  ) {
			break;
		}

		// to purge all previous old messages
		welt->get_message()->set_message_flags(env_t::message_flags[0], env_t::message_flags[1], env_t::message_flags[2], env_t::message_flags[3]);

		if (pakset_manager_t::needs_doubled_warning_message()) {
			welt->get_message()->add_message(pakset_manager_t::get_doubled_warning_message(), koord3d::invalid, message_t::general | message_t::do_not_rdwr_flag);
		}

		if(  !env_t::networkmode  &&  !env_t::server  ) {
			welt->set_pause( pause_after_load );
			pause_after_load = false;
		}

		if(  new_world  ) {
			modal_dialogue( new welt_gui_t(&env_t::default_settings), magic_welt_gui_t, welt, never_quit );
			if(  env_t::quit_simutrans  ) {
				break;
			}
		}
		dbg->message("simu_main()", "Running world, pause=%i, fast forward=%i ... ", welt->is_paused(), welt->is_fast_forward() );
		loadgame = ""; // only first time

		// run the loop
		welt->interactive(quit_month);

		new_world = true;
		welt->get_message()->get_message_flags(&env_t::message_flags[0], &env_t::message_flags[1], &env_t::message_flags[2], &env_t::message_flags[3]);
		welt->set_fast_forward(false);
		welt->set_pause(false);
		setsimrand(dr_time(), dr_time());

		dbg->message("simu_main()", "World finished ..." );
	}

	intr_disable();

	// save settings
	{
		dr_chdir( env_t::user_dir );
		loadsave_t settings_file;
		if(  settings_file.wr_open("settings.xml",loadsave_t::xml,0,"settings only/",SAVEGAME_VER_NR) == loadsave_t::FILE_STATUS_OK  ) {
			env_t::rdwr(&settings_file);
			env_t::default_settings.rdwr(&settings_file);
			settings_file.close();
		}
	}

	destroy_all_win( true );
	tool_t::exit_menu();

	delete welt;
	welt = NULL;

	delete view;
	view = NULL;

	delete eventmanager;
	eventmanager = NULL;

	remove_port_forwarding( env_t::server );
	network_core_shutdown();

	simgraph_exit();

	close_midi();

#if 0
	// free all list memories (not working, since there seems to be unitialized list still waiting for automated destruction)
	freelist_t::free_all_nodes();
#endif

	return EXIT_SUCCESS;
}
