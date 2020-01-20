#if defined(_M_X64)  ||  defined(__x86_64__)
#if   __GNUC__
#warning "Simutrans is preferably compiled as 32 bit binary!"
#endif
#endif


#include <stdio.h>
#include <string>
#include <new>

#include "pathes.h"

#include "simmain.h"
#include "simworld.h"
#include "simware.h"
#include "display/simview.h"
#include "gui/simwin.h"
#include "gui/gui_theme.h"
#include "simhalt.h"
#include "display/simimg.h"
#include "simcolor.h"
#include "simskin.h"
#include "simconst.h"
#include "boden/boden.h"
#include "boden/wasser.h"
#include "simcity.h"
#include "player/simplay.h"
#include "simsound.h"
#include "simintr.h"
#include "simloadingscreen.h"
#include "simticker.h"
#include "simmesg.h"
#include "simmenu.h"
#include "siminteraction.h"

#include "simsys.h"
#include "display/simgraph.h"
#include "simevent.h"

#include "simversion.h"

#include "gui/banner.h"
#include "gui/pakselector.h"
#include "gui/welt.h"
#include "gui/help_frame.h"
#include "gui/sprachen.h"
#include "gui/climates.h"
#include "gui/messagebox.h"
#include "gui/loadsave_frame.h"
#include "gui/load_relief_frame.h"
#include "gui/scenario_frame.h"

#include "obj/baum.h"

#include "utils/simstring.h"
#include "utils/searchfolder.h"

#include "network/network.h"	// must be before any "windows.h" is included via bzlib2.h ...
#include "dataobj/loadsave.h"
#include "dataobj/environment.h"
#include "dataobj/tabfile.h"
#include "dataobj/settings.h"
#include "dataobj/translator.h"
#include "network/pakset_info.h"

#include "descriptor/reader/obj_reader.h"
#include "descriptor/sound_desc.h"
#include "descriptor/ground_desc.h"

#include "music/music.h"
#include "sound/sound.h"

#include "utils/cbuffer_t.h"
#include "utils/simrandom.h"

#include "bauer/vehikelbauer.h"

#include "vehicle/simvehicle.h"
#include "vehicle/simroadtraffic.h"

using std::string;

/* diagnostic routine:
 * show the size of several internal structures
 */
static void show_sizes()
{
	DBG_MESSAGE("Debug", "size of structures");

	DBG_MESSAGE("sizes", "koord: %d", sizeof(koord));
	DBG_MESSAGE("sizes", "koord3d: %d", sizeof(koord3d));
	DBG_MESSAGE("sizes", "ribi_t::ribi: %d", sizeof(ribi_t::ribi));
	DBG_MESSAGE("sizes", "halthandle_t: %d\n", sizeof(halthandle_t));

	DBG_MESSAGE("sizes", "obj_t: %d", sizeof(obj_t));
	DBG_MESSAGE("sizes", "gebaeude_t: %d", sizeof(gebaeude_t));
	DBG_MESSAGE("sizes", "baum_t: %d", sizeof(baum_t));
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
}



// render tests ...
static void show_times(karte_t *welt, main_view_t *view)
{
 	intr_set(welt, view);
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
		FOR( slist_tpl<weg_t *>, const w, weg_t::get_alle_wege() ) {
			grund_t *dummy;
			welt->lookup( w->get_pos() )->get_neighbour( dummy, invalid_wt, ribi_t::north );
		}
	}
	dbg->message( "grund_t::get_neighbour()", "%i iterations took %li ms", i*weg_t::get_alle_wege().get_count(), dr_time() - ms );

 	ms = dr_time();
	for (i = 0; i < 2000; i++) {
 		welt->sync_step(200,true,true);
 		welt->step();
 	}
	dbg->message( "welt->sync_step/step(200,1,1)", "%i iterations took %li ms", i, dr_time() - ms );
}



void modal_dialogue( gui_frame_t *gui, ptrdiff_t magic, karte_t *welt, bool (*quit)() )
{
	if(  display_get_width()==0  ) {
		dbg->error( "modal_dialogue", "called without a display driver => nothing will be shown!" );
		env_t::quit_simutrans = true;
		// cannot handle this!
		return;
	}

	// switch off autosave
	sint32 old_autosave = env_t::autosave;
	env_t::autosave = 0;

	event_t ev;
	create_win( (display_get_width()-gui->get_windowsize().w)/2, (display_get_height()-gui->get_windowsize().h)/2, gui, w_info, magic );

	if(  welt  ) {
		welt->set_pause( false );
		welt->reset_interaction();
		welt->reset_timer();

		uint32 ms_pause = max( 25, 1000/env_t::fps );
		uint32 last_step = dr_time();
		uint step_count = 5;

		while(  win_is_open(gui)  &&  !env_t::quit_simutrans  &&  !quit()  ) {
			do {
				DBG_DEBUG4("modal_dialogue", "calling win_poll_event");
				win_poll_event(&ev);
				// no toolbar events
				if(  ev.my < env_t::iconsize.h  ) {
					ev.my = env_t::iconsize.h;
				}
				if(  ev.cy < env_t::iconsize.h  ) {
					ev.cy = env_t::iconsize.h;
				}
				if(  ev.ev_class == EVENT_KEYBOARD  &&  ev.ev_code == SIM_KEY_F1  ) {
					if(  gui_frame_t *win = win_get_top()  ) {
						if(  const char *helpfile = win->get_help_filename()  ) {
							help_frame_t::open_help_on( helpfile );
							continue;
						}
					}
				}
				DBG_DEBUG4("modal_dialogue", "calling check_pos_win");
				check_pos_win(&ev);
				if(  ev.ev_class == EVENT_SYSTEM  &&  ev.ev_code == SYSTEM_QUIT  ) {
					env_t::quit_simutrans = true;
					break;
				}
				dr_sleep(5);
			} while(  dr_time() - last_step < ms_pause );

			DBG_DEBUG4("modal_dialogue", "calling welt->sync_step");
			welt->sync_step( ms_pause, true, true );

			if(  step_count--==0  ) {
				DBG_DEBUG4("modal_dialogue", "calling welt->step");
				intr_set_last_time(last_step); // do not call sync_step twice unless step takes too long
				welt->step();
				step_count = 5;
			}
			last_step += ms_pause;
		}
	}
	else {
		display_show_pointer(true);
		display_show_load_pointer(0);
		display_fillbox_wh_rgb( 0, 0, display_get_width(), display_get_height(), color_idx_to_rgb(COL_BLACK), true );
		while(  win_is_open(gui)  &&  !env_t::quit_simutrans  &&  !quit()  ) {
			// do not move, do not close it!
			dr_sleep(50);
			// check for events again after waiting
			if (quit()) {
				break;
			}
			dr_prepare_flush();
			gui->draw(win_get_pos(gui), gui->get_windowsize());
			dr_flush();

			display_poll_event(&ev);
			if(ev.ev_class==EVENT_SYSTEM) {
				if (ev.ev_code==SYSTEM_RESIZE) {
					// main window resized
					simgraph_resize( ev.size_x, ev.size_y );
					dr_prepare_flush();
					display_fillbox_wh_rgb( 0, 0, ev.size_x, ev.size_y, color_idx_to_rgb(COL_BLACK), true );
					dr_flush();
				}
				else if (ev.ev_code == SYSTEM_QUIT) {
					env_t::quit_simutrans = true;
					break;
				}
			}
			else {
				// other events
				check_pos_win(&ev);
			}
		}
		display_show_load_pointer(1);
		dr_prepare_flush();
		display_fillbox_wh_rgb( 0, 0, display_get_width(), display_get_height(), color_idx_to_rgb(COL_BLACK), true );
		dr_flush();
	}

	// just trigger not another following window => wait for button release
	if (IS_LEFTCLICK(&ev)) {
		do {
			display_get_event(&ev);
		} while (!IS_LEFTRELEASE(&ev));
	}

	// restore autosave
	env_t::autosave = old_autosave;
}


// some routines for the modal display
static bool never_quit() { return false; }
static bool empty_objfilename() { return !env_t::objfilename.empty(); }
static bool no_language() { return translator::get_language()!=-1; }

static bool wait_for_key()
{
	event_t ev;
	display_poll_event(&ev);
	if(  ev.ev_class==EVENT_KEYBOARD  ) {
		if(  ev.ev_code==SIM_KEY_ESCAPE  ||  ev.ev_code==SIM_KEY_SPACE  ||  ev.ev_code==SIM_KEY_BACKSPACE  ) {
			return true;
		}
	}
	event_t *nev = new event_t();
	*nev = ev;
	queue_event(nev);
	return false;
}


/**
 * Show pak selector
 */
static void ask_objfilename()
{
	pakselector_t* sel = new pakselector_t();
	// notify gui to load list of paksets
	event_t ev;
	ev.ev_class = INFOWIN;
	ev.ev_code  = WIN_OPEN;
	sel->infowin_event(&ev);

	if(sel->has_pak()) {
		destroy_all_win(true);	// since eventually the successful load message is still there ....
		modal_dialogue( sel, magic_none, NULL, empty_objfilename );
	}
	else {
		delete sel;
	}
}



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
		destroy_all_win(true);	// since eventually the successful load message is still there ....
		modal_dialogue( sel, magic_none, NULL, no_language );
		destroy_win( sel );
	}
}



/**
 * This function will be set in the main function as the handler the runtime environment will
 * call in the case it lacks memory for new()
 */
static void sim_new_handler()
{
	dbg->fatal("sim_new_handler()", "OUT OF MEMORY");
}


static const char *gimme_arg(int argc, char *argv[], const char *arg, int off)
{
	for(  int i = 1;  i < argc;  i++  ) {
		if(strcmp(argv[i], arg) == 0  &&  i < argc - off  ) {
			return argv[i + off];
		}
	}
	return NULL;
}


int simu_main(int argc, char** argv)
{
	static const sint16 resolutions[][2] = {
		{  640,  480 },
		{  800,  600 },
		{ 1024,  768 },
		{ 1280, 1024 },
		{  704,  560 } // try to force window mode with allegro
	};

	sint16 disp_width = 0;
	sint16 disp_height = 0;
	sint16 fullscreen = false;

	uint32 quit_month = 0x7FFFFFFFu;

	std::set_new_handler(sim_new_handler);

	env_t::init();

	// you really want help with this?
	if (gimme_arg(argc, argv, "-h",     0) ||
			gimme_arg(argc, argv, "-?",     0) ||
			gimme_arg(argc, argv, "-help",  0) ||
			gimme_arg(argc, argv, "--help", 0)) {
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
			"  by Hansj�rg Malthaner et. al.\n"
			"---------------------------------------\n"
			"command line parameters available: \n"
			" -addons             loads also addons (with -objects)\n"
			" -async              asynchronous images, only for SDL\n"
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
			" -midicmd COMMAND    set external midi command.(default environment variable SIMUTRANS_MIDI_CMD)\n"
			" -nosound            turns off ambient sounds\n"
			" -objects DIR_NAME/  load the pakset in specified directory\n"
			" -pause              starts game with paused after loading\n"
			" -res N              starts in specified resolution: \n"
			"                      1=640x480, 2=800x600, 3=1024x768, 4=1280x1024\n"
			" -screensize WxH     set screensize to width W and height H\n"
			" -server [PORT]      starts program as server (for network game)\n"
			"                     without port specified uses 13353\n"
			" -announce           Enable server announcements\n"
			" -autodpi            Scale for high DPI screens\n"
			" -server_dns FQDN/IP FQDN or IP address of server for announcements\n"
			" -server_name NAME   Name of server for announcements\n"
			" -server_admin_pw PW password for server administration\n"
			" -singleuser         Save everything in program directory (portable version)\n"
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
			" -until YEAR.MONTH   quits when MONTH of YEAR starts\n"
#endif
			" -use_workdir        use current dir as basedir\n"
		);
		return 0;
	}

#ifdef __BEOS__
	if (1) // since BeOS only supports relative paths ...
#else
	// use current dir as basedir, else use program_dir
	if (gimme_arg(argc, argv, "-use_workdir", 0))
#endif
	{
		// save the current directories
		dr_getcwd(env_t::program_dir, lengthof(env_t::program_dir));
		strcat( env_t::program_dir, PATH_SEPARATOR );
	}
	else {
		strcpy( env_t::program_dir, argv[0] );
		*(strrchr( env_t::program_dir, PATH_SEPARATOR[0] )+1) = 0;

#ifdef __APPLE__
		// change working directory from binary dir to bundle dir
		if(  !strcmp((env_t::program_dir + (strlen(env_t::program_dir) - 20 )), ".app/Contents/MacOS/")  ) {
			env_t::program_dir[strlen(env_t::program_dir) - 20] = 0;
			while(  env_t::program_dir[strlen(env_t::program_dir) - 1] != '/'  ) {
				env_t::program_dir[strlen(env_t::program_dir) - 1] = 0;
			}
		}
#endif

#ifdef __APPLE__
		// Detect if the binary is started inside an application bundle
		// Change working dir to bundle dir if that is the case or the game will search for the files inside the bundle
		if (!strcmp((env_t::program_dir + (strlen(env_t::program_dir) - 20 )), ".app/Contents/MacOS/"))
		{
			env_t::program_dir[strlen(env_t::program_dir) - 20] = 0;
			while (env_t::program_dir[strlen(env_t::program_dir) - 1] != '/') {
				env_t::program_dir[strlen(env_t::program_dir) - 1] = 0;
			}
		}
#endif

		dr_chdir( env_t::program_dir );
	}
	printf("Use work dir %s\n", env_t::program_dir);

	// only the specified pak conf should override this!
	uint16 pak_diagonal_multiplier = env_t::default_settings.get_pak_diagonal_multiplier();
	sint8 pak_tile_height = TILE_HEIGHT_STEP;
	sint8 pak_height_conversion_factor = env_t::pak_height_conversion_factor;

	// parsing config/simuconf.tab
	printf("Reading low level config data ...\n");
	bool found_settings = false;
	bool found_simuconf = false;
	bool multiuser = (gimme_arg(argc, argv, "-singleuser", 0) == NULL);

	tabfile_t simuconf;
	char path_to_simuconf[24];
	// was  config/simuconf.tab
	sprintf(path_to_simuconf, "config%csimuconf.tab", PATH_SEPARATOR[0]);
	if(simuconf.open(path_to_simuconf)) {
		tabfileobj_t contents;
		simuconf.read(contents);
		// use different save directories
		multiuser = !(contents.get_int("singleuser_install", !multiuser)==1  ||  !multiuser);
		found_simuconf = true;
		simuconf.close();
	}

	// init dirs now
	if(multiuser) {
		env_t::user_dir = dr_query_homedir();
	}
	else {
		// save in program directory
		env_t::user_dir = env_t::program_dir;
	}
	dr_chdir( env_t::user_dir );
	dr_mkdir("maps");
	dr_mkdir(SAVE_PATH);
	dr_mkdir(SCREENSHOT_PATH);


#ifdef REVISION
	const char *version = "Simutrans version " VERSION_NUMBER " from " VERSION_DATE " r" QUOTEME(REVISION) "\n";
#else
	const char *version = "Simutrans version " VERSION_NUMBER " from " VERSION_DATE "\n";
#endif


	/*** Begin logging set up ***/

#ifdef SYSLOG
	bool cli_syslog_enabled = (gimme_arg( argc, argv, "-syslog", 0 ) != NULL);
	const char* cli_syslog_tag = gimme_arg( argc, argv, "-tag", 1 );
#else
	bool cli_syslog_enabled = false;
	const char* cli_syslog_tag = NULL;
#endif

	env_t::verbose_debug = 0;
	if(  gimme_arg(argc, argv, "-debug", 0) != NULL  ) {
		const char *s = gimme_arg(argc, argv, "-debug", 1);
		int level = 4;
		if(s!=NULL  &&  s[0]>='0'  &&  s[0]<='9'  ) {
			level = atoi(s);
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
	else if (gimme_arg(argc, argv, "-log", 0)) {
		dr_chdir( env_t::user_dir );
		char temp_log_name[256];
		const char *logname = "simu.log";
		if(  gimme_arg(argc, argv, "-server", 0)  ) {
			const char *p = gimme_arg(argc, argv, "-server", 1);
			int portadress = p ? atoi( p ) : 13353;
			sprintf( temp_log_name, "simu-server%d.log", portadress==0 ? 13353 : portadress );
			logname = temp_log_name;
		}
		init_logging( logname, true, gimme_arg(argc, argv, "-log", 0 ) != NULL, version, NULL );
	}
	else if (gimme_arg(argc, argv, "-debug", 0) != NULL) {
		init_logging( "stderr", true, gimme_arg(argc, argv, "-debug", 0 ) != NULL, version, NULL );
	}
	else {
		init_logging(NULL, false, false, version, NULL);
	}
	/*** End logging set up ***/

	// now read last setting (might be overwritten by the tab-files)
	loadsave_t file;
	if(file.rd_open("settings.xml"))  {
		if(  file.get_version_int()>loadsave_t::int_version(SAVEGAME_VER_NR, NULL )  ) {
			// too new => remove it
			file.close();
			dr_remove("settings.xml");
		}
		else {
			found_settings = true;
			env_t::rdwr(&file);
			env_t::default_settings.rdwr(&file);
			file.close();
			// reset to false (otherwise these settings will persist)
			env_t::default_settings.set_freeplay( false );
			env_t::default_settings.set_allow_player_change( true );
			env_t::server_announce = 0;
		}
	}

	// continue parsing ...
	dr_chdir( env_t::program_dir );
	if(  found_simuconf  ) {
		if(simuconf.open(path_to_simuconf)) {
			// we do not allow to change the global font name
			std::string old_fontname = env_t::fontname;
			printf("parse_simuconf() at config/simuconf.tab: ");
			env_t::default_settings.parse_simuconf( simuconf, disp_width, disp_height, fullscreen, env_t::objfilename );
			simuconf.close();
			env_t::fontname = old_fontname;
		}
	}

	// a portable installation could have a personal simuconf.tab in the main dir of simutrans
	// otherwise it is in ~/simutrans/simuconf.tab
	string obj_conf = string(env_t::user_dir) + "simuconf.tab";
	if (simuconf.open(obj_conf.c_str())) {
		printf("parse_simuconf() at %s: ", obj_conf.c_str() );
		env_t::default_settings.parse_simuconf( simuconf, disp_width, disp_height, fullscreen, env_t::objfilename );
		simuconf.close();
	}

	// env: override previous settings
	if(  (gimme_arg(argc, argv, "-freeplay", 0) != NULL)  ) {
		env_t::default_settings.set_freeplay( true );
	}

	// now set the desired objectfilename (override all previous settings)
	if(  const char *fn = gimme_arg(argc, argv, "-objects", 1)  ) {
		env_t::objfilename = fn;
		// append slash / replace trailing backslash if necessary
		uint16 len = env_t::objfilename.length();
		if (len > 0) {
			if (env_t::objfilename[len-1]=='\\') {
				env_t::objfilename.erase(len-1);
				env_t::objfilename += "/";
			}
			else if (env_t::objfilename[len-1]!='/') {
				env_t::objfilename += "/";
			}
		}
	}
	else if(  const char *filename = gimme_arg(argc, argv, "-load", 1)  ) {
		// try to get a pak file path from a savegame file
		// read pak_extension from file
		loadsave_t test;
		std::string fn = env_t::user_dir;
		fn += "save/";
		fn += filename;
		if(  test.rd_open(fn.c_str())  ) {
			// add pak extension
			std::string pak_extension = test.get_pak_extension();
			if(  pak_extension!="(unknown)"  ) {
				env_t::objfilename = pak_extension + "/";
			}
		}
	}

	// starting a server?
	if(  gimme_arg(argc, argv, "-easyserver", 0)  ) {
		const char *p = gimme_arg(argc, argv, "-easyserver", 1);
		int portadress = p ? atoi( p ) : 13353;
		if(  portadress!=0  ) {
			env_t::server_port = portadress;
		}
		// will fail fatal on the opening routine ...
		dbg->message( "simmain()", "Server started on port %i", env_t::server_port );
		env_t::networkmode = network_init_server( env_t::server_port );
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
		if(  gimme_arg(argc, argv, "-server", 0)  ) {
			const char *p = gimme_arg(argc, argv, "-server", 1);
			int portadress = p ? atoi( p ) : 13353;
			if(  portadress!=0  ) {
				env_t::server_port = portadress;
			}
			// will fail fatal on the opening routine ...
			dbg->message( "simmain()", "Server started on port %i", env_t::server_port );
			env_t::networkmode = network_init_server( env_t::server_port );
		}
		else {
			// no announce for clients ...
			env_t::server_announce = 0;
		}
	}

	DBG_MESSAGE( "simmain::main()", "Version: " VERSION_NUMBER "  Date: " VERSION_DATE);
	DBG_MESSAGE("Debuglevel",  "%i", env_t::verbose_debug);
	DBG_MESSAGE("program_dir", "%s", env_t::program_dir);
	DBG_MESSAGE("home_dir",    "%s", env_t::user_dir);
	DBG_MESSAGE("locale",      "%s", dr_get_locale_string());
#ifdef DEBUG
	if (gimme_arg(argc, argv, "-sizes", 0) != NULL) {
		// show the size of some structures ...
		show_sizes();
	}
#endif

	// likely only the program without graphics was downloaded
	if (gimme_arg(argc, argv, "-res", 0) != NULL) {
		const char* res_str = gimme_arg(argc, argv, "-res", 1);
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
					"invalid resolution, argument must be 1,2,3 or 4\n"
					"1=640x480, 2=800x600, 3=1024x768, 4=1280x1024, 5=windowed\n"
				);
				return 0;
		}
	}

	fullscreen |= (gimme_arg(argc, argv, "-fullscreen", 0) != NULL);

	if(gimme_arg(argc, argv, "-screensize", 0) != NULL) {
		const char* res_str = gimme_arg(argc, argv, "-screensize", 1);
		int n = 0;

		if (res_str != NULL) {
			n = sscanf(res_str, "%hdx%hd", &disp_width, &disp_height);
		}

		if (n != 2) {
			fprintf(stderr,
				"invalid argument for -screensize option\n"
				"argument must be of format like 800x600\n"
			);
			return 1;
		}
	}

	if(  gimme_arg( argc, argv, "-autodpi", 0)  ) {
		dr_auto_scale( true );
	}

	int parameter[2];
	parameter[0] = gimme_arg( argc, argv, "-async", 0) != NULL;
	parameter[1] = gimme_arg( argc, argv, "-use_hw", 0) != NULL;
	if (!dr_os_init(parameter)) {
		dr_fatal_notify("Failed to initialize backend.\n");
		return EXIT_FAILURE;
	}

	// Get optimal resolution.
	if (disp_width == 0 || disp_height == 0) {
		resolution const res = dr_query_screen_resolution();
		if (fullscreen) {
			disp_width  = res.w;
			disp_height = res.h;
		}
		else {
			disp_width  = min(704, res.w);
			disp_height = min(560, res.h);
		}
	}

	DBG_MESSAGE("simmain", "simgraph_init disp_width=%d, disp_height=%d, fullscreen=%d", disp_width, disp_height, fullscreen);
	simgraph_init(disp_width, disp_height, fullscreen);
	DBG_MESSAGE("simmain", ".. results in disp_width=%d, disp_height=%d", display_get_width(), display_get_height());

	// now that the graphics system has already started
	// the saved colours can be converted to the system format
	env_t_rgb_to_system_colors();

	// parse colours now that the graphics system has started
	// default simuconf.tab
	if(  found_simuconf  ) {
		if(simuconf.open(path_to_simuconf)) {
			// we do not allow to change the global font name also from the pakset ...
			std::string old_fontname = env_t::fontname;
			printf("parse_colours() at config/simuconf.tab: ");
			env_t::default_settings.parse_colours( simuconf );
			simuconf.close();
			env_t::fontname = old_fontname;
		}
	}
	// a portable installation could have a personal simuconf.tab in the main dir of simutrans
	// otherwise it is in ~/simutrans/simuconf.tab
	obj_conf = string(env_t::user_dir) + "simuconf.tab";
	if (simuconf.open(obj_conf.c_str())) {
		printf("parse_simuconf() at %s: ", obj_conf.c_str() );
		env_t::default_settings.parse_colours( simuconf );
		simuconf.close();
	}

	// prepare skins first
	bool themes_ok = false;
	if(  const char *themestr = gimme_arg(argc, argv, "-theme", 1)  ) {
		dr_chdir( env_t::user_dir );
		dr_chdir( "themes" );
		themes_ok = gui_theme_t::themes_init(themestr, true);
		if(  !themes_ok  ) {
			dr_chdir( env_t::program_dir );
			dr_chdir( "themes" );
			themes_ok = gui_theme_t::themes_init(themestr, true);
		}
	}
	// next try the last used theme
	if(  !themes_ok  &&  env_t::default_theme.c_str()!=NULL  ) {
		dr_chdir( env_t::user_dir );
		dr_chdir( "themes" );
		themes_ok = gui_theme_t::themes_init( env_t::default_theme, true );
		if(  !themes_ok  ) {
			dr_chdir( env_t::program_dir );
			dr_chdir( "themes" );
			themes_ok = gui_theme_t::themes_init( env_t::default_theme, true );
		}
	}
	// specified themes not found => try default themes
	if(  !themes_ok  ) {
		dr_chdir( env_t::program_dir );
		dr_chdir( "themes" );
		themes_ok = gui_theme_t::themes_init("themes.tab",true);
	}
	if(  !themes_ok  ) {
		dbg->fatal( "simmain()", "No GUI themes found! Please re-install!" );
	}
	dr_chdir( env_t::program_dir );

	// The loading screen needs to be initialized
	display_show_pointer(1);

	// if no object files given, we ask the user
	if(  env_t::objfilename.empty()  ) {
		ask_objfilename();
		if(  env_t::quit_simutrans  ) {
			simgraph_exit();
			return 0;
		}
		if(  env_t::objfilename.empty()  ) {
			// try to download missing paks
			if(  dr_download_pakset( env_t::program_dir, env_t::program_dir == env_t::user_dir )  ) {
				ask_objfilename();
				if(  env_t::quit_simutrans  ) {
					simgraph_exit();
					return 0;
				}
			}
			// still nothing?
			if(  env_t::objfilename.empty()  ) {
				// nothing to be loaded => exit
				dr_fatal_notify("*** No pak set found ***\n\nMost likely, you have no pak set installed.\nPlease download and install a pak set (graphics).\n");
				simgraph_exit();
				return 0;
			}
		}
	}

	// check for valid pak path
	{
		cbuffer_t buf;
		buf.append( env_t::program_dir );
		buf.append( env_t::objfilename.c_str() );
		buf.append("ground.Outside.pak");

		FILE* const f = dr_fopen(buf, "r");
		if(  !f  ) {
			dr_fatal_notify("*** No pak set found ***\n\nMost likely, you have no pak set installed.\nPlease download and install a pak set (graphics).\n");
			simgraph_exit();
			return 0;
		}
		fclose(f);
	}

	// now find the pak specific tab file ...
	obj_conf = env_t::objfilename + path_to_simuconf;
	if(  simuconf.open(obj_conf.c_str())  ) {
		sint16 idummy;
		string dummy;
		env_t::default_settings.set_way_height_clearance( 0 );
		DBG_DEBUG("karte_t::distribute_groundobjs_cities()","parse_simuconf() at %s: ", obj_conf.c_str());
		env_t::default_settings.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
		env_t::default_settings.parse_colours( simuconf );
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
		sint16 idummy;
		string dummy;
		dbg->message("simmain()", "parse_simuconf() at %s: ", obj_conf.c_str());
		env_t::default_settings.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
		env_t::default_settings.parse_colours( simuconf );
		simuconf.close();
	}

	// load with private addons (now in addons/pak-name either in simutrans main dir or in userdir)
	if(  gimme_arg(argc, argv, "-objects", 1) != NULL  ) {
		if(gimme_arg(argc, argv, "-addons", 0) != NULL) {
			env_t::default_settings.set_with_private_paks( true );
		}
		if(gimme_arg(argc, argv, "-noaddons", 0) != NULL) {
			env_t::default_settings.set_with_private_paks( false );
		}
	}
	// Set external midi command.
	if(const char *  midi_cmd = gimme_arg(argc, argv, "-midicmd", 1) ) {
			env_t::default_settings.set_midi_command( midi_cmd );
	}
	// parse ~/simutrans/pakxyz/config.tab"
	if(  env_t::default_settings.get_with_private_paks()  ) {
		obj_conf = string(env_t::user_dir) + "addons/" + env_t::objfilename + "config/simuconf.tab";
		sint16 idummy;
		string dummy;
		if (simuconf.open(obj_conf.c_str())) {
			dbg->message("simmain()","parse_simuconf() at %s: ", obj_conf.c_str());
			env_t::default_settings.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
			env_t::default_settings.parse_colours( simuconf );
			simuconf.close();
		}
		// and parse user settings again ...
		obj_conf = string(env_t::user_dir) + "simuconf.tab";
		if (simuconf.open(obj_conf.c_str())) {
			dbg->message("simmain()","parse_simuconf() at %s: ", obj_conf.c_str());
			env_t::default_settings.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
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
	if(  const char *ref_str = gimme_arg(argc, argv, "-threads", 1)  ) {
		int want_threads = atoi(ref_str);
		env_t::num_threads = clamp(want_threads, 1, MAX_THREADS);
	}
#else
	if(  env_t::num_threads > 1  ) {
		env_t::num_threads = 1;
		dbg->message("simmain()","Multithreading not enabled: threads = %d ignored.", env_t::num_threads );
	}
#endif

	// just check before loading objects
	if(  !gimme_arg(argc, argv, "-nosound", 0)  &&  dr_init_sound()  ) {
		dbg->message("simmain()","Reading compatibility sound data ...");
		sound_desc_t::init();
	}
	else {
		sound_set_mute(true);
	}

	// Adam - Moved away loading from simmain and placed into translator for better modularization
	if(  !translator::load(env_t::objfilename)  ) {
		// installation error: likely only program started
		dbg->fatal("simmain::main()", "Unable to load any language files\n"
		                              "*** PLEASE INSTALL PROPER BASE FILES ***\n\n"
							"either run ./get_lang_files.sh\n\nor\n\n"
							"download a complete simutrans archive and put the text/ folder here."
		);
		exit(11);
	}

	// use requested language (if available)
	if(  gimme_arg(argc, argv, "-lang", 1)  ) {
		const char *iso = gimme_arg(argc, argv, "-lang", 1);
		if(  strlen(iso)>=2  ) {
			translator::set_language( iso );
		}
		if(  translator::get_language()==-1  ) {
			dbg->fatal("simmain", "Illegal language definition \"%s\"", iso );
		}
		env_t::language_iso = translator::get_lang()->iso_base;
	}
	else if(  found_settings  ) {
		translator::set_language( env_t::language_iso );
	}

	// Hajo: simgraph init loads default fonts, now we need to load (if not set otherwise)
	sprachengui_t::init_font_from_lang( strcmp(env_t::fontname.c_str(), FONT_PATH_X "prop.fnt")==0 );
	dr_chdir(env_t::program_dir);

	dbg->message("simmain()","Reading city configuration ...");
	stadt_t::cityrules_init(env_t::objfilename);

	dbg->message("simmain()","Reading speedbonus configuration ...");
	vehicle_builder_t::speedbonus_init(env_t::objfilename);

	dbg->message("simmain()","Reading menu configuration ...");
	tool_t::init_menu();

	// loading all objects in the pak
	dbg->message("simmain()","Reading object data from %s...", env_t::objfilename.c_str());
	obj_reader_t::load( env_t::objfilename.c_str(), translator::translate("Loading paks ...") );
	std::string overlaid_warning;	// more prominent handling of double objects
	if(  dbg->had_overlaid()  ) {
		overlaid_warning = translator::translate("<h1>Error</h1><p><strong>");
		overlaid_warning.append( env_t::objfilename + translator::translate("contains the following doubled objects:</strong><p>") + dbg->get_overlaid() + "<p>" );
		dbg->clear_overlaid();
	}

	if(  env_t::default_settings.get_with_private_paks()  ) {
		// try to read addons from private directory
		dr_chdir( env_t::user_dir );
		if(!obj_reader_t::load(("addons/" + env_t::objfilename).c_str(), translator::translate("Loading addon paks ..."))) {
			fprintf(stderr, "reading addon object data failed (disabling).\n");
			env_t::default_settings.set_with_private_paks( false );
		}
		dr_chdir( env_t::program_dir );
		if(  dbg->had_overlaid()  ) {
			overlaid_warning.append( translator::translate("<h1>Warning</h1><p><strong>addons for") + env_t::objfilename + translator::translate("contains the following doubled objects:</strong><p>") + dbg->get_overlaid() );
			dbg->clear_overlaid();
		}
	}
	obj_reader_t::finish_loading();
	pakset_info_t::calculate_checksum();
	pakset_info_t::debug();

	if(  !overlaid_warning.empty()  ) {
		overlaid_warning.append( "<p>Continue by ESC, SPACE, or BACKSPACE.<br>" );
		help_frame_t *win = new help_frame_t();
		win->set_text( overlaid_warning.c_str() );
		modal_dialogue( win, magic_pakset_info_t, NULL, wait_for_key );
		destroy_all_win(true);
	}

	dbg->message("simmain()","Reading menu configuration ...");
	tool_t::read_menu(env_t::objfilename);

	if(  translator::get_language()==-1  ) {
		ask_language();
	}

	bool new_world = true;
	std::string loadgame;

	bool pause_after_load = false;
	if (gimme_arg(argc, argv, "-pause", 0)) {
		pause_after_load = true;
	}

	if(  gimme_arg(argc, argv, "-load", 0) != NULL  ) {
		cbuffer_t buf;
		dr_chdir( env_t::user_dir );
		/**
		 * Added automatic adding of extension
		 */
		const char *name = gimme_arg(argc, argv, "-load", 1);
		if (strstart(name, "net:")) {
			buf.append( name );
		}
		else {
			buf.printf( SAVE_PATH_X "%s", searchfolder_t::complete(name, "sve").c_str() );
		}
		dbg->message("simmain()", "loading savegame \"%s\"", name );
		loadgame = buf;
		new_world = false;
	}

	// recover last server game
	if(  new_world  &&  env_t::server  ) {
		dr_chdir( env_t::user_dir );
		loadsave_t file;
		static char servername[128];
		sprintf( servername, "server%d-network.sve", env_t::server );
		// try recover with the latest savegame
		if(  file.rd_open(servername)  ) {
			// compare pakset (objfilename has trailing path separator, pak_extension not)
			if (strstart(env_t::objfilename.c_str(), file.get_pak_extension())) {
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
		pak_name.append( env_t::objfilename );
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
		dr_chdir( env_t::program_dir );

		const std::string path = env_t::program_dir + env_t::objfilename + "demo.sve";

		// access did not work!
		if(  FILE *const f = dr_fopen(path.c_str(), "rb")  ) {
			// there is a demo game to load
			loadgame = path;
			fclose(f);
DBG_MESSAGE("simmain","loadgame file found at %s",path.c_str());
		}
	}

	if(  gimme_arg(argc, argv, "-timeline", 0) != NULL  ) {
		const char* ref_str = gimme_arg(argc, argv, "-timeline", 1);
		if(  ref_str != NULL  ) {
			env_t::default_settings.set_use_timeline( atoi(ref_str) );
		}
	}

	if(  gimme_arg(argc, argv, "-startyear", 0) != NULL  ) {
		const char * ref_str = gimme_arg(argc, argv, "-startyear", 1); //1930
		if(  ref_str != NULL  ) {
			env_t::default_settings.set_starting_year( clamp(atoi(ref_str),1,2999) );
		}
	}

	// now always writing in user dir (which points the the program dir in multiuser mode)
	dr_chdir( env_t::user_dir );

	// init midi before loading sounds
	if(  dr_init_midi()  ) {
		dbg->message("simmain()","Reading midi data ...");
		char pak_dir[PATH_MAX];
		sprintf( pak_dir, "%s%s", env_t::program_dir, env_t::objfilename.c_str() );
		if(  !midi_init(pak_dir)  ) {
			if(  !midi_init(env_t::user_dir)  ) {
				if(  !midi_init(env_t::program_dir)  ) {
					dbg->message("simmain()","Midi disabled ...");
				}
			}
		}
		if(gimme_arg(argc, argv, "-nomidi", 0)) {
			midi_set_mute(true);
		}
	}
	else {
		dbg->message("simmain()","Midi disabled ...");
		midi_set_mute(true);
	}

	if(  gimme_arg(argc, argv, "-mute", 0)  ) {
		sound_set_mute(true);
		midi_set_mute(true);
	}

	// restore previous sound settings ...
	sound_set_shuffle_midi( env_t::shuffle_midi!=0 );
	sound_set_mute(  env_t::mute_sound  ||  sound_get_mute() );
	midi_set_mute(  env_t::mute_midi  ||  midi_get_mute() );
	sound_set_global_volume( env_t::global_volume );
	sound_set_midi_volume( env_t::midi_volume );
	if(  !midi_get_mute()  ) {
		// not muted => play random song
		midi_play(-1);
		// reset volume after first play call else no/low sound or music with win32 and sdl
		sound_set_midi_volume( env_t::midi_volume );
	}

	karte_t *welt = new karte_t();
	main_view_t *view = new main_view_t(welt);
	welt->set_view( view );

	interaction_t *eventmanager = new interaction_t();
	welt->set_eventmanager( eventmanager );

	// some messages about old vehicle may appear ...
	welt->get_message()->set_message_flags(0, 0, 0, 0);

	// set the frame per second
	if(  const char *ref_str = gimme_arg(argc, argv, "-fps", 1)  ) {
		int want_refresh = atoi(ref_str);
		env_t::fps = want_refresh < 5 ? 5 : (want_refresh > 100 ? 100 : want_refresh);
	}

	// query server stuff
	// Enable server announcements
	if(  gimme_arg(argc, argv, "-announce", 0) != NULL  ) {
		env_t::server_announce = 1;
		DBG_DEBUG( "simmain()", "Server will be announced." );
	}

	if(  const char *ref_str = gimme_arg(argc, argv, "-server_dns", 1)  ) {
		env_t::server_dns = ref_str;
		DBG_DEBUG( "simmain()", "Server IP set to '%s'.", ref_str );
	}

	if(  const char *ref_str = gimme_arg(argc, argv, "-server_altdns", 1)  ) {
		env_t::server_alt_dns = ref_str;
		DBG_DEBUG( "simmain()", "Server IP set to '%s'.", ref_str );
	}

	if(  const char *ref_str = gimme_arg(argc, argv, "-server_name", 1)  ) {
		env_t::server_name = ref_str;
		DBG_DEBUG( "simmain()", "Server name set to '%s'.", ref_str );
	}

	if(  const char *ref_str = gimme_arg(argc, argv, "-server_admin_pw", 1)  ) {
		env_t::server_admin_pw = ref_str;
	}

	if(  env_t::server_dns.empty()  &&  !env_t::server_alt_dns.empty()  ) {
		dbg->warning( "simmain", "server_altdns but not server_dns set. Please use server_dns first!" );
		env_t::server_dns = env_t::server_alt_dns;
		env_t::server_alt_dns.clear();
	}

	dr_chdir(env_t::user_dir);

	// reset random counter to true randomness
	setsimrand(dr_time(), dr_time());
	clear_random_mode( 7 );	// allow all

	if(  loadgame==""  ||  !welt->load(loadgame.c_str())  ) {
		// create a default map
		DBG_MESSAGE("init with default map","(failing will be a pak error!)");
		// no autosave on initial map during the first six month ...
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
		intr_set(welt, view);
		win_set_world(welt);
		tool_t::toolbar_tool[0]->init(welt->get_active_player());
		welt->set_fast_forward(true);
		welt->sync_step(5000,true,false);
		welt->step_month(5);
		welt->step();
		welt->step();
		env_t::autosave = old_autosave;
	}
	else {
		// override freeplay setting when provided on command line
		if(  (gimme_arg(argc, argv, "-freeplay", 0) != NULL)  ) {
			welt->get_settings().set_freeplay( true );
		}
		// just init view (world was loaded from file)
		intr_set(welt, view);
		win_set_world(welt);
		tool_t::toolbar_tool[0]->init(welt->get_active_player());
	}

	welt->set_fast_forward(false);
	baum_t::recalc_outline_color();
#if defined DEBUG || defined PROFILE
	// do a render test?
	if (gimme_arg(argc, argv, "-times", 0) != NULL) {
		show_times(welt, view);
	}

	// finish after a certain month? (must be entered decimal, i.e. 12*year+month
	if(  gimme_arg(argc, argv, "-until", 0) != NULL  ) {
		const char *until = gimme_arg(argc, argv, "-until", 1);
		int year = -1, month = -1;
		if ( sscanf(until, "%i.%i", &year, &month) == 2) {
			quit_month = month+year*12-1;
		}
		else {
			quit_month = atoi(until);
		}
		welt->set_fast_forward(true);
	}
#endif

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
	// Hajo: give user a mouse to work with
	if (skinverwaltung_t::mouse_cursor != NULL) {
		// we must use our softpointer (only Allegro!)
		display_set_pointer(skinverwaltung_t::mouse_cursor->get_image_id(0));
	}
#endif
	display_show_pointer(true);
	display_show_load_pointer(0);

	welt->set_dirty();

	// Hajo: simgraph init loads default fonts, now we need to load
	// the real fonts for the current language, if not set otherwise
	sprachengui_t::init_font_from_lang( strcmp(env_t::fontname.c_str(), FONT_PATH_X "prop.fnt")==0 );

	if(   !(env_t::reload_and_save_on_quit  &&  !new_world)  ) {
		destroy_all_win(true);
	}
	env_t::restore_UI = old_restore_UI;

	if(  !env_t::networkmode  &&  !env_t::server  &&  new_world  ) {
		welt->get_message()->clear();
	}
	while(  !env_t::quit_simutrans  ) {
		// play next tune?
		check_midi();

		if(  !env_t::networkmode  &&  new_world  ) {
			dbg->message("simmain()", "Show banner ... " );
			ticker::add_msg("Welcome to Simutrans", koord::invalid, PLAYER_FLAG | color_idx_to_rgb(COL_SOFT_BLUE));
			modal_dialogue( new banner_t(), magic_none, welt, never_quit );
			// only show new world, if no other dialogue is active ...
			new_world = win_get_open_count()==0;
		}
		if(  env_t::quit_simutrans  ) {
			break;
		}

		// to purge all previous old messages
		welt->get_message()->set_message_flags(env_t::message_flags[0], env_t::message_flags[1], env_t::message_flags[2], env_t::message_flags[3]);

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
		dbg->message("simmain()", "Running world, pause=%i, fast forward=%i ... ", welt->is_paused(), welt->is_fast_forward() );
		loadgame = ""; // only first time

		// run the loop
		welt->interactive(quit_month);

		new_world = true;
		welt->get_message()->get_message_flags(&env_t::message_flags[0], &env_t::message_flags[1], &env_t::message_flags[2], &env_t::message_flags[3]);
		welt->set_fast_forward(false);
		welt->set_pause(false);
		setsimrand(dr_time(), dr_time());

		dbg->message("simmain()", "World finished ..." );
	}

	intr_disable();

	// save setting ...
	dr_chdir( env_t::user_dir );
	if(  file.wr_open("settings.xml",loadsave_t::xml,"settings only/",SAVEGAME_VER_NR)  ) {
		env_t::rdwr(&file);
		env_t::default_settings.rdwr(&file);
		file.close();
	}

	destroy_all_win( true );
	tool_t::exit_menu();

	delete welt;
	welt = NULL;

	delete view;
	view = 0;

	delete eventmanager;
	eventmanager = 0;

	remove_port_forwarding( env_t::server );
	network_core_shutdown();

	simgraph_exit();

	close_midi();

#if 0
	// free all list memories (not working, since there seems to be unitialized list still waiting for automated destruction)
	freelist_t::free_all_nodes();
#endif

	return 0;
}
