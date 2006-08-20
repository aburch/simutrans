#ifndef _simmover_h
#define _simmover_h

/**
 * simverkehr.h
 *
 * Bewegliche Objekte fuer Simutrans.
 * Transportfahrzeuge sind in simvehikel.h definiert, da sie sich
 * stark von den hier definierten Fahrzeugen fuer den Individualverkehr
 * unterscheiden.
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#include "simvehikel.h"

#include "tpl/stringhashtable_tpl.h"
#include "ifc/sync_steppable.h"

class stadtauto_besch_t;

template<class T> class slist_mit_gewichten_tpl;

/**
 * Base class for traffic participants with random movement
 * @author Hj. Malthaner
 */
class verkehrsteilnehmer_t : public vehikel_basis_t, public sync_steppable
{
    /**
     * Hoechstgeschwindigkeit
     * @author Hj. Malthaner
     */
    int max_speed;


    /**
     * Aktuelle Geschwindigkeit
     * @author Hj. Malthaner
     */
    int current_speed;


    /**
     * Entfernungszaehler
     * @author Hj. Malthaner
     */
    uint32 weg_next;


    /**
     * aktuelle Bewegungsrichtung
     * @author Hj. Malthaner
     */
    int dx, dy;


    ribi_t::ribi fahrtrichtung;


    koord3d naechstes_feld;


    int hoff;


protected:

    virtual ribi_t::ribi gib_fahrtrichtung() const {return fahrtrichtung;};


    virtual int  gib_dx() const {return dx;};
    virtual int  gib_dy() const {return dy;};
    virtual int  gib_hoff() const {return hoff;};
    virtual weg_t::typ gib_wegtyp() const { return weg_t::strasse; };

    virtual bool hop_check() {return true;};
    virtual void hop();
    virtual void age() { };
   virtual int gib_age() { return 1; };

    virtual void calc_bild();

    void setze_max_speed(int s) {max_speed = s;};
    int gib_max_speed() const {return max_speed;};

    void calc_current_speed();

    verkehrsteilnehmer_t(karte_t *welt);
    verkehrsteilnehmer_t(karte_t *welt, koord3d pos);

public:

    const char *gib_name() const = 0;
    enum ding_t::typ gib_typ() const  = 0;


    void sync_prepare() {};
    bool sync_step(long delta_t);


    /**
     * Öffnet ein neues Beobachtungsfenster für das Objekt.
     * @author Hj. Malthaner
     */
    virtual void zeige_info();


    /**
     * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird.
     * @author Hj. Malthaner
     * @see simwin
     */
    virtual char *info(char *buf) const;

    void rdwr(loadsave_t *file);
};


class stadtauto_t : public verkehrsteilnehmer_t {
    static slist_mit_gewichten_tpl<const stadtauto_besch_t *> stadtauto_t::liste_timeline;
    static slist_mit_gewichten_tpl<const stadtauto_besch_t *> liste;
    static stringhashtable_tpl<const stadtauto_besch_t *> table;

    const stadtauto_besch_t *besch;

	// prissi: time to life in blocks
    int time_to_life;

protected:
    void rdwr(loadsave_t *file);
public:
    stadtauto_t(karte_t *welt, loadsave_t *file);
    stadtauto_t(karte_t *welt, koord3d pos);

    /**
     * Ensures that this object is removed correctly from the list
     * of sync steppable things!
     * @author Hj. Malthaner
     */
    virtual ~stadtauto_t();

   virtual void age() { time_to_life--; };
   virtual int gib_age() { return time_to_life; };

    virtual void calc_bild();


    /* this function builts the list of the allowed citycars
     * it should be called every month and in the beginning of a new game
     * @author prissi
     */
    static void built_timeline_liste();
    static int gib_anzahl_besch();

    static bool register_besch(const stadtauto_besch_t *besch);
    static bool laden_erfolgreich();

    const char *gib_name() const {return "Verkehrsteilnehmer";};
    enum ding_t::typ gib_typ() const {return verkehr;};
};

#endif
