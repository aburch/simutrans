#ifndef sync_steppable_h
#define sync_steppable_h


/**
 * Alle synchron bewegten Dinge müssen dieses Interface implementieren.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.4 $
 */
class sync_steppable
{
public:

    /**
     * Vorbereitungsmethode für Echtzeitfunktionen eines Objekts.
     * @author Hj. Malthaner
     */
    virtual void sync_prepare() = 0;

    /**
     * Methode für Echtzeitfunktionen eines Objekts.
     * @return false wenn Objekt aus der Liste der synchronen
     * Objekte entfernt werden sol
     * @author Hj. Malthaner
     */
    virtual bool sync_step(long delta_t) = 0;

    virtual ~sync_steppable() {}
};

#endif
