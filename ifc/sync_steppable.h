#ifndef sync_steppable_h
#define sync_steppable_h

#include "../simtypes.h"

/**
 * All synchronously moving things must implement this interface.
 *
 * @author Hj. Malthaner
 */
class sync_steppable
{
public:
    /**
     * Method for real-time features of an object.
     * @return false when object is part of synchronous list
     * Objekte entfernt werden sol
     * @author Hj. Malthaner
     */
    virtual bool sync_step(uint32 delta_t) = 0;

    virtual ~sync_steppable() {}
};

#endif
