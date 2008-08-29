/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Fahrzeuge in der Welt von Sim
 *
 * 01.11.99  getrennt von simdings.h
 * Hj. Malthaner
 */

#ifndef _simvehikel_h
#define _simvehikel_h

#include "../simtypes.h"
#include "../simdings.h"
#include "../halthandle_t.h"
#include "../convoihandle_t.h"
#include "../ifc/fahrer.h"
#include "../boden/grund.h"
#include "../boden/wege/weg.h"
#include "../besch/vehikel_besch.h"
#include "../tpl/slist_tpl.h"

class convoi_t;
class fahrplan_t;
class signal_t;
class ware_t;
class route_t;

/*----------------------- Fahrdings ------------------------------------*/

/**
 * Basisklasse für alle Fahrzeuge
 *
 * @author Hj. Malthaner
 */
class vehikel_basis_t : public ding_t
{
protected:
	// offsets for different directions
	static sint8 dxdy[16];

	/**
	 * Aktuelle Fahrtrichtung in Bildschirm-Koordinaten
	 * @author Hj. Malthaner
	 */
	ribi_t::ribi fahrtrichtung;

	// true on slope (make calc_height much faster)
	uint8 use_calc_height:1;

	// true, if hop_check failed
	uint8 hop_check_failed:1;

sint8 dx, dy;
	// number of steps in this tile (255 per tile)
	uint8 steps, steps_next;

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

	sint16 calc_height();		// Offset Bergauf/Bergab

	virtual bool hop_check() = 0;
	virtual void hop() = 0;

	virtual void calc_bild() = 0;

	// check for road vehicle, if next tile is free
	vehikel_basis_t *no_cars_blocking( const grund_t *gr, const convoi_t *cnv, const uint8 current_fahrtrichtung, const uint8 next_fahrtrichtung, const uint8 next_90fahrtrichtung );

	// only needed for old way of moving vehicles to determine position at loading time
	bool is_about_to_hop( const sint8 neu_xoff, const sint8 neu_yoff ) const;

public:
	uint32 fahre_basis(uint32 dist);	// basis movement code

	inline void setze_bild( image_id b ) { bild = b; }
	virtual image_id gib_bild() const {return bild;}

	sint16 gib_hoff() const {return hoff;}

	// to make smaller steps than the tile granularity, we have to calculate our offsets ourselves!
	void get_screen_offset( int &xoff, int &yoff ) const;

	virtual void rotate90();

	ribi_t::ribi calc_richtung(koord start, koord ende) const;
	ribi_t::ribi calc_set_richtung(koord start, koord ende);

	ribi_t::ribi gib_fahrtrichtung() const {return fahrtrichtung;}

	virtual waytype_t gib_waytype() const = 0;

	// true, if this vehicle did not moved for some time
	virtual bool is_stuck() { return true; }

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

	bool hop_check();

	/**
	 * berechnet aktuelle Geschwindigkeit aufgrund der Steigung
	 * (Hoehendifferenz) der Fahrbahn
	 * @param h_alt alte Hoehe
	 * @param h_neu neue Hoehe
	 */
	virtual void calc_akt_speed(const grund_t *gr);

	/**
	 * Unload freight to halt
	 * @return sum of unloaded goods
	 * @author Hj. Malthaner
	 */
	uint16 unload_freight(halthandle_t halt);

	/**
	 * Load freight from halt
	 * @return loading successful?
	 * @author Hj. Malthaner
	 */
	bool load_freight(halthandle_t halt);

protected:
	virtual void hop();

	// current limit (due to track etc.)
	uint32 speed_limit;

	ribi_t::ribi alte_fahrtrichtung;

	// for target reservation and search
	halthandle_t target_halt;

	/* The friction is calculated new every step, so we save it too
	* @author prissi
	*/
	sint16 current_friction;

	/**
	* Current index on the route
	* @author Hj. Malthaner
	*/
	uint16 route_index;

	uint16 total_freight;	// since the sum is needed quite often, it is chached
	slist_tpl<ware_t> fracht;   // liste der gerade transportierten güter

	const vehikel_besch_t *besch;

	convoi_t *cnv;		// != NULL falls das vehikel zu einem Convoi gehoert

	/**
	* Previous position on our path
	* @author Hj. Malthaner
	*/
	koord3d pos_prev;

	bool ist_erstes:1;				// falls vehikel im convoi fährt, geben diese
	bool ist_letztes:1;				// flags auskunft über die position
	bool rauchen:1;
	bool check_for_finish:1;		// true, if on the last tile

	virtual void calc_bild();

	virtual bool ist_befahrbar(const grund_t* ) const {return false;}

public:
	// the coordinates, where the vehicle was loaded the last time
	koord last_stop_pos;

	enum { SPEED_UNLIMITED=0x07FFFFFF };

	convoi_t *get_convoi() const { return cnv; }

	virtual void rotate90();

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

	sint32 gib_insta_zeit() const {return insta_zeit;}

	void darf_rauchen(bool yesno ) { rauchen = yesno;}

	virtual bool calc_route(koord3d start, koord3d ziel, uint32 max_speed, route_t* route);
	uint16 gib_route_index() const {return route_index;}
	const koord3d gib_pos_prev() const {return pos_prev;}

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
	vehikel_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp);

	~vehikel_t();

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
	* Ermittelt fahrtrichtung
	* @author Hj. Malthaner
	*/
	ribi_t::ribi richtung();

	/* return friction constant: changes in hill and curves; may even negative downhill *
	* @author prissi
	*/
	inline int gib_frictionfactor() const { return current_friction; }

	/* Return total weight including freight*
	* @author prissi
	*/
	inline int gib_gesamtgewicht() const { return sum_weight; }

	const slist_tpl<ware_t> & gib_fracht() const { return fracht;}   // liste der gerade transportierten güter

	/**
	* berechnet die gesamtmenge der beförderten waren
	*/
	uint16 gib_fracht_menge() const { return total_freight; }

	/**
	* Berechnet Gesamtgewicht der transportierten Fracht in KG
	* @author Hj. Malthaner
	*/
	uint32 gib_fracht_gewicht() const;

	const char * gib_fracht_name() const;

	/**
	* setzt den typ der beförderbaren ware
	*/
	const ware_besch_t* gib_fracht_typ() const { return besch->gib_ware(); }

	/**
	* setzt die maximale Kapazitaet
	*/
	uint16 gib_fracht_max() const {return besch->gib_zuladung(); }

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
	sint64  calc_gewinn(koord start, koord end) const;

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

	uint32 calc_restwert() const;

	// true, if this vehicle did not moved for some time
	virtual bool is_stuck() { return cnv==NULL  ||  cnv->is_waiting(); }

	// this draws a tooltips for things stucked on depot order or lost
	virtual void display_after(int xpos, int ypos, bool dirty) const;
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
	automobil_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cnv); // start und fahrplan

	virtual void setze_convoi(convoi_t *c);

	// how expensive to go here (for way search)
	virtual int gib_kosten(const grund_t *,const uint32 ) const;

	virtual bool calc_route(koord3d start, koord3d ziel, uint32 max_speed, route_t* route);

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
	bool calc_route(koord3d start, koord3d ziel, uint32 max_speed, route_t* route);

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
	waggon_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t *cnv); // start und fahrplan
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
	monorail_waggon_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cnv) : waggon_t(pos, besch, sp, cnv) {}

	enum ding_t::typ gib_typ() const { return monorailwaggon; }

	fahrplan_t * erzeuge_neuen_fahrplan() const;
};



/**
 * very similar to normal railroad, so wie can implement it here completely ...
 * @author prissi
 * @see vehikel_t
 */
class maglev_waggon_t : public waggon_t
{
public:
	virtual waytype_t gib_waytype() const { return maglev_wt; }

	// all handled by waggon_t
	maglev_waggon_t(karte_t *welt, loadsave_t *file) : waggon_t(welt, file) {}
	maglev_waggon_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cnv) : waggon_t(pos, besch, sp, cnv) {}

	enum ding_t::typ gib_typ() const { return maglevwaggon; }

	fahrplan_t * erzeuge_neuen_fahrplan() const;
};



/**
 * very similar to normal railroad, so wie can implement it here completely ...
 * @author prissi
 * @see vehikel_t
 */
class narrowgauge_waggon_t : public waggon_t
{
public:
	virtual waytype_t gib_waytype() const { return narrowgauge_wt; }

	// all handled by waggon_t
	narrowgauge_waggon_t(karte_t *welt, loadsave_t *file) : waggon_t(welt, file) {}
	narrowgauge_waggon_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cnv) : waggon_t(pos, besch, sp, cnv) {}

	enum ding_t::typ gib_typ() const { return narrowgaugewaggon; }

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
	schiff_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cnv); // start und fahrplan

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
	// just to mark dirty afterwards
	sint16 old_x, old_y;
	image_id old_bild;

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
	// jumps to next tile and correct the height ...
	void hop();

	bool ist_befahrbar(const grund_t *bd) const;

	void betrete_feld();

	void verlasse_feld();

	bool block_reserver( uint32 start, uint32 end, bool reserve );

	// find a route and reserve the stop position
	bool find_route_to_stop_position();

public:
	aircraft_t(karte_t *welt, loadsave_t *file);
	aircraft_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cnv); // start und fahrplan

	// since we are drawing ourselves, we must mark ourselves dirty during deletion
	~aircraft_t();

	virtual waytype_t gib_waytype() const { return air_wt; }

	// returns true for the way search to an unknown target.
	virtual bool ist_ziel(const grund_t *,const grund_t *) const;

	// return valid direction
	virtual ribi_t::ribi gib_ribi(const grund_t* ) const;

	// how expensive to go here (for way search)
	virtual int gib_kosten(const grund_t *,const uint32 ) const;

	virtual bool ist_weg_frei(int &restart_speed);

	virtual void setze_convoi(convoi_t *c);

	bool calc_route(koord3d start, koord3d ziel, uint32 max_speed, route_t* route);

	enum ding_t::typ gib_typ() const { return aircraft; }

	fahrplan_t * erzeuge_neuen_fahrplan() const;

	void rdwr(loadsave_t *file);
	void rdwr(loadsave_t *file, bool force);

	int gib_flyingheight() const {return flughoehe-hoff-2;}

	// since our image is the shadow ...
	virtual image_id gib_bild() const {return IMG_LEER;}

	// outline is the planes shadow
	virtual PLAYER_COLOR_VAL gib_outline_bild() const {return bild;}

	// shadow has black color
	virtual PLAYER_COLOR_VAL gib_outline_colour() const {return TRANSPARENT75_FLAG | OUTLINE_FLAG | COL_BLACK;}

	// this draws the "real" aircrafts!
	virtual void display_after(int xpos, int ypos, bool dirty) const;

	// the speed calculation happens it calc_height
	void calc_akt_speed(const grund_t*) {}
};

#endif
