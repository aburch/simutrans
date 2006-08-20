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
#include "../simtypes.h"

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
  long next_t;
  uint32 capacity[8];
  uint8 current_capacity;

  uint32 last_capacity;
  uint32 max_capacity;

  uint32 power_last;
  uint32 power_this;

 public:

  uint32 get_capacity() const;
  uint32 set_max_capacity(uint32 max) { uint32 m=max_capacity;  if(max>0){max_capacity=max;} return m; }

  /**
   * Adds some power to the net
   * @author Hj. Malthaner
   */
  void add_power(uint32 amount);


  /**
   * Tries toget a certain amount of power from the net.
   * @return granted amount of power
   * @author Hj. Malthaner
   */
  uint32 withdraw_power(uint32 want);


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
