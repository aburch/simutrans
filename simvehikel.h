/*
 * simvehikel.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/* simvehikel.h
 *
 * Fahrzeuge in der Welt von Sim
 *
 * 01.11.99  getrennt von simdings.h
 * Hj. Malthaner
 */

#ifndef _simvehikel_h
#define _simvehikel_h

#include "simtypes.h"
#include "simdings.h"
#include "simware.h"
#include "simconvoi.h"
#include "halthandle_t.h"
#include "convoihandle_t.h"
#include "ifc/fahrer.h"
#include "boden/grund.h"
#include "boden/wege/weg.h"
#include "besch/vehikel_besch.h"
#include "dataobj/route.h"
#include "tpl/slist_tpl.h"

class vehikel_besch_t;
class fahrplan_t;
class signal_t;


/*
 * Global vehicle speed conversion factor between Simutrans speed
 * and km/h
 * @author Hj. Malthaner
 */
#define VEHICLE_SPEED_FACTOR  80


/**
 * Converts speed value to km/h
 * @author Hj. Matthaner
 */
#define speed_to_kmh(speed) (((speed)*VEHICLE_SPEED_FACTOR+511) >> 10)


/**
 * Converts km/h value to speed
 * @author Hj. Matthaner
 */
#define kmh_to_speed(speed) (((speed) << 10) / VEHICLE_SPEED_FACTOR)





/*----------------------- Fahrdings ------------------------------------*/

/**
 * Basisklasse für alle Fahrzeuge
 *
 * @author Hj. Malthaner
 */
class vehikel_basis_t : public ding_t
{
protected:
	/**
	 * Aktuelle Fahrtrichtung in Bildschirm-Koordinaten
	 * @author Hj. Malthaner
	 */
	ribi_t::ribi fahrtrichtung;

	sint8 dx, dy;

	/**
	 * Next position on our path
	 * @author Hj. Malthaner
	 */
	koord3d pos_next;

	/**
	 * Offsets fuer Bergauf/Bergab
	 * @author Hj. Malthaner
	 */
	sint8 hoff;

	// cached image
	image_id bild;

	virtual void fahre();
	virtual int  calc_height();		// Offset Bergauf/Bergab

	virtual int  gib_dx() const {return dx;}
	virtual int  gib_dy() const {return dy;}
	virtual int  gib_hoff() const {return hoff;}

	virtual bool hop_check() = 0;
	virtual void hop() = 0;

	virtual void calc_bild() = 0;

public:
	inline void setze_bild( image_id b ) { bild = b; }
	image_id gib_bild() const {return bild;}	ribi_t::ribi calc_richtung(koord start, koord ende, sint8 &dx, sint8 &dy) const;

	ribi_t::ribi gib_fahrtrichtung() const {return fahrtrichtung;}

	virtual waytype_t gib_waytype() const = 0;

	virtual void betrete_feld();

	virtual void verlasse_feld();

	vehikel_basis_t(karte_t *welt);

	vehikel_basis_t(karte_t *welt, koord3d pos);
};


/**
 * Klasse für alle Fahrzeuge mit Route
 *
 * @author Hj. Malthaner
 */

class vehikel_t : public vehikel_basis_t, public fahrer_t
{
private:
	/**
	* Kaufdatum in months
	* @author Hj. Malthaner
	*/
	sint32 insta_zeit;

	/* For the more physical acceleration model friction is introduced
	* frictionforce = gamma*speed*weight
	* since the total weight is needed a lot of times, we save it
	* @author prissi
	*/
	uint16 sum_weight;

	sint32 speed_limit;

	bool hop_check();

	void hop();

	/**
	 * berechnet aktuelle Geschwindigkeit aufgrund der Steigung
	 * (Hoehendifferenz) der Fahrbahn
	 * @param h_alt alte Hoehe
	 * @param h_neu neue Hoehe
	 */
	virtual void calc_akt_speed(const grund_t *gr);

	void fahre();

protected:
	ribi_t::ribi alte_fahrtrichtung;

	// for target reservation and search
	halthandle_t target_halt;

	/* The friction is calculated new every step, so we save it too
	* @author prissi
	*/
	sint16 current_friction;

	void setze_speed_limit(int limit);
	sint32 get_speed_limit() { return speed_limit; }

	/**
	* Current index on the route
	* @author Hj. Malthaner
	*/
	uint16 route_index;

	/**
	* Previous position on our path
	* @author Hj. Malthaner
	*/
	koord pos_prev;

	const vehikel_besch_t *besch;

	slist_tpl<ware_t> fracht;   // liste der gerade transportierten güter

	convoi_t *cnv;		// != NULL falls das vehikel zu einem Convoi gehoert

	bool ist_erstes:1;            // falls vehikel im convoi fährt, geben diese
	bool ist_letztes:1;           // flags auskunft über die position
	bool rauchen:1;

	virtual void calc_bild();

	virtual bool ist_befahrbar(const grund_t* ) const {return false;}

public:
	virtual bool ist_weg_frei(int &/*restart_speed*/) {return true;}

	virtual void betrete_feld();

	virtual void verlasse_feld();

	virtual waytype_t gib_waytype() const = 0;

	/**
	* Ermittelt die für das Fahrzeug geltenden Richtungsbits,
	* abhängig vom Untergrund.
	*
	* @author Hj. Malthaner, 04.01.01
	*/
	virtual ribi_t::ribi gib_ribi(const grund_t* gr) const { return gr->gib_weg_ribi(gib_waytype()); }

	const sint32 gib_insta_zeit() const {return insta_zeit;}

	void darf_rauchen(bool yesno ) { rauchen = yesno;}

	virtual bool calc_route(karte_t * welt, koord3d start, koord3d ziel, uint32 max_speed, route_t * route) { return route->calc_route(welt, start, ziel, this, max_speed ); }
	const uint16 gib_route_index() const {return route_index;}

	void setze_offsets(int x, int y);

	/**
	* gibt das Basisbild zurueck
	* @author Hj. Malthaner
	*/
	int gib_basis_bild() const { return besch->gib_basis_bild(); }

	/**
	* @return vehicle description object
	* @author Hj. Malthaner
	*/
	const vehikel_besch_t *gib_besch() const {return besch; }

	/**
	* @return die Betriebskosten in Cr/100Km
	* @author Hj. Malthaner
	*/
	int gib_betriebskosten() const { return besch->gib_betriebskosten(); }

	/**
	* spielt den Sound, wenn das Vehikel sichtbar ist
	* @author Hj. Malthaner
	*/
	void play_sound() const;

	/**
	* Bereitet Fahrzeiug auf neue Fahrt vor - wird aufgerufen wenn
	* der Convoi eine neue Route ermittelt
	* @author Hj. Malthaner
	*/
	void neue_fahrt( uint16 start_route_index, bool recalc );

	vehikel_t(karte_t *welt);
	vehikel_t(karte_t *welt,
		koord3d pos,
		const vehikel_besch_t *besch,
		spieler_t *sp);

	~vehikel_t();

	/**
	* Vom Convoi aufgerufen.
	* @author Hj. Malthaner
	*/
	void sync_step();

	void rauche();

	/**
	* Öffnet ein neues Beobachtungsfenster für das Objekt.
	* @author Hj. Malthaner
	*/
	void zeige_info();

	/**
	* der normale Infotext
	* @author Hj. Malthaner
	*/
	void info(cbuffer_t & buf) const;

	/**
	* debug info into buffer
	* @author Hj. Malthaner
	*/
	char *debug_info(char *buf) const;

	/**
	* Debug info to stderr
	* @author Hj. Malthaner
	* @date 26-Aug-03
	*/
	void dump() const;

	/**
	* Ermittelt fahrtrichtung
	* @author Hj. Malthaner
	*/
	ribi_t::ribi richtung();

	inline const int gib_speed() const { return kmh_to_speed(besch->gib_geschw()); }

	/* return friction constant: changes in hill and curves; may even negative downhill *
	* @author prissi
	*/
	inline const int gib_frictionfactor() const { return current_friction; }

	/* Return total weight including freight*
	* @author prissi
	*/
	inline const int gib_gesamtgewicht() const { return sum_weight; }

	const slist_tpl<ware_t> & gib_fracht() const { return fracht;}   // liste der gerade transportierten güter

	/**
	* berechnet die gesamtmenge der beförderten waren
	*/
	int gib_fracht_menge() const;

	/**
	* Berechnet Gesamtgewicht der transportierten Fracht in KG
	* @author Hj. Malthaner
	*/
	int gib_fracht_gewicht() const;

	const char * gib_fracht_name() const;

	/**
	* setzt den typ der beförderbaren ware
	*/
	const ware_besch_t* gib_fracht_typ() const { return besch->gib_ware(); }

	/**
	* setzt die maximale Kapazitaet
	*/
	const int gib_fracht_max() const {return besch->gib_zuladung(); }

	const char * gib_fracht_mass() const;

	/**
	* erstellt einen Info-Text zur Fracht, z.B. zur Darstellung in einem
	* Info-Fenster
	* @author Hj. Malthaner
	*/
	void gib_fracht_info(cbuffer_t & buf);

	/**
	* loescht alle fracht aus dem Fahrzeug
	* @author Hj. Malthaner
	*/
	void loesche_fracht();

	/**
	* Payment is done per hop. It iterates all goods and calculates
	* the income for the last hop. This method must be called upon
	* every stop.
	* @return income total for last hop
	* @author Hj. Malthaner
	*/
	sint64  calc_gewinn(koord3d start, koord3d end) const;

	/**
	* fahrzeug an haltestelle entladen
	* @author Hj. Malthaner
	*/
	bool entladen(koord k, halthandle_t halt);

	/**
	* fahrzeug an haltestelle beladen
	*/
	bool beladen(koord k, halthandle_t halt);

	// sets or querey begin and end of convois
	void setze_erstes(bool janein) {ist_erstes = janein;}
	bool is_first() {return ist_erstes;}

	void setze_letztes(bool janein) {ist_letztes = janein;}
	bool is_last() {return ist_letztes;}

	virtual void setze_convoi(convoi_t *c);
	convoihandle_t get_convoi() const { return cnv->self; }

	/**
	* Remove freight that no longer can reach it's destination
	* i.e. because of a changed schedule
	* @author Hj. Malthaner
	*/
	void remove_stale_freight();

	/**
	* erzeuge einen für diesen Vehikeltyp passenden Fahrplan
	* @author Hj. Malthaner
	*/
	virtual fahrplan_t * erzeuge_neuen_fahrplan() const = 0;

	const char * ist_entfernbar(const spieler_t *sp);

	void rdwr(loadsave_t *file);
	virtual void rdwr(loadsave_t *file, bool force) = 0;

	int calc_restwert() const;
};


/**
 * Eine Klasse für Strassenfahrzeuge. Verwaltet das Aussehen der
 * Fahrzeuge und die Befahrbarkeit des Untergrundes.
 *
 * @author Hj. Malthaner
 * @see vehikel_t
 */
class automobil_t : public vehikel_t
{
protected:
	bool ist_befahrbar(const grund_t *bd) const;

public:
	virtual void betrete_feld();

	virtual waytype_t gib_waytype() const { return road_wt; }

	automobil_t(karte_t *welt, loadsave_t *file);
	automobil_t(karte_t *welt, koord3d pos, const vehikel_besch_t *besch, spieler_t *sp, convoi_t *cnv); // start und fahrplan

	virtual void setze_convoi(convoi_t *c);

	// how expensive to go here (for way search)
	virtual int gib_kosten(const grund_t *,const uint32 ) const;

	virtual bool calc_route(karte_t * welt, koord3d start, koord3d ziel, uint32 max_speed, route_t * route);

	virtual bool ist_weg_frei(int &restart_speed);

	// returns true for the way search to an unknown target.
	virtual bool ist_ziel(const grund_t *,const grund_t *) const;

	ding_t::typ gib_typ() const { return automobil; }

	fahrplan_t * erzeuge_neuen_fahrplan() const;

	void rdwr(loadsave_t *file);
	void rdwr(loadsave_t *file, bool force);
};


/**
 * Eine Klasse für Schienenfahrzeuge. Verwaltet das Aussehen der
 * Fahrzeuge und die Befahrbarkeit des Untergrundes.
 *
 * @author Hj. Malthaner
 * @see vehikel_t
 */
class waggon_t : public vehikel_t
{
private:
	signal_t *ist_blockwechsel(koord3d k2) const;

protected:
	bool ist_befahrbar(const grund_t *bd) const;

	void betrete_feld();

public:
	virtual waytype_t gib_waytype() const { return track_wt; }

	// since we might need to unreserve previously used blocks, we must do this before calculation a new route
	bool calc_route(karte_t * welt, koord3d start, koord3d ziel, uint32 max_speed, route_t * route);

	// how expensive to go here (for way search)
	virtual int gib_kosten(const grund_t *,const uint32 ) const;

	// returns true for the way search to an unknown target.
	virtual bool ist_ziel(const grund_t *,const grund_t *) const;

	// handles all block stuff and route choosing ...
	virtual bool ist_weg_frei(int &restart_speed);

	// reserves or unreserves all blocks and returns the handle to the next block (if there)
	// if count is larger than 1, maximum 64 tiles will be checked (freeing or reserving a choose signal path)
	// return the last checked block
	uint16 block_reserver(const route_t *route,uint16 start_index,int count, bool reserve) const;

	void verlasse_feld();

	enum ding_t::typ gib_typ() const { return waggon; }

	waggon_t(karte_t *welt, loadsave_t *file);
	waggon_t(karte_t *welt, koord3d pos, const vehikel_besch_t *besch, spieler_t *sp, convoi_t *cnv); // start und fahrplan
	~waggon_t();

	virtual void setze_convoi(convoi_t *c);

	virtual fahrplan_t * erzeuge_neuen_fahrplan() const;

	void rdwr(loadsave_t *file);
	void rdwr(loadsave_t *file, bool force);
};



/**
 * very similar to normal railroad, so wie can implement it here completely ...
 * @author prissi
 * @see vehikel_t
 */
class monorail_waggon_t : public waggon_t
{
public:
	virtual waytype_t gib_waytype() const { return monorail_wt; }

	// all handled by waggon_t
	monorail_waggon_t(karte_t *welt, loadsave_t *file) : waggon_t(welt, file) {}
	monorail_waggon_t(karte_t *welt, koord3d pos, const vehikel_besch_t *besch, spieler_t *sp, convoi_t *cnv) : waggon_t(welt, pos, besch, sp, cnv ) {}

	enum ding_t::typ gib_typ() const { return monorailwaggon; }

	fahrplan_t * erzeuge_neuen_fahrplan() const;
};



/**
 * Eine Klasse für Wasserfahrzeuge. Verwaltet das Aussehen der
 * Fahrzeuge und die Befahrbarkeit des Untergrundes.
 *
 * @author Hj. Malthaner
 * @see vehikel_t
 */
class schiff_t : public vehikel_t
{
protected:
	// how expensive to go here (for way search)
	virtual int gib_kosten(const grund_t*, const uint32) const { return 1; }

	void calc_akt_speed(const grund_t *gr);

	bool ist_befahrbar(const grund_t *bd) const;

public:
	waytype_t gib_waytype() const { return water_wt; }

	virtual bool ist_weg_frei(int &restart_speed);

	// returns true for the way search to an unknown target.
	virtual bool ist_ziel(const grund_t *,const grund_t *) const {return 0;}

	schiff_t(karte_t *welt, loadsave_t *file);
	schiff_t(karte_t *welt, koord3d pos, const vehikel_besch_t *besch, spieler_t *sp, convoi_t *cnv); // start und fahrplan

	ding_t::typ gib_typ() const { return schiff; }

	fahrplan_t * erzeuge_neuen_fahrplan() const;

	void rdwr(loadsave_t *file);
	void rdwr(loadsave_t *file, bool force);
};


/**
 * Eine Klasse für Flugzeuge. Verwaltet das Aussehen der
 * Fahrzeuge und die Befahrbarkeit des Untergrundes.
 *
 * @author hsiegeln
 * @see vehikel_t
 */
class aircraft_t : public vehikel_t
{
private:
	// only used for ist_ziel() (do not need saving)
	ribi_t::ribi approach_dir;
#ifdef USE_DIFFERENT_WIND
	static uint8 get_approach_ribi( koord3d start, koord3d ziel );
#endif
	// only used for route search and approach vectors of gib_ribi() (do not need saving)
	koord3d search_start;
	koord3d search_end;

	enum flight_state { taxiing=0, departing=1, flying=2, landing=3, looking_for_parking=4, flying2=5, taxiing_to_halt=6  };

	enum flight_state state;	// functions needed for the search without destination from find_route

	sint16 flughoehe;
	sint16 target_height;
	uint32 suchen, touchdown, takeoff;

protected:
	bool ist_befahrbar(const grund_t *bd) const;

	void betrete_feld();

	// find a route and reserve the stop position
	bool find_route_to_stop_position();

public:
	virtual waytype_t gib_waytype() const { return air_wt; }

	// returns true for the way search to an unknown target.
	virtual bool ist_ziel(const grund_t *,const grund_t *) const;

	// return valid direction
	virtual ribi_t::ribi gib_ribi(const grund_t* ) const;

	// how expensive to go here (for way search)
	virtual int gib_kosten(const grund_t *,const uint32 ) const;

	virtual bool ist_weg_frei(int &restart_speed);

	virtual void setze_convoi(convoi_t *c);

	bool calc_route(karte_t * welt, koord3d start, koord3d ziel, uint32 max_speed, route_t * route);

	enum ding_t::typ gib_typ() const { return aircraft; }

	aircraft_t(karte_t *welt, loadsave_t *file);
	aircraft_t(karte_t *welt, koord3d pos, const vehikel_besch_t *besch, spieler_t *sp, convoi_t *cnv); // start und fahrplan

	fahrplan_t * erzeuge_neuen_fahrplan() const;

	void rdwr(loadsave_t *file);
	void rdwr(loadsave_t *file, bool force);

	virtual int calc_height();

	// the speed calculation happens it calc_height
	void calc_akt_speed(const grund_t*) {}
};

#endif
