#include <stdio.h>
#include <new>
#include <time.h>

#ifdef _MSC_VER
#include <new.h> // for _set_new_handler
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "pathes.h"

#include "simmain.h"
#include "simworld.h"
#include "simware.h"
#include "simview.h"
#include "simwin.h"
#include "simhalt.h"
#include "simimg.h"
#include "simcolor.h"
#include "simdepot.h"
#include "simskin.h"
#include "simconst.h"
#include "boden/boden.h"
#include "boden/wasser.h"
#include "simcity.h"
#include "simfab.h"
#include "player/simplay.h"
#include "simsound.h"
#include "simintr.h"
#include "simticker.h"
#include "simmesg.h"
#include "simwerkz.h"

#include "linehandle_t.h"

#include "simsys.h"
#include "simgraph.h"
#include "simevent.h"
#include "simtools.h"

#include "simversion.h"

#include "gui/banner.h"
#include "gui/pakselector.h"
#include "gui/welt.h"
#include "gui/sprachen.h"
#include "gui/climates.h"
#include "gui/messagebox.h"
#include "gui/loadsave_frame.h"
#include "gui/load_relief_frame.h"
#include "gui/scenario_frame.h"

#include "dings/baum.h"

#include "utils/simstring.h"
#include "utils/cstring_t.h"
#include "utils/searchfolder.h"

#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"
#include "dataobj/tabfile.h"
#include "dataobj/einstellungen.h"
#include "dataobj/translator.h"
#include "dataobj/network.h"

#include "besch/reader/obj_reader.h"
#include "besch/sound_besch.h"

#include "music/music.h"
#include "sound/sound.h"

#include "bauer/vehikelbauer.h"
#include "vehicle/simvehikel.h"
#include "vehicle/simverkehr.h"


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
	DBG_MESSAGE("test", "testing img ... ");
	int i;

	long ms = dr_time();
	for (i = 0;  i < 300000;  i++)
		display_img(10, 50, 50, 1);
	DBG_MESSAGE("test", "display_img(): %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0;  i < 300000;  i++)
		display_color_img(2000, 120, 100, 0, 1, 1);
	DBG_MESSAGE("test", "display_color_img(): %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0;  i < 300000;  i++)
		display_color_img(2000, 160, 150, 16, 1, 1);
	DBG_MESSAGE("test", "display_color_img(): next AI: %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0;  i < 300000;  i++)
		display_color_img(2000, 220, 200, 20, 1, 1);
	DBG_MESSAGE("test", "display_color_img(), other AI: %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0;  i < 300;  i++)
		dr_flush();
	DBG_MESSAGE("test", "display_flush_buffer(): %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0;  i < 300000;  i++)
		display_text_proportional_len_clip(100, 120, "Dies ist ein kurzer Textetxt ...", 0, 0, -1);
	DBG_MESSAGE("test", "display_text_proportional_len_clip(): %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0;  i < 300000;  i++)
		display_fillbox_wh(100, 120, 300, 50, 0, false);
	DBG_MESSAGE("test", "display_fillbox_wh(): %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0; i < 200; i++) {
		view->display(true);
	}
	DBG_MESSAGE("test", "view->display(true): %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	for (i = 0; i < 200; i++) {
		view->display(true);
		win_display_flush(0.0);
	}
	DBG_MESSAGE("test", "view->display(true) and flush: %i iterations took %i ms", i, dr_time() - ms);

	ms = dr_time();
	intr_set(welt, view);
	welt->set_fast_forward(true);
	intr_disable();
	for (i = 0; i < 200; i++) {
		welt->sync_step(200,true,true);
		welt->step();
	}
	DBG_MESSAGE("test", "welt->sync_step/step(200,1,1): %i iterations took %i ms", i, dr_time() - ms);
}



/**
 * Show Intro Screen
 */
static void zeige_banner(karte_t *welt)
{
	banner_t* b = new banner_t();
	event_t ev;

	destroy_all_win();	// since eventually the successful load message is still there ....

	create_win(0, -48, b, w_info, magic_none );

	// hide titelbar with this trick
	win_set_pos( b, 0, -48 );

	do {
		static long ms_pause = 1000/max(100,umgebung_t::fps);
		win_poll_event(&ev);
		check_pos_win(&ev);
		if(  ev.ev_class == EVENT_SYSTEM  &&  ev.ev_code == SYSTEM_QUIT  ) {
			umgebung_t::quit_simutrans = true;
		}
		dr_sleep(ms_pause);
		static uint32 last_step = dr_time();
		uint32 next_step = dr_time();
		welt->sync_step( next_step-last_step, true, true );
		welt->step();
		last_step = next_step;
	} while(win_is_top(b)  &&  !umgebung_t::quit_simutrans  );


	if (IS_LEFTCLICK(&ev)) {
		do {
			display_get_event(&ev);
		} while (!IS_LEFTRELEASE(&ev));
	}
}



/**
 * Show pak selector
 */
static void ask_objfilename()
{
	// more than one => show selector box (ugly and without translations ...)
	set_pointer(0);
	pakselector_t* sel = new pakselector_t();
	sel->fill_list();
	if(!sel->has_pak()) {
		// nothing there ...
		set_pointer(1);
		delete sel;
		return;
	}
	koord xy( display_get_width()/2 - 180, display_get_height()/2 - sel->get_fenstergroesse().y/2 );
	event_t ev;

	destroy_all_win();	// since eventually the successful load message is still there ....

	create_win( xy.x, xy.y, sel, w_info, magic_none );

	while(umgebung_t::objfilename.empty()) {
		// do not move, do not close it!
		dr_prepare_flush();
		sel->zeichnen( xy, sel->get_fenstergroesse() );
		display_poll_event(&ev);
		// main window resized
		check_pos_win(&ev);
		dr_flush();
		dr_sleep(50);
		// main window resized
		if(ev.ev_class==EVENT_SYSTEM  &&  ev.ev_code==SYSTEM_RESIZE) {
			// main window resized
			simgraph_resize( ev.mx, ev.my );
			display_fillbox_wh( 0, 0, ev.mx, ev.my, COL_BLACK, true );
		}
	}
	set_pointer(1);
}



/**
 * Show language selector
 */
static void ask_language()
{
	display_show_pointer(true);
	show_pointer(1);
	set_pointer(0);
	sprachengui_t* sel = new sprachengui_t();
	koord xy( display_get_width()/2 - sel->get_fenstergroesse().x/2, display_get_height()/2 - sel->get_fenstergroesse().y/2 );
	event_t ev;

	destroy_all_win();	// since eventually the successful load message is still there ....
	create_win( xy.x, xy.y, sel, w_info, magic_none );

	while(  translator::get_language()==-1  ) {
		// do not move, do not close it!
		dr_prepare_flush();
		sel->zeichnen( xy, sel->get_fenstergroesse() );
		display_poll_event(&ev);
		// main window resized
		check_pos_win(&ev);
		dr_flush();
		dr_sleep(50);
		// main window resized
		if(ev.ev_class==EVENT_SYSTEM  &&  ev.ev_code==SYSTEM_RESIZE) {
			// main window resized
			simgraph_resize( ev.mx, ev.my );
			display_fillbox_wh( 0, 0, ev.mx, ev.my, COL_BLACK, true );
		}
	}
	destroy_win( sel );
	set_pointer(0);
}



/**
 * Dies wird in main mittels set_new_handler gesetzt und von der
 * Laufzeitumgebung im Falle des Speichermangels bei new() aufgerufen
 */
#ifdef _MSC_VER
int sim_new_handler(unsigned int)
#else
void sim_new_handler()
#endif
{
	dbg->fatal("sim_new_handler()", "OUT OF MEMORY");
#ifdef _MSC_VER
	return 0;
#endif
}


static const char *gimme_arg(int argc, char *argv[], const char *arg, int off)
{
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], arg) == 0 && i < argc - off) {
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

#ifdef _MSC_VER
	_set_new_handler(sim_new_handler);
#else
	std::set_new_handler(sim_new_handler);
#endif
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
			"  <hansjoerg.malthaner@gmx.de>\n"
			"---------------------------------------\n"
		);
		return 0;
	}

#ifdef __BEOS__
	if(1) { // since BeOS only supports relative paths ...
#else
	// use current dir as basedir, else use program_dir
	if (gimme_arg(argc, argv, "-use_workdir",0)) {
#endif
		// save the current directories
		getcwd( umgebung_t::program_dir, 1024 );
#ifdef _WIN32
		strcat( umgebung_t::program_dir, "\\" );
#else
		strcat( umgebung_t::program_dir, "/" );
#endif
	}
	else {
		strcpy( umgebung_t::program_dir, argv[0] );
#ifdef _WIN32
		*(strrchr( umgebung_t::program_dir, '\\' )+1) = 0;
#else
		*(strrchr( umgebung_t::program_dir, '/' )+1) = 0;
#endif
		chdir( umgebung_t::program_dir );
	}

	// only the pak specifiy conf should overide this!
	uint16 pak_diagonal_multiplier = umgebung_t::default_einstellungen.get_pak_diagonal_multiplier();

	// parsing config/simuconf.tab
	printf("Reading low level config data ...\n");
	bool found_settings = false;
	bool found_simuconf = false;
	bool multiuser = (gimme_arg(argc, argv, "-singleuser", 0) == NULL);

	tabfile_t simuconf;
	if(simuconf.open("config/simuconf.tab")) {
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
			// reset to false (otherwise freeplay will persist)
			umgebung_t::default_einstellungen.set_freeplay( false );
		}
	}

	// continue parsing ...
	chdir( umgebung_t::program_dir );
	if(  found_simuconf  ) {
		if(simuconf.open("config/simuconf.tab")) {
			printf("parse_simuconf() at config/simuconf.tab: ");
			umgebung_t::default_einstellungen.parse_simuconf( simuconf, disp_width, disp_height, fullscreen, umgebung_t::objfilename );
		}
	}

	// if set for multiuser, then parses the users config (if there)
	// retrieve everything (but we must do this again once more ... )
	if(multiuser) {
		cstring_t obj_conf( cstring_t(umgebung_t::user_dir) + "simuconf.tab" );
		if(simuconf.open(obj_conf)) {
			printf("parse_simuconf() at %s: ", (const char *)obj_conf );
			umgebung_t::default_einstellungen.parse_simuconf( simuconf, disp_width, disp_height, fullscreen, umgebung_t::objfilename );
		}
	}

	// unmgebung: overide previous settings
	if(  (gimme_arg(argc, argv, "-freeplay", 0) != NULL)  ) {
		umgebung_t::default_einstellungen.set_freeplay( true );
	}
	if(  gimme_arg(argc, argv, "-debug", 0) != NULL  ) {
		const char *s = gimme_arg(argc, argv, "-debug", 1);
		int level = 4;
		if(s!=NULL  &&  s[0]>='0'  &&  s[0]<='9'  ) {
			level = atoi(s);
		}
		umgebung_t::verbose_debug = level;
	}

	// now set the desired objectfilename (overide all previous settings)
	if (gimme_arg(argc, argv, "-objects", 1)) {
		umgebung_t::objfilename = gimme_arg(argc, argv, "-objects", 1);
	}

	if (gimme_arg(argc, argv, "-log", 0)) {
		chdir( umgebung_t::user_dir );
		init_logging("simu.log", true, gimme_arg(argc, argv, "-log", 0) != NULL);
	} else if (gimme_arg(argc, argv, "-debug", 0) != NULL) {
		init_logging("stderr", true, gimme_arg(argc, argv, "-debug", 0) != NULL);
	} else {
		init_logging(NULL, false, false);
	}

#if 0	// server not yet fully implemented, be patient
	// starting a server?
	if(  gimme_arg(argc, argv, "-server", 0)  ) {
		const char *p = gimme_arg(argc, argv, "-server", 1);
		int portadress = p ? atoi( p ) : 13353;
		if(  portadress==0  ) {
			portadress = 13353;
		}
		// will fail fatal on the opening routine ...
		dbg->message( "simmain()", "Server started on port %i", portadress );
		umgebung_t::server = umgebung_t::networkmode = network_init_server( portadress );
	}
#endif

	DBG_MESSAGE( "simmain::main()", "Version: " VERSION_NUMBER "  Date: " VERSION_DATE);
	DBG_MESSAGE( "Debuglevel","%i", umgebung_t::verbose_debug );
	DBG_MESSAGE( "program_dir", umgebung_t::program_dir );
	DBG_MESSAGE( "home_dir", umgebung_t::user_dir );
#ifdef DEBUG
	if (gimme_arg(argc, argv, "-sizes", 0) != NULL) {
		// show the size of some structures ...
		show_sizes();
	}
#endif

	// prepare skins first
	obj_reader_t::init( translator::translate("Loading skins ...") );
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
			n = sscanf(res_str, "%dx%d", &disp_width, &disp_height);
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
	dr_os_init(parameter);

	// get optimal resolution ...
	if(  disp_width==0  ||  disp_height==0  ) {
		int scr_x = dr_query_screen_width();
		int scr_y = dr_query_screen_height();
		if(  fullscreen  ) {
			disp_width = scr_x;
			disp_height = scr_y;
		}
		else {
			disp_width = min( 704, scr_x );
			disp_height = min( 560, scr_y );
		}
	}

	printf("Preparing display ...\n");
	simgraph_init(disp_width, disp_height, fullscreen);

	// if no object files given, we ask the user
	if(  umgebung_t::objfilename.empty()  ) {
		show_pointer(1);
		ask_objfilename();
		if(  umgebung_t::objfilename.empty()  ) {
			// nothing to be loaded => exit
			fprintf(stderr, "*** No pak set found ***\n\nMost likely, you have no pak set installed.\nPlease download and install also graphics (pak).\n");
			dr_fatal_notify( "*** No pak set found ***\n\nMost likely, you have no pak set installed.\nPlease download and install also graphcis (pak).\n", 0 );
			simgraph_exit();
			return 0;
		}
		show_pointer(0);
	}

	// now find the pak specific tab file ...
	cstring_t obj_conf = umgebung_t::objfilename + "config/simuconf.tab";
	cstring_t dummy("");
	if(simuconf.open((const char *)obj_conf)) {
		sint16 idummy;
		printf("parse_simuconf() at %s: ", (const char *)obj_conf);
		umgebung_t::default_einstellungen.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
		pak_diagonal_multiplier = umgebung_t::default_einstellungen.get_pak_diagonal_multiplier();
		simuconf.close();
	}
	// and parse again parse the user settings
	if(umgebung_t::user_dir!=umgebung_t::program_dir) {
		cstring_t obj_conf( cstring_t(umgebung_t::user_dir) + "simuconf.tab" );
		if(simuconf.open(obj_conf)) {
			sint16 idummy;
			printf("parse_simuconf() at %s: ", (const char *)obj_conf);
			umgebung_t::default_einstellungen.parse_simuconf( simuconf, idummy, idummy, idummy, dummy );
			simuconf.close();
		}
		if(  gimme_arg(argc, argv, "-objects", 1) != NULL  ) {
			if(gimme_arg(argc, argv, "-addons", 0) != NULL) {
				umgebung_t::default_einstellungen.set_with_private_paks( true );
			}
			if(gimme_arg(argc, argv, "-noaddons", 0) != NULL) {
				umgebung_t::default_einstellungen.set_with_private_paks( false );
			}
		}
	}
	else {
		// not possible for single user
		umgebung_t::default_einstellungen.set_with_private_paks( false );
	}

	// now (re)set the correct length from the pak
	umgebung_t::default_einstellungen.set_pak_diagonal_multiplier( pak_diagonal_multiplier );
	vehikel_basis_t::set_diagonal_multiplier( pak_diagonal_multiplier, pak_diagonal_multiplier );

	convoihandle_t::init( 1024 );
	linehandle_t::init( 1024 );
	halthandle_t::init( 1024 );

	// just check before loading objects
	if (!gimme_arg(argc, argv, "-nosound", 0)  &&  dr_init_sound()) {
		printf("Reading compatibility sound data ...\n");
		sound_besch_t::init();
	}
	else {
		sound_set_mute(true);
	}

	// Adam - Moved away loading from simmain and placed into translator for better modularisation
	if (!translator::load(umgebung_t::objfilename)) {
		// installation error: likely only program started
		dbg->fatal("simmain::main()", "Unable to load any language files\n*** PLEASE INSTALL PROPER BASE FILES ***\n");
		exit(11);
	}

	// use requested (if available)
	if(  found_settings  ) {
		translator::set_language( umgebung_t::language_iso );
	}

	// Hajo: simgraph init loads default fonts, now we need to load
	// the real fonts for the current language
	sprachengui_t::init_font_from_lang();
	chdir(umgebung_t::program_dir);

	printf("Reading city configuration ...\n");
	stadt_t::cityrules_init(umgebung_t::objfilename);

	printf("Reading speedbonus configuration ...\n");
	vehikelbauer_t::speedbonus_init(umgebung_t::objfilename);

	printf("Reading forest configuration ...\n");
	baum_t::forestrules_init(umgebung_t::objfilename);

	printf("Reading menu configuration ...\n");
	werkzeug_t::init_menu();

	// loading all paks
	printf("Reading object data from %s...\n", (const char*)umgebung_t::objfilename);
	obj_reader_t::load(umgebung_t::objfilename, translator::translate("Loading paks ...") );
	if(  umgebung_t::default_einstellungen.get_with_private_paks()  ) {
		// try to read addons from private directory
		chdir( umgebung_t::user_dir );
		if(!obj_reader_t::load(umgebung_t::objfilename,translator::translate("Loading addon paks ..."))) {
			fprintf(stderr, "reading addon object data failed (disabling).\n");
			umgebung_t::default_einstellungen.set_with_private_paks( false );
		}
		chdir( umgebung_t::program_dir );
	}
	obj_reader_t::laden_abschliessen();

	// set overtaking offsets
	vehikel_basis_t::set_overtaking_offsets( umgebung_t::drive_on_left );

	printf("Reading menu configuration ...\n");
	werkzeug_t::read_menu(umgebung_t::objfilename);

	if(  translator::get_language()==-1  ) {
		ask_language();
	}

	bool new_world = true;
	cstring_t loadgame = "";

	if (gimme_arg(argc, argv, "-load", 0) != NULL) {
		char buf[256];
		chdir( umgebung_t::user_dir );
		/**
		 * Added automatic adding of extension
		 */
		const char *name = gimme_arg(argc, argv, "-load", 1);
		if(  strstr(name,"net:")==name  ) {
			strcpy( buf, name );
		}
		else {
			sprintf(buf, SAVE_PATH_X "%s", (const char*)searchfolder_t::complete(name, "sve"));
		}
		loadgame = buf;
		new_world = false;
	}
	else {
		chdir( umgebung_t::program_dir );
		char buffer[256];
		sprintf(buffer, "%s%sdemo.sve", (const char*)umgebung_t::program_dir, (const char*)umgebung_t::objfilename);
		// access did not work!
		FILE *f=fopen(buffer,"rb");
		if(f) {
			// there is a demo game to load
			loadgame = buffer;
			fclose(f);
DBG_MESSAGE("simmain","loadgame file found at %s",buffer);
		}
	}

	if (gimme_arg(argc, argv, "-timeline", 0) != NULL) {
		const char* ref_str = gimme_arg(argc, argv, "-timeline", 1);
		if (ref_str != NULL) {
			umgebung_t::default_einstellungen.set_use_timeline( atoi(ref_str) );
		}
	}

	if (gimme_arg(argc, argv, "-startyear", 0) != NULL) {
		const char * ref_str = gimme_arg(argc, argv, "-startyear", 1); //1930
		if (ref_str != NULL) {
			umgebung_t::default_einstellungen.set_starting_year( clamp(atoi(ref_str),1,2999) );
		}
	}

	// now always writing in user dir (which points the the program dir in multiuser mode)
	chdir( umgebung_t::user_dir );

	// init midi before loading sounds
	if(dr_init_midi()) {
		printf("Reading midi data ...\n");
		if(!midi_init(umgebung_t::user_dir)) {
			if(!midi_init(umgebung_t::program_dir)) {
				printf("Midi disabled ...\n");
			}
		}
		if(gimme_arg(argc, argv, "-nomidi", 0)) {
			midi_set_mute(true);
		}
	}
	else {
		printf("Midi disabled ...\n");
		midi_set_mute(true);
	}

	// restore previous sound settings ...
	sound_set_shuffle_midi( umgebung_t::shuffle_midi!=0 );
	sound_set_mute(  umgebung_t::mute_sound  ||  sound_get_mute() );
	midi_set_mute(  umgebung_t::mute_midi  ||  midi_get_mute() );
	sound_set_global_volume( umgebung_t::global_volume );
	sound_set_midi_volume( umgebung_t::midi_volume );
	if(!midi_get_mute()) {
		// not muted => play first song
		midi_play(0);
	}

	karte_t *welt = new karte_t();
	karte_ansicht_t *view = new karte_ansicht_t(welt);
	welt->set_ansicht( view );

	// some messages about old vehicle may appear ...
	welt->get_message()->set_message_flags(0, 0, 0, 0);

	// set the frame per second
	const char *ref_str = gimme_arg(argc, argv, "-fps", 1);
	if (ref_str != NULL) {
		int want_refresh = atoi(ref_str);
		umgebung_t::fps = want_refresh < 5 ? 5 : (want_refresh > 100 ? 100 : want_refresh);
	}

	chdir(umgebung_t::user_dir);

	// reset random counter to true randomness
	setsimrand(dr_time(), dr_time());
	set_random_allowed( true );

	if(loadgame==""  ||  !welt->laden(loadgame)) {
		// create a default map
		DBG_MESSAGE("init with default map","(failing will be a pak error!)");
		// no autosave on initial map during the first six month ...
		loadgame = "";
		new_world = true;
		sint32 old_autosave = umgebung_t::autosave;
		umgebung_t::autosave = false;
		einstellungen_t sets = umgebung_t::default_einstellungen;
		sets.set_default_climates();
		sets.set_use_timeline( 1 );
		sets.set_grundwasser( -2*Z_TILE_STEP );
		sets.set_max_mountain_height( 160 );
		sets.set_map_roughness( 0.6 );
		sets.set_groesse(64,64);
		sets.set_anzahl_staedte(1);
		sets.set_mittlere_einwohnerzahl( 1600 );
		sets.set_land_industry_chains(1);
		sets.set_tourist_attractions(1);
		sets.set_verkehr_level(7);
		sets.set_karte_nummer( 33 );
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
		// just init view (world was loaded from file)
		intr_set(welt, view);
		win_set_welt(welt);
		werkzeug_t::toolbar_tool[0]->init(welt,welt->get_active_player());
	}

#if defined DEBUG || defined PROFILE
	// do a render test?
	if (gimme_arg(argc, argv, "-times", 0) != NULL) {
		show_times(welt, view);
	}

	// finish after a certain month? (must be entered decimal, i.e. 12*year+month
	if(  gimme_arg(argc, argv, "-until", 0) != NULL  ) {
		quit_month = atoi( gimme_arg(argc, argv, "-until", 1) );
	}
#endif

	welt->set_fast_forward(false);
	if(  !umgebung_t::networkmode  &&  !umgebung_t::server  ) {
#ifdef PROFILE
		welt->set_fast_forward(true);
		if( loadgame == "" ) {
			dbg->fatal("simmain", "no game loaden in profile mode. Use -load");
		}
#endif
		view->display(true);
		intr_refresh_display(true);
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

	welt->get_message()->clear();

#ifndef PROFILE
	if(  !umgebung_t::networkmode  ||  new_world  ) {
		ticker::add_msg("Welcome to Simutrans, a game created by Hj. Malthaner and the Simutrans community.", koord::invalid, PLAYER_FLAG + 1);
		zeige_banner(welt);
	}
#endif


	while (!umgebung_t::quit_simutrans) {
		// play next tune?
		check_midi();

		// to purge all previous old messages
		welt->get_message()->set_message_flags(umgebung_t::message_flags[0], umgebung_t::message_flags[1], umgebung_t::message_flags[2], umgebung_t::message_flags[3]);

		if(  !umgebung_t::networkmode  &&  !umgebung_t::server  ) {
			welt->set_pause( false );
		}

		if (new_world) {
			climate_gui_t *cg = new climate_gui_t(&umgebung_t::default_einstellungen);
			event_t ev;

			view->display(true);

			create_win((disp_width - cg->get_fenstergroesse().x-10), 40, cg, w_info, magic_climate );

			// we want to center wg (width 260) between sg (width 220) and cg (176)
			welt_gui_t *wg = new welt_gui_t(welt, &umgebung_t::default_einstellungen);
			create_win((disp_width - 220 - cg->get_fenstergroesse().x -10 -10- 260)/2 + 220 + 10, (disp_height - 300) / 2, wg, w_do_not_delete, magic_welt_gui_t );

			do {
				INT_CHECK("simmain 803");
				win_poll_event(&ev);
				INT_CHECK("simmain 805");
				check_pos_win(&ev);
				if(  ev.ev_class == EVENT_SYSTEM  &&  ev.ev_code == SYSTEM_QUIT  ) {
					umgebung_t::quit_simutrans = true;
				}
				INT_CHECK("simmain 807");
				if(  umgebung_t::networkmode  ) {
					static int count = 0;
					if(  ((count++)&7)==0 ) {
						static uint32 last_step = dr_time();
						uint32 next_step = dr_time();
						welt->sync_step( next_step-last_step, true, true );
						welt->step();
						last_step = next_step;
					}
				}
				dr_sleep(5);
				welt->reset_interaction();
			} while(
				!wg->get_load() &&
				!wg->get_scenario() &&
				!wg->get_load_heightfield() &&
				!wg->get_start() &&
				!wg->get_close() &&
				!wg->get_quit() &&
				!umgebung_t::quit_simutrans
			);

			if (IS_LEFTCLICK(&ev)) {
				do {
					display_get_event(&ev);
				} while (!IS_LEFTRELEASE(&ev));
			}

			destroy_all_win();
			// scenario?
			if(wg->get_scenario()) {
				char path[1024];
				sprintf( path, "%s%sscenario/", umgebung_t::program_dir, (const char *)umgebung_t::objfilename );
				chdir( path );
				delete wg;
				create_win( new scenario_frame_t(welt), w_info, magic_load_t );
				chdir( umgebung_t::user_dir );
			}
			// Neue Karte erzeugen
			else if (wg->get_start()) {
				// since not autodelete
				delete wg;

				create_win(200, 100, new news_img("Erzeuge neue Karte.\n", skinverwaltung_t::neueweltsymbol->get_bild_nr(0)), w_info, magic_none);
				intr_refresh_display(true);

				umgebung_t::default_einstellungen.heightfield = "";
				welt->init(&umgebung_t::default_einstellungen,0);

				// save setting ...
				loadsave_t file;
				if(file.wr_open("default.sve",loadsave_t::binary,"settings only")) {
					// save default setting
					umgebung_t::default_einstellungen.rdwr(&file);
					file.close();
				}
				destroy_all_win();
				welt->step_month( umgebung_t::default_einstellungen.get_starting_month() );
			}
			else if(wg->get_load()) {
				delete wg;
				create_win( new loadsave_frame_t(welt, true), w_info, magic_load_t);
			}
			else if(wg->get_load_heightfield()) {
				delete wg;
				welt->load_heightfield(&umgebung_t::default_einstellungen);
				welt->step_month( umgebung_t::default_einstellungen.get_starting_month() );
			}
			else {
				// quit the game
				if (wg->get_quit()  ||  umgebung_t::quit_simutrans  ) {
					delete wg;
					break;
				}
			}
		}

		loadgame = ""; // only first time

		// run the loop
		welt->interactive(quit_month);

		new_world = true;
		welt->get_message()->get_message_flags(&umgebung_t::message_flags[0], &umgebung_t::message_flags[1], &umgebung_t::message_flags[2], &umgebung_t::message_flags[3]);
		welt->set_fast_forward(false);
		setsimrand(dr_time(), dr_time());
	}

	intr_disable();

	// save setting ...
	chdir( umgebung_t::user_dir );
	if(file.wr_open("settings.xml",loadsave_t::xml,"settings only/")) {
		umgebung_t::rdwr(&file);
		umgebung_t::default_einstellungen.rdwr(&file);
		file.close();
	}

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
