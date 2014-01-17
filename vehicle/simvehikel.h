/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

/*
 * Vehicle base type.
 */

#ifndef _simvehikel_h
#define _simvehikel_h

#include "../simtypes.h"
#include "../simconvoi.h"
#include "../simobj.h"
#include "../halthandle_t.h"
#include "../convoihandle_t.h"
#include "../ifc/fahrer.h"
#include "../boden/grund.h"
#include "../besch/vehikel_besch.h"
#include "../vehicle/overtaker.h"
#include "../tpl/slist_tpl.h"

class convoi_t;
class schedule_t;
class signal_t;
class ware_t;
class route_t;

/*----------------------- Movables ------------------------------------*/

/**
 * Base class for all vehicles
 *
 * @author Hj. Malthaner
 */
class vehikel_basis_t : public obj_t
{
protected:
	// offsets for different directions
	static sint8 dxdy[16];

	// to make the length on diagonals configurable
	// Number of vehicle steps along a diagonal...
	// remember to subtract one when stepping down to 0
	static uint8 diagonal_vehicle_steps_per_tile;
	static uint8 old_diagonal_vehicle_steps_per_tile;
	static uint16 diagonal_multiplier;

	// [0]=xoff [1]=yoff
	static sint8 overtaking_base_offsets[8][2];

	/**
	 * Actual travel direction in screen coordinates
	 * @author Hj. Malthaner
	 */
	ribi_t::ribi fahrtrichtung;

	// true on slope (make calc_height much faster)
	uint8 use_calc_height:1;

	// if true, use offsets to emulate driving on other side
	uint8 drives_on_left:1;

	sint8 dx, dy;

	// number of steps in this tile (255 per tile)
	uint8 steps, steps_next;

	/**
	 * Next position on our path
	 * @author Hj. Malthaner
	 */
	koord3d pos_next;

	/**
	 * Offsets for uphill/downhill
	 * @author Hj. Malthaner
	 */
	sint8 hoff;

	// cached image
	image_id bild;

	/**
	 * Vehicle movement: calculates z-offset of vehicles on slopes,
	 * handles vehicles that are invisible in tunnels.
	 * @param gr vehicle is on this ground (can be NULL)
	 * @return new offset
	 */
	sint8 calc_height(grund_t *gr);

	/**
	 * Vehicle movement: check whether this vehicle can enter the next tile (pos_next).
	 */
	virtual bool hop_check() = 0;

	/**
	 * Vehicle movement: change tiles, calls verlasse_feld and betrete_feld.
	 * @return pointer to ground of new position (never NULL)
	 */
	virtual grund_t* hop() = 0;

	virtual void calc_bild() = 0;

	// check for road vehicle, if next tile is free
	vehikel_basis_t *no_cars_blocking( const grund_t *gr, const convoi_t *cnv, const uint8 current_fahrtrichtung, const uint8 next_fahrtrichtung, const uint8 next_90fahrtrichtung );

	// only needed for old way of moving vehicles to determine position at loading time
	bool is_about_to_hop( const sint8 neu_xoff, const sint8 neu_yoff ) const;

public:
	// only called during load time: set some offsets
	static void set_diagonal_multiplier( uint32 multiplier, uint32 old_multiplier );
	static uint16 get_diagonal_multiplier() { return diagonal_multiplier; }
	static uint8 get_diagonal_vehicle_steps_per_tile() { return diagonal_vehicle_steps_per_tile; }

	static void set_overtaking_offsets( bool driving_on_the_left );

	// if true, this convoi needs to restart for correct alignment
	bool need_realignment() const;

	uint32 fahre_basis(uint32 dist);	// basis movement code

	inline void set_bild( image_id b ) { bild = b; }
	virtual image_id get_bild() const {return bild;}

	sint8 get_hoff() const {return hoff;}
	uint8 get_steps() const {return steps;}

	// to make smaller steps than the tile granularity, we have to calculate our offsets ourselves!
	virtual void get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const;

	virtual void rotate90();

	static ribi_t::ribi calc_richtung(koord start, koord ende);
	ribi_t::ribi calc_set_richtung(const koord3d& start, const koord3d& ende);

	ribi_t::ribi get_fahrtrichtung() const {return fahrtrichtung;}

	koord3d get_pos_next() const {return pos_next;}

	virtual waytype_t get_waytype() const = 0;

	// true, if this vehicle did not moved for some time
	virtual bool is_stuck() { return true; }

	/**
	 * Vehicle movement: enter tile, add this to the ground.
	 * @pre position (obj_t::pos) needs to be updated prior to calling this functions
	 * @return pointer to ground (never NULL)
	 */
	virtual grund_t* betrete_feld();

	/**
	 * Vehicle movement: leave tile, release reserved crossing, remove vehicle from the ground.
	 */
	virtual void verlasse_feld();

	virtual overtaker_t *get_overtaker() { return NULL; }

	vehikel_basis_t();

	vehikel_basis_t(koord3d pos);
};


template<> inline vehikel_basis_t* obj_cast<vehikel_basis_t>(obj_t* const d)
{
	return d->is_moving() ? static_cast<vehikel_basis_t*>(d) : 0;
}


/**
 * Class for all vehicles with route
 *
 * @author Hj. Malthaner
 */

class vehikel_t : public vehikel_basis_t, public fahrer_t
{
private:
	/**
	* Date of purchase in months
	* @author Hj. Malthaner
	*/
	sint32 insta_zeit;

	/* For the more physical acceleration model friction is introduced
	* frictionforce = gamma*speed*weight
	* since the total weight is needed a lot of times, we save it
	* @author prissi
	*/
	uint32 sum_weight;

	bool hop_check();

	/**
	 * Calculate friction caused by slopes and curves.
	 */
	virtual void calc_friction(const grund_t *gr);

protected:
	virtual grund_t* hop();

	// current limit (due to track etc.)
	sint32 speed_limit;

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

	uint16 total_freight;	// since the sum is needed quite often, it is cached
	slist_tpl<ware_t> fracht;   // list of goods being transported

	const vehikel_besch_t *besch;

	convoi_t *cnv;		// != NULL if the vehicle is part of a Convoi

	/**
	* Previous position on our path
	* @author Hj. Malthaner
	*/
	koord3d pos_prev;

	bool ist_erstes:1;	// true, if vehicle is first vehicle of a convoi
	bool ist_letztes:1;	// true, if vehicle is last vehicle of a convoi
	bool rauchen:1;
	bool check_for_finish:1;		// true, if on the last tile
	bool has_driven:1;

	virtual bool ist_befahrbar(const grund_t* ) const {return false;}

public:
	virtual void calc_bild();

	// the coordinates, where the vehicle was loaded the last time
	koord3d last_stop_pos;

	convoi_t *get_convoi() const { return cnv; }

	virtual void rotate90();

	virtual bool ist_weg_frei( int &/*restart_speed*/, bool /*second_check*/ ) { return true; }

	virtual grund_t* betrete_feld();

	virtual void verlasse_feld();

	virtual waytype_t get_waytype() const = 0;

	/**
	* Determine the direction bits for this kind of vehicle.
	*
	* @author Hj. Malthaner, 04.01.01
	*/
	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return gr->get_weg_ribi(get_waytype()); }

	sint32 get_insta_zeit() const {return insta_zeit;}

	void darf_rauchen(bool yesno ) { rauchen = yesno;}

	virtual bool calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route);
	uint16 get_route_index() const {return route_index;}
	const koord3d get_pos_prev() const {return pos_prev;}

	/**
	* Get the base image.
	* @author Hj. Malthaner
	*/
	int get_basis_bild() const { return besch->get_basis_bild(); }

	/**
	* @return vehicle description object
	* @author Hj. Malthaner
	*/
	const vehikel_besch_t *get_besch() const {return besch; }

	/**
	* @return die running_cost in Cr/100Km
	* @author Hj. Malthaner
	*/
	int get_betriebskosten() const { return besch->get_betriebskosten(); }

	/**
	* Play sound, when the vehicle is visible on screen
	* @author Hj. Malthaner
	*/
	void play_sound() const;

	/**
	* Prepare vehicle for new ride - called when the Convoi
	* determines a new route
	* @author Hj. Malthaner
	*/
	void neue_fahrt( uint16 start_route_index, bool recalc );

	vehikel_t();
	vehikel_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp);

	~vehikel_t();

	void rauche() const;

	void zeige_info();

	void info(cbuffer_t & buf) const;

	/**
	* Determine travel direction
	* @author Hj. Malthaner
	*/
	ribi_t::ribi richtung() const;

	/* return friction constant: changes in hill and curves; may even negative downhill *
	* @author prissi
	*/
	inline sint16 get_frictionfactor() const { return current_friction; }

	/* Return total weight including freight (in kg!)
	* @author prissi
	*/
	inline uint32 get_gesamtgewicht() const { return sum_weight; }

	// returns speedlimit of ways (and if convoi enters station etc)
	// the convoi takes care of the max_speed of the vehicle
	sint32 get_speed_limit() const { return speed_limit; }

	const slist_tpl<ware_t> & get_fracht() const { return fracht;}   // list of goods being transported

	/**
	 * Rotate freight target coordinates, has to be called after rotating factories.
	 */
	void rotate90_freight_destinations(const sint16 y_size);

	/**
	* Calculate the total quantity of goods moved
	*/
	uint16 get_fracht_menge() const { return total_freight; }

	/**
	* Calculate transported cargo total weight in KG
	* @author Hj. Malthaner
	*/
	uint32 get_fracht_gewicht() const;

	const char * get_fracht_name() const;

	/**
	* get the type of cargo this vehicle can transport
	*/
	const ware_besch_t* get_fracht_typ() const { return besch->get_ware(); }

	/**
	* Get the maximum capacity
	*/
	uint16 get_fracht_max() const {return besch->get_zuladung(); }

	const char * get_fracht_mass() const;

	/**
	* create an info text for the freight
	* e.g. to display in a info window
	* @author Hj. Malthaner
	*/
	void get_fracht_info(cbuffer_t & buf) const;

	/**
	* Delete all vehicle load
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

	// sets or query begin and end of convois
	void set_erstes(bool janein) {ist_erstes = janein;}
	bool is_first() {return ist_erstes;}

	void set_letztes(bool janein) {ist_letztes = janein;}
	bool is_last() {return ist_letztes;}

	// marks the vehicle as really used
	void set_driven() { has_driven = true; }

	virtual void set_convoi(convoi_t *c);

	/**
	 * Unload freight to halt
	 * @return sum of unloaded goods
	 */
	uint16 unload_freight(halthandle_t halt);

	/**
	 * Load freight from halt
	 * @return amount loaded
	 */
	uint16 load_freight(halthandle_t halt);

	/**
	* Remove freight that no longer can reach it's destination
	* i.e. because of a changed schedule
	* @author Hj. Malthaner
	*/
	void remove_stale_freight();

	/**
	* Generate a matching schedule for the vehicle type
	* @author Hj. Malthaner
	*/
	virtual schedule_t *erzeuge_neuen_fahrplan() const = 0;

	const char *ist_entfernbar(const spieler_t *sp);

	void rdwr(loadsave_t *file);
	virtual void rdwr_from_convoi(loadsave_t *file);

	uint32 calc_restwert() const;

	// true, if this vehicle did not moved for some time
	virtual bool is_stuck() { return cnv==NULL  ||  cnv->is_waiting(); }

	// this routine will display a tooltip for lost, on depot order, and stuck vehicles
#ifdef MULTI_THREAD
	virtual void display_overlay(int xpos, int ypos) const;
#else
	virtual void display_after(int xpos, int ypos, bool dirty) const;
#endif
};


template<> inline vehikel_t* obj_cast<vehikel_t>(obj_t* const d)
{
	return dynamic_cast<vehikel_t*>(d);
}


/**
 * A class for road vehicles. Manages the look of the vehicles
 * and the navigability of tiles.
 *
 * @author Hj. Malthaner
 * @see vehikel_t
 */
class automobil_t : public vehikel_t
{
private:
	// called internally only from ist_weg_frei()
	// returns true on success
	bool choose_route( int &restart_speed, ribi_t::dir richtung, uint16 index );

protected:
	bool ist_befahrbar(const grund_t *bd) const;

public:
	virtual grund_t* betrete_feld();

	virtual waytype_t get_waytype() const { return road_wt; }

	automobil_t(loadsave_t *file, bool first, bool last);
	automobil_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cnv); // start and schedule

	virtual void set_convoi(convoi_t *c);

	// how expensive to go here (for way search)
	virtual int get_kosten(const grund_t *, const sint32, koord) const;

	virtual bool calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route);

	virtual bool ist_weg_frei(int &restart_speed, bool second_check );

	// returns true for the way search to an unknown target.
	virtual bool ist_ziel(const grund_t *,const grund_t *) const;

	// since we must consider overtaking, we use this for offset calculation
	virtual void get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const;

	obj_t::typ get_typ() const { return automobil; }

	schedule_t * erzeuge_neuen_fahrplan() const;

	virtual overtaker_t* get_overtaker() { return cnv; }
};


/**
 * A class for rail vehicles (trains). Manages the look of the vehicles
 * and the navigability of tiles.
 *
 * @author Hj. Malthaner
 * @see vehikel_t
 */
class waggon_t : public vehikel_t
{
protected:
	bool ist_befahrbar(const grund_t *bd) const;

	grund_t* betrete_feld();

	bool is_weg_frei_signal( uint16 start_index, int &restart_speed );

	bool is_weg_frei_pre_signal( signal_t *sig, uint16 start_index, int &restart_speed );

	bool is_weg_frei_longblock_signal( signal_t *sig, uint16 start_index, int &restart_speed );

	bool is_weg_frei_choose_signal( signal_t *sig, uint16 start_index, int &restart_speed );


public:
	virtual waytype_t get_waytype() const { return track_wt; }

	// since we might need to un-reserve previously used blocks, we must do this before calculation a new route
	bool calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route);

	// how expensive to go here (for way search)
	virtual int get_kosten(const grund_t *, const sint32, koord) const;

	// returns true for the way search to an unknown target.
	virtual bool ist_ziel(const grund_t *,const grund_t *) const;

	// handles all block stuff and route choosing ...
	virtual bool ist_weg_frei(int &restart_speed, bool );

	// reserves or un-reserves all blocks and returns the handle to the next block (if there)
	// returns true on successful reservation
	bool block_reserver(const route_t *route, uint16 start_index, uint16 &next_signal, uint16 &next_crossing, int signal_count, bool reserve, bool force_unreserve ) const;

	void verlasse_feld();

	typ get_typ() const { return waggon; }

	waggon_t(loadsave_t *file, bool is_first, bool is_last);
	waggon_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t *cnv);
	~waggon_t();

	virtual void set_convoi(convoi_t *c);

	virtual schedule_t * erzeuge_neuen_fahrplan() const;
};



/**
 * very similar to normal railroad, so we can implement it here completely ...
 * @author prissi
 * @see vehikel_t
 */
class monorail_waggon_t : public waggon_t
{
public:
	virtual waytype_t get_waytype() const { return monorail_wt; }

	// all handled by waggon_t
	monorail_waggon_t(loadsave_t *file, bool is_first, bool is_last) : waggon_t(file,is_first, is_last) {}
	monorail_waggon_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cnv) : waggon_t(pos, besch, sp, cnv) {}

	typ get_typ() const { return monorailwaggon; }

	schedule_t * erzeuge_neuen_fahrplan() const;
};



/**
 * very similar to normal railroad, so we can implement it here completely ...
 * @author prissi
 * @see vehikel_t
 */
class maglev_waggon_t : public waggon_t
{
public:
	virtual waytype_t get_waytype() const { return maglev_wt; }

	// all handled by waggon_t
	maglev_waggon_t(loadsave_t *file, bool is_first, bool is_last) : waggon_t(file, is_first, is_last) {}
	maglev_waggon_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cnv) : waggon_t(pos, besch, sp, cnv) {}

	typ get_typ() const { return maglevwaggon; }

	schedule_t * erzeuge_neuen_fahrplan() const;
};



/**
 * very similar to normal railroad, so we can implement it here completely ...
 * @author prissi
 * @see vehikel_t
 */
class narrowgauge_waggon_t : public waggon_t
{
public:
	virtual waytype_t get_waytype() const { return narrowgauge_wt; }

	// all handled by waggon_t
	narrowgauge_waggon_t(loadsave_t *file, bool is_first, bool is_last) : waggon_t(file, is_first, is_last) {}
	narrowgauge_waggon_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cnv) : waggon_t(pos, besch, sp, cnv) {}

	typ get_typ() const { return narrowgaugewaggon; }

	schedule_t * erzeuge_neuen_fahrplan() const;
};



/**
 * A class for naval vehicles. Manages the look of the vehicles
 * and the navigability of tiles.
 *
 * @author Hj. Malthaner
 * @see vehikel_t
 */
class schiff_t : public vehikel_t
{
protected:
	// how expensive to go here (for way search)
	virtual int get_kosten(const grund_t *, const sint32, koord) const { return 1; }

	void calc_friction(const grund_t *gr);

	bool ist_befahrbar(const grund_t *bd) const;

	grund_t* betrete_feld();

public:
	waytype_t get_waytype() const { return water_wt; }

	virtual bool ist_weg_frei(int &restart_speed, bool);

	// returns true for the way search to an unknown target.
	virtual bool ist_ziel(const grund_t *,const grund_t *) const {return 0;}

	schiff_t(loadsave_t *file, bool is_first, bool is_last);
	schiff_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cnv);

	obj_t::typ get_typ() const { return schiff; }

	schedule_t * erzeuge_neuen_fahrplan() const;
};


/**
 * A class for aircrafts. Manages the look of the vehicles
 * and the navigability of tiles.
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
	// only used for route search and approach vectors of get_ribi() (do not need saving)
	koord3d search_start;
	koord3d search_end;

	enum flight_state { taxiing=0, departing=1, flying=2, landing=3, looking_for_parking=4, circling=5, taxiing_to_halt=6  };

	flight_state state;	// functions needed for the search without destination from find_route

	sint16 flughoehe;
	sint16 target_height;
	uint32 suchen, touchdown, takeoff;

protected:
	// jumps to next tile and correct the height ...
	grund_t* hop();

	bool ist_befahrbar(const grund_t *bd) const;

	grund_t* betrete_feld();

	bool block_reserver( uint32 start, uint32 end, bool reserve ) const;

	// find a route and reserve the stop position
	bool find_route_to_stop_position();

public:
	aircraft_t(loadsave_t *file, bool is_first, bool is_last);
	aircraft_t(koord3d pos, const vehikel_besch_t* besch, spieler_t* sp, convoi_t* cnv); // start and schedule

	// since we are drawing ourselves, we must mark ourselves dirty during deletion
	~aircraft_t();

	virtual waytype_t get_waytype() const { return air_wt; }

	// returns true for the way search to an unknown target.
	virtual bool ist_ziel(const grund_t *,const grund_t *) const;

	// return valid direction
	virtual ribi_t::ribi get_ribi(const grund_t* ) const;

	// how expensive to go here (for way search)
	virtual int get_kosten(const grund_t *, const sint32, koord) const;

	virtual bool ist_weg_frei(int &restart_speed, bool);

	virtual void set_convoi(convoi_t *c);

	bool calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route);

	typ get_typ() const { return aircraft; }

	schedule_t * erzeuge_neuen_fahrplan() const;

	void rdwr_from_convoi(loadsave_t *file);

	int get_flyingheight() const {return flughoehe-hoff-2;}

	// image: when flying empty, on ground the plane
	virtual image_id get_bild() const {return !is_on_ground() ? IMG_LEER : bild;}

	// image: when flying the shadow, on ground empty
	virtual image_id get_outline_bild() const {return !is_on_ground() ? bild : IMG_LEER;}

	// shadow has black color (when flying)
	virtual PLAYER_COLOR_VAL get_outline_colour() const {return !is_on_ground() ? TRANSPARENT75_FLAG | OUTLINE_FLAG | COL_BLACK : 0;}

#ifdef MULTI_THREAD
	// this draws the "real" aircrafts (when flying)
	virtual void display_after(int xpos, int ypos, const sint8 clip_num) const;

	// this routine will display a tooltip for lost, on depot order, and stuck vehicles
	virtual void display_overlay(int xpos, int ypos) const;
#else
	// this draws the "real" aircrafts (when flying)
	virtual void display_after(int xpos, int ypos, bool dirty) const;
#endif

	// friction calculation happens in calc_height
	void calc_friction(const grund_t*) {}

	bool is_on_ground() const { return flughoehe==0  &&  !(state==circling  ||  state==flying); }

	const char * ist_entfernbar(const spieler_t *sp);
};

#endif
