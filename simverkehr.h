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
protected:
    /**
     * Hoechstgeschwindigkeit
     * @author Hj. Malthaner
     */
    sint32 max_speed;


    /**
     * Aktuelle Geschwindigkeit
     * @author Hj. Malthaner
     */
    sint32 current_speed;


    /**
     * Entfernungszaehler
     * @author Hj. Malthaner
     */
    uint32 weg_next;


    /**
     * aktuelle Bewegungsrichtung
     * @author Hj. Malthaner
     */
    sint8 dx, dy;


    ribi_t::ribi fahrtrichtung;


    koord3d pos_next;


    sint16 hoff;


    virtual bool ist_weg_frei() {return 1; };	// always free

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
private:
    static slist_mit_gewichten_tpl<const stadtauto_besch_t *> liste_timeline;
    static slist_mit_gewichten_tpl<const stadtauto_besch_t *> liste;
    static stringhashtable_tpl<const stadtauto_besch_t *> table;

    const stadtauto_besch_t *besch;

	// prissi: time to life in blocks
#ifdef DESTINATION_CITYCARS
	koord target;
#endif
    int time_to_life;
    sint32 ms_traffic_jam;

    virtual bool hop_check();
    virtual bool ist_weg_frei();

protected:
    void rdwr(loadsave_t *file);

	void calc_bild();

public:
    stadtauto_t(karte_t *welt, loadsave_t *file);
    stadtauto_t(karte_t *welt, koord3d pos, koord target);

    /**
     * Ensures that this object is removed correctly from the list
     * of sync steppable things!
     * @author Hj. Malthaner
     */
    virtual ~stadtauto_t();

    virtual void hop();
   virtual void age() { time_to_life--; };
   virtual int gib_age() { return time_to_life; };
   void destroy() {time_to_life=0; };

    /* this function builts the list of the allowed citycars
     * it should be called every month and in the beginning of a new game
     * @author prissi
     */
    static void built_timeline_liste();
    static int gib_anzahl_besch();

    static bool register_besch(const stadtauto_besch_t *besch);
    static bool laden_erfolgreich();

   virtual void betrete_feld();

    /**
     * Methode für asynchrone Funktionen eines Objekts.
     * @return false to be deleted after the step, true to live on
     * @author Hj. Malthaner
     */
    virtual bool step(long delta_t);

    const char *gib_name() const {return "Verkehrsteilnehmer";};
    enum ding_t::typ gib_typ() const {return verkehr;};
};

#endif
