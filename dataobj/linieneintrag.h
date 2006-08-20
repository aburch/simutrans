#ifndef linieneintrag_t_h
#define linieneintrag_t_h

#include "koord3d.h"

/**
 * Ein Fahrplaneintrag.
 * @author Hj. Malthaner
 */
class linieneintrag_t {

 public:

  /**
   * Halteposition
   * @author Hj. Malthaner
   */
  koord3d pos;

  /**
   * Geforderter Beladungsgrad an diesem Halt
   * @author Hj. Malthaner
   */
  int ladegrad;
};




#endif // linieneintrag_t_h
