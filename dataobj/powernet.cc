/*
 * powernet_t.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "../simdebug.h"

#include "../tpl/ptrhashtable_tpl.h"
#include "powernet.h"

static ptrhashtable_tpl <powernet_t *, powernet_t *> loading_table;


/**
 * Must be caled before powernets get loaded. Clears the
 * table of networks
 * @author Hj. Malthaner
 */
void powernet_t::prepare_loading()
{
  loading_table.clear();
}


/**
 * Loads a powernet object or hand back already loaded object
 * @author Hj. Malthaner
 */
powernet_t * powernet_t::load_net(powernet_t *key)
{
  powernet_t * result = loading_table.get(key);

  if(result == 0) {
    result = new powernet_t ();
    loading_table.put(key, result);
  }

  return result;
}


/**
 * Adds some power to the net
 * @author Hj. Malthaner
 */
void powernet_t::add_power(int amount)
{
  power_this += amount;
}


/**
 * Tries toget a certain amount of power from the net.
 * @return granted amount of power
 * @author Hj. Malthaner
 */
int powernet_t::withdraw_power(int want)
{
  const int result = power_last > want ? want : power_last;
  power_last -= result;

  return result;
}


powernet_t::powernet_t()
{
  current_capacity = 0;
  for( int i=0;  i<8;  i++ ) {
  	capacity[i] = 0;
  }
  power_this = 0;
  power_last = 0;
}


powernet_t::~powernet_t()
{

}


/**
 * Vorbereitungsmethode für Echtzeitfunktionen eines Objekts.
 * @author Hj. Malthaner
 */
void powernet_t::sync_prepare()
{
  // DBG_MESSAGE("powernet_t::sync_prepare()", "this=%d, last=%d", power_this, power_last);

  last_capacity = power_last;

  power_last = power_this;
  power_this = 0;
}



/* calculates the last amout of power draw (a little smoothed)
 * @author prissi
 */
int powernet_t::get_capacity() const
{
	int medium_capacity=0;
	for( int i=0;  i<8;  i++  ) {
		medium_capacity += capacity[i];
	}
	return medium_capacity>>3;
}




/**
 * Methode für Echtzeitfunktionen eines Objekts.
 * @return false wenn Objekt aus der Liste der synchronen
 * Objekte entfernt werden sol
 * @author Hj. Malthaner
 */
bool powernet_t::sync_step(long /* delta_t */)
{
  current_capacity &= 7;
  capacity[current_capacity++] = power_last;
  return true;
}
