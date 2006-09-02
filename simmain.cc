#include <stdio.h>
#include <new>
#include <time.h>

#ifdef _MSC_VER
#include <new.h> // for _set_new_handler
#endif

#include "pathes.h"

#include "simworld.h"
#include "simview.h"
#include "simwin.h"
#include "simhalt.h"
#include "simimg.h"
#include "simcolor.h"
#include "simdepot.h"
#include "simskin.h"
#include "simtime.h"
#include "simconst.h"
#include "boden/boden.h"
#include "boden/wasser.h"
#include "simcity.h"
#include "simvehikel.h"
#include "simfab.h"
#include "simplay.h"
#include "simsound.h"
#include "simintr.h"
#include "simmesg.h"

#include "linehandle_t.h"

#include "simsys.h"
#include "simgraph.h"
#include "simevent.h"
#include "simdisplay.h"
#include "simtools.h"

#include "simversion.h"

#include "gui/welt.h"
#include "gui/sprachen.h"
#include "gui/messagebox.h"
#include "gui/loadsave_frame.h"
#include "gui/load_relief_frame.h"
#include "dings/baum.h"
#include "utils/simstring.h"
#include "utils/cstring_t.h"
#include "utils/searchfolder.h"

#include "dataobj/loadsave.h"
#include "dataobj/umgebung.h"
#include "dataobj/tabfile.h"
#include "dataobj/einstellungen.h"
#include "dataobj/translator.h"

#include "besch/reader/obj_reader.h"
#include "besch/sound_besch.h"

#include "bauer/hausbauer.h"

#include "simverkehr.h"


/* diagnostic routine:
 * show the size of several internal structures
 */
static void show_sizes()
{
	DBG_MESSAGE("test", "size of strutures");

	DBG_MESSAGE("test", "koord: %d", sizeof(koord));
	DBG_MESSAGE("test", "koord3d: %d", sizeof(koord3d));
	DBG_MESSAGE("test", "ribi_t::ribi: %d", sizeof(ribi_t::ribi));

	DBG_MESSAGE("test", "ding_t: %d", sizeof(ding_t));
	DBG_MESSAGE("test", "gebaeude_t: %d", sizeof(gebaeude_t));
	DBG_MESSAGE("test", "baum_t: %d", sizeof(baum_t));

	DBG_MESSAGE("test", "grund_t: %d", sizeof(grund_t));
	DBG_MESSAGE("test", "boden_t: %d", sizeof(boden_t));
	DBG_MESSAGE("test", "wasser_t: %d", sizeof(wasser_t));

	DBG_MESSAGE("test", "weg_t: %d", sizeof(weg_t));

	DBG_MESSAGE("test", "planquadrat_t: %d", sizeof(planquadrat_t));
	DBG_MESSAGE("test", "karte_t: %d", sizeof(karte_t));
	DBG_MESSAGE("test", "spieler_t: %d\n", sizeof(spieler_t));
}


// render tests ...
static void show_times(karte_t *welt, karte_ansicht_t *view)
{
	DBG_MESSAGE("test", "testing img ... ");
	int i;

	long ms = get_current_time_millis();
	for (i = 0;  i < 300000;  i++)
		display_img(2000, 100, 100, 1);
	DBG_MESSAGE("test", "display_img(): %i iterations took %i ms", i, get_current_time_millis() - ms);

	ms = get_current_time_millis();
	for (i = 0;  i < 300000;  i++)
		display_color_img(2000, 120, 100, 16, 1, 1);
	DBG_MESSAGE("test", "display_color_img(): %i iterations took %i ms", i, get_current_time_millis() - ms);

	ms = get_current_time_millis();
	for (i = 0;  i < 300000;  i++)
		display_color_img(2000, 120, 100, 20, 1, 1);
	DBG_MESSAGE("test", "display_color_img(), other AI: %i iterations took %i ms", i, get_current_time_millis() - ms);

	ms = get_current_time_millis();
	for (i = 0;  i < 300;  i++)
		display_flush_buffer();
	DBG_MESSAGE("test", "display_flush_buffer(): %i iterations took %i ms", i, get_current_time_millis() - ms);

	ms = get_current_time_millis();
	for (i = 0;  i < 300000;  i++)
		display_text_proportional_len_clip(100, 120, "Dies ist ein kurzer Textetxt ...", 0, 0, false, true, -1, false);
	DBG_MESSAGE("test", "display_text_proportional_len_clip(): %i iterations took %i ms", i, get_current_time_millis() - ms);

	ms = get_current_time_millis();
	for (i = 0;  i < 300000;  i++)
		display_fillbox_wh(100, 120, 300, 50, 0, false);
	DBG_MESSAGE("test", "display_fillbox_wh(): %i iterations took %i ms", i, get_current_time_millis() - ms);

	ms = get_current_time_millis();
	for (i = 0; i < 200; i++) {
		view->display(true);
	}
	DBG_MESSAGE("test", "view->display(true): %i iterations took %i ms", i, get_current_time_millis() - ms);

	ms = get_current_time_millis();
	for (i = 0; i < 200; i++) {
		view->display(true);
		win_display_flush(0, 0, 0.0);
	}
	DBG_MESSAGE("test", "view->display(true) and flush: %i iterations took %i ms", i, get_current_time_millis() - ms);

	ms = get_current_time_millis();
	intr_set(welt, view, 1);
	for (i = 0; i < 200; i++) {
		welt->step(200);
	}
	intr_disable();
	DBG_MESSAGE("test", "welt->step(200): %i iterations took %i ms", i, get_current_time_millis() - ms);
}


/**
 * Intro Scroller
 */
static void scroll_intro(const int xoff, const int yoff)
{
	static const char* const scrolltext[] = {
#include "scrolltext.h"
	};

	static int line = 0;

	const int text_line = (line / 9) * 2;
	const int text_offset = line % 9;
	const int left = 60;
	const int top = 196;

	display_fillbox_wh(xoff + left, yoff + top, 240, 48, COL_GREY1, true);

	display_proportional(xoff + left +   4, yoff +  1 + top - text_offset, scrolltext[text_line +  0], ALIGN_LEFT,  COL_WHITE, true);
	display_proportional(xoff + left + 236, yoff +  1 + top - text_offset, scrolltext[text_line +  1], ALIGN_RIGHT, COL_WHITE, true);
	display_proportional(xoff + left +   4, yoff + 10 + top - text_offset, scrolltext[text_line +  2], ALIGN_LEFT,  COL_WHITE, true);
	display_proportional(xoff + left + 236, yoff + 10 + top - text_offset, scrolltext[text_line +  3], ALIGN_RIGHT, COL_WHITE, true);
	display_proportional(xoff + left +   4, yoff + 19 + top - text_offset, scrolltext[text_line +  4], ALIGN_LEFT,  COL_GREY6, true);
	display_proportional(xoff + left + 236, yoff + 19 + top - text_offset, scrolltext[text_line +  5], ALIGN_RIGHT, COL_GREY6, true);
	display_proportional(xoff + left +   4, yoff + 28 + top - text_offset, scrolltext[text_line +  6], ALIGN_LEFT,  COL_GREY5, true);
	display_proportional(xoff + left + 236, yoff + 28 + top - text_offset, scrolltext[text_line +  7], ALIGN_RIGHT, COL_GREY5, true);
	display_proportional(xoff + left +   4, yoff + 37 + top - text_offset, scrolltext[text_line +  8], ALIGN_LEFT,  COL_GREY4, true);
	display_proportional(xoff + left + 236, yoff + 37 + top - text_offset, scrolltext[text_line +  9], ALIGN_RIGHT, COL_GREY4, true);
	display_proportional(xoff + left +   4, yoff + 46 + top - text_offset, scrolltext[text_line + 10], ALIGN_LEFT,  COL_GREY3, true);
	display_proportional(xoff + left + 236, yoff + 46 + top - text_offset, scrolltext[text_line + 11], ALIGN_RIGHT, COL_GREY3, true);

	display_fillbox_wh(xoff + left, yoff + top - 8, 240, 7, COL_GREY4, true);
	display_fillbox_wh(xoff + left, yoff + top - 1, 240, 1, COL_GREY3, true);

	display_fillbox_wh(xoff + left, yoff + top + 48, 240, 1, COL_GREY6, true);
	display_fillbox_wh(xoff + left, yoff + top + 49, 240, 7, COL_GREY4, true);

	line++;

	if (scrolltext[text_line + 12] == 0) line = 0;
}


/**
 * Intro Screen
 */
static void zeige_banner()
{
	int xoff = (display_get_width()  / 2) - 180;
	int yoff = (display_get_height() / 2) - 125;
	struct event_t ev;

#if 0
	display_show_pointer(false);
#endif

	display_ddd_box(xoff, yoff, 360, 270, COL_GREY6, COL_GREY2);
	display_fillbox_wh(xoff + 1, yoff + 1, 358, 268, COL_GREY5, true);
	display_ddd_box(xoff + 4, yoff + 4, 352, 262, COL_GREY2, COL_GREY6);
	display_fillbox_wh(xoff + 5, yoff + 5, 350, 260, COL_GREY4, true);

	display_color_img(skinverwaltung_t::logosymbol->gib_bild_nr(0), xoff + 264, yoff + 40, 0, false, true);

	xoff += 30;

	// shadow effect by drawing two times with an offset
	for (int s = 1; s >= 0; s--) {
		int heading = (s == 0 ?         3 : COL_BLACK);
		int color   = (s == 0 ? COL_WHITE : COL_BLACK);

		display_proportional(xoff + s + 24, yoff + s +  16, "This is a beta version of Simutrans:", ALIGN_LEFT, heading, true);

		display_proportional(xoff + s + 48, yoff + s +  28, "Version " VERSION_NUMBER " " VERSION_DATE, ALIGN_LEFT, color, true);

		display_proportional(xoff + s + 24, yoff + s +  48, "This version is developed by", ALIGN_LEFT, heading, true);

		display_proportional(xoff + s + 48, yoff + s +  64, "the simutrans team, based on", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48, yoff + s +  76, "Simutrans 0.84.21.2 by", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48, yoff + s +  88, "Hansjörg Malthaner et al.", ALIGN_LEFT, color, true);

		display_proportional(xoff + s + 48, yoff + s + 102, "All rights reserved.", ALIGN_LEFT, color, true);

		display_proportional(xoff + s + 24, yoff + s + 122, "Please send ideas and questions to:", ALIGN_LEFT, heading, true);

		display_proportional(xoff + s + 48, yoff + s + 138, "Markus Pristovsek", ALIGN_LEFT, color, true);
		display_proportional(xoff + s + 48, yoff + s + 138 + 12, "<markus@pristovsek.de>", ALIGN_LEFT, color, true);

		display_proportional(xoff + s + 24, yoff + s + 158 + 12, "or visit the Simutrans pages on the web:", ALIGN_LEFT, heading, true);

		display_proportional(xoff + s + 48, yoff + s + 184, "http://www.simutrans.com", ALIGN_LEFT, color, true);
	}

	xoff -= 30;
	do {
		// check if we need to play a new midi file
		check_midi();

		scroll_intro(xoff, yoff + 10);
		display_flush(IMG_LEER, 0, 0, 0.0, "    Welcome to Simutrans", "", NULL, 0);
		dr_sleep(70000);
		display_poll_event(&ev);
	} while(!IS_LEFTCLICK(&ev) && (ev.ev_class != EVENT_KEYBOARD || ev.ev_code == 0));

	// wait for mouse release, of click
	if (IS_LEFTCLICK(&ev)) {
		do {
			display_get_event(&ev);
		} while (!IS_LEFTRELEASE(&ev));
	}
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
		if (tstrequ(argv[i], arg) && i < argc - off) return argv[i + off];
	}
	return NULL;
}


extern "C" int simu_main(int argc, char** argv)
{
	// Try to catch all exceptions and print them
	try {
		static const int resolutions[][2] = {
			{  640,  480 },
			{  800,  600 },
			{ 1024,  768 },
			{ 1280, 1024 },
			{  672,  496 } // try to force window mode with allegro
		};

		bool quit_simutrans = false;

		FILE * config = NULL; // die konfigurationsdatei

		int disp_width = 800;
		int disp_height = 600;
		int fullscreen = false;

		cstring_t loadgame = "";
		cstring_t objfilename = DEFAULT_OBJPATH;

#ifdef _MSC_VER
		_set_new_handler(sim_new_handler);
#else
		std::set_new_handler(sim_new_handler);
#endif

		if (gimme_arg(argc, argv, "-log", 0)) {
			init_logging("simu.log", true, gimme_arg(argc, argv, "-debug", 0) != NULL);
		} else if (gimme_arg(argc, argv, "-debug", 0) != NULL) {
			init_logging("stderr", true, gimme_arg(argc, argv, "-debug", 0) != NULL);
		} else {
			init_logging(NULL, false, false);
		}

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


		DBG_MESSAGE("simmain::main()", "Version: " VERSION_NUMBER "  Date: " VERSION_DATE);
#ifdef DEBUG
		// show the size of some structures ...
		show_sizes();
#endif

		// unmgebung init
		umgebung_t::testlauf      = (gimme_arg(argc, argv, "-test",     0) != NULL);
		umgebung_t::freeplay      = (gimme_arg(argc, argv, "-freeplay", 0) != NULL);
		umgebung_t::verbose_debug = (gimme_arg(argc, argv, "-debug",    0) != NULL);

		// parsing config/simuconf.tab
		print("Reading low level config data ...\n");
		tabfile_t simuconf;
		if (simuconf.open("config/simuconf.tab")) {
			tabfileobj_t contents;

			simuconf.read(contents);

			print("Initializing tombstones ...\n");

			convoihandle_t::init(contents.get_int("convoys", 8192));
			linehandle_t::init(contents.get_int("lines", 8192));
			halthandle_t::init(contents.get_int("stations", 8192));
			umgebung_t::max_route_steps = contents.get_int("max_route_steps", 100000);

			umgebung_t::bodenanimation = (contents.get_int("animated_grounds", 1) != 0);

			umgebung_t::fussgaenger             = contents.get_int("random_pedestrians",     0) != 0;
			umgebung_t::verkehrsteilnehmer_info = contents.get_int("pedes_and_car_info",     0) != 0;
			umgebung_t::stadtauto_duration      = contents.get_int("citycar_life",       31457280);	// ten normal years
			umgebung_t::drive_on_left           = contents.get_int("drive_left",         false);

			umgebung_t::tree_info     = contents.get_int("tree_info",        0) != 0;
			umgebung_t::townhall_info = contents.get_int("townhall_info",    0) != 0;
			umgebung_t::single_info   = contents.get_int("only_single_info", 0);

			umgebung_t::window_buttons_right = contents.get_int("window_buttons_right", false);

			umgebung_t::starting_money = contents.get_int("starting_money",       15000000);
			umgebung_t::maint_building = contents.get_int("maintenance_building",     5000);
			umgebung_t::maint_way      = contents.get_int("maintenance_ways",          800);
			umgebung_t::maint_overhead = contents.get_int("maintenance_overhead",      200);


			umgebung_t::show_names            = contents.get_int("show_names",        3);
			umgebung_t::numbered_stations     = contents.get_int("numbered_stations", 0) != 0;
			umgebung_t::station_coverage_size = contents.get_int("station_coverage",  2);

			// time stuff
			umgebung_t::show_month     = contents.get_int("show_month",       0);
			umgebung_t::bits_per_month = contents.get_int("bits_per_month",  18);
			umgebung_t::use_timeline   = contents.get_int("use_timeline",     2);
			umgebung_t::starting_year  = contents.get_int("starting_year", 1930);

			umgebung_t::intercity_road_length = contents.get_int("intercity_road_length", 8000);
			umgebung_t::intercity_road_type   = new cstring_t(ltrim(contents.get("intercity_road_type")));
			umgebung_t::city_road_type        = new cstring_t(ltrim(contents.get("city_road_type")));
			if (umgebung_t::city_road_type->len() == 0) {
				*umgebung_t::city_road_type = "city_road";
			}

			umgebung_t::autosave = (contents.get_int("autosave", 0));

			umgebung_t::crossconnect_factories =
#ifdef OTTD_LIKE
				contents.get_int("crossconnect_factories", 1) != 0;
#else
				contents.get_int("crossconnect_factories", 0) != 0;
#endif
			umgebung_t::just_in_time          = contents.get_int("just_in_time",             1) != 0;
			umgebung_t::passenger_factor      = contents.get_int("passenger_factor",        16); /* this can manipulate the passenger generation */
			umgebung_t::beginner_price_factor = contents.get_int("beginner_price_factor", 1500); /* this manipulates the good prices in beginner mode */
			umgebung_t::beginner_mode_first   = contents.get_int("first_beginner",           0); /* start in beginner mode */

			/* now the cost section */
			umgebung_t::cst_multiply_dock        = contents.get_int("cost_multiply_dock",          500) * -100;
			umgebung_t::cst_multiply_station     = contents.get_int("cost_multiply_station",       600) * -100;
			umgebung_t::cst_multiply_roadstop    = contents.get_int("cost_multiply_roadstop",      400) * -100;
			umgebung_t::cst_multiply_airterminal = contents.get_int("cost_multiply_airterminal",  3000) * -100;
			umgebung_t::cst_multiply_post        = contents.get_int("cost_multiply_post",          300) * -100;
			umgebung_t::cst_multiply_headquarter = contents.get_int("cost_multiply_headquarter", 10000) * -100;
			umgebung_t::cst_depot_rail           = contents.get_int("cost_depot_rail",            1000) * -100;
			umgebung_t::cst_depot_road           = contents.get_int("cost_depot_road",            1300) * -100;
			umgebung_t::cst_depot_ship           = contents.get_int("cost_depot_ship",            2500) * -100;
			umgebung_t::cst_signal               = contents.get_int("cost_signal",                 500) * -100;
			umgebung_t::cst_tunnel               = contents.get_int("cost_tunnel",               10000) * -100;
			umgebung_t::cst_third_rail           = contents.get_int("cost_third_rail",              80) * -100;

			// alter landscape
			umgebung_t::cst_alter_land              = contents.get_int("cost_alter_land",                  500) * -100;
			umgebung_t::cst_set_slope               = contents.get_int("cost_set_slope",                  2500) * -100;
			umgebung_t::cst_found_city              = contents.get_int("cost_found_city",              5000000) * -100;
			umgebung_t::cst_multiply_found_industry = contents.get_int("cost_multiply_found_industry", 1000000) * -100;
			umgebung_t::cst_remove_tree             = contents.get_int("cost_remove_tree",                 100) * -100;
			umgebung_t::cst_multiply_remove_haus    = contents.get_int("cost_multiply_remove_haus",       1000) * -100;

			/* now the way builder */
			umgebung_t::way_count_curve        = contents.get_int("way_curve",            10);
			umgebung_t::way_count_double_curve = contents.get_int("way_double_curve",     40);
			umgebung_t::way_count_90_curve     = contents.get_int("way_90_curve",       2000);
			umgebung_t::way_count_slope        = contents.get_int("way_slope",            80);
			umgebung_t::way_count_tunnel       = contents.get_int("way_tunnel",            8);
			umgebung_t::way_max_bridge_len     = contents.get_int("way_max_bridge_len",   15);

			/*
			* Selection of savegame format through inifile
			*/
			const char *str = contents.get("saveformat");
			while (*str == ' ') str++;
			if (strcmp(str, "binary") == 0) {
				loadsave_t::set_savemode(loadsave_t::binary);
			} else if(strcmp(str, "zipped") == 0) {
				loadsave_t::set_savemode(loadsave_t::zipped);
			} else if(strcmp(str, "text") == 0) {
				loadsave_t::set_savemode(loadsave_t::text);
			}

			/*
			 * Default resolution
			 */
			disp_width  = contents.get_int("display_width",  800);
			disp_height = contents.get_int("display_height", 600);
			fullscreen  = contents.get_int("fullscreen",       0);

			// Default pak file path
			objfilename = ltrim(contents.get_string("pak_file_path", DEFAULT_OBJPATH));

			// Max number of steps in goods pathfinding
			haltestelle_t::set_max_hops(contents.get_int("max_hops", 300));

			// Max number of transfers in goods pathfinding
			umgebung_t::max_transfers = contents.get_int("max_transfers", 7);

			print("Reading simuconf.tab successful!\n");

			simuconf.close();
		} else {
			// likely only the programm without graphics started
			fprintf(stderr, "*** No simuconf.tab found ***\n\nPlease install a complete system\n");
			getc(stdin);
			return 0;
		}

		if (gimme_arg(argc, argv, "-objects", 1)) {
			objfilename = gimme_arg(argc, argv, "-objects", 1);
		}

		if (gimme_arg(argc, argv, "-res", 0) != NULL) {
			const char* res_str = gimme_arg(argc, argv, "-res", 1);
			const int res = *res_str - '1';

			switch (res) {
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
					disp_width  = resolutions[res][0];
					disp_height = resolutions[res][1];
					fullscreen = true;
					break;

				case 5:
					fullscreen = false;
					// XXX missing break?

				default:
					fprintf(stderr,
						"invalid resolution, argument must be 1,2,3 or 4\n"
						"1=640x480, 2=800x600, 3=1024x768, 4=1280x1024\n"
					);
					return 0;
			}
		}

		fullscreen |= (gimme_arg(argc, argv, "-fullscreen", 0) != NULL);

		if (gimme_arg(argc, argv, "-screensize", 0) != NULL) {
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

		const char* use_shm = gimme_arg(argc, argv, "-net",   0);
		const char* do_sync = gimme_arg(argc, argv, "-async", 0);

		print("Preparing display ...\n");
		simgraph_init(disp_width, disp_height, use_shm == NULL, do_sync == NULL, fullscreen);

		print("Init timer module ...\n");
		time_init();

		// just check before loading objects
		if (!gimme_arg(argc, argv, "-nosound", 0)) {
			print("Reading compatibility sound data ...\n");
			sound_besch_t::init(objfilename);
		}

		// Adam - Moved away loading from simmain and placed into translator for better modularisation
		if (!translator::load(objfilename)) {
			// installation error: likely only program started
			dbg->fatal("simmain::main()", "Unable to load any language files\n*** PLEASE INSTALL PROPER BASE FILES ***\n");
			exit(11);
		}

		print("Reading city configuration ...\n");
		stadt_t::init();

		// loading all paks
		print("Reading object data from %s...\n", objfilename.chars());
		if (!obj_reader_t::init(objfilename)) {
			fprintf(stderr, "reading object data failed.\n");
			exit(11);
		}

		if (gimme_arg(argc, argv, "-load", 0) != NULL) {
			char buf[128];
			/**
			 * Added automatic adding of extension
			 */
			sprintf(buf, SAVE_PATH_X "%s", searchfolder_t::complete(gimme_arg(argc, argv, "-load", 1), "sve").chars());
			loadgame = buf;
		}

		if (gimme_arg(argc, argv, "-timeline", 0) != NULL) {
			const char* ref_str = gimme_arg(argc, argv, "-timeline", 1);
			if (ref_str != NULL) umgebung_t::use_timeline = atoi(ref_str);
		}

		if (gimme_arg(argc, argv, "-startyear", 0) != NULL) {
			const char * ref_str = gimme_arg(argc, argv, "-startyear", 1); //1930
			if (ref_str != NULL) umgebung_t::starting_year = atoi(ref_str);
		}

		/* Jetzt, nachdem die Kommandozeile ausgewertet ist, koennen wir die
		 * Konfigurationsdatei lesen, und ggf. einige Einstellungen setzen
		 */
		config = fopen("simworld.cfg", "rb");
		int sprache = -1;
		if (config) {
			fscanf(config, "Lang=%d\n", &sprache);

			int dn = 0;
			fscanf(config, "DayNight=%d\n", &dn);
			umgebung_t::night_shift = (dn != 0);

			int b[6];
			fscanf(config, "AIs=%i,%i,%i,%i,%i,%i\n", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
			for (int i = 0; i < 6; i++) umgebung_t::automaten[i] = b[i];

			fscanf(
				config, "Messages=%d,%d,%d,%d\n",
				&umgebung_t::message_flags[0],
				&umgebung_t::message_flags[1],
				&umgebung_t::message_flags[2],
				&umgebung_t::message_flags[3]
			);

			dn = 0;
			fscanf(config, "ShowCoverage=%d\n", &dn);
			umgebung_t::station_coverage_show = (dn != 0);

			fclose(config);
		}

		karte_t *welt = new karte_t();
		karte_ansicht_t *view = new karte_ansicht_t(welt);

		if (!gimme_arg(argc, argv, "-nomidi", 0)) {
			print("Reading midi data ...\n");
			midi_init();
			midi_play(0);
		}

#ifdef DEBUG
		// do a render test?
		if (gimme_arg(argc, argv, "-times", 0) != NULL) {
			show_times(welt, view);
		}
#endif

		// suche nach refresh-einstellungen
		int refresh = 1;
		const char *ref_str = gimme_arg(argc, argv, "-refresh", 1);

		if (ref_str != NULL) {
			int want_refresh = atoi(ref_str);
			refresh = want_refresh < 1 ? 1 : want_refresh > 16 ? 16 : want_refresh;
		}

		if (loadgame != "") welt->laden(loadgame);

		intr_set(welt, view, refresh);

		win_setze_welt(welt);
		win_display_menu();
		view->display(true);
		welt->sync_prepare();

		// Bringe welt in ansehnlichen Zustand
		// bevor sie als Hintergrund für das intro dient
		if (loadgame == "") {
			welt->sync_step(welt->gib_zeit_ms() + welt->ticks_per_tag / 2);
			for (int i = 0; i < 20; i++) welt->step(10000);
		}
		intr_refresh_display(true);

#ifdef USE_SOFTPOINTER
		// Hajo: give user a mouse to work with
		if (skinverwaltung_t::mouse_cursor != NULL) {
			// we must use our softpointer (only Allegro!)
			display_set_pointer(skinverwaltung_t::mouse_cursor->gib_bild_nr(0));
		}
#endif
		display_show_pointer(TRUE);

		translator::set_language("en");
		zeige_banner();

		// Hajo: simgraph init loads default fonts, now we need to load
		// the real fonts for the current language
		if (sprache != -1) translator::set_language(sprache);
		sprachengui_t::init_font_from_lang();

		welt->setze_dirty();

		einstellungen_t *sets = new einstellungen_t(*welt->gib_einstellungen());
		sets->setze_groesse(256, 256);
		sets->setze_anzahl_staedte(16);
		sets->setze_land_industry_chains(6);
		sets->setze_city_industry_chains(0);
		sets->setze_tourist_attractions(12);
		sets->setze_karte_nummer(simrand(999));
		sets->setze_station_coverage(umgebung_t::station_coverage_size);
		sets->setze_allow_player_change(true);
		sets->setze_use_timeline(umgebung_t::use_timeline == 1);
		sets->setze_starting_year(umgebung_t::starting_year);

		do {
			// play next tune?
			check_midi();

			if (!umgebung_t::testlauf && loadgame == "") {
				welt_gui_t* wg = new welt_gui_t(welt, sets);
				sprachengui_t* sg = new sprachengui_t(welt);
				event_t ev;

				view->display(true);

				create_win(10, 40, -1, sg, w_info);
				create_win((disp_width - 250) / 2, (disp_height - 300) / 2, -1, wg, w_info);


				setsimrand(get_current_time_millis(), get_current_time_millis());

				do {
					win_poll_event(&ev);
					check_pos_win(&ev);
					simusleep(10);
				} while(
					!wg->gib_load() &&
					!wg->gib_load_heightfield() &&
					!wg->gib_start() &&
					!wg->gib_close() &&
					!wg->gib_quit()
				);

				if (IS_LEFTCLICK(&ev)) {
					do {
						display_get_event(&ev);
					} while (!IS_LEFTRELEASE(&ev));
				}

				// Neue Karte erzeugen
				if (wg->gib_start()) {
					destroy_win(wg);
					destroy_win(sg);

					nachrichtenfenster_t *nd = new nachrichtenfenster_t(welt, "Erzeuge neue Karte.\n", skinverwaltung_t::neueweltsymbol->gib_bild_nr(0));
					create_win(200, 100, nd, w_autodelete);
					intr_refresh_display(true);

					sets->heightfield = "";
					welt->init(sets);
					destroy_all_win();
				} else if(wg->gib_load()) {
					destroy_win(wg);
					destroy_win(sg);
					create_win(new loadsave_frame_t(welt, true), w_info, magic_load_t);
				} else if(wg->gib_load_heightfield()) {
					destroy_win(wg);
					destroy_win(sg);
					einstellungen_t *sets = wg->gib_sets();
					welt->load_heightfield(sets);
				} else {
					destroy_win(wg);
					destroy_win(sg);
					// quit the game
					if (wg->gib_quit()) break;
				}
			}

			loadgame = ""; // only first time

			message_t *msg = new message_t(welt);
			msg->add_message("Welcome to Simutrans, a game created by Hj. Malthaner.", koord::invalid, message_t::general, 0);
			msg->set_flags(umgebung_t::message_flags[0], umgebung_t::message_flags[1], umgebung_t::message_flags[2], umgebung_t::message_flags[3]);

			quit_simutrans = welt->interactive();

			msg->get_flags(&umgebung_t::message_flags[0], &umgebung_t::message_flags[1], &umgebung_t::message_flags[2], &umgebung_t::message_flags[3]);
			delete msg;
		} while (!umgebung_t::testlauf && !quit_simutrans);

		intr_disable();

		delete welt;
		welt = NULL;

		delete view;
		view = 0;

		simgraph_exit();

		/* Zu guter Letzt schreiben wir noch die akuellen Einstellungen in eine
		 * Konfigurationsdatei
		 */
		config = fopen("simworld.cfg", "wb");
		if (config != NULL) {
			fprintf(config, "Lang=%d\n", translator::get_language());
			fprintf(config, "DayNight=%d\n", umgebung_t::night_shift);
			fprintf(
				config, "AIs=%d,%d,%d,%d,%d,%d\n",
				umgebung_t::automaten[0],
				umgebung_t::automaten[1],
				umgebung_t::automaten[2],
				umgebung_t::automaten[3],
				umgebung_t::automaten[4],
				umgebung_t::automaten[5]
			);
			fprintf(
				config, "Messages=%d,%d,%d,%d\n",
				umgebung_t::message_flags[0],
				umgebung_t::message_flags[1],
				umgebung_t::message_flags[2],
				umgebung_t::message_flags[3]
			);
			fprintf(config, "ShowCoverage=%d\n", umgebung_t::station_coverage_show);
			fclose(config);
		}

		close_midi();
	}
	catch (no_such_element_exception nse) {
		dbg->fatal("simmain.cc", "simu_main()", nse.message);
	}

	// free all list memories (not working, since there seems to be unitialized list stiil waiting for automated destruction)
#if 0
	freelist_t::free_all_nodes();
#endif

	return 0;
}
