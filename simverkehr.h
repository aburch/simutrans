#ifndef _simmover_h
#define _simmover_h

/**
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
class karte_t;

/**
 * Base class for traffic participants with random movement
 * @author Hj. Malthaner
 */
class verkehrsteilnehmer_t : public vehikel_basis_t, public sync_steppable
{
protected:
	/**
	 * Entfernungszaehler
	 * @author Hj. Malthaner
	 */
	uint32 weg_next;

	/* ms until destruction
	 */
	sint32 time_to_life;

	virtual bool ist_weg_frei() {return 1; }	// always free

protected:
	virtual waytype_t gib_waytype() const { return road_wt; }

	virtual bool hop_check() {return true;}
	virtual void hop();

	verkehrsteilnehmer_t(karte_t *welt);
	verkehrsteilnehmer_t(karte_t *welt, koord3d pos);

public:
	virtual ~	verkehrsteilnehmer_t();

	const char *gib_name() const = 0;
	enum ding_t::typ gib_typ() const  = 0;

	/**
	 * Öffnet ein neues Beobachtungsfenster für das Objekt.
	 * @author Hj. Malthaner
	 */
	virtual void zeige_info();

	void rdwr(loadsave_t *file);

	// finalizes direction
	void laden_abschliessen() {calc_bild();}

	// we allow to remove all cars etc.
	const char *ist_entfernbar(const spieler_t *) { return NULL; }
};


class stadtauto_t : public verkehrsteilnehmer_t {
private:
	static stringhashtable_tpl<const stadtauto_besch_t *> table;

	const stadtauto_besch_t *besch;

	// prissi: time to life in blocks
#ifdef DESTINATION_CITYCARS
	koord target;
#endif
	koord3d pos_next_next;

	/**
	 * Aktuelle Geschwindigkeit
	 * @author Hj. Malthaner
	 */
	uint16 current_speed;

	uint32 ms_traffic_jam;

	virtual bool hop_check();
	virtual bool ist_weg_frei();

protected:
	void rdwr(loadsave_t *file);

	void calc_bild();

public:
	stadtauto_t(karte_t *welt, loadsave_t *file);
	stadtauto_t(karte_t *welt, koord3d pos, koord target);

	bool sync_step(long delta_t);

	void hop();

	void betrete_feld();

	void calc_current_speed();

	const char *gib_name() const {return "Verkehrsteilnehmer";}
	enum ding_t::typ gib_typ() const {return verkehr;}

	/**
	 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 * @see simwin
	 */
	virtual void info(cbuffer_t & buf) const;

	// true, if this vehicle did not moved for some time
	virtual bool is_stuck() { return current_speed==0;}

	/* this function builts the list of the allowed citycars
	 * it should be called every month and in the beginning of a new game
	 * @author prissi
	 */
	static void built_timeline_liste(karte_t *welt);
	static bool list_empty();

	static bool register_besch(const stadtauto_besch_t *besch);
	static bool laden_erfolgreich();
};

#endif
