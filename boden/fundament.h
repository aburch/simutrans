#ifndef boden_fundament_h
#define boden_fundament_h

//#include "grund.h"
#include "boden.h"
#include "../simimg.h"


/**
 * Das Fundament dient als Untergrund fuer alle Bauwerke in Simutrans.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.9 $
 */
class fundament_t : public boden_t
{
public:

    fundament_t(karte_t *welt, loadsave_t *file);
    fundament_t(karte_t *welt, koord3d pos);

    /**
     * Das Fundament hat immer das gleiche Bild.
     * @author Hj. Malthaner
     */
    void calc_bild();

    /**
     * Das Fundament heisst 'Fundament'.
     * @return gibt 'Fundament' zurueck.
     * @author Hj. Malthaner
     */
    inline const char *gib_name() const {return "Fundament";};


    /**
     * Auffforderung, ein Infofenster zu öffnen.
     * @author Hj. Malthaner
     */
    virtual void zeige_info();


    inline enum typ gib_typ() const {return fundament;};


    void * operator new(size_t s);
    void operator delete(void *p);
};

#endif
