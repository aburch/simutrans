/*
 * procedures for automated tests
 *
 * written by Hj. Maltahner, September 2000
 */

#include <stdio.h>
#include <string.h>

#include "../simworld.h"
#include "../boden/grund.h"
#include "../boden/wege/strasse.h"
#include "../boden/wege/schiene.h"
#include "../blockmanager.h"
#include "../simplay.h"
#include "../simhalt.h"
#include "../bauer/brueckenbauer.h"
#include "../simwerkz.h"
#include "../simwin.h"
#include "../simintr.h"

#include "../dings/zeiger.h"
#include "../bauer/wegbauer.h"
#include "../bauer/hausbauer.h"

#include "testtool.h"

static int nr_failed = 0;

static bool check(const char *msg, const bool cond)
{
    char buf[256];

    strcpy(buf, msg);
    strcat(buf, " %s\n");


    if(!cond) {
	fprintf(stderr, buf, "FAILED");
	nr_failed ++;
    } else {
	fprintf(stderr, buf, "ok");
    }

    return cond;
}

static bool check(const bool cond)
{
    if(!cond) {
	nr_failed ++;
    }

    return cond;
}



static void start_testing()
{
    nr_failed = 0;
}

static void summarize_results()
{
    fprintf(stderr, "--------------------------------------------------------------------\n");
    if(nr_failed == 0) {
	fprintf(stderr, "All test succeeded, no errors detected\n");
    } else {
	fprintf(stderr, "%d tests FAILED\n", nr_failed);
    }
    fprintf(stderr, "--------------------------------------------------------------------\n");
}

static void show(const karte_t *)
{
    intr_refresh_display( true );
}


static void railtest(spieler_t *sp, karte_t *welt, koord pos)
{
    printf("Schienenbautest\n");

    if(welt->ist_in_kartengrenzen(pos)) {
        bool ok;
	wegbauer_t bauigel(welt, sp);
        blockhandle_t bs;

	bauigel.route_fuer(wegbauer_t::schiene, wegbauer_t::gib_besch("wooden_sleeper_track"));
	bauigel.calc_route(pos, pos+koord(0, 5));


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Length': calc a route, check length\n");

	check("Length must be five squares:", bauigel.max_n == 5);


        const int block_count = blockmanager::gib_manager()->gib_strecken()->count();
	bauigel.baue();


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Build': Building rails, checking for build rails and same rail block\n");

	grund_t *gr = welt->lookup(pos)->gib_kartenboden();

	check("Ground must be rails:", gr->gib_weg(track_wt));

	bs = (dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke();

	check("Block must be valid:", bs.is_bound());
	check("Block must be managed:", blockmanager::gib_manager()->gib_strecken()->contains(bs));
	check("Block must be empty:", bs->ist_frei());
	check("Block must not contain signals:", bs->gib_signale().empty());

	for(int i=1; i<6; i++) {
	    gr = welt->lookup(pos+koord(0,i))->gib_kartenboden();

	    check("Ground must be rails:", gr->gib_weg(track_wt));
	    check("Ground must have right owner:", gr->gib_besitzer() == sp);
	    check("Block must be same:", bs == 	(dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke());
	}

	show(welt);

	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Number 1': Checking number of blocks\n");

	ok = check("Number must be one more than at start:", block_count+1 == blockmanager::gib_manager()->gib_strecken()->count());
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", block_count, blockmanager::gib_manager()->gib_strecken()->count());
	}


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Remove': Removing rails, checking for bare grounds\n");

	for(int i=0; i<6; i++) {

	    wkz_remover(sp, welt, pos+koord(0,i));

	    gr = welt->lookup(pos+koord(0,i))->gib_kartenboden();
	    check("Ground must not be rails:", !gr->gib_weg(track_wt));
	    check("Ground must not have owner:", gr->gib_besitzer() == NULL);
	}

	show(welt);

	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Number 2': Checking number of blocks\n");

	ok = check("Number must be same as before build:", block_count == blockmanager::gib_manager()->gib_strecken()->count());
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", block_count, blockmanager::gib_manager()->gib_strecken()->count());
	}


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Split': Building rails, splitting apart\n");

	bauigel.calc_route(pos, pos+koord(0, 5));
	bauigel.baue();

	wkz_remover(sp, welt, pos+koord(0,2));
	gr = welt->lookup(pos+koord(0,2))->gib_kartenboden();
	check("Ground (split square) must not be rails:", !gr->gib_weg(track_wt));


	gr = welt->lookup(pos)->gib_kartenboden();
	check("Ground (track 1) must be rails:", gr->gib_weg(track_wt));
	bs = (dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke();
	check("Block must be valid:", bs.is_bound());
	check("Block must be managed:", blockmanager::gib_manager()->gib_strecken()->contains(bs));
	check("Block must be empty:", bs->ist_frei());
	check("Block must not contain signals:", bs->gib_signale().empty());

	for(int i=0; i<2; i++) {
	    gr = welt->lookup(pos+koord(0,i))->gib_kartenboden();

	    check("Ground (track 1) must be rails:", gr->gib_weg(track_wt));
	    check("Block (track 1) must be same:", bs == (dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke());
	}


	gr = welt->lookup(pos+koord(0,3))->gib_kartenboden();
	check("Ground (track 2) must be rails:", gr->gib_weg(track_wt));
	bs = (dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke();
	check("Block must be valid:", bs.is_bound());
	check("Block must be managed:", blockmanager::gib_manager()->gib_strecken()->contains(bs));
	check("Block must be empty:", bs->ist_frei());
	check("Block must not contain signals:", bs->gib_signale().empty());

	for(int i=3; i<6; i++) {
	    gr = welt->lookup(pos+koord(0,i))->gib_kartenboden();

	    check("Ground (track 2) must be rails:", gr->gib_weg(track_wt));
	    check("Block (track 2) must be same:", bs == (dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke());
	}

	show(welt);


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'different blocks': Checking blocks\n");

	gr = welt->lookup(pos)->gib_kartenboden();

	check("Block numbers must differ:", bs != (dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke());



	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Number 3': Checking number of blocks\n");

	ok = check("Number must be two more than at start:", block_count+2 == blockmanager::gib_manager()->gib_strecken()->count());
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", block_count, blockmanager::gib_manager()->gib_strecken()->count());
	}


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Connect': Joining split blocks\n");

	bauigel.calc_route(pos+koord(0, 1), pos+koord(0, 3));
	bauigel.baue();

	gr = welt->lookup(pos)->gib_kartenboden();
	check("Ground must be rails:", gr->gib_weg(track_wt));
	bs = (dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke();
	check("Block must be valid:", bs.is_bound());
	check("Block must be managed:", blockmanager::gib_manager()->gib_strecken()->contains(bs));
	check("Block must be empty:", bs->ist_frei());
	check("Block must not contain signals:", bs->gib_signale().empty());

	for(int i=0; i<2; i++) {
	    gr = welt->lookup(pos+koord(0,i))->gib_kartenboden();

	    check("Ground must be rails:", gr->gib_weg(track_wt));
	    check("Block must be same:", bs == (dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke());

	}


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Number 4': Checking number of blocks\n");

	ok = check("Number must be one more than at start:", block_count+1 == blockmanager::gib_manager()->gib_strecken()->count());
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", block_count, blockmanager::gib_manager()->gib_strecken()->count());
	}


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'cleanup': cleaning all touched squares\n");

	for(int i=0; i<6; i++) {

	    wkz_remover(sp, welt, pos+koord(0,i));;


	    check("Square must be emty:", welt->lookup(pos+koord(0,i))->gib_kartenboden()->gib_top() == 0);

	    gr = welt->lookup(pos+koord(0,i))->gib_kartenboden();

	    check("Ground must not be rails:", !gr->gib_weg(track_wt));
	    check("Ground must be nature:", gr->ist_natur());
	    check("Ground must not have owner:", gr->gib_besitzer() == NULL);
	}

	ok = check("Block number must be same more like at start:", block_count == blockmanager::gib_manager()->gib_strecken()->count());
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", block_count, blockmanager::gib_manager()->gib_strecken()->count());
	}



	show(welt);

    }
}

static void bautest(spieler_t *sp, karte_t *welt, koord pos)
{
    if(welt->ist_in_kartengrenzen(pos)) {

        bool ok;
	wegbauer_t bauigel(welt, sp);
        blockhandle_t bs;

	bauigel.route_fuer(wegbauer_t::schiene, wegbauer_t::gib_besch("wooden_sleeper_track"));
	bauigel.calc_route(pos, pos+koord(0, 5));


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Length': calc a route, check length\n");

	check("Length must be five squares:", bauigel.max_n == 5);


        const int block_count = blockmanager::gib_manager()->gib_strecken()->count();
	bauigel.baue();


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Build': Building rails, checking for build rails and same rail block\n");

	grund_t *gr = welt->lookup(pos)->gib_kartenboden();

	check("Ground must be rails:", gr->gib_weg(track_wt));

	bs = (dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke();
	check("Block must be valid:", bs.is_bound());
	check("Block must be managed:", blockmanager::gib_manager()->gib_strecken()->contains(bs));
	check("Block must be empty:", bs->ist_frei());
	check("Block must not contain signals:", bs->gib_signale().empty());

	for(int i=1; i<6; i++) {
	    gr = welt->lookup(pos+koord(0,i))->gib_kartenboden();

	    check("Ground must be rails:", gr->gib_weg(track_wt));
	    check("Ground must have right owner:", gr->gib_besitzer() == sp);
	    check("Block must be same:", bs == (dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke());

	}

	show(welt);

	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Number 1': Checking number of blocks\n");

	ok = check("Number must be one more than at start:", block_count+1 == blockmanager::gib_manager()->gib_strecken()->count());
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", block_count, blockmanager::gib_manager()->gib_strecken()->count());
	}


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Depot 1': Building rail depot\n");

	wkz_bahndepot(sp, welt, pos);

	ok = check("Checking if object exists:" ,
		   welt->access(pos)->gib_kartenboden()->obj_bei(PRI_DEPOT) != NULL);
	ok = check("Checking if object is rail depot:" ,
	           welt->access(pos)->gib_kartenboden()->obj_bei(PRI_DEPOT)->gib_typ() == ding_t::bahndepot);



	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Depot 2': Removing rail depot\n");

	wkz_remover(sp, welt, pos);

	ok = check("Checking if object doesn't exist:" ,
		   welt->access(pos)->gib_kartenboden()->obj_bei(PRI_DEPOT) == NULL);

	ok = check("Checking if ground is rails:" ,
		   welt->access(pos)->gib_kartenboden()->gib_weg(track_wt));



	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Signal 1': building signals\n");

	(dynamic_cast<zeiger_t*>(welt->gib_zeiger()))->setze_richtung(ribi_t::nord);
	wkz_signale(sp, welt, pos+koord(0, 2));

	ok = check("Checking for one object only at (+0, +2):" ,
		   welt->lookup(pos+koord(0, 2))->gib_kartenboden()->obj_count() == 1);

	ok = check("Checking for one object only at (+0, +1):" ,
		   welt->lookup(pos+koord(0, 1))->gib_kartenboden()->obj_count() == 1);

	ok = check("Checking for rails at (+0, +2):" ,
		   welt->lookup(pos+koord(0, 2))->gib_kartenboden()->gib_weg(track_wt));

	ok = check("Checking for rails at (+0, +1):" ,
		   welt->lookup(pos+koord(0, 1))->gib_kartenboden()->gib_weg(track_wt));


	ok = check("Checking for different rail blocks:" ,
	           (dynamic_cast<schiene_t*> (welt->lookup(pos+koord(0, 1))->gib_kartenboden()->gib_weg(track_wt)))->gib_blockstrecke() !=
		   (dynamic_cast<schiene_t*> (welt->lookup(pos+koord(0, 2))->gib_kartenboden()->gib_weg(track_wt)))->gib_blockstrecke());



	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Number 2': Checking number of blocks\n");

	ok = check("Number must be two more than at start:", block_count+2 == blockmanager::gib_manager()->gib_strecken()->count());
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", block_count, blockmanager::gib_manager()->gib_strecken()->count());
	}

	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Overlay': building track again\n");

	bauigel.route_fuer(wegbauer_t::schiene, wegbauer_t::gib_besch("wooden_sleeper_track"));;
	bauigel.calc_route(pos, pos+koord(0, 5));
	bauigel.baue();

	ok = check("Checking for one object at (+0, +2):" ,
		   welt->lookup(pos+koord(0, 2))->gib_kartenboden()->obj_count() == 1);

	ok = check("Checking for one object at (+0, +1):" ,
		   welt->lookup(pos+koord(0, 1))->gib_kartenboden()->obj_count() == 1);

	ok = check("Checking for rails at (+0, +2):" ,
		   welt->lookup(pos+koord(0, 2))->gib_kartenboden()->gib_weg(track_wt));

	ok = check("Checking for rails at (+0, +1):" ,
		   welt->lookup(pos+koord(0, 1))->gib_kartenboden()->gib_weg(track_wt));

	ok = check("Checking for different rail blocks:" ,
	           (dynamic_cast<schiene_t*> (welt->lookup(pos+koord(0, 1))->gib_kartenboden()->gib_weg(track_wt)))->gib_blockstrecke() !=
		   (dynamic_cast<schiene_t*> (welt->lookup(pos+koord(0, 2))->gib_kartenboden()->gib_weg(track_wt)))->gib_blockstrecke());

	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Remove signal 1':\n");

	wkz_remover(sp, welt, pos+koord(0, 2));

	ok = check("Checking zero objects at (+0, +2):" ,
		   welt->lookup(pos+koord(0, 2))->gib_kartenboden()->obj_count() == 0);

	ok = check("Checking for zero objects at (+0, +1):" ,
		   welt->lookup(pos+koord(0, 1))->gib_kartenboden()->obj_count() == 0);

	ok = check("Checking for rails at (+0, +2):" ,
		   welt->lookup(pos+koord(0, 2))->gib_kartenboden()->gib_weg(track_wt));

	ok = check("Checking for rails at (+0, +1):" ,
		   welt->lookup(pos+koord(0, 1))->gib_kartenboden()->gib_weg(track_wt));

	ok = check("Checking for same rail block:" ,
		   ((schiene_t *)welt->lookup(pos+koord(0, 2))->gib_kartenboden())->gib_blockstrecke() ==
		   ((schiene_t *)welt->lookup(pos+koord(0, 1))->gib_kartenboden())->gib_blockstrecke());


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'cleanup': cleaning all touched squares\n");

	for(int i=0; i<6; i++) {

	    wkz_remover(sp, welt, pos+koord(0,i));;


            printf("%d Strecken\n", blockmanager::gib_manager()->gib_strecken()->count());

	    check("Square must be emty:", welt->lookup(pos+koord(0,i))->gib_kartenboden()->gib_top() == 0);

	    gr = welt->lookup(pos+koord(0,i))->gib_kartenboden();

	    check("Ground must not be rails:", !gr->gib_weg(track_wt));
	    check("Ground must be nature:", gr->ist_natur());
	    check("Ground must not have owner:", gr->gib_besitzer() == NULL);
	}

	ok = check("Block number must be same as at start:", block_count == blockmanager::gib_manager()->gib_strecken()->count());
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", block_count, blockmanager::gib_manager()->gib_strecken()->count());
	}

    }
}


/*
 * Testet den Bau von Bahnhöfen
 * @author Hj. Malthaner
 */
static void stationstest(spieler_t *sp, karte_t *welt, koord pos)
{
    if(welt->ist_in_kartengrenzen(pos)) {

        bool ok;
	wegbauer_t bauigel(welt, sp);
        blockhandle_t bs;

	bauigel.route_fuer(wegbauer_t::schiene, wegbauer_t::gib_besch("wooden_sleeper_track"));
	bauigel.calc_route(pos, pos+koord(0, 5));


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Length': calc a route, check length\n");

	check("Length must be five squares:", bauigel.max_n == 5);


        const int block_count = blockmanager::gib_manager()->gib_strecken()->count();
	const int station_count = haltestelle_t::gib_anzahl();

	bauigel.baue();


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Build': Building rails, checking for build rails and same rail block\n");

	grund_t *gr = welt->lookup(pos)->gib_kartenboden();

	check("Ground must be rails:", gr->gib_weg(track_wt));

	bs = (dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke();
	check("Block must be valid:", bs.is_bound());
	check("Block must be managed:", blockmanager::gib_manager()->gib_strecken()->contains(bs));
	check("Block must be empty:", bs->ist_frei());
	check("Block must not contain signals:", bs->gib_signale().empty());

	for(int i=1; i<6; i++) {
	    gr = welt->lookup(pos+koord(0,i))->gib_kartenboden();

	    check("Ground must be rails:", gr->gib_weg(track_wt));
	    check("Ground must have right owner:", gr->gib_besitzer() == sp);
	    check("Block must be same:", bs == (dynamic_cast<schiene_t*> (gr->gib_weg(track_wt)))->gib_blockstrecke());

	}

	show(welt);

	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Number 1': Checking number of blocks\n");

	ok = check("Number must be one more than at start:", block_count+1 == blockmanager::gib_manager()->gib_strecken()->count());
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", block_count, blockmanager::gib_manager()->gib_strecken()->count());
	}


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Station 1': Building one station segment\n");

	wkz_bahnhof(sp, welt, pos, hausbauer_t::bahnhof_besch);

	ok = check("Checking if station object exists:" ,
		   welt->access(pos)->gib_kartenboden()->obj_count() == 1);
	if( !ok ) {
	    fprintf(stderr, "  Expected 1 object but found %d\n", welt->access(pos)->gib_kartenboden()->obj_count());
	}


	ok = check("Station count must be one more than at start:" ,
		   haltestelle_t::gib_anzahl() == station_count + 1);
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", station_count, haltestelle_t::gib_anzahl());
	}


	check("Checking if ground has station:" ,
	      welt->access(pos)->gib_kartenboden()->gib_halt().is_bound());

	check("Checking if station lookup works:" ,
              haltestelle_t::gib_halt(welt, pos).is_bound());


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Station 0': Removing segment\n");

	wkz_remover(sp, welt, pos);

	ok = check("Checking if station object no longer exists:" ,
		   welt->access(pos)->gib_kartenboden()->obj_count() == 0);
	if( !ok ) {
	    fprintf(stderr, "  Expected 0 objects but found %d\n", welt->access(pos)->gib_kartenboden()->obj_count());
	}

	ok = check("Station count must be same as at start:" ,
		   haltestelle_t::gib_anzahl() == station_count);
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", station_count, haltestelle_t::gib_anzahl());
	}


	check("Checking if ground has no station:" ,
	      !welt->access(pos)->gib_kartenboden()->gib_halt().is_bound());

	check("Checking if station lookup yields unbound handle:" ,
              !haltestelle_t::gib_halt(welt, pos).is_bound());



	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Station 2': Building two station segments\n");

	wkz_bahnhof(sp, welt, pos, hausbauer_t::bahnhof_besch);
	wkz_bahnhof(sp, welt, pos+koord(0,1), hausbauer_t::bahnhof_besch);

	ok = check("Checking if first station object exists:" ,
		   welt->access(pos)->gib_kartenboden()->obj_count() == 1);
	if( !ok ) {
	    fprintf(stderr, "  Expected 1 object but found %d\n", welt->access(pos)->gib_kartenboden()->obj_count());
	}

	ok = check("Checking if second station object exists:" ,
		   welt->access(pos+koord(0,1))->gib_kartenboden()->obj_count() == 1);
	if( !ok ) {
	    fprintf(stderr, "  Expected 1 object but found %d\n", welt->access(pos)->gib_kartenboden()->obj_count());
	}


	ok = check("Station count must be one more than at start:" ,
		   haltestelle_t::gib_anzahl() == station_count + 1);
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", station_count, haltestelle_t::gib_anzahl());
	}


	check("Checking if ground 1 has station:" ,
	      welt->access(pos)->gib_kartenboden()->gib_halt().is_bound());

	check("Checking if station lookup works:" ,
              haltestelle_t::gib_halt(welt, pos).is_bound());

	check("Checking if ground 2 has station:" ,
	      welt->access(pos+koord(0,1))->gib_kartenboden()->gib_halt().is_bound());

	check("Checking if station lookup works:" ,
              haltestelle_t::gib_halt(welt, pos+koord(0,1)).is_bound());


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Station 3': Removing first segment\n");

	wkz_remover(sp, welt, pos);

	ok = check("Checking if station object no longer exists:" ,
		   welt->access(pos)->gib_kartenboden()->obj_count() == 0);
	if( !ok ) {
	    fprintf(stderr, "  Expected 0 objects but found %d\n", welt->access(pos)->gib_kartenboden()->obj_count());
	}

	ok = check("Station count must be one more than at start:" ,
		   haltestelle_t::gib_anzahl() == station_count+1);
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", station_count, haltestelle_t::gib_anzahl());
	}


	check("Checking if ground has no station:" ,
	      !welt->access(pos)->gib_kartenboden()->gib_halt().is_bound());

	check("Checking if station lookup yields unbound handle:" ,
              !haltestelle_t::gib_halt(welt, pos).is_bound());


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Station 4': Removing second segment\n");

	wkz_remover(sp, welt, pos+koord(0,1));

	ok = check("Checking if station object no longer exists:" ,
		   welt->access(pos+koord(0,1))->gib_kartenboden()->obj_count() == 0);
	if( !ok ) {
	    fprintf(stderr, "  Expected 0 objects but found %d\n", welt->access(pos+koord(0,1))->gib_kartenboden()->obj_count());
	}

	ok = check("Station count must be same as at start:" ,
		   haltestelle_t::gib_anzahl() == station_count);
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", station_count, haltestelle_t::gib_anzahl());
	}


	check("Checking if ground has no station:" ,
	      !welt->access(pos+koord(0,1))->gib_kartenboden()->gib_halt().is_bound());

	check("Checking if station lookup yields unbound handle:" ,
              !haltestelle_t::gib_halt(welt, pos+koord(0,1)).is_bound());


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'station cleanup': cleaning all touched squares\n");

	for(int i=0; i<6; i++) {

	    wkz_remover(sp, welt, pos+koord(0,i));;


            printf("%d Strecken\n", blockmanager::gib_manager()->gib_strecken()->count());

	    check("Square must be emty:", welt->lookup(pos+koord(0,i))->gib_kartenboden()->gib_top() == 0);

	    gr = welt->lookup(pos+koord(0,i))->gib_kartenboden();

	    check("Ground must not be rails:", !gr->gib_weg(track_wt));
	    check("Ground must be nature:", gr->ist_natur());
	    check("Ground must not have owner:", gr->gib_besitzer() == NULL);
	}

	ok = check("Block number must be same as at start:", block_count == blockmanager::gib_manager()->gib_strecken()->count());
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", block_count, blockmanager::gib_manager()->gib_strecken()->count());
	}
    }
}


static void blockschleifentest(spieler_t *sp, karte_t *welt, koord pos)
{
    if(welt->ist_in_kartengrenzen(pos)) {

        const int block_count = blockmanager::gib_manager()->gib_strecken()->count();
        bool ok;

	wegbauer_t bauigel(welt, sp);
//        blockhandle_t bs = NULL;

	bauigel.route_fuer(wegbauer_t::schiene, wegbauer_t::gib_besch("wooden_sleeper_track"));


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Schleife':\n");

	bauigel.calc_route(pos, pos+koord(0, 3));
	bauigel.baue();

	bauigel.calc_route(pos, pos+koord(3, 0));
	bauigel.baue();

	bauigel.calc_route(pos+koord(3, 0), pos+koord(3,6));
	bauigel.baue();

	bauigel.calc_route(pos+koord(3,6), pos+koord(6,6));
	bauigel.baue();

	bauigel.calc_route(pos+koord(6,6), pos+koord(6,3));
	bauigel.baue();

	welt->raise(pos+koord(2,3));
	welt->raise(pos+koord(2,4));

	welt->raise(pos+koord(5,3));
	welt->raise(pos+koord(5,4));


	bauigel.calc_route(pos+koord(6,3), pos+koord(4,3));
	bauigel.baue();

	bauigel.calc_route(pos+koord(0,3), pos+koord(2,3));
	bauigel.baue();

	// wkz_schienenbruecke(sp, welt, pos+koord(2,3));
        brueckenbauer_t::baue(sp, welt, pos+koord(2,3), track_wt);

	ok = check("Block count must be one more as before:", block_count+1 == blockmanager::gib_manager()->gib_strecken()->count());
	if( !ok ) {
	    fprintf(stderr, "  Number before was %d, now it is %d\n", block_count, blockmanager::gib_manager()->gib_strecken()->count());
	}

	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test 'Signal 1':\n");

	(dynamic_cast<zeiger_t*>(welt->gib_zeiger()))->setze_richtung(ribi_t::west);
	wkz_signale(sp, welt, pos+koord(1,0));

	ok = check("Left of Signal must be rails:", welt->lookup(pos)->gib_kartenboden()->gib_weg(track_wt));
	ok = check("Right of Signal must be rails:", welt->lookup(pos+koord(1,0))->gib_kartenboden()->gib_weg(track_wt));

	schiene_t *s1_left  = (schiene_t *)welt->lookup(pos)->gib_kartenboden();
	schiene_t *s1_right = (schiene_t *)welt->lookup(pos+koord(1,0))->gib_kartenboden();

	ok = check("Blocks left and right must be same:", s1_left->gib_blockstrecke() == s1_right->gib_blockstrecke());

    }
}

static void plantest_clear_square(planquadrat_t *plan, spieler_t *sp)
{
    bool ok;

    fprintf(stderr, "--------------------------------------------------------------------\n");
    fprintf(stderr, "Test: Leeres Planquadrat\n");

    plan->gib_kartenboden()->obj_loesche_alle(sp);

    ok = check("top muss 0 sein:", plan->gib_kartenboden()->gib_top() == 0);
    if( !ok ) {
	fprintf(stderr, "  top ist %d\n", plan->gib_kartenboden()->gib_top());
    }

    for(int i=-1000; i<1000; i++) {
	ok = check(plan->gib_kartenboden()->obj_bei(i) == NULL);
	if( !ok ) {
	    fprintf(stderr, "  obj_bei(%d) ist %p statt NULL\n", i, plan->gib_kartenboden()->obj_bei(i));
	}
    }
}



static void plantest(spieler_t *sp, karte_t *welt, koord pos)
{
    if(welt->ist_in_kartengrenzen(pos)) {
	planquadrat_t *plan = welt->access( pos );
	ding_t * dt = NULL;
	ding_t * dt2 = NULL;

	bool ok;

	plantest_clear_square(plan, sp);

	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test: Planquadrat mit 1 Object\n");

	dt = new zeiger_t(welt, plan->gib_kartenboden()->gib_pos(), sp);

	plan->gib_kartenboden()->obj_add(dt);
	ok = check("top muss 1 sein:", plan->gib_kartenboden()->gib_top() == 1);
	if( !ok ) {
	    fprintf(stderr, "  topist %d\n", plan->gib_kartenboden()->gib_top());
	}

	for(int i=-1000; i<1000; i++) {
	    if(i != 0) {
		ok = check(plan->gib_kartenboden()->obj_bei(i) == NULL);
		if( !ok ) {
		    fprintf(stderr, "  obj_bei(%d) ist %p statt NULL\n", i, plan->gib_kartenboden()->obj_bei(i));
		}
	    } else {
		ok = check("ding_t * muss == dt sein:", plan->gib_kartenboden()->obj_bei(i) == dt);
		if( !ok ) {
		    fprintf(stderr, "  ding_t * ist %p, sollte %p sein\n", plan->gib_kartenboden()->obj_bei(i), dt);
		}
	    }
	}

	plantest_clear_square(plan, sp);


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test: Planquadrat mit 1 Object pri 7\n");

	dt = new zeiger_t(welt, plan->gib_kartenboden()->gib_pos(), sp);

	plan->gib_kartenboden()->obj_pri_add(dt, 7);
	ok = check("top muss 8 sein:", plan->gib_kartenboden()->gib_top() == 8);
	if( !ok ) {
	    fprintf(stderr, "  top ist %d\n", plan->gib_kartenboden()->gib_top());
	}

	for(int i=-1000; i<1000; i++) {
	    if(i != 7) {
		ok = check(plan->gib_kartenboden()->obj_bei(i) == NULL);
		if( !ok ) {
		    fprintf(stderr, "  obj_bei(%d) ist %p statt NULL\n", i, plan->gib_kartenboden()->obj_bei(i));
		}
	    } else {
		ok = check("ding_t * muss == dt sein:", plan->gib_kartenboden()->obj_bei(i) == dt);
		if( !ok ) {
		    fprintf(stderr, "  ding_t * ist %p, sollte %p sein\n", plan->gib_kartenboden()->obj_bei(i), dt);
		}
	    }
	}


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test: Planquadrat mit 2 Object pri 7\n");

	dt2 = new zeiger_t(welt, plan->gib_kartenboden()->gib_pos(), sp);

	plan->gib_kartenboden()->obj_pri_add(dt2, 7);
	ok = check("top muss 9 sein:", plan->gib_kartenboden()->gib_top() == 9);
	if( !ok ) {
	    fprintf(stderr, "  top ist %d\n", plan->gib_kartenboden()->gib_top());
	}

	for(int i=-1000; i<1000; i++) {
	    if(i != 7 && i!= 8) {
		ok = check(plan->gib_kartenboden()->obj_bei(i) == NULL);
		if( !ok ) {
		    fprintf(stderr, "  obj_bei(%d) ist %p statt NULL\n", i, plan->gib_kartenboden()->obj_bei(i));
		}
	    } else {
		ok = check("ding_t * muss == dt sein:", plan->gib_kartenboden()->obj_bei(i) == ((i == 7) ? dt : dt2));
		if( !ok ) {
		    fprintf(stderr, "  ding_t * ist %p, sollte %p sein\n", plan->gib_kartenboden()->obj_bei(i), ((i == 7) ? dt : dt2));
		}
	    }
	}


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test: Remove ein Object\n");

	plan->gib_kartenboden()->obj_remove(dt2);
	ok = check("top muss 8 sein:", plan->gib_kartenboden()->gib_top() == 8);
	if( !ok ) {
	    fprintf(stderr, "  top ist %d\n", plan->gib_kartenboden()->gib_top());
	}

	for(int i=-1000; i<1000; i++) {
	    if(i != 7) {
		ok = check(plan->gib_kartenboden()->obj_bei(i) == NULL);
		if( !ok ) {
		    fprintf(stderr, "  obj_bei(%d) ist %p statt NULL\n", i, plan->gib_kartenboden()->obj_bei(i));
		}
	    } else {
		ok = check("ding_t * muss == dt sein:", plan->gib_kartenboden()->obj_bei(i) == dt);
		if( !ok ) {
		    fprintf(stderr, "  ding_t * ist %p, sollte %p sein\n", plan->gib_kartenboden()->obj_bei(i), dt);
		}
	    }
	}


	fprintf(stderr, "--------------------------------------------------------------------\n");
	fprintf(stderr, "Test: Planquadrat mit 2 Object pri 7, 14\n");

	plan->gib_kartenboden()->obj_pri_add(dt2, 14);
	ok = check("top muss 15 sein:", plan->gib_kartenboden()->gib_top() == 15);
	if( !ok ) {
	    fprintf(stderr, "  top ist %d\n", plan->gib_kartenboden()->gib_top());
	}

	for(int i=-1000; i<1000; i++) {
	    if(i != 7 && i!= 14) {
		ok = check(plan->gib_kartenboden()->obj_bei(i) == NULL);
		if( !ok ) {
		    fprintf(stderr, "  obj_bei(%d) ist %p statt NULL\n", i, plan->gib_kartenboden()->obj_bei(i));
		}
	    } else {
		ok = check("ding_t * muss == dt sein:", plan->gib_kartenboden()->obj_bei(i) == ((i == 7) ? dt : dt2));
		if( !ok ) {
		    fprintf(stderr, "  ding_t * ist %p, sollte %p sein\n", plan->gib_kartenboden()->obj_bei(i), ((i == 7) ? dt : dt2));
		}
	    }
	}


	plantest_clear_square(plan, sp);
    }
}


int tst_railtest(spieler_t *sp, karte_t *welt, koord pos)
{
    if(welt->ist_in_kartengrenzen(pos)) {

	start_testing();


	// Prueft die Objektverwlatung eines Planquadrates
	plantest(sp, welt, pos);


	railtest(sp, welt, pos);
	bautest(sp, welt, pos);

	stationstest(sp, welt, pos);


        blockschleifentest(sp, welt, pos);

	summarize_results();
    }

    return true;
}
