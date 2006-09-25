/* worldtest.cc
 *
 * written by Hj. Maltahner, September 2000
 */

#include "../simworld.h"
#include "../simdings.h"
#include "../simplay.h"
#include "../simhalt.h"
#include "../utils/log.h"
#include "../boden/grund.h"
#include "../boden/wege/weg.h"
#include "../boden/wege/schiene.h"

#include "worldtest.h"

void
worldtest_t::check_halt_consistency(halthandle_t halt, int i, int j)
{
    if(!halt.is_bound()) {
        log->error("worldtest_t::check_halt_consistency",
		   "station %d is bound to a station object which is no longer existent");

	return;
    }


    // position checks

    if(haltestelle_t::gib_halt(welt, koord(i,j)) != halt) {
	log->warning("worldtest_t::check_halt_consistency",
	          "station has invalid position info: haltestelle_t::gib_halt(welt, new koord(%d,%d)) != halt)", i,j);
    }

    // owner check

    spieler_t *sp = halt->gib_besitzer();

    if(sp != NULL) {
	int n = welt->sp2num( sp );

	if(n == -1) {
	    log->warning("worldtest_t::check_halt_consistency", "station has invalid owner %p", sp);
	}
    }

    // TODO
    // warenziele

    // TODO
    // fab_list
}

void
worldtest_t::check_ground_consistency(grund_t *gr, int i, int j)
{
    if(gr->gib_name() == NULL) {
	log->warning("worldtest_t::check_ground_consistency", "ground without name");
    }

    spieler_t *sp = gr->gib_besitzer();

    if(sp != NULL) {
	int n = welt->sp2num( sp );

	if(n == -1) {
	    log->warning("worldtest_t::check_ground_consistency", "ground '%s' has invalid owner %p", gr->gib_name(), sp);
	}
    }

    koord3d pos = gr->gib_pos();

    if(pos.x != i || pos.y != j) {
	log->warning("worldtest_t::check_ground_consistency", "ground '%s' has invalid koordinate %d %d (should be %d %d)", gr->gib_name(), pos.x, pos.y, i, j);
    }


    grund_t *other = welt->lookup(pos);

    if(other != gr) {
	log->warning("worldtest_t::check_ground_consistency", "ground '%s' could not be found at %d %d %d", gr->gib_name(), pos.x, pos.y, pos.z);
    }



    int ribi = gr->gib_weg_ribi(road_wt);

    if(ribi < 0 || ribi > 15) {
	log->warning("worldtest_t::check_ground_consistency", "ground '%s' has invalid ribi %d for street", gr->gib_name(), ribi);
    }


    ribi = gr->gib_weg_ribi(track_wt);

    if(ribi < 0 || ribi > 15) {
	log->warning("worldtest_t::check_ground_consistency", "ground '%s' has invalid ribi %d for street", gr->gib_name(), ribi);
    }


    halthandle_t halt = gr->gib_halt();

    if(halt.is_bound()) {
        check_halt_consistency(halt, i, j);
    }

    schiene_t *sch = dynamic_cast<schiene_t *>(gr);

    if(sch) {
      blockhandle_t bs = sch->gib_blockstrecke();

      if(!bs.is_bound()) {
	log->warning("worldtest_t::check_ground_consistency", "rail block handle %d is not bound to a rail block object", bs.get_id());
      }
    }
}


void
worldtest_t::check_ding_consistency(ding_t *dt, int n, int i, int j)
{
    if(dt->gib_name() == NULL) {
	log->warning("worldtest_t::check_ground_consistency", "thing %d without name", n);
    }


    spieler_t *sp = dt->gib_besitzer();

    if(sp != NULL) {
	int p = welt->sp2num( sp );

	if(p == -1) {
	    log->warning("worldtest_t::check_ding_consistency", "thing '%s' (%d) has invalid owner %p", dt->gib_name(), n, sp);
	}
    }

    // TODO
    // fabrik

    koord3d pos = dt->gib_pos();

    if(pos.x != i || pos.y != j) {
	log->warning("worldtest_t::check_ding_consistency", "thing '%s' (%d) has invalid koordinate %d %d", dt->gib_name(), n, pos.x, pos.y);
    }


}


void
worldtest_t::check_plan_consistency(planquadrat_t *plan, int i, int j)
{
    log->message("worldtest_t::check_plan_consistency", "checking %d, %d", i, j);

    for(unsigned int n=0; n<plan->gib_boden_count(); n++) {

	// checking ground

	grund_t * gr = plan->gib_boden_bei(n);

	if(gr == NULL) {
	    log->warning("worldtest_t::check_plan_consistency", "ground is NULL");
	} else {
	    check_ground_consistency(gr, i, j);
	}

	// checking objects

	for(int o=0; o<gr->gib_top(); o++) {
	    ding_t *dt = gr->obj_bei(o);

	    if(dt != NULL) {
		check_ding_consistency(dt, o, i, j);
	    }
	}
    }
}


void
worldtest_t::check_consistency()
{
    const int groesse = welt->gib_groesse();


    for(int j=0; j<groesse; j++) {
	for(int i=0; i<groesse; i++) {

	    planquadrat_t *plan = welt->access(i,j);

	    // check if plan is available

	    if(plan == NULL) {
		log->warning("worldtest_t::check_consistency", "plan %d, %d is NULL", i, j);
	    } else {
		check_plan_consistency(plan, i, j);
	    }
	}
    }
}



worldtest_t::worldtest_t(karte_t *welt, char *log_name)
{
    this->welt = welt;

    log = new log_t(log_name, true, true);
}
