/* worldtest.h
 *
 * written by Hj. Maltahner, September 2000
 */

#ifndef test_worldtest_h
#define test_worldtest_h

#include "../simhalt.h"
#include "../railblocks.h"

class karte_t;
class planquadrat_t;
class grund_t;
class ding_t;
class log_t;

/**
 * class for checking the consistency of the world data structures
 * tries to check all pointer and data values
 *
 * written by Hj. Malthaner, September 2000
 */
class worldtest_t
{
private:
    log_t *log;
    karte_t *welt;

    void check_halt_consistency(halthandle_t halt, int i, int j);
    void check_ground_consistency(grund_t *gr, int i, int j);


    void check_ding_consistency(ding_t *dt, int n, int i, int j);

    void check_plan_consistency(planquadrat_t *plan, int i, int j);

public:

    /**
     * this method checks all objects of the world as far as
     * thei can be reached from planquadrat_t and
     * their base classes allow checking
     */
    void check_consistency();


    worldtest_t(karte_t *welt, char *log_name);
};


#endif
