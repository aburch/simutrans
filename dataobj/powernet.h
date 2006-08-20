/*
 * powernet_t.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef powernet_t_h
#define powernet_t_h


#include "../ifc/sync_steppable.h"

/**
 * Data class for power networks. A two phase queue to store
 * and hand out power.
 * @author Hj. Malthaner
 */
class powernet_t : public sync_steppable
{
 public:

  /**
   * Must be caled before powernets get loaded. Clears the
   * table of networks
   * @author Hj. Malthaner
   */
  static void prepare_loading();


  /**
   * Loads a powernet object or hand back already loaded object
   * @author Hj. Malthaner
   */
  static powernet_t * load_net(powernet_t *key);



 private:

  int last_capacity;

  int power_last;
  int power_this;

 public:


  int get_capacity() const {return last_capacity;};


  /**
   * Adds some power to the net
   * @author Hj. Malthaner
   */
  void add_power(int amount);


  /**
   * Tries toget a certain amount of power from the net.
   * @return granted amount of power
   * @author Hj. Malthaner
   */
  int withdraw_power(int want);


  /**
   * Vorbereitungsmethode für Echtzeitfunktionen eines Objekts.
   * @author Hj. Malthaner
   */
  virtual void sync_prepare();


  /**
   * Methode für Echtzeitfunktionen eines Objekts.
   * @return false wenn Objekt aus der Liste der synchronen
   * Objekte entfernt werden sol
   * @author Hj. Malthaner
   */
  virtual bool sync_step(long delta_t);

  powernet_t();

  virtual ~powernet_t();
};

#endif // powernet_t_h
