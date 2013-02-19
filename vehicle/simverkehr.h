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
#include "overtaker.h"

#include "../tpl/stringhashtable_tpl.h"
#include "../ifc/sync_steppable.h"

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

protected:
	virtual waytype_t get_waytype() const { return road_wt; }

	virtual bool hop_check() {return true;}

	verkehrsteilnehmer_t(karte_t *welt);
	verkehrsteilnehmer_t(karte_t *welt, koord3d pos, uint16 random);

public:
	virtual ~verkehrsteilnehmer_t();

	const char *get_name() const = 0;
	typ get_typ() const  = 0;

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


class stadtauto_t : public verkehrsteilnehmer_t, public overtaker_t
{
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

	bool hop_check();

protected:
	void rdwr(loadsave_t *file);

	void calc_bild();

public:
	stadtauto_t(karte_t *welt, loadsave_t *file);
	stadtauto_t(karte_t *welt, koord3d pos, koord target);

	virtual ~stadtauto_t();

	const stadtauto_besch_t *get_besch() const { return besch; }

	bool sync_step(long delta_t);

	grund_t* hop();
	bool ist_weg_frei(grund_t *gr);

	grund_t* betrete_feld();

	void calc_current_speed();
	uint16 get_current_speed() const {return current_speed;}

	const char *get_name() const {return "Verkehrsteilnehmer";}
	typ get_typ() const { return verkehr; }

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
	static bool alles_geladen();

	// since we must consider overtaking, we use this for offset calculation
	virtual void get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const;

	virtual overtaker_t *get_overtaker() { return this; }

	// Overtaking for city cars
	virtual bool can_overtake(overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other);
};

#endif
