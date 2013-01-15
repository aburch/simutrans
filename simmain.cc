#if defined(_M_X64)  ||  defined(__x86_64__)
#warning "Simutrans is preferably compiled as 32 bit binary!"
#endif


#include <stdio.h>
#include <string>
#include <new>

#include "pathes.h"

#include "simmain.h"
#include "simworld.h"
#include "simware.h"
#include "simview.h"
#include "simwin.h"
#include "simhalt.h"
#include "simimg.h"
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
#include "simwerkz.h"

#include "simsys.h"
#include "simgraph.h"
#include "simevent.h"
#include "simtools.h"

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

#include "dings/baum.h"

#include "utils/simstring.h"
#include "utils/searchfolder.h"

#include "dataobj/network.h"	// must be before any "windows.h" is included via bzlib2.h ...
#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"
#include "dataobj/tabfile.h"
#include "dataobj/einstellungen.h"
#include "dataobj/translator.h"
#include "dataobj/pakset_info.h"

#include "besch/reader/obj_reader.h"
#include "besch/sound_besch.h"
#include "besch/grund_besch.h"

#include "music/music.h"
#include "sound/sound.h"

#include "utils/cbuffer_t.h"

#include "bauer/vehikelbauer.h"

#include "vehicle/simvehikel.h"
#include "vehicle/simverkehr.h"

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

	DBG_MESSAGE("sizes", "ding_t: %d", sizeof(ding_t));
	DBG_MESSAGE("sizes", "gebaeude_t: %d", sizeof(gebaeude_t));
	DBG_MESSAGE("sizes", "baum_t: %d", sizeof(baum_t));
	DBG_MESSAGE("sizes", "weg_t: %d", sizeof(weg_t));
	DBG_MESSAGE("sizes", "stadtauto_t: %d\n", sizeof(stadtauto_t));

	DBG_MESSAGE("sizes", "grund_t: %d", sizeof(grund_t));
	DBG_MESSAGE("sizes", "boden_t: %d", sizeof(boden_t));
	DBG_MESSAGE("sizes", "wasser_t: %d", sizeof(wasser_t));
	DBG_MESSAGE("sizes", "planquadrat_t: %d\n", sizeof(planquadrat_t));

	DBG_MESSAGE("sizes", "ware_t: %d", sizeof(ware_t));
	DBG_MESSAGE("sizes", "vehikel_t: %d", sizeof(vehikel_t));
	DBG_MESSAGE("sizes", "haltestelle_t: %d\n", sizeof(haltestelle_t));

	DBG_MESSAGE("sizes", "karte_t: %d", sizeof(karte_t));
	DBG_MESSAGE("sizes", "spieler_t: %d\n", sizeof(spieler_t));
}



// render tests ...
static void show_times(karte_t *welt, karte_ansicht_t *view)
{
 	intr_set(welt, view);
	welt->set_fast_forward(true);
	intr_disable();

	dbg->message( "show_times()", "simple profiling of drawing routines" );
 	int i;

	image_id img = grund_besch_t::ausserhalb->get_bild(0,0);

 	long ms = dr_time();
	for (i = 0;  i < 6000000;  i++) {
 		display_img( img, 50, 50, 1);
	}
	dbg->message( "display_img()", "%i iterations took %li ms", i, dr_time() - ms );

	image_id player_img = skinverwaltung_t::color_options->get_bild_nr(0);
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
 		display_text_proportional_len_clip(100, 120, "Dies ist ein kurzer Textetxt ...", 0, 0, -1);
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
		FOR( slist_tpl<weg_t *>, const w, weg_t::get_alle_wege() ) {
			grund_t *dummy;
			welt->lookup( w->get_pos() )->get_neighbour( dummy, invalid_wt, ribi_t::nord );
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
		dbg->error( "modal_dialogue()", "called without a display driver => nothing will be shown!" );
		umgebung_t::quit_simutrans = true;
		// cannot handle this!
		return;
	}

	event_t ev;
	create_win( (display_get_width()-gui->get_fenstergroesse().x)/2, (display_get_height()-gui->get_fenstergroesse().y)/2, gui, w_info, magic );

	if(  welt  ) {
		welt->set_pause( false );
		welt->reset_interaction();
		welt->reset_timer();

		long ms_pause = max( 25, 1000/umgebung_t::fps );
		uint32 last_step = dr_time()+ms_pause;
		uint step_count = 5;
		while(  win_is_open(gui)  &&  !umgebung_t::quit_simutrans  &&  !quit()  ) {
			do {
				DBG_DEBUG4("zeige_banner", "calling win_poll_event");
				win_poll_event(&ev);
				// no toolbar events
				if(  ev.my < werkzeug_t::toolbar_tool[0]->iconsize.y  ) {
					ev.my = werkzeug_t::toolbar_tool[0]->iconsize.y;
				}
				if(  ev.cy < werkzeug_t::toolbar_tool[0]->iconsize.y  ) {
					ev.cy = werkzeug_t::toolbar_tool[0]->iconsize.y;
				}
				if(  ev.ev_class == EVENT_KEYBOARD  &&  ev.ev_code == SIM_KEY_F1  ) {
					if(  gui_frame_t *win = win_get_top()  ) {
						if(  const char *helpfile = win->get_hilfe_datei()  ) {
							help_frame_t::open_help_on( helpfile );
							continue;
						}
					}
				}
				DBG_DEBUG4("zeige_banner", "calling check_pos_win");
				check_pos_win(&ev);
				if(  ev.ev_class == EVENT_SYSTEM  &&  ev.ev_code == SYSTEM_QUIT  ) {
					umgebung_t::quit_simutrans = true;
					break;
				}
				dr_sleep(5);
			} while(  dr_time()<last_step  );
			DBG_DEBUG4("zeige_banner", "calling welt->sync_step");
			welt->sync_step( ms_pause, true, true );
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
		while(  win_is_open(gui)  &&  !umgebung_t::quit_simutrans  &&  !quit()  ) {
			// do not move, do not close it!
			dr_sleep(50);
			dr_prepare_flush();
			gui->zeichnen(win_get_pos(gui), gui->get_fenstergroesse());
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
					umgebung_t::quit_simutrans = true;
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
	if (IS_LEFTCLICK(&ev)) {
		do {
			display_get_event(&ev);
		} while (!IS_LEFTRELEASE(&ev));
	}
}


// some routines for the modal display
static bool never_quit() { return false; }
static bool empty_objfilename() { return !umgebung_t::objfilename.empty(); }
static bool no_language() { return translator::get_language()!=-1; }



/**
 * Show pak selector
 */
static void ask_objfilename()
{
	pakselector_t* sel = new pakselector_t();
	sel->fill_list();
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
 * Dies wird in main mittels set_new_handler gesetzt und von der
 * Laufzeitumgebung im Falle des Speichermangels bei new() aufgerufen
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

	umgebung_t::init();

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
			"  by Hansjörg Malthaner et. al.\n"
			"---------------------------------------\n"
			"command line parameters available: \n"
			" -addons             loads also addons (with -objects)\n"
			" -async              asynchronic images, only for SDL\n"
			" -debug NUM          enables debuging (1..5)\n"
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
			" -server_dns FQDN/IP FQDN or IP address of server for announcements\n"
			" -server_name NAME   Name of server for announcements\n"
			" -server_admin_pw PW password for server administration\n"
			" -singleuser         Save everything in program directory (portable version)\n"
#ifdef DEBUG
			" -sizes              Show current size of some structures\n"
#endif
			" -startyear N        start in year N\n"
			" -timeline           enables timeline\n"
#if defined DEBUG || defined PROFILE
			" -times              does some simple profiling\n"
			" -until MONTH        quits when MONTH = (month*12+year-1) starts\n"
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
		getcwd(umgebung_t::program_dir, lengthof(umgebung_t::program_dir));
		strcat( umgebung_t::program_dir, path_sep );
	}
	else {
		strcpy( umgebung_t::program_dir, argv[0] );
		*(strrchr( umgebung_t::program_dir, path_sep[0] )+1) = 0;

		chdir( umgebung_t::program_dir );
	}
	printf("Use work dir %s\n", umgebung_t::program_dir);

	// only the pak specifiy conf should overide this!
	uint16 pak_diagonal_multiplier = umgebung_t::default_einstellungen.get_pak_diagonal_multiplier();
	sint8 pak_tile_height = TILE_HEIGHT_STEP;

	// parsing config/simuconf.tab
	printf("Reading low level config data ...\n");
	bool found_settings = false;
	bool found_simuconf = false;
	bool multiuser = (gimme_arg(argc, argv, "-singleuser", 0) == NULL);

	tabfile_t simuconf;
	char path_to_simuconf[24];
	// was  config/simuconf.tab
	sprintf(path_to_simuconf, "config%csimuconf.tab", path_sep[0]);
	if(simuconf.open(path_to_simuconf)) {
		{
			tabfileobj_t contents;
			simuconf.read(contents);
			// use different save directories
			multiuser = !(contents.get_int("singleuser_install", !multiuser)==1  ||  !multiuser);
			found_simuconf = true;
		}
		simuconf.close();
	}

	// init dirs now
	if(multiuser) {
		umgebung_t::user_dir = dr_query_homedir();
	}
	else {
		// save in program directory
		umgebung_t::user_dir = umgebung_t::program_dir;
	}
	chdir( umgebung_t::user_dir );


#ifdef REVISION
	const char *version = "Simutrans version " VERSION_NUMBER " from " VERSION_DATE " r" QUOTEME(REVISION) "\n";
#else
	const char *version = "Simutrans version " VERSION_NUMBER " from " VERSION_DATE "\n";
#endif


	/*** Begin logging set up ***/

#ifdef SYSLOG
	bool cli_syslog_enabled = (gimme_arg( argc, argv, "-syslog", 0 ) != NULL);
	const char* cli_syslog_tag = gimme_arg( argc, argv, "-tag", 1 );
#else //SYSLOG
	bool cli_syslog_enabled = false;
	const char* cli_syslog_tag = NULL;
#endif //SYSLOG

	umgebung_t::verbose_debug = 0;
	if(  gimme_arg(argc, argv, "-debug", 0) != NULL  ) {
		const char *s = gimme_arg(argc, argv, "-debug", 1);
		int level = 4;
		if(s!=NULL  &&  s[0]>='0'  &&  s[0]<='9'  ) {
			level = atoi(s);
		}
		umgebung_t::verbose_debug = level;
	}

	if (  cli_syslog_enabled  ) {
		printf("syslog enabled\n");
		if (  cli_syslog_tag  ) {
			printf("Init logging with syslog tag: %s\n", cli_syslog_tag);
			init_logging( "syslog", true, true, version, cli_syslog_tag );
		} else {
			printf("Init logging with default syslog tag\n");
			init_logging( "syslog", true, true, version, "simutrans" );
		}
	} else if (gimme_arg(argc, argv, "-log", 0)) {
		chdir( umgebung_t::user_dir );
		char temp_log_name[256];
		const char *logname = "simu.log";
		if(  gimme_arg(argc, argv, "-server", 0)  ) {
			const char *p = gimme_arg(argc, argv, "-server", 1);
			int portadress = p ? atoi( p ) : 13353;
			sprintf( temp_log_name, "simu-server%d.log", portadress==0 ? 13353 : portadress );
			logname = temp_log_name;
		}
		init_logging( logname, true, gimme_arg(argc, argv, "-log", 0 ) != NULL, version, NULL );
	} else if (gimme_arg(argc, argv, "-debug", 0) != NULL) {
		init_logging( "stderr", true, gimme_arg(argc, argv, "-debug", 0 ) != NULL, version, NULL );
	} else {
		init_logging(NULL, false, false, version, NULL);
	}

	/*** End logging set up ***/


	// now read last setting (might be overwritten by the tab-files)
	loadsave_t file;
	if(file.rd_open("settings.xml"))  {
		if(  file.get_version()>loadsave_t::int_version(SAVEGAME_VER_NR, NULL, NULL )  ) {
			// too new => remove it
			file.close();
			remove( "settings.xml" );
		}
		else {
			found_settings = true;
			umgebung_t::rdwr(&file);
			umgebung_t::default_einstellungen.rdwr(&file);
			file.close();
			// reset to false (otherwise these settings will persist)
			umgebung_t::default_einstellungen.set_freeplay( false );
			umgebung_t::default_einstellungen.set_allow_player_change( true );
			umgebung_t::server_announce = 0;
		}
	}

	// continue parsing ...
	chdir( umgebung_t::program_dir );
	if(  found_simuconf  ) {
		if(simuconf.open(path_to_simuconf)) {
			printf("parse_simuconf() at config/simuconf.tab: ");
			umgebung_t::default_einstellungen.parse_simuconf( simuconf, disp_width, disp_height, fullscreen, umgebung_t::objfilename );
		}
	}

	// a portable installation could have a personal simuconf.tab in the main dir of simutrans
	// otherwise it is in ~/simutrans/simuconf.tab
	string obj_conf = string(umgebung_t::user_dir) + "simuconf.tab";
	if (simuconf.open(obj_conf.c_str())) {
		printf("parse_simuconf() at %s: ", obj_conf.c_str() );
		umgebung_t::default_einstellungen.parse_simuconf( simuconf, disp_width, disp_height, fullscreen, umgebung_t::objfilename );
	}

	// umgebung: overide previous settings
	if(  (gimme_arg(argc, argv, "-freeplay", 0) != NULL)  ) {
		umgebung_t::default_einstellungen.set_freeplay( true );
	}

	// now set the desired objectfilename (overide all previous settings)
	if (gimme_arg(argc, argv, "-objects", 1)) {
		umgebung_t::objfilename = gimme_arg(argc, argv, "-objects", 1);
		// append slash / replace trailing backslash if necessary
		uint16 len = umgebung_t::objfilename.length();
		if (len > 0) {
			if (umgebung_t::objfilename[len-1]=='\\') {
				umgebung_t::objfilename.erase(len-1);
				umgebung_t::objfilename += "/";
			}
			else if (umgebung_t::objfilename[len-1]!='/') {
				umgebung_t::objfilename += "/";
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
		umgebung_t::networkmode = network_init_server( portadress );
	}
	else {
		// no announce for clients ...
		umgebung_t::server_announce = 0;
	}

	DBG_MESSAGE( "simmain::main()", "Version: " VERSION_NUMBER "  Date: " VERSION_DATE);
	DBG_MESSAGE("Debuglevel",  "%i", umgebung_t::verbose_debug);
	DBG_MESSAGE("program_dir", "%s", umgebung_t::program_dir);
	DBG_MESSAGE("home_dir",    "%s", umgebung_t::user_dir);
	DBG_MESSAGE("locale",      "%s", dr_get_locale_string());
//#ifdef DEBUG
	if (gimme_arg(argc, argv, "-sizes", 0) != NULL) {
		// show the size of some structures ...
		show_sizes();
	}
//#endif

	// prepare skins first
	obj_reader_t::init();
	chdir( umgebung_t::program_dir );

	// likely only the programm without graphics was downloaded
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

	int parameter[2];
	parameter[0] = gimme_arg(argc, argv, "-net",   0)==NULL;
	parameter[1] = gimme_arg(argc, argv, "-async", 0)==NULL;
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
		} else {
			disp_width  = min(704, res.w);
			disp_height = min(560, res.h);
		}
	}

	dbg->important("Preparing display ...");
	DBG_MESSAGE("simmain", "simgraph_init disp_width=%d, disp_height=%d, fullscreen=%d", disp_width, disp_height, fullscreen);
	simgraph_init(disp_width, disp_height, fullscreen);
	DBG_MESSAGE("simmain", ".. results in disp_width=%d, disp_height=%d", display_get_width(), display_get_height());

	// The loading screen needs to be initialized
	loadingscreen::bootstrap();

	show_pointer(1);

	// if no object files given, we ask the user
	if(  umgebung_t::objfilename.empty()  ) {
		ask_objfilename();
		if(  umgebung_t::quit_simutrans  ) {
			simgraph_exit();
			return 0;
		}
		if(  umgebung_t::objfilename.empty()  ) {
			// try to download missing paks
			if(  dr_download_pakset( umgebung_t::program_dir, umgebung_t::program_dir == umgebung_t::user_dir )  ) {
				ask_objfilename();
				if(  umgebung_t::quit_simutrans  ) {
					simgraph_exit();
					return 0;
				}
			}
			// still nothing?
			if(  umgebung_t::objfilename.empty()  ) {
				// nothing to be loaded => exit
				dr_fatal_notify("*** No pak set found ***\n\nMost likely, you have no pak set installed.\nPlease download and install a pak set (graphics).\n");
				simgraph_exit();
				return 0;
			}
		}
	}

	// now find the pak specific tab file ...
	obj_conf = umgebung_t::objfilename + path_to_simuconf;
	if(  simuconf.open(obj_conf.c_str())  ) {
		sint16 idummy;
		string dummy;
		dbg->important("parse_simuconf() at %s: ", obj_conf.c_str());
		umgebung_t::default_einstellungen.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
		pak_diagonal_multiplier = umgebung_t::default_einstellungen.get_pak_diagonal_multiplier();
		pak_tile_height = TILE_HEIGHT_STEP;
		simuconf.close();
	}
	// and parse again the user settings
	obj_conf = string(umgebung_t::user_dir) + "simuconf.tab";
	if (simuconf.open(obj_conf.c_str())) {
		sint16 idummy;
		string dummy;
		dbg->important("parse_simuconf() at %s: ", obj_conf.c_str());
		umgebung_t::default_einstellungen.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
		simuconf.close();
	}

	// load with private addons (now in addons/pak-name either in simutrans main dir or in userdir)
	if(  gimme_arg(argc, argv, "-objects", 1) != NULL  ) {
		if(gimme_arg(argc, argv, "-addons", 0) != NULL) {
			umgebung_t::default_einstellungen.set_with_private_paks( true );
		}
		if(gimme_arg(argc, argv, "-noaddons", 0) != NULL) {
			umgebung_t::default_einstellungen.set_with_private_paks( false );
		}
	}

	// parse ~/simutrans/pakxyz/config.tab"
	if(  umgebung_t::default_einstellungen.get_with_private_paks()  ) {
		obj_conf = string(umgebung_t::user_dir) + "addons/" + umgebung_t::objfilename + "config/simuconf.tab";
		sint16 idummy;
		string dummy;
		if (simuconf.open(obj_conf.c_str())) {
			dbg->important("parse_simuconf() at %s: ", obj_conf.c_str());
			umgebung_t::default_einstellungen.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
			simuconf.close();
		}
		// and parse user settings again ...
		obj_conf = string(umgebung_t::user_dir) + "simuconf.tab";
		if (simuconf.open(obj_conf.c_str())) {
			dbg->important("parse_simuconf() at %s: ", obj_conf.c_str());
			umgebung_t::default_einstellungen.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
			simuconf.close();
		}
	}

	// now (re)set the correct length from the pak
	umgebung_t::default_einstellungen.set_pak_diagonal_multiplier( pak_diagonal_multiplier );
	vehikel_basis_t::set_diagonal_multiplier( pak_diagonal_multiplier, pak_diagonal_multiplier );
	TILE_HEIGHT_STEP = pak_tile_height;

	convoihandle_t::init( 1024 );
	linehandle_t::init( 1024 );
	halthandle_t::init( 1024 );

	// just check before loading objects
	if (!gimme_arg(argc, argv, "-nosound", 0)  &&  dr_init_sound()) {
		dbg->important("Reading compatibility sound data ...");
		sound_besch_t::init();
	}
	else {
		sound_set_mute(true);
	}

	// Adam - Moved away loading from simmain and placed into translator for better modularisation
	if(  !translator::load(umgebung_t::objfilename)  ) {
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
			dbg->fatal("simmain", "Illegal language defintion \"%s\"", iso );
		}
		umgebung_t::language_iso = translator::get_lang()->iso_base;
	}
	else if(  found_settings  ) {
		translator::set_language( umgebung_t::language_iso );
	}

	// Hajo: simgraph init loads default fonts, now we need to load
	// the real fonts for the current language
	sprachengui_t::init_font_from_lang();
	chdir(umgebung_t::program_dir);

	dbg->important("Reading city configuration ...");
	stadt_t::cityrules_init(umgebung_t::objfilename);

	dbg->important("Reading speedbonus configuration ...");
	vehikelbauer_t::speedbonus_init(umgebung_t::objfilename);

	dbg->important("Reading menu configuration ...");
	werkzeug_t::init_menu();

	// loading all paks
	dbg->important("Reading object data from %s...", umgebung_t::objfilename.c_str());
	obj_reader_t::load(umgebung_t::objfilename.c_str(), translator::translate("Loading paks ...") );
	if(  umgebung_t::default_einstellungen.get_with_private_paks()  ) {
		// try to read addons from private directory
		chdir( umgebung_t::user_dir );
		if(!obj_reader_t::load(("addons/" + umgebung_t::objfilename).c_str(), translator::translate("Loading addon paks ..."))) {
			fprintf(stderr, "reading addon object data failed (disabling).\n");
			umgebung_t::default_einstellungen.set_with_private_paks( false );
		}
		chdir( umgebung_t::program_dir );
	}
	obj_reader_t::laden_abschliessen();
	pakset_info_t::calculate_checksum();
	pakset_info_t::debug();

	dbg->important("Reading menu configuration ...");
	werkzeug_t::read_menu(umgebung_t::objfilename);

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
		chdir( umgebung_t::user_dir );
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
	if(  new_world  &&  umgebung_t::server  ) {
		chdir( umgebung_t::user_dir );
		loadsave_t file;
		static char servername[128];
		sprintf( servername, "server%d-network.sve", umgebung_t::server );
		// try recover with the latest savegame
		if(  file.rd_open(servername)  ) {
			// compare pakset (objfilename has trailing path separator, pak_extension not)
			if (strstart(umgebung_t::objfilename.c_str(), file.get_pak_extension())) {
				// same pak directory - load this
				loadgame = servername;
				new_world = false;
			}
			file.close();
		}
	}

	// still nothing to be loaded => search for demo games
	if(  new_world  ) {
		chdir( umgebung_t::program_dir );
		char buffer[256];
		sprintf(buffer, "%s%sdemo.sve", (const char*)umgebung_t::program_dir, umgebung_t::objfilename.c_str());
		// access did not work!
		if (FILE* const f = fopen(buffer, "rb")) {
			// there is a demo game to load
			loadgame = buffer;
			fclose(f);
DBG_MESSAGE("simmain","loadgame file found at %s",buffer);
		}
	}

	if(  gimme_arg(argc, argv, "-timeline", 0) != NULL  ) {
		const char* ref_str = gimme_arg(argc, argv, "-timeline", 1);
		if(  ref_str != NULL  ) {
			umgebung_t::default_einstellungen.set_use_timeline( atoi(ref_str) );
		}
	}

	if(  gimme_arg(argc, argv, "-startyear", 0) != NULL  ) {
		const char * ref_str = gimme_arg(argc, argv, "-startyear", 1); //1930
		if(  ref_str != NULL  ) {
			umgebung_t::default_einstellungen.set_starting_year( clamp(atoi(ref_str),1,2999) );
		}
	}

	// now always writing in user dir (which points the the program dir in multiuser mode)
	chdir( umgebung_t::user_dir );

	// init midi before loading sounds
	if(  dr_init_midi()  ) {
		dbg->important("Reading midi data ...");
		if(!midi_init(umgebung_t::user_dir)) {
			if(!midi_init(umgebung_t::program_dir)) {
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
	sound_set_shuffle_midi( umgebung_t::shuffle_midi!=0 );
	sound_set_mute(  umgebung_t::mute_sound  ||  sound_get_mute() );
	midi_set_mute(  umgebung_t::mute_midi  ||  midi_get_mute() );
	sound_set_global_volume( umgebung_t::global_volume );
	sound_set_midi_volume( umgebung_t::midi_volume );
	if(  !midi_get_mute()  ) {
		// not muted => play first song
		midi_play(0);
		// reset volume after first play call else no/low sound or music with win32 and sdl
		sound_set_midi_volume( umgebung_t::midi_volume );
	}

	karte_t *welt = new karte_t();
	karte_ansicht_t *view = new karte_ansicht_t(welt);
	welt->set_ansicht( view );

	// some messages about old vehicle may appear ...
	welt->get_message()->set_message_flags(0, 0, 0, 0);

	// set the frame per second
	if(  const char *ref_str = gimme_arg(argc, argv, "-fps", 1)  ) {
		int want_refresh = atoi(ref_str);
		umgebung_t::fps = want_refresh < 5 ? 5 : (want_refresh > 100 ? 100 : want_refresh);
	}

	// query server stuff
	// Enable server announcements
	if(  gimme_arg(argc, argv, "-announce", 0) != NULL  ) {
		umgebung_t::server_announce = 1;
	}

	if(  const char *ref_str = gimme_arg(argc, argv, "-server_dns", 1)  ) {
		umgebung_t::server_dns = ref_str;
	}

	if(  const char *ref_str = gimme_arg(argc, argv, "-server_name", 1)  ) {
		umgebung_t::server_name = ref_str;
	}

	if(  const char *ref_str = gimme_arg(argc, argv, "-server_admin_pw", 1)  ) {
		umgebung_t::server_admin_pw = ref_str;
	}

	chdir(umgebung_t::user_dir);

	// reset random counter to true randomness
	setsimrand(dr_time(), dr_time());
	clear_random_mode( 7 );	// allow all

	if(  loadgame==""  ||  !welt->laden(loadgame.c_str())  ) {
		// create a default map
		DBG_MESSAGE("init with default map","(failing will be a pak error!)");
		// no autosave on initial map during the first six month ...
		loadgame = "";
		new_world = true;
		sint32 old_autosave = umgebung_t::autosave;
		umgebung_t::autosave = false;
		settings_t sets;
		sets.copy_city_road( umgebung_t::default_einstellungen );
		sets.set_default_climates();
		sets.set_use_timeline( 1 );
		sets.set_groesse(64,64);
		sets.set_anzahl_staedte(1);
		sets.set_factory_count(3);
		sets.set_tourist_attractions(1);
		sets.set_verkehr_level(7);
		welt->init(&sets,0);
		//  start in June ...
		intr_set(welt, view);
		win_set_welt(welt);
		werkzeug_t::toolbar_tool[0]->init(welt,welt->get_active_player());
		welt->set_fast_forward(true);
		welt->sync_step(5000,true,false);
		welt->step_month(5);
		welt->step();
		welt->step();
		umgebung_t::autosave = old_autosave;
	}
	else {
		// override freeplay setting when provided on command line
		if(  (gimme_arg(argc, argv, "-freeplay", 0) != NULL)  ) {
			welt->get_settings().set_freeplay( true );
		}
		// just init view (world was loaded from file)
		intr_set(welt, view);
		win_set_welt(welt);
		werkzeug_t::toolbar_tool[0]->init(welt,welt->get_active_player());
	}

	welt->set_fast_forward(false);
	baum_t::recalc_outline_color();
//#if defined DEBUG || defined PROFILE
	// do a render test?
	if (gimme_arg(argc, argv, "-times", 0) != NULL) {
		show_times(welt, view);
	}

	// finish after a certain month? (must be entered decimal, i.e. 12*year+month
	if(  gimme_arg(argc, argv, "-until", 0) != NULL  ) {
		quit_month = atoi( gimme_arg(argc, argv, "-until", 1) );
		welt->set_fast_forward(true);
	}
//#endif

	welt->reset_timer();
	if(  !umgebung_t::networkmode  &&  !umgebung_t::server  ) {
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
		display_set_pointer(skinverwaltung_t::mouse_cursor->get_bild_nr(0));
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
	if(  !umgebung_t::networkmode  &&  !umgebung_t::server  ) {
		welt->get_message()->clear();
	}
	while(  !umgebung_t::quit_simutrans  ) {
		// play next tune?
		check_midi();

		if(  !umgebung_t::networkmode  &&  new_world  ) {
			dbg->important( "Show banner ... " );
			ticker::add_msg("Welcome to Simutrans", koord::invalid, PLAYER_FLAG + 1);
			modal_dialogue( new banner_t(welt), magic_none, welt, never_quit );
			// only show new world, if no other dialoge is active ...
			new_world = win_get_open_count()==0;
		}
		if(  umgebung_t::quit_simutrans  ) {
			break;
		}

		// to purge all previous old messages
		welt->get_message()->set_message_flags(umgebung_t::message_flags[0], umgebung_t::message_flags[1], umgebung_t::message_flags[2], umgebung_t::message_flags[3]);

		if(  !umgebung_t::networkmode  &&  !umgebung_t::server  ) {
			welt->set_pause( pause_after_load );
			pause_after_load = false;
		}

		if(  new_world  ) {
			modal_dialogue( new welt_gui_t(welt, &umgebung_t::default_einstellungen), magic_welt_gui_t, welt, never_quit );
			if(  umgebung_t::quit_simutrans  ) {
				break;
			}
		}
		dbg->important( "Running world, pause=%i, fast forward=%i ... ", welt->is_paused(), welt->is_fast_forward() );
		loadgame = ""; // only first time

		// run the loop
		welt->interactive(quit_month);

		new_world = true;
		welt->get_message()->get_message_flags(&umgebung_t::message_flags[0], &umgebung_t::message_flags[1], &umgebung_t::message_flags[2], &umgebung_t::message_flags[3]);
		welt->set_fast_forward(false);
		welt->set_pause(false);
		setsimrand(dr_time(), dr_time());

		dbg->important( "World finished ..." );
	}

	intr_disable();

	// save setting ...
	chdir( umgebung_t::user_dir );
	if(  file.wr_open("settings.xml",loadsave_t::xml,"settings only/",SAVEGAME_VER_NR)  ) {
		umgebung_t::rdwr(&file);
		umgebung_t::default_einstellungen.rdwr(&file);
		file.close();
	}

	destroy_all_win( true );
	werkzeug_t::exit_menu();

	delete welt;
	welt = NULL;

	delete view;
	view = 0;

	network_core_shutdown();

	simgraph_exit();

	close_midi();

#if 0
	// free all list memories (not working, since there seems to be unitialized list still waiting for automated destruction)
	freelist_t::free_all_nodes();
#endif

	return 0;
}
