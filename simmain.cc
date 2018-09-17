#if defined(_M_X64)  ||  defined(__x86_64__)
#if __GNUC__
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
#include "utils/simrandom.h"

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

#include "bauer/vehikelbauer.h"

#include "vehicle/simvehicle.h"
#include "vehicle/simroadtraffic.h"

using std::string;

#if defined DEBUG || defined PROFILE
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
static void show_times(karte_t *welt, karte_ansicht_t *view)
{
 	intr_set(welt, view);
	welt->set_fast_forward(true);
	intr_disable();

	dbg->message( "show_times()", "simple profiling of drawing routines" );
 	int i;

	image_id img = ground_desc_t::outside->get_image(0,0);

 	long ms = dr_time();
	for (i = 0;  i < 6000000;  i++) {
		display_img_aux(img, 50, 50, 1, 0, true  CLIP_NUM_DEFAULT);
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
 		display_text_proportional_len_clip(100, 120, "Dies ist ein kurzer Textetxt ...", 0, 0, false, -1);
	}
	dbg->message( "display_text_proportional_len_clip()", "%i iterations took %li ms", i, dr_time() - ms );

 	ms = dr_time();
	for (i = 0;  i < 300000;  i++) {
 		display_fillbox_wh(100, 120, 300, 50, 0, false);
	}
	dbg->message( "display_fillbox_wh()", "%i iterations took %li ms", i, dr_time() - ms );

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
		FOR(vector_tpl<weg_t *>, const w, weg_t::get_alle_wege() ) {
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
#endif


void modal_dialogue( gui_frame_t *gui, ptrdiff_t magic, karte_t *welt, bool (*quit)() )
{
	if(  display_get_width()==0  ) {
		dbg->error( "modal_dialogue()", "called without a display driver => nothing will be shown!" );
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
		uint32 last_step = dr_time()+ms_pause;
		uint step_count = 5;
		while(  win_is_open(gui)  &&  !env_t::quit_simutrans  &&  !quit()  ) {
			do {
				DBG_DEBUG4("zeige_banner", "calling win_poll_event");
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
				DBG_DEBUG4("zeige_banner", "calling check_pos_win");
				check_pos_win(&ev);
				if(  ev.ev_class == EVENT_SYSTEM  &&  ev.ev_code == SYSTEM_QUIT  ) {
					env_t::quit_simutrans = true;
					break;
				}
				dr_sleep(5);
			} while(  dr_time()<last_step  );
			DBG_DEBUG4("zeige_banner", "calling welt->sync_step");
			intr_disable();
			welt->sync_step( ms_pause, true, true );
			intr_enable();
			DBG_DEBUG4("zeige_banner", "calling welt->step");
			if(  step_count--==0  ) {
				welt->step();
				step_count = 5;
			}
			last_step += ms_pause;
		}
	}
	else {
		display_show_pointer(true);
		show_pointer(1);
		set_pointer(0);
		display_fillbox_wh( 0, 0, display_get_width(), display_get_height(), COL_BLACK, true );
		while(  win_is_open(gui)  &&  !env_t::quit_simutrans  &&  !quit()  ) {
			// do not move, do not close it!
			dr_sleep(50);
			dr_prepare_flush();
			gui->draw(win_get_pos(gui), gui->get_windowsize());
			dr_flush();

			display_poll_event(&ev);
			if(ev.ev_class==EVENT_SYSTEM) {
				if (ev.ev_code==SYSTEM_RESIZE) {
					// main window resized
					simgraph_resize( ev.mx, ev.my );
					dr_prepare_flush();
					display_fillbox_wh( 0, 0, ev.mx, ev.my, COL_BLACK, true );
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
		set_pointer(1);
		dr_prepare_flush();
		display_fillbox_wh( 0, 0, display_get_width(), display_get_height(), COL_BLACK, true );
		dr_flush();
	}

	// just trigger not another following window => wait for button release
	display_get_event(&ev);
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



/**
 * Show pak selector
 */
static void ask_objfilename()
{
	pakselector_t* sel = new pakselector_t();
	sel->fill_list();
	if(  sel->check_only_one_option()  ) {
		// If there's only one option, we selected it; don't even show the window
		delete sel;
	}
	else if(  sel->has_pak()  ) {
		destroy_all_win(true);	// since eventually the successful load message is still there ....
		dbg->important("modal_dialogue( sel, magic_none, NULL, empty_objfilename );" );
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
		dbg->important("modal_dialogue( sel, magic_none, NULL, no_language );" );
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
	dbg->fatal("sim_new_handler()", "OUT OF MEMORY or other error allocating new object");
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
			"  Simutrans " VERSION_NUMBER EXTENDED_VERSION "\n"
			"  released " VERSION_DATE "\n"
			"  an extended version of Simutrans\n"
			"  developed by the Simutrans\n"
			"  community under the Artistic Licence.\n"
			"\n"
			"  For more information, please\n"
			"  visit the Simutrans website or forum\n"
			"  http://www.simutrans.com\n"
		    "  http://forum.simutrans.com"
			"\n"
			"  Based on Simutrans 0.84.21.2\n"
			"  by Hansjörg Malthaner et. al.\n"
			"---------------------------------------\n"
			"command line parameters available: \n"
			" -addons             loads also addons (with -objects)\n"
			" -async              asynchronous images, only for SDL\n"
			" -use_hw             hardware double buffering, only for SDL\n"
			" -debug NUM          enables debugging (1..5)\n"
			" -freeplay           play with endless money\n"
			" -fullscreen         starts simutrans in fullscreen mode\n"
			" -fps COUNT          framerate (from 5 to 100)\n"
			" -h | -help | --help displays this help\n"
			" -lang CODE          starts with specified language\n"
			" -load FILE[.sve]    loads game in file 'save/FILE.sve'\n"
			" -log                enables logging to file 'simu.log'\n"
#ifdef SYSLOG
			" -syslog             enable logging to syslog\n"
			"                     mutually exclusive with -log\n"
			" -tag TAG            sets syslog tag (default 'simutrans')\n"
#endif
			" -noaddons           does not load any addon (default)\n"
			" -nomidi             turns off background music\n"
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

#ifdef _WIN32
#define PATHSEP "\\"
#else
#define PATHSEP "/"
#endif
	const char* path_sep = PATHSEP;


#ifdef __BEOS__
	if (1) // since BeOS only supports relative paths ...
#else
	// use current dir as basedir, else use program_dir
	if (gimme_arg(argc, argv, "-use_workdir", 0))
#endif
	{
		// save the current directories
		getcwd(env_t::program_dir, lengthof(env_t::program_dir));
		strcat( env_t::program_dir, path_sep );
	}
	else {
		strcpy( env_t::program_dir, argv[0] );
		*(strrchr( env_t::program_dir, path_sep[0] )+1) = 0;

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

		chdir( env_t::program_dir );
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
	sprintf(path_to_simuconf, "config%csimuconf.tab", path_sep[0]);
	if(simuconf.open(path_to_simuconf)) 
	{		
		found_simuconf = true;
	}
	else
	{
		// Settings file not found. Try the Debian default instead, in which
		// data files are in /usr/share/games/simutrans
        char backup_program_dir[1024];
		strcpy(backup_program_dir, env_t::program_dir);
		strcpy( env_t::program_dir, "/usr/share/games/simutrans-ex/" );
        chdir( env_t::program_dir );
		if(simuconf.open("config/simuconf.tab")) 
		{
			found_simuconf = true;
		}
		else
		{
			 strcpy(env_t::program_dir, backup_program_dir);
			 chdir(env_t::program_dir);
		}
	}

	if(found_simuconf)
	{
		tabfileobj_t contents;
		simuconf.read(contents);
		// use different save directories
		multiuser = !(contents.get_int("singleuser_install", !multiuser)==1  ||  !multiuser);
		printf("Parsed simuconf.tab for directory layout; multiuser = %i\n", multiuser);
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
	chdir( env_t::user_dir );


#ifdef REVISION
	const char *version = "Simutrans version " VERSION_NUMBER EXTENDED_VERSION " from " VERSION_DATE " #" QUOTEME(REVISION) "\n";
#else
	const char *version = "Simutrans version " VERSION_NUMBER EXTENDED_VERSION " from " VERSION_DATE "\n";
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
		chdir( env_t::user_dir );
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
	
#ifdef DEBUG
	const char xml_filename[32] = "settings-extended-debug.xml";
#else
	const char xml_filename[26] = "settings-extended.xml";
#endif
	bool xml_settings_found = file.rd_open(xml_filename);
	if(!xml_settings_found)
	{
		// Again, attempt to use the Debian directory.
		char backup_program_dir[1024];
		strcpy(backup_program_dir, env_t::program_dir);
		strcpy( env_t::program_dir, "/usr/share/games/simutrans-ex/" );
        chdir( env_t::program_dir );
		xml_settings_found = file.rd_open(xml_filename);
		if(!xml_settings_found)
		{
			 strcpy(env_t::program_dir, backup_program_dir);
			 chdir(env_t::program_dir);
		}
	}

	if(xml_settings_found)  
	{
		if(file.get_version() > loadsave_t::int_version(SAVEGAME_VER_NR, NULL, NULL).version 
			|| file.get_extended_version() > loadsave_t::int_version(EXTENDED_SAVEGAME_VERSION, NULL, NULL).extended_version
			|| file.get_extended_revision() > EX_SAVE_MINOR)
		{
			// too new => remove it
			file.close();
			remove(xml_filename);
		}
		else 
		{
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
	chdir( env_t::program_dir );
	if(  found_simuconf  ) {
		if(simuconf.open(path_to_simuconf)) {
			printf("parse_simuconf() in program dir (%s): ", path_to_simuconf);
			env_t::default_settings.parse_simuconf( simuconf, disp_width, disp_height, fullscreen, env_t::objfilename );
		}
	}

	// a portable installation could have a personal simuconf.tab in the main dir of simutrans
	// otherwise it is in ~/simutrans/simuconf.tab
	string obj_conf = string(env_t::user_dir) + "simuconf.tab";
	if (simuconf.open(obj_conf.c_str())) {
		printf("parse_simuconf() in user dir (%s): ", obj_conf.c_str());
		env_t::default_settings.parse_simuconf( simuconf, disp_width, disp_height, fullscreen, env_t::objfilename );
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
	if(  gimme_arg(argc, argv, "-server", 0)  ) {
		const char *p = gimme_arg(argc, argv, "-server", 1);
		int portadress = p ? atoi( p ) : 13353;
		if(  portadress==0  ) {
			portadress = 13353;
		}
		// will fail fatal on the opening routine ...
		dbg->message( "simmain()", "Server started on port %i", portadress );
		env_t::networkmode = network_init_server( portadress );
	}
	else {
		// no announce for clients ...
		env_t::server_announce = 0;
	}

#ifdef DEBUG
	DBG_MESSAGE( "simmain::main()", "Version: " VERSION_NUMBER EXTENDED_VERSION "  Date: " VERSION_DATE);
	DBG_MESSAGE( "Debuglevel","%i", env_t::verbose_debug );
	DBG_MESSAGE( "program_dir", env_t::program_dir );
	DBG_MESSAGE( "home_dir", env_t::user_dir );
	DBG_MESSAGE( "locale", dr_get_locale_string());
	if (gimme_arg(argc, argv, "-sizes", 0) != NULL) {
		// show the size of some structures ...
		show_sizes();
	}
#endif

	// prepare skins first
	bool themes_ok = false;
	if(  const char *themestr = gimme_arg(argc, argv, "-theme", 1)  ) {
		chdir( env_t::user_dir );
		chdir( "themes" );
		themes_ok = gui_theme_t::themes_init(themestr);
		if(  !themes_ok  ) {
			chdir( env_t::program_dir );
			chdir( "themes" );
			themes_ok = gui_theme_t::themes_init(themestr);
		}
	}
	// next try the last used theme
	if(  !themes_ok  &&  env_t::default_theme.c_str()!=NULL  ) {
		chdir( env_t::user_dir );
		chdir( "themes" );
		themes_ok = gui_theme_t::themes_init( env_t::default_theme );
		if(  !themes_ok  ) {
			chdir( env_t::program_dir );
			chdir( "themes" );
			themes_ok = gui_theme_t::themes_init( env_t::default_theme );
		}
	}
	// specified themes not found => try default themes
	if(  !themes_ok  ) {
		chdir( env_t::program_dir );
		chdir( "themes" );
		themes_ok = gui_theme_t::themes_init("themes.tab");
	}
	if(  !themes_ok  ) {
		dbg->fatal( "simmain()", "No GUI themes found! Please re-install!" );
	}
	chdir( env_t::program_dir );

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

	if (gimme_arg(argc, argv, "-autodpi", 0)) {
		dr_auto_scale(true);
		
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

	dbg->important("Preparing display ...");
	DBG_MESSAGE("simmain", "simgraph_init disp_width=%d, disp_height=%d, fullscreen=%d", disp_width, disp_height, fullscreen);
	simgraph_init(disp_width, disp_height, fullscreen);
	DBG_MESSAGE("simmain", ".. results in disp_width=%d, disp_height=%d", display_get_width(), display_get_height());

	// The loading screen needs to be initialized
	show_pointer(1);

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
	printf("Pak found: %s\n", env_t::objfilename.c_str());

	// check for valid pak path
	{
		cbuffer_t buf;
		buf.append( env_t::program_dir );
		buf.append( env_t::objfilename.c_str() );
		buf.append("ground.Outside.pak");

		FILE* const f = fopen(buf, "r");
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
		dbg->important("parse_simuconf() at %s: ", obj_conf.c_str());
		env_t::default_settings.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
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
		printf("parse_simuconf() in user dir, second time (%s): ", obj_conf.c_str());
		env_t::default_settings.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
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

	// parse ~/simutrans/pakxyz/config.tab"
	if(  env_t::default_settings.get_with_private_paks()  ) {
		obj_conf = string(env_t::user_dir) + "addons/" + env_t::objfilename + "config/simuconf.tab";
		sint16 idummy;
		string dummy;
		if (simuconf.open(obj_conf.c_str())) {
			printf("parse_simuconf() in addons: %s", obj_conf.c_str());
			env_t::default_settings.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
			simuconf.close();
		}
		// and parse user settings again ...
		obj_conf = string(env_t::user_dir) + "simuconf.tab";
		if (simuconf.open(obj_conf.c_str())) {
			printf("parse_simuconf() in user dir, third time (%s): ", obj_conf.c_str());
			env_t::default_settings.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
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
		dbg->important("Multithreading not enabled: threads = %d ignored.", env_t::num_threads );
	}
#endif

	// just check before loading objects
	if (!gimme_arg(argc, argv, "-nosound", 0)  &&  dr_init_sound()) {
		dbg->important("Reading compatibility sound data ...");
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

	// Hajo: simgraph init loads default fonts, now we need to load
	// the real fonts for the current language
	sprachengui_t::init_font_from_lang();
	chdir(env_t::program_dir);

	dbg->important("Reading city configuration ...");
	stadt_t::cityrules_init(env_t::objfilename);

	dbg->important("Reading electricity consumption configuration ...");
	stadt_t::electricity_consumption_init(env_t::objfilename);
	
	dbg->important("Reading menu configuration ...");
	tool_t::init_menu();

	// loading all paks
	dbg->important("Reading object data from %s...", env_t::objfilename.c_str());
	obj_reader_t::load(env_t::objfilename.c_str(), translator::translate("Loading paks ...") );
	if(  env_t::default_settings.get_with_private_paks()  ) {
		// try to read addons from private directory
		chdir( env_t::user_dir );
		if(!obj_reader_t::load(("addons/" + env_t::objfilename).c_str(), translator::translate("Loading addon paks ..."))) {
			fprintf(stderr, "reading addon object data failed (disabling).\n");
			env_t::default_settings.set_with_private_paks( false );
		}
		chdir( env_t::program_dir );
	}
	obj_reader_t::finish_rd();
	pakset_info_t::calculate_checksum();
	pakset_info_t::debug();

	dbg->important("Reading menu configuration ...");
	tool_t::read_menu(env_t::objfilename);

	dbg->important("Reading private car ownership configuration ...");
	karte_t::privatecar_init(env_t::objfilename);

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
		chdir( env_t::user_dir );
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
		dbg->important( "loading savegame \"%s\"", name );
		loadgame = buf;
		new_world = false;
	}

	// recover last server game
	if(  new_world  &&  env_t::server  ) {
		chdir( env_t::user_dir );
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

	if(  new_world  &&  env_t::reload_and_save_on_quit  ) {
		// construct from pak name an autosave if requested
		loadsave_t file;
		std::string pak_name( "autosave-" );
		pak_name.append( env_t::objfilename );
		pak_name.erase( pak_name.length()-1 );
		pak_name.append( ".sve" );
		chdir( env_t::user_dir );
		unlink( "temp-load.sve" );
		if(  rename( pak_name.c_str(), "temp-load.sve" )==0  ) {
			loadgame = "temp-load.sve";
			new_world = false;
		}
		env_t::restore_UI = true;
	}

	// still nothing to be loaded => search for demo games
	if(  new_world  ) {
		cbuffer_t buf;
		// It's in the pakfile, in the program directory
		// (unlike every other saved game)
		buf.append( env_t::program_dir ); // has trailing slash
		buf.append( env_t::objfilename.c_str() ); //has trailing slash
		buf.append("demo.sve");

		if (FILE* const f = fopen(buf.get_str(), "rb")) {
			// there is a demo game to load
			loadgame = buf;
			fclose(f);
DBG_MESSAGE("simmain","loadgame file found at %s",buf.get_str() );
		} else {
DBG_MESSAGE("simmain","demo file not found at %s",buf.get_str() );
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
	chdir( env_t::user_dir );

	// init midi before loading sounds
	if(  dr_init_midi()  ) {
		dbg->important("Reading midi data ...");
		if(!midi_init(env_t::user_dir)) {
			if(!midi_init(env_t::program_dir)) {
				dbg->important("Midi disabled ...");
			}
		}
		if(gimme_arg(argc, argv, "-nomidi", 0)) {
			midi_set_mute(true);
		}
	}
	else {
		dbg->important("Midi disabled ...");
		midi_set_mute(true);
	}

	// restore previous sound settings ...
	sound_set_shuffle_midi( env_t::shuffle_midi!=0 );
	sound_set_mute(  env_t::mute_sound  ||  sound_get_mute() );
	midi_set_mute(  env_t::mute_midi  ||  midi_get_mute() );
	sound_set_global_volume( env_t::global_volume );
	sound_set_midi_volume( env_t::midi_volume );
	if(  !midi_get_mute()  ) {
		// not muted => play first song
		midi_play(0);
		// reset volume after first play call else no/low sound or music with win32 and sdl
		sound_set_midi_volume( env_t::midi_volume );
	}

	karte_t *welt = new karte_t();
	karte_ansicht_t *view = new karte_ansicht_t(welt);
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
	}

	if(  const char *ref_str = gimme_arg(argc, argv, "-server_dns", 1)  ) {
		env_t::server_dns = ref_str;
	}

	if(  const char *ref_str = gimme_arg(argc, argv, "-server_name", 1)  ) {
		env_t::server_name = ref_str;
	}

	if(  const char *ref_str = gimme_arg(argc, argv, "-server_admin_pw", 1)  ) {
		env_t::server_admin_pw = ref_str;
	}

	chdir(env_t::user_dir);

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
		uint32 old_number_of_big_cities = env_t::number_of_big_cities;
		env_t::number_of_big_cities = 0;
		settings_t sets;
		sets.copy_city_road( env_t::default_settings );
		sets.set_default_climates();
		sets.set_use_timeline( 1 );
		sets.set_groesse(64,64);
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
		env_t::number_of_big_cities = old_number_of_big_cities;
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
	if(  !env_t::networkmode  &&  !env_t::server  &&  new_world  ) {
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
	show_pointer(1);
	set_pointer(0);

	welt->set_dirty();

	// Hajo: simgraph init loads default fonts, now we need to load
	// the real fonts for the current language
	sprachengui_t::init_font_from_lang();

	destroy_all_win(true);
	if(  !env_t::networkmode  &&  !env_t::server  ) {
		welt->get_message()->clear();
	}
	while(  !env_t::quit_simutrans  ) {
		// play next tune?
		check_midi();

		if(  !env_t::networkmode  &&  new_world  ) {
			dbg->important( "Show banner ... " );
			ticker::add_msg("Welcome to Simutrans-Extended (formerly Simutrans-Experimental), a fork of Simutrans-Standard, extended and maintained by the Simutrans community.", koord::invalid, PLAYER_FLAG + 1);
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
			dbg->important("modal_dialogue( new welt_gui_t(&env_t::default_settings), magic_welt_gui_t, welt, never_quit );" );
			modal_dialogue( new welt_gui_t(&env_t::default_settings), magic_welt_gui_t, welt, never_quit );
			if(  env_t::quit_simutrans  ) {
				break;
			}
		}

		dbg->important( "Running world, pause=%i, fast forward=%i ... ", welt->is_paused(), welt->is_fast_forward() );
		loadgame = ""; // only first time

		// run the loop
		welt->interactive(quit_month);

		new_world = true;
		welt->get_message()->get_message_flags(&env_t::message_flags[0], &env_t::message_flags[1], &env_t::message_flags[2], &env_t::message_flags[3]);
		welt->set_fast_forward(false);
		welt->set_pause(false);
		setsimrand(dr_time(), dr_time());

		dbg->important( "World finished ..." );
	}

	intr_disable();

	// save setting ...
	chdir( env_t::user_dir );
	if(file.wr_open(xml_filename,loadsave_t::xml,"settings only/",SAVEGAME_VER_NR, EXTENDED_VER_NR, EXTENDED_REVISION_NR)) 
	{
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

	network_core_shutdown();

	simgraph_exit();

	close_midi();

#if 0
	// free all list memories (not working, since there seems to be unitialized list still waiting for automated destruction)
	freelist_t::free_all_nodes();
#endif

	return 0;
}
