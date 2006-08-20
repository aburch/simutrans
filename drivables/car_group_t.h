/*
 * car_group_t.h
 *
 * Copyright (c) 2003 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */



#ifndef car_group_t_h
#define car_group_t_h


#include "../ifc/sync_steppable.h"
#include "../ifc/route_block_tester_t.h"

class grund_t;
class karte_t;
class car_t;
class fahrplan_t;

/**
 * This class guides a group of cars (i.e. a train) to their
 * destination.
 *
 * It implements frame synchroneous stepping
 *
 * @author Hj. Malthaner
 */
class car_group_t : public sync_steppable, public route_block_tester_t
{
 private:

  enum constants {MAX_CARS = 32};
  enum states {INITIAL, DRIVING, ENTER_STATION, LOADING};

  enum states state;


  /**
   * General purpose timer, i.e. for waiting tasks
   * @author Hj. Malthaner
   */
  unsigned long block_timer;


  /**
   * The cars of this group. 32 seems to be large enough.
   * @author Hj. Malthaner
   */
  car_t * cars[MAX_CARS];


  /**
   * current number of cars
   * @author Hj. Malthaner
   */
  int num_cars;



  /**
   * The schedule of this car group. Must not be NULL
   * while driving. Will not be created or delted by
   * the car_group_t class (except for testing purposes)
   *
   * @author Hj. Malthaner
   */
  fahrplan_t *fpl;


  /**
   * The map to use
   * @author Hj. Malthaner
   */
  karte_t * welt;


  unsigned char besitzer_n;


  /**
   * Calc next position for leading car of this group
   * This is the actual pathfinding!
   * @return next position or invalid koordinate if no next position could
   * be found currently
   * @author Hj. Malthaner
   */
  koord3d calc_next_pos();


  /**
   * Guess direction to drive from current position
   * This is the actual pathfinding!
   * @return next position or invalid koordinate if no next position could
   * be found currently
   * @author Hj. Malthaner
   */
  koord3d guess_direction(grund_t **nb, int num_nb);



  /**
   * Check if dir is blocked. Returns 0 if blocked, direction
   * otheriwse
   * @author Hj. Malthaner
   */
  ribi_t::ribi is_dir_blocked(ribi_t::ribi new_dir) const;


  /**
   * Checks if the square is blocked
   * @author Hj. Malthaner
   */
  bool is_blocked(koord3d pos, koord3d dest) const;


  /**
   * Checks if there is a red signal in our path. If yes, wait a while.
   * If waiting to long, turn, and modify destination (next) square
   *
   * @author Hj. Malthaner
   */
  koord3d wait_blocking(koord3d dest);


  /**
   * Swap two cars
   */
  void car_group_t::swap(int n1, int n2);


  /**
   * Turn car group
   * @author Hj. Malthaner
   */
  void turn();


  /**
   * Drive one step
   * @author Hj. Malthaner
   */
  void go();


  /**
   * Advance schedule
   * @author Hj. Malthaner
   */
  void advance_schedule();


  // Methods to implement the states
  void do_driving();


  /**
   * Vehicles are supposed to enter a station fully
   * @author Hansjoerg Malthaner
   */
  void do_enter_station();


  /**
   * Start car group. There must be at least one car.
   * @author Hj. Malthaner
   */
  void start();


 public:

  car_group_t(karte_t * welt, koord3d pos);

  virtual ~car_group_t();


  /**
   * Implements sync_steppable.
   * Vorbereitungsmethode für Echtzeitfunktionen eines Objekts.
   * @author Hj. Malthaner
   */
  void sync_prepare();


  /**
   * Implements sync_steppable.
   * Methode für Echtzeitfunktionen eines Objekts.
   * @author Hj. Malthaner
   */
  bool sync_step(long delta_t);



  /**
   * Debug info
   */
  void dump() const;



  /**
   * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
   * Beobachtungsfenster angezeigt wird.
   * @author Hj. Malthaner
   * @see simwin
   */
  char * info(char *buf) const;

};

#endif // car_group_t_h
