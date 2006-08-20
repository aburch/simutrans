/*
 * car_group_t.cpp
 *
 * Copyright (c) 2003 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "car_group_t.h"
#include "car_t.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../blockmanager.h"
#include "../dataobj/koord3d.h"
#include "../dataobj/fahrplan.h"
#include "../bauer/warenbauer.h"
#include "../bauer/vehikelbauer.h"
#include "../simdebug.h"

// Shortest path finding
#include "../ifc/fahrer.h"
#include "../dataobj/route.h"


// Testing only
#include "../gui/fahrplan_gui.h"


/**
 * How long to wait in front of signals?
 * @author Hj. Malthaner
 */
static const unsigned int MAX_WAIT = 200;


/**
 * Very primimitive helper class to calculate routes
 * @see route_t
 * @author Hj. Malthaner
 */
class pathfinder_t : public fahrer_t
{
private:

  /**
   * This defines the type of drivable ground
   * @author Hj. Malthaner
   */
  weg_t::typ wegtyp;


public:

  pathfinder_t(weg_t::typ wt) {
    wegtyp = wt;
  }


  virtual bool ist_befahrbar(const grund_t * gr) const {
    return gr->gib_weg(wegtyp) != 0;
  }


  /**
   * Ermittelt die für das Fahrzeug geltenden Richtungsbits,
   * abhängig vom Untergrund.
   *
   * @author Hj. Malthaner, 03.01.01
   */
  virtual ribi_t::ribi gib_ribi(const grund_t * gr) const {
    return gr->gib_weg_ribi(wegtyp);
  }


  virtual weg_t::typ gib_wegtyp() const {
    return wegtyp;
  }
};




car_group_t::car_group_t(karte_t * w, koord3d pos)
{
  welt = w;
  num_cars = 0;

dbg->message( "car_group_t::car_group_t()","Alien-Alarm!");
  for(int i = 0; i<MAX_CARS; i++) {
    cars[i] = 0;
  }

  besitzer_n = 0;
  block_timer = 0;

  // Testing stuff

  fpl = new zugfahrplan_t();

  fahrplan_gui_t *fg = new fahrplan_gui_t(welt, fpl, welt->gib_spieler(0));
  fg->zeige_info();


  num_cars = 4;

  for(int i=0; i<num_cars; i++) {
    cars[i] = new car_t(welt, pos, this);
    cars[i]->setze_pos(pos);
    cars[i]->set_prev_pos(koord3d::invalid);
    cars[i]->set_next_pos(koord3d::invalid);


    const vehikel_besch_t *vb = 0;

    if(i == 0) {
//  vb = vehikelbauer_t::vehikel_search( vehikel_besch_t::schiene, 0xFFFFFFFF, 2060, 200, NULL );
      vb = vehikelbauer_t::vehikel_fuer_leistung(200,
             vehikel_besch_t::schiene,0xFFFFFFFF);  // no timeline
    } else {
      vb = vehikelbauer_t::gib_info(warenbauer_t::passagiere,
            vehikel_besch_t::schiene,
            0);
    }

    cars[i]->set_kind(vb);
  }



  state = INITIAL;
}


car_group_t::~car_group_t()
{

}


/**
 * Start car group. There must be at least one car.
 * @author Hj. Malthaner
 */
void car_group_t::start()
{
  koord3d pos = cars[0]->gib_pos();
  koord3d next;

  route_t route;
  pathfinder_t pathfinder (cars[0]->gib_wegtyp());

  const bool ok = route.calc_route(welt,
           pos,
           fpl->eintrag.at(fpl->aktuell).pos,
           &pathfinder);

  if(ok) {
    next = route.position_bei(1);

    dbg->message("car_group_t::start()", "route found");
  } else {
    // Any available direction;

    const grund_t * gr = welt->lookup(pos);

    for(int i=0; i<4; i++) {
      const koord dir = koord::nsow[i];
      grund_t *to = 0;

      gr->get_neighbour(to, cars[0]->gib_wegtyp(), dir);

      if(to) {
  next = to->gib_pos();
  break;
      }
    }

    dbg->message("car_group_t::start()", "no route found, using first possible direction");
  }


  dbg->message("car_group_t::start()", "pos=%d,%d,%d  next=%d,%d,%d", pos.x, pos.y, pos.z, next.x, next.y, next.z);

  for(int i=0; i<num_cars; i++) {
    cars[i]->setze_pos(pos);
    cars[i]->set_next_pos(next);
    cars[i]->set_prev_pos(koord3d::invalid);

    cars[i]->calc_fahrtrichtung();

    cars[i]->add_to_map();

    // Odd cars must first move backwards half a square
    if((i & 1) == 1) {
      cars[i]->turn();
    }
  }

  for(int i=0; i<num_cars; i++) {
    for(int j=0; j<4 ; j++) {
      cars[i]->step();
    }
  }

  // Now turn odd cars in proper direction
  for(int i=1; i<num_cars; i+=2) {
    cars[i]->turn();
  }

  state = DRIVING;
}


/**
 * Drive one step
 * @author Hj. Malthaner
 */
void car_group_t::go()
{
  if(block_timer > 0) {
    block_timer --;
  }

  if(block_timer == 0) {
    for(int i=0; i<num_cars; i++) {
      cars[i]->step();

      if(i>0 && cars[i]->is_about_to_hop()) {
  cars[i]->set_next_next_pos(cars[i-1]->get_next_pos());
      }
    }
  }
}



/**
 * Advance schedule
 * @author Hj. Malthaner
 */
void car_group_t::advance_schedule()
{
  fpl->aktuell ++;

  if(fpl->aktuell > fpl->maxi) {
    fpl->aktuell = 0;
  }
}


void car_group_t::do_driving()
{
  // Hajo: see if the first car needs to enter a new square
  // decide how to move on

  if(cars[0]->is_about_to_hop()) {

    // Did we reach the destination?
    // Two cases: waypoint or station

    halthandle_t halt =
      haltestelle_t::gib_halt(welt, fpl->eintrag.at(fpl->aktuell).pos);


    const bool station_reached =
      (halt.is_bound() && halt == haltestelle_t::gib_halt(welt, cars[0]->gib_pos()));

    const bool waypoint_reached =
      cars[0]->gib_pos() == fpl->eintrag.at(fpl->aktuell).pos;


    if( station_reached || waypoint_reached ) {

      if(station_reached) {
  state = ENTER_STATION;
      } else {
  advance_schedule();
      }

      // Don't step anymore
      return;

    } else {
      // drive further
      koord3d dest = calc_next_pos();

      dbg->message("car_group_t::sync_step()", "Hop detected, guess next field %d,%d,%d", dest.x, dest.y, dest.z);

      cars[0]->set_next_next_pos(dest);

      if(dest == koord3d::invalid) {
  // Don't step now
  return;
      }
    }
  }

  go();
}


/**
 * Vehicles are supposed to enter a station fully
 * @author Hansjoerg Malthaner
 */
void car_group_t::do_enter_station()
{
  // go until end of station, then advance schedule

  if(cars[0]->is_about_to_hop()) {

    halthandle_t halt =
      haltestelle_t::gib_halt(welt, fpl->eintrag.at(fpl->aktuell).pos);

    const koord3d pos = cars[0]->gib_pos();
    const koord3d next_pos = cars[0]->get_next_pos();
    const koord3d next_next_pos = next_pos + (next_pos - pos);

    if(halt == haltestelle_t::gib_halt(welt, pos)
       && !is_blocked(pos, next_pos)
     ) {

      cars[0]->set_next_next_pos(next_next_pos);
    }
    else {

      cars[0]->set_next_next_pos(next_next_pos);

      // End of station reached.

      advance_schedule();

      /*
      // Train is now centered ... 4 more steps needed
      for(int i=0; i<15; i++) {
  go();

  assert(!cars[0]->is_about_to_hop());
      }
      */

      // TODO: Loading
      block_timer = 200;
      state = LOADING;

      koord3d tmp = cars[0]->gib_pos();
      dbg->message("car_group_t::do_enter_station()", "Changing to LOADING at %d,%d,%d", tmp.x, tmp.y, tmp.z);
      return;
    }
  }

  go();
}


/**
 * Implements sync_steppable.
 * Vorbereitungsmethode für Echtzeitfunktionen eines Objekts.
 * @author Hj. Malthaner
 */
void car_group_t::sync_prepare()
{
  // nothing to do
}


/**
 * Implements sync_steppable.
 * Methode für Echtzeitfunktionen eines Objekts.
 * @author Hj. Malthaner
 */
bool car_group_t::sync_step(long delta_t)
{
  // guide cars

  if(fpl->ist_abgeschlossen()) {


    switch(state) {
    case INITIAL:
      start();
      break;

    case DRIVING:
      do_driving();
      break;

    case ENTER_STATION:
      do_enter_station();
      break;

    case LOADING:
      if(block_timer > 0) {
  block_timer --;
      } else {
  state = DRIVING;
      }
      break;
   }
  }

  // Hajo: we still live
  return true;
}


/**
 * Checks if there is a red signal in our path. If yes, wait a while.
 * If waiting to long, turn, and modify destination (next) square
 *
 * @author Hj. Malthaner
 */
koord3d car_group_t::wait_blocking(koord3d dest)
{
  // is the direction blocked? Then we must turn
  if(is_blocked(cars[0]->gib_pos(), cars[0]->get_next_pos())) {
    // first we wait a while, then we turn

    // bugs ... ???
    if(block_timer > MAX_WAIT) {
      block_timer = MAX_WAIT;
    }

    //do we wait already ?
    if(block_timer > 1) {
      block_timer --;
    } else if (block_timer == 0) {
      block_timer = MAX_WAIT;
    } else {
      // waited long enough, turn

      turn();
      dest = cars[0]->get_next_pos();

      block_timer = 0;
    }

  } else {
    block_timer = 0;
  }

  return dest;
}


/**
 * Calc next position for leading car of this group
 * This is the actual pathfinding!
 * @return next position or invalid koordinate if no next position could
 * be found currently
 * @author Hj. Malthaner
 */
koord3d car_group_t::calc_next_pos()
{
  assert(cars[0]);

  // Hajo: we are called immediately _before_ the car changes its position
  // thus we need to use what is now the next pos.
  const koord3d pos = cars[0]->get_next_pos();


  // Hajo: this variable holds the calculated next square
  koord3d next;


  // Hajo: calculate neighbours
  const grund_t * gr = welt->lookup(pos);

  grund_t * nb[4];
  int num_nb = 0;


  // Hajo: fill list with available neighbours. The list is packed.
  // Also count number of neighbours.
  for(int i = 0; i < 4; i++) {
    const koord dir = koord::nsow[i];
    nb[num_nb] = 0;

    gr->get_neighbour(nb[num_nb], cars[0]->gib_wegtyp(), dir);

    if(nb[num_nb]) {
      num_nb++;
    }
  }

  if(num_nb == 0) {
    // Hajo: Ooops, this is an isolated square -> nowhere to move

    dbg->warning("car_group_t::calc_next_pos()", "isolated square detected.");

    next = koord3d::invalid;

  } else if(num_nb == 1) {
    // TODO: real dead end, or one-way signal ?

    turn();

    // Hajo: We turned. Don't move now, calculate next step in next call
    next = koord3d::invalid;

    dbg->message("car_group_t::direction()", "Dead end, turning.");


  } else if(num_nb == 2) {
    const koord3d cur_pos = cars[0]->gib_pos();

    // Hajo: preferably don't turn

    const int i = (nb[0]->gib_pos() == cur_pos) ? 1 : 0;
    next = nb[i]->gib_pos();

    dbg->message("car_group_t::direction()",
     "Straight track, go for %d,%d,%d", next.x, next.y, next.z);

  } else {
    // We reached a crossing

    next = guess_direction(nb, num_nb);

    dbg->message("car_group_t::direction()",
     "Crossing, go for %d,%d,%d", next.x, next.y, next.z);
  }

  next = wait_blocking(next);

  return next;
}


/**
 * Guess direction to drive from current position
 * This is the actual pathfinding!
 * @return next position or invalid koordinate if no next position could
 * be found currently
 * @author Hj. Malthaner
 */
koord3d car_group_t::guess_direction(grund_t **nb, int num_nb)
{
  // Hajo: we are called immediately _before_ the car changes its position
  // thus we need to use what is now the next pos.
  const koord3d pos =  cars[0]->get_next_pos();

  const koord3d cur_pos = cars[0]->gib_pos();


  koord3d next = koord3d::invalid;

  // Direction of next destination (preferred direction)
  const koord3d pref_dir = fpl->eintrag.at(fpl->aktuell).pos - pos;


  // Hajo: check shortest path if there is a long distance to go
  // near stations it is better to always use stepfinder, otherwise
  // choosing alternatives doesn't work well

  route_t route;
  pathfinder_t pathfinder (cars[0]->gib_wegtyp());

  const bool ok =
    route.calc_unblocked_route(welt,
             pos,
             fpl->eintrag.at(fpl->aktuell).pos,
             &pathfinder,
             this);

  if(ok) {
    // Hajo: unblocked routes are listed backwards!
    const koord3d dest = route.position_bei(route.gib_max_n()-1);

    dbg->message("car_group_t::guess_direction()", "got unblocked (shortest) path 1");

    if(!is_blocked(pos, dest)) {
      // Hajo: Got a unblocked shortes path -> use it if we don't need to turn!

      dbg->message("car_group_t::guess_direction()", "got unblocked (shortest) path 2");

      if(dest != cur_pos) {
  return dest;
      }
    }
  }


  dbg->message("car_group_t::guess_direction()", "try to guess");

  // Hajo: no shortes path, or path blocked -> we need a guess!

  for(int i=0; i<num_nb; i++) {
    if(nb[i]->gib_pos() != cur_pos) {
      next = nb[i]->gib_pos();
      break;
    }
  }


  // Hajo: no way out of here? -> turn!
  if(next == koord3d::invalid) {
    turn();
  }

  return next;
}


/**
 * Check if dir is blocked. Returns 0 if blocked, direction
 * otheriwse
 * @author Hj. Malthaner
 */
ribi_t::ribi car_group_t::is_dir_blocked(ribi_t::ribi new_dir) const
{
  const koord3d pos = cars[0]->get_next_pos();
  const koord3d dest = pos + koord(new_dir);

  if(is_blocked(pos, dest)) {
    // we can't go there ...
    new_dir = 0;
  }

  return new_dir;
}


/**
 * Checks if the square is blocked
 * @author Hj. Malthaner
 */
bool car_group_t::is_blocked(koord3d pos, koord3d dest) const
{
  const grund_t *gr = welt->lookup(dest);
  bool ok;

  weg_t * weg = gr->gib_weg(cars[0]->gib_wegtyp());

  // Existiert ein weg
  ok = (weg != 0);

  // if this is a block change, we must check if the new block is free

  if(ok && car_t::ist_blockwechsel(pos, dest)) {
    ok &= weg->ist_frei();
  }


  // we check for blocked
  return !ok;
}


/**
 * Swap two cars
 */
void car_group_t::swap(int n1, int n2)
{
  welt->lookup(cars[n1]->gib_pos())->obj_remove(cars[n1], welt->gib_spieler(besitzer_n));
  welt->lookup(cars[n2]->gib_pos())->obj_remove(cars[n2], welt->gib_spieler(besitzer_n));

  cars[n1]->swap_state(cars[n2]);

  welt->lookup(cars[n1]->gib_pos())->obj_add(cars[n1]);
  welt->lookup(cars[n2]->gib_pos())->obj_add(cars[n2]);
}


/**
 * Turn car group
 * @author Hj. Malthaner
 */
void car_group_t::turn()
{

  for(int i=0; i<num_cars/2; i++) {
    swap(i, num_cars-i-1);
  }

  for(int i=0; i<num_cars; i++) {
    cars[i]->turn();
  }

}


/**
 * Debug info
 */
void car_group_t::dump() const
{
  for(int i=0; i<num_cars; i++) {
    cars[i]->dump();
  }
}



/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 * @see simwin
 */
char * car_group_t::info(char *buf) const
{
  buf += sprintf(buf,
     "State=%d\nBlock timer=%ld\n",
     state, block_timer);

  return buf;
}
