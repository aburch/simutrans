/*
 * simmain.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#include <new>
#include <time.h>

#ifdef _MSC_VER
#endif

#include "pathes.h"

#include "simworld.h"
#include "simworldview.h"
#include "simwin.h"
#include "simhalt.h"
#include "simimg.h"
#include "simcolor.h"
#include "simdepot.h"
#include "simskin.h"
#include "simtime.h"
#include "boden/boden.h"
#include "boden/wasser.h"
#include "simcity.h"
#include "simvehikel.h"
#include "simfab.h"
#include "simplay.h"
#include "railblocks.h"
#include "simsound.h"
#include "simintr.h"


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

#include "bauer/hausbauer.h"

/*******************************************************************************
 * The follownig code is for moving files to new locations. It can be removed
 * afterwards.
 * @author Volker Meyer
 * @date  18.06.2003
 */
#ifdef _MSC_VER
#include <io.h>
#include <direct.h>
#define W_OK	2
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#endif

static void check_location(const char *file, const char *dir)
{
    char src[128];
    char dest[128];

    sprintf(src, "%s", file);
    sprintf(dest, "%s/%s", dir, file);

    if(access(dir, W_OK) == -1) {
#ifdef __MINGW32__
	mkdir(dir);
#else
	mkdir(dir, 0700);
#endif
    }
    if(access(dest, W_OK) == -1 && access(src, W_OK) != -1) {
	rename(src, dest);  // move file from source to dest
    }
}

static void check_locations()
{
    check_location("draw.fnt", FONT_PATH);
    check_location("prop.fnt", FONT_PATH);
    check_location("4x7.hex",  FONT_PATH);

    check_location("simud100.pal", PALETTE_PATH);
    check_location("simud90.pal", PALETTE_PATH);
    check_location("simud80.pal", PALETTE_PATH);
    check_location("simud70.pal", PALETTE_PATH);
    check_location("special.pal", PALETTE_PATH);

    searchfolder_t folder;
    int i;

    for(i = folder.search("./", "sve"); i-- > 0; ) {
	check_location(folder.at(i), SAVE_PATH);
    }
    for(i = folder.search("./", "bmp"); i-- > 0; ) {
	cstring_t f = folder.at(i);

	if(f.left(8) == "./simscr") {
	    check_location(f, SCRENSHOT_PATH);
	}
    }
}
/*
 * End of file moving code. Call to check locations beneath has to be removed,
 * too.
 ******************************************************************************/


static void show_sizes()
{
    print("koord:\t\t%d\n", sizeof(koord));
    print("koord3d:\t%d\n", sizeof(koord3d));
    print("ribi_t::ribi:\t%d\n\n", sizeof(ribi_t::ribi));

    print("ding_t:\t\t%d\n", sizeof(ding_t));
    print("gebaeude_t:\t%d\n", sizeof(gebaeude_t));
    print("baum_t:\t\t%d\n\n", sizeof(baum_t));

    print("grund_t:\t%d\n", sizeof(grund_t));
    print("boden_t:\t%d\n", sizeof(boden_t));
    print("wasser_t:\t%d\n\n", sizeof(wasser_t));

    print("weg_t:\t%d\n", sizeof(weg_t));

    print("planquadrat_t:\t%d\n", sizeof(planquadrat_t));
    print("karte_t:\t%d\n\n", sizeof(karte_t));
    print("spieler_t:\t%d\n\n", sizeof(spieler_t));

}

static void show_times(karte_t *welt, karte_vollansicht_t *view)
{

    long t0 = get_current_time_millis();
    int i;

    for(i=0; i<200; i++) {
	view->display( true );
//        win_display_flush(0, 0, 0.0);
    }

    // long t1 = get_current_time_millis();
    long t1 = clock();

    print("200 display loops in %ld ms\n", (t1-t0)*1000/CLOCKS_PER_SEC);

    intr_set(welt, view, 1);

    long last_step_time = get_current_time_millis();
    long this_step_time = 0;

    for(i=0; i<100; i++) {

	this_step_time = get_current_time_millis();
        welt->step((long)(this_step_time-last_step_time));
	last_step_time = this_step_time;

//        welt->interactive_update();
    }

    intr_disable();

    // long t2 = get_current_time_millis();
    long t2 = clock();

    print("100 displayed world steps in %ld ms\n", (t2-t1)*1000/CLOCKS_PER_SEC);

    for(i=0; i<1000; i++) {
        get_current_time_millis();
    }

    // long t3 = get_current_time_millis();
    long t3 = clock();

    print("1000 time_millis in %ld ms\n", (t3-t2)*1000/CLOCKS_PER_SEC);

    for(i=0; i<200; i++) {
        win_display_flush(0, 0, 0.0);
    }

    // long t4 = get_current_time_millis();
    long t4 = clock();

    print("200 display flushes in %ld ms\n", (t4-t3)*1000/CLOCKS_PER_SEC);
}

static const char *gimme_arg(int argc, char *argv[], const char *arg, int off)
{
//    printf("Searching parameter %s, ", arg);

    for(int i=1; i<argc; i++) {
	if(tstrequ(argv[i], arg) && i<argc-off) {
//	    printf("found %s\n", argv[i+off]);
	    return argv[i+off];
	}
    }
//    printf("<- failed.\n");
    return NULL;
}


static const char *scrolltext[] =
{
#include "scrolltext.h"
};

/**
 * Intro Scroller
 * @author Hj. Malthaner
 */
static void scroll_intro(const int xoff, const int yoff)
{
    static int line = 0;

    const int text_line = line / 9;
    const int text_offset = line % 9;
    const int left = 60;
    const int top = 196;

    display_fillbox_wh(xoff+left, yoff+top, 240, 48, GRAU1, true);
    display_text(1, xoff+left+4, yoff+top-text_offset, scrolltext[text_line], WEISS, true);
    display_text(1, xoff+left+4, yoff+top+9-text_offset, scrolltext[text_line+1], WEISS, true);
    display_text(1, xoff+left+4, yoff+top+18-text_offset, scrolltext[text_line+2], GRAU6, true);
    display_text(1, xoff+left+4, yoff+top+27-text_offset, scrolltext[text_line+3], GRAU5, true);
    display_text(1, xoff+left+4, yoff+top+36-text_offset, scrolltext[text_line+4], GRAU4, true);
    display_text(1, xoff+left+4, yoff+top+45-text_offset, scrolltext[text_line+5], GRAU3, true);
    // display_text(1, xoff+left+4, yoff+top+54-text_offset, scrolltext[text_line+6], GRAU2, true);

    display_fillbox_wh(xoff+left, yoff+top-8, 240, 7, GRAU4, true);
    display_fillbox_wh(xoff+left, yoff+top-1, 240, 1, GRAU3, true);

    display_fillbox_wh(xoff+left, yoff+top+48, 240, 1, GRAU6, true);
    display_fillbox_wh(xoff+left, yoff+top+49, 240, 7, GRAU4, true);


    line ++;

    if(line > (104-7)*9) {
	line = 0;
    }
}


/**
 * Intro Screen
 * @author Hj. Malthaner
 */
static void zeige_banner()
{
    int xoff = (display_get_width()/2)-180;
    const int yoff = (display_get_height()/2)-125;
    struct event_t ev;

    int s;

    // display_show_pointer( false );

    display_ddd_box(xoff, yoff, 360, 270, GRAU6, GRAU2);
    display_fillbox_wh(xoff+1, yoff+1, 358, 268, GRAU5, true);
    display_ddd_box(xoff+4, yoff+4, 352, 262, GRAU2, GRAU6);
    display_fillbox_wh(xoff+5, yoff+5, 350, 260, GRAU4, true);

    display_color_img(skinverwaltung_t::logosymbol->gib_bild_nr(0),
		      xoff+264, yoff+40, 0, false, true);

    const int heading = 3;

    xoff += 30;

    for(s=1; s>=0; s--) {
        int color;

	if(s == 0) {
            color = WEISS;
	} else {
            color = SCHWARZ;
	}

        if(s == 0) color = heading;
        display_proportional(xoff+s+24,yoff+s+16,
			     "This is a beta version of Simutrans:",
			     ALIGN_LEFT,
			     color, true);

        if(s == 0) color = WEISS;
        display_proportional(xoff+s+48,yoff+s+28,
			     "Version " VERSION_NUMBER " " VERSION_DATE,
			     ALIGN_LEFT,
			     color, true);

        if(s == 0) color = heading;
        display_proportional(xoff+s+24,yoff+s+48,
			     "Simutrans was created by:",
			     ALIGN_LEFT,
			     color, true);

        if(s == 0) color = WEISS;
        display_proportional(xoff+s+48,yoff+s+64,
			     "Hansjörg Malthaner",
			     ALIGN_LEFT,
			     color, true);
        display_proportional(xoff+s+48,yoff+s+76,
			     "D 70563 Stuttgart",
			     ALIGN_LEFT,
			     color, true);
        display_proportional(xoff+s+48,yoff+s+88,
			     "Fuggerstr. 1",
			     ALIGN_LEFT,
			     color, true);

        display_proportional(xoff+s+48,yoff+s+102,
			     "All rights reserved.",
			     ALIGN_LEFT,
			     color, true);

        if(s == 0) color = heading;
        display_proportional(xoff+s+24,yoff+s+122,
			     "Please send ideas and questions to:",
			     ALIGN_LEFT,
			     color, true);

        if(s == 0) color = WEISS;
        display_proportional(xoff+s+48,yoff+s+138,
			      "hansjoerg.malthaner@gmx.de",
			     ALIGN_LEFT,
			     color, true);
        // display_proportional( xoff+s+48,yoff+s+142,"hansjoerg.malthaner@danet.de", color, true);

        if(s == 0) color = heading;
        display_proportional(xoff+s+24,yoff+s+158,
			     "or visit the Simutrans pages on the web:",
			     ALIGN_LEFT,
			     color, true);

        if(s == 0) color = WEISS;
        display_proportional(xoff+s+48,yoff+s+172,
			     "http://www.simutrans.de",
			     ALIGN_LEFT,
			     color, true);
        display_proportional(xoff+s+48,yoff+s+184,
			     "http://www.simutrans.cjb.net",
			     ALIGN_LEFT,
			     color, true);
    }

    xoff -= 30;

    do {
	// check if we need to play a new midi file
	check_midi();

	scroll_intro(xoff, yoff+10);
        display_flush(0, 0, 0.0,
		      "    Welcome to Simutrans", "");
        dr_sleep(70000);
        display_poll_event(&ev);

    } while( !(IS_LEFTCLICK(&ev) ||
               (ev.ev_class == EVENT_KEYBOARD && ev.ev_code != 0)
              )
           );

    if(IS_LEFTCLICK(&ev)) {
	do {
	    display_get_event(&ev);
	} while( !IS_LEFTRELEASE(&ev));
    }


    // display_show_pointer( true );
}



/**
 * dies wird in main mittels set_new_handler gesetzt und von der
 * laufzeitumgebung im falle des speichermangels bei new() aufgerufen
 */
#ifdef _MSC_VER
int sim_new_handler(unsigned int)
{
    dbg->fatal("sim_new_handler()",
	       "OUT OF MEMORY");
    return 0;
}
#else
void sim_new_handler()
{
    dbg->fatal("sim_new_handler()",
	       "OUT OF MEMORY");
}
#endif


int simu_cpp_main(int argc, char ** argv)
{
  // Try to catch all exceptions and print them
  try {

    int resolutions[] = {
	640, 480,
	800, 600,
	1024, 768,
	1280, 1024,
	672, 496            // try to force window mode with allegro
    };
    int	i;
    bool quit_simutrans = false;  // 02-Nov-2001 Markus Weber    Added

    FILE * config = NULL;         // die konfigurationsdatei

    int disp_width = 800;
    int disp_height = 600;

    cstring_t loadgame = "";
    cstring_t objfilename = "pak/";


    // Hajo: currently not needed
    // check_locations();


    // init. fehlerbehandlung
#ifdef _MSC_VER
    _set_new_handler( sim_new_handler );
#else
    std::set_new_handler( sim_new_handler );
#endif


    if(gimme_arg(argc, argv, "-log", 0)) {
      	init_logging("simu.log", true, gimme_arg(argc, argv, "-debug", 0));
    } else {
        init_logging("stderr", true, gimme_arg(argc, argv, "-debug", 0));
    }


    if(gimme_arg(argc, argv, "-h", 0) ||
       gimme_arg(argc, argv, "-?", 0) ||
       gimme_arg(argc, argv, "-help", 0) ||
       gimme_arg(argc, argv, "--help", 0)) {

        print("\n");
        print("+----------------------------------------------------+\n");
        print("| This is a beta version of Simutrans:               |\n");
	print("| Version: " VERSION_NUMBER "  Build date: " VERSION_DATE "     |\n");
        print("|                                                    |\n");
        print("|            Simutrans was created by:               |\n");
        print("|               Hansjörg Malthaner                   |\n");
        print("|               D 70563 Stuttgart                    |\n");
        print("|               Fuggerstr. 1                         |\n");
        print("|            All rights reserved.                    |\n");
        print("|                                                    |\n");
	print("| Email: hansjoerg.malthaner@gmx.de                  |\n");
        print("|                                                    |\n");
	print("| Web: http://www.simutrans.de                       |\n");
	print("| Web: http://www.simutrans.cjb.net                  |\n");
        print("|                                                    |\n");
	print("| For further help please read the readme.txt file   |\n");
        print("+----------------------------------------------------+\n\n");


	// quit after printing that message
	return 0;
    }


    // dies gibt nur ein paar infos aus, deshalb kommt die
    // abfrage zuerst
    if(gimme_arg(argc, argv, "-sizes", 0) != NULL) {
	show_sizes();
	exit(0);
    }

    dbg->message("simmain::main()" ,
		 "Version: " VERSION_NUMBER "  Date: " VERSION_DATE);

    // unmgebung init
    umgebung_t::testlauf = (gimme_arg(argc, argv, "-test", 0) != NULL);
    umgebung_t::freeplay = (gimme_arg(argc, argv, "-freeplay", 0) != NULL);
    umgebung_t::verbose_debug = (gimme_arg(argc, argv, "-debug", 0) != NULL);
    umgebung_t::night_shift = true;

    print("Reading low level config data ...\n");
    tabfile_t simuconf;

    if(simuconf.open("config/simuconf.tab")) {
      tabfileobj_t contents;

      simuconf.read(contents);


      print("Initializing tombstones ...\n");
      convoihandle_t::init(contents.get_int("convoys", 8192));
      blockhandle_t::init(contents.get_int("railblocks", 8192));
      halthandle_t::init(contents.get_int("stations", 8192));

      umgebung_t::bodenanimation =
	(contents.get_int("animated_grounds", 1) != 0);

      umgebung_t::fussgaenger =
	(contents.get_int("random_pedestrians", 0) != 0);

      umgebung_t::verkehrsteilnehmer_info =
	(contents.get_int("pedes_and_car_info", 0) != 0);

      umgebung_t::starting_money =
	(contents.get_int("starting_money", 15000000));


      umgebung_t::maint_building =
	(contents.get_int("maintenance_building", 5000));


      umgebung_t::maint_way =
	(contents.get_int("maintenance_ways", 800));


      umgebung_t::maint_overhead =
	(contents.get_int("maintenance_overhead", 200));


      umgebung_t::numbered_stations =
	(contents.get_int("numbered_stations", 0)) != 0;

      umgebung_t::show_month =
	(contents.get_int("show_month", 0)) != 0;


      umgebung_t::intercity_road_length =
	(contents.get_int("intercity_road_length", 8000));


      umgebung_t::intercity_road_type =
	new cstring_t(ltrim(contents.get("intercity_road_type")));


      umgebung_t::use_timeline =
	(contents.get_int("use_timeline", 0)) != 0;


      umgebung_t::show_names =
	(contents.get_int("show_names", 3));


      umgebung_t::starting_year =
	(contents.get_int("starting_year", 1930));




      /*
       * Selection of savegame format through inifile
       * @author Volker Meyer
       * @date  20.06.2003
       */
      const char *str = contents.get("saveformat");
      while(*str == ' ') str++;
      if(!strcmp(str, "binary")) {
	loadsave_t::set_savemode(loadsave_t::binary);
      } else if(!strcmp(str, "zipped")) {
	loadsave_t::set_savemode(loadsave_t::zipped);
      } else if(!strcmp(str, "text")) {
	loadsave_t::set_savemode(loadsave_t::text);
      }


      /*
       * Default resolution
       */
      disp_width  = contents.get_int("display_width",  800);
      disp_height = contents.get_int("display_height", 600);

      /*
       * Default pak file path
       */
      objfilename = ltrim(contents.get_string("pak_file_path", "pak/"));


      /*
       * Max number of steps in goods pathfinding
       */
      haltestelle_t::set_max_hops(contents.get_int("max_hops", 300));


      /*
       * Max number of transfers in goods pathfinding
       */
      haltestelle_t::set_max_transfers(contents.get_int("max_transfers", 6));


      simuconf.close();
    } else {
      fprintf(stderr, "reading low level config failed, using defaults.\n");

      convoihandle_t::init(8192);
      blockhandle_t::init(8192);
      halthandle_t::init(8192);

      umgebung_t::starting_money = 15000000;

      umgebung_t::maint_building = 5000;
      umgebung_t::maint_way = 800;

      haltestelle_t::set_max_hops(300);
    }


    if(gimme_arg(argc, argv, "-objects", 1)) {
      objfilename = gimme_arg(argc, argv, "-objects", 1);
    }


    // Adam - Moved away loading from simmain and placed into translator for
    // better modularisation
    if(translator::load(objfilename) == false)
    {
        dbg->fatal("simmain::main()", "Unable to load any language files");
        exit(11);
    }


    print("Reading city configuration ...\n");
    stadt_t::init();



    print("Reading object data from %s...\n", objfilename.chars());
    if(!obj_reader_t::init(objfilename)) {
	fprintf(stderr, "reading object data failed.\n");
	exit(11);
    }


    if(gimme_arg(argc, argv, "-res", 0) != NULL) {
	const char * res_str = gimme_arg(argc, argv, "-res", 1);
	const int res = *res_str - '1';

	switch(res) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	    disp_width = resolutions[res*2];
	    disp_height = resolutions[res*2+1];
	    break;
	default:
	    print("invalid resolution, argument must be 1,2,3 or 4\n");
	    print("1=640x480, 2=800x600, 3=1024x768, 4=1280x1024\n");
	    return 0;
	}
    }


    if(gimme_arg(argc, argv, "-screensize", 0) != NULL) {
	const char * res_str = gimme_arg(argc, argv, "-screensize", 1);

	int n = 0;

	if(res_str) {
	    n = sscanf(res_str, "%dx%d", &disp_width, &disp_height);
	}


	if(n != 2) {
	    print("invalid argument for -screensize option\n");
	    print("argument must be of format like 800x600\n");
	    return 0;
	}
    }

    if(gimme_arg(argc, argv, "-load", 0) != NULL) {
	char buf[128];
	/**
	 * Added automatic adding of extension
	 * @author Volker Meyer
	 * @date  20.06.2003
	 */
	sprintf(buf, SAVE_PATH_X "%s", searchfolder_t::complete(gimme_arg(argc, argv, "-load", 1), "sve").chars());
	loadgame = buf;
    }

    // jetzt, nachdem die Kommandozeile ausgewertet ist
    // koennen wir das config file lesen, ung ggf.
    // einige einstellungen setzen

    config = fopen("simworld.cfg","rb");

    int AIs[6] = { 0, 0, 0, 0, 1, 0 };

    if(config) {
	int dn = 0;
	int sprache = 0;
	fscanf(config, "Lang=%d\n", &sprache);
        translator::set_language(sprache);

	fscanf(config, "DayNight=%d\n", &dn);

        umgebung_t::night_shift = (dn != 0);

	fscanf(config, "AIs=%d,%d,%d,%d,%d,%d\n",
	    &AIs[0], &AIs[1], &AIs[2],
	    &AIs[3], &AIs[4], &AIs[5]);

	fclose(config);
    }
    for(i = 0; i < 6; i++) {
	umgebung_t::automaten[i] = AIs[i] > 0;
    }



    karte_t *welt = new karte_t();

    const char * use_shm = gimme_arg(argc, argv, "-net", 0);
    const char * do_sync = gimme_arg(argc, argv, "-async", 0);

    print("Preparing display ...\n");
    simgraph_init(disp_width, disp_height, use_shm == NULL, do_sync == NULL);

    karte_vollansicht_t *view = new karte_vollansicht_t(welt);


    // Hajo: simgraph init loads default fonts, now we need to load
    // the real fonts for the current language
    sprachengui_t::init_font_from_lang();



    print("Init timer module ...\n");
    time_init();

    if(!gimme_arg(argc, argv, "-nosound", 0)) {
      print("Reading sound data ...\n");
      sound_init();
    }

    if(!gimme_arg(argc, argv, "-nomidi", 0)) {
      print("Reading midi data ...\n");
      midi_init();
      midi_play(0);
    }

    if(gimme_arg(argc, argv, "-times", 0) != NULL) {
	show_times(welt, view);
	exit(0);
    }

    /*
#if defined(MSDOS) || defined(__MINGW32__)
display_show_pointer( false );
#endif
    */

    // suche nach refresh-einstellungen

    int refresh = 1;
    const char *ref_str = gimme_arg(argc, argv, "-refresh", 1);

    if(ref_str != NULL) {
	int want_refresh = atoi(ref_str);

	refresh = want_refresh < 1 ? 1 : want_refresh > 16 ? 16 : want_refresh;
    }

    if(loadgame != "") {
	welt->laden(loadgame);
    }

    intr_set(welt, view, refresh);

    win_setze_welt( welt );
    view->display( true );

    // Bringe welt in ansehnlichen Zustand
    // bevor sie als Hintergrund für das intro dient

    welt->sync_prepare();
    if(loadgame == "") {
	welt->sync_step(welt->gib_zeit_ms() + welt->ticks_per_tag/2);
	for(i=0; i<20; i++) {
	    welt->step(10000);
	}
    }
    intr_refresh_display( true );

    // Hajo: give user a mouse to work with
    display_show_pointer(TRUE);


    zeige_banner();

    welt->setze_dirty();

    /*
#if defined(MSDOS) || defined(__MINGW32__)
display_show_pointer( true );
#endif
    */

    welt_gui_t *wg = NULL;
    sprachengui_t *sg = NULL;
    event_t ev;

    einstellungen_t *sets = new einstellungen_t( *welt->gib_einstellungen() );
    sets->setze_groesse(256);
    sets->setze_anzahl_staedte(16);
    sets->setze_karte_nummer(simrand(999));


    do {
	check_midi();

	if(!umgebung_t::testlauf && loadgame == "") {
	    wg = new welt_gui_t(welt, sets);
	    sg = new sprachengui_t(welt);

	    view->display( true );

	    create_win(10, 40, -1, sg, w_info);
	    create_win((disp_width-250)/2, (disp_height-300)/2, -1, wg, w_info);


	    setsimrand( get_current_time_millis() );

	    do {
		win_poll_event(&ev);
		check_pos_win(&ev);

		simusleep(1024);
	    } while(! (wg->gib_load() ||
		       wg->gib_load_heightfield() ||
		       wg->gib_start() ||
		       wg->gib_quit())
		    );

	    if(IS_LEFTCLICK(&ev)) {
		do {
		    display_get_event(&ev);
		} while( !IS_LEFTRELEASE(&ev));
	    }

            // Neue Karte erzeugen
	    if(wg->gib_start()) {
		destroy_win( wg );

		nachrichtenfenster_t *nd = new nachrichtenfenster_t(welt, "Erzeuge neue Karte.\n", skinverwaltung_t::neueweltsymbol->gib_bild_nr(0), koord(-16, 16));

		create_win(200, 100, nd, w_autodelete);

		intr_refresh_display( true );

		sets->heightfield = "";
		welt->init(sets);
		destroy_all_win();

	    } else if(wg->gib_load()) {
		destroy_win( wg );
		destroy_win( sg );
		create_win(new loadsave_frame_t(welt, true), w_info, magic_load_t);
	    } else if(wg->gib_load_heightfield()) {
	        einstellungen_t *sets = wg->gib_sets();
		destroy_win( wg );
		destroy_win( sg );

		create_win(new load_relief_frame_t(welt, sets), w_info, magic_load_t);
	    } else if(wg->gib_quit()) {
		// quit the game
		break;
	    }
	}
	loadgame = "";    // only first time

        // 02-Nov-2001     Markus Weber    Function returns a boolean now
	quit_simutrans = welt->interactive();


    } while(!(umgebung_t::testlauf || quit_simutrans));

    intr_disable();

    delete welt;
    welt = NULL;

    simgraph_exit();


    // zu guter letzt schreiben wir noch die akuellen
    // einstellungen in ein config-file


    config = fopen("simworld.cfg","wb");

    if(config) {
	fprintf(config, "Lang=%d\n", translator::get_language());
	fprintf(config, "DayNight=%d\n", umgebung_t::night_shift);
	fprintf(config, "AIs=%d,%d,%d,%d,%d,%d\n",
	    umgebung_t::automaten[0],
	    umgebung_t::automaten[1],
	    umgebung_t::automaten[2],
	    umgebung_t::automaten[3],
	    umgebung_t::automaten[4],
	    umgebung_t::automaten[5]);
	fclose(config);
    }

    close_midi();
  } catch(no_such_element_exception nse) {
    dbg->fatal("simmain.cc",
	       "simu_cpp_main()",
	       nse.message);
  }

  return 0;
}




extern "C" {

int
simu_main(int argc , char** argv)
{
  return simu_cpp_main(argc, argv);
}

} // extern "C"
