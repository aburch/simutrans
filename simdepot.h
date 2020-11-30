/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMDEPOT_H
#define SIMDEPOT_H


#include "tpl/slist_tpl.h"
#include "obj/gebaeude.h"
#include "convoihandle_t.h"
#include "simline.h"

class karte_t;
class vehicle_t;
class depot_frame_t;
class vehicle_desc_t;
class gui_convoy_assembler;


/**
 * In Depots werden Fahrzeuge gekauft, gewartet, verkauft und gelagert.
 */
class depot_t : public gebaeude_t
{
protected:
	/**
	 * Reworked depot data!
	 *
	 * It can now contain any number of vehicles bought by the user (as before).
	 * And it can hold any number of convois (before only one).
	 * It is possible to have 0 convois in a depot, but an empty one shall be
	 * automatically created, when necessary.
	 * Convois are numbered 0...(n-1).
	 * Vehicles are accessed by type.
	 */
	slist_tpl<vehicle_t *> vehicles;
	slist_tpl<convoihandle_t> convois;

	void rdwr_vehicle(slist_tpl<vehicle_t*> &list, loadsave_t *file);

	static slist_tpl<depot_t *> all_depots;

public:
	// Last selected vehicle filter
	int selected_filter;

	/**
	 * Is this depot suitable for this vehicle?
	 * Must be same waytype, same owner, suitable traction, etc.
	 * @param test_vehicle -- must not be NULL
	 * @param traction_types
	 *   - 0 if we don't want to filter by traction type
	 *   - a bitmask of possible traction types; we need only match one
	 */
	bool is_suitable_for( const vehicle_t * test_vehicle, const uint16 traction_types = 0) const;

	// finds the next/previous depot relative to the current position
	static depot_t *find_depot( koord3d start, const obj_t::typ depot_type, const player_t *player, bool next);

	static const slist_tpl<depot_t *>& get_depot_list() { return all_depots; }

	static unsigned get_max_convoy_length(waytype_t wt);

#ifdef INLINE_OBJ_TYPE
	depot_t(obj_t::typ type, loadsave_t *file);
	depot_t(obj_t::typ type, koord3d pos, player_t *player, const building_tile_desc_t *t);
#else
	depot_t(loadsave_t *file);
	depot_t(koord3d pos, player_t *player, const building_tile_desc_t *t);
#endif
	virtual ~depot_t();

	void call_depot_tool( char tool, convoihandle_t cnv, const char *extra, uint16 livery_scheme_index = 0 );

	virtual simline_t::linetype get_line_type() const = 0;

	void rdwr(loadsave_t *file) OVERRIDE;

	// text for the tabs is defaulted to the train names
	virtual const char * get_electrics_name() { return "Electrics_tab"; };
	virtual const char * get_passenger_name() { return "Pas_tab"; }
	virtual const char * get_zieher_name() { return "Lokomotive_tab"; }
	virtual const char * get_haenger_name() { return "Waggon_tab"; }
	virtual const char * get_passenger2_name() { return "Pas2_tab"; }

	/**
	 * Access to convoi list.
	 */
	unsigned convoi_count() const { return convois.get_count(); }

	convoihandle_t get_convoi(unsigned int icnv) const { return icnv < convoi_count() ? convois.at(icnv) : convoihandle_t(); }

	convoihandle_t add_convoi(bool local_execution);

	slist_tpl<convoihandle_t> const& get_convoy_list() { return convois; }

	// checks if cnv can be copied by using only stored vehicles and non-obsolete purchased vehicles
	bool check_obsolete_inventory(convoihandle_t cnv);

	/**
	 * copies convoi and its schedule or line
	 */
	convoihandle_t copy_convoi(convoihandle_t old_cnv, bool local_execution);

	/**
	 * Let convoi leave the depot.
	 * If not possible, a message is displayed and the function returns false.
	 * @param if local_execution is true, this method creates pop-ups in case of errors
	 */
	bool start_convoi(convoihandle_t cnv, bool local_execution);

	bool start_all_convoys();

	/**
	 * Destroy the convoi and put the vehicles in the vehicles list (sell==false),
	 * or sell all immediately (sell==true).
	 */
	bool disassemble_convoi(convoihandle_t cnv, bool sell);

	/**
	 * Remove the convoi from the depot lists
	 * updating depot gui frame as necessary
	 */
	void remove_convoi( convoihandle_t cnv );

	/**
	 * Remove vehicle from vehicle list and add it to the convoi. Two positions
	 * are possible - in front or at the rear.
	 */
	void append_vehicle(convoihandle_t &cnv, vehicle_t* veh, bool infront, bool local_execution);

	/**
	 * Remove the vehicle at given position from the convoi and put it in the
	 * vehicle list.
	 */
	void remove_vehicle(convoihandle_t cnv, int ipos);
	void remove_vehicles_to_end(convoihandle_t cnv, int ipos);

	/**
	 * Access to vehicles not bound to a convoi. They are not ordered
	 * in any way.
	 */
	unsigned vehicle_count() const { return vehicles.get_count(); }
	slist_tpl<vehicle_t*> & get_vehicle_list() { return vehicles; }

	/**
	 * A new vehicle is bought and added to the vehicle list.
	 * The new vehicle in the list is returned.
	 */
	vehicle_t* buy_vehicle(const vehicle_desc_t* info, uint16 livery_scheme_index);

	// This upgrades a vehicle in the convoy to the type specified.
	// @author: jamespetts, February 2010
	void upgrade_vehicle(convoihandle_t cnv, const vehicle_desc_t* vb);

	/**
	 * Sell a vehicle from the vehicle list.
	 */
	void sell_vehicle(vehicle_t* veh);

	/**
	 * Access to vehicle types which can be bought in the depot.
	 */
	slist_tpl<vehicle_desc_t*> const & get_vehicle_type();

	/**
	 * Returns the waytype for a certain vehicle; only way to distinguish differnt depots ...
	 */
	virtual waytype_t get_wegtyp() const { return invalid_wt; }

	/**
	 * Parameters to determine layout and behaviour of the depot_frame_t.
	 */
	void convoi_arrived(convoihandle_t cnv, bool fpl_adjust);

	/**
	 * Opens a new info window for that object.
	 */
	void show_info() OVERRIDE;

	/**
	 * Can object be removed?
	 * @return NULL when OK, otherwise an error message
	 */
	const char * is_deletable(const player_t *player) OVERRIDE;

	/**
	 * identifies the oldest vehicle of a certain type
	 * @return NULL if no vehicle is found
	 */
	vehicle_t* get_oldest_vehicle(const vehicle_desc_t* desc);

	/**
	 * Sets/gets the line that was selected the last time in the depot dialog
	 */
	void set_last_selected_line(const linehandle_t last_line) { last_selected_line=last_line; }
	linehandle_t get_last_selected_line() const { return last_selected_line; }

	/*
	 * Find the oldest/newest vehicle in the depot
	 */
	vehicle_t* find_oldest_newest(const vehicle_desc_t* desc, bool old, vector_tpl<vehicle_t*> *avoid = NULL);

	// true if already stored here
	bool is_contained(const vehicle_desc_t *info);

	/**
	* new month
	*/
	void new_month();

	/**
	 * Will update all depot_frame_t (new vehicles!)
	 */
	static void update_all_win();

	/**
	 * Update the depot_frame_t.
	 */
	void update_win();

	// Helper function
	inline unsigned get_max_convoi_length() const { return get_max_convoy_length(get_wegtyp()); }

	void add_to_world_list(bool lock = false);

private:
	linehandle_t last_selected_line;

	/**
	 * Used to block new actions from depot frame gui when convois are being added to the depot.
	 * Otherwise lag in multipler results in actions being performed on the wrong convoi.
	 * Only works for a single client making changes in a depot at once. Multiple clients can still result in wrong convois being changed.
	 */
	bool command_pending;

public:
	bool is_command_pending() const { return command_pending; }
	void clear_command_pending() { command_pending = false; }
	void set_command_pending() { command_pending = true; }
};


/**
 * Depots for train vehicles.
 * @see depot_t
 * @see gebaeude_t
 */
class bahndepot_t : public depot_t
{
#ifdef INLINE_OBJ_TYPE
protected:
	bahndepot_t(obj_t::typ type, loadsave_t *file) : depot_t(type, file) {}
	bahndepot_t(obj_t::typ type, koord3d pos,player_t *player, const building_tile_desc_t *t) : depot_t(type, pos, player, t) {}
public:
	bahndepot_t(loadsave_t *file) : depot_t(bahndepot, file) {}
	bahndepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t) : depot_t(bahndepot, pos, player, t) {}
#else
public:
	bahndepot_t(loadsave_t *file) : depot_t(file) {}
	bahndepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t) : depot_t(pos,player,t) {}
#endif

	simline_t::linetype get_line_type() const OVERRIDE { return simline_t::trainline; }

	void rdwr_vehicles(loadsave_t *file) { depot_t::rdwr_vehicle(vehicles,file); }

	waytype_t get_wegtyp() const OVERRIDE {return track_wt;}
#ifdef INLINE_OBJ_TYPE
#else
	virtual ding_t::typ get_typ() const {return bahndepot;}
#endif
	///**
	// * Parameters to determine layout and behaviour of the depot_frame_t.
	// */
	//int get_x_placement() const {return -25; }
	//int get_y_placement() const {return -28; }
	//int get_x_grid() const { return 24; }
	//int get_y_grid() const { return 24; }
	//unsigned get_max_convoi_length() const;

	//virtual const char *get_name() const {return "Bahndepot"; }
};


class tramdepot_t : public bahndepot_t
{
public:
#ifdef INLINE_OBJ_TYPE
	tramdepot_t(loadsave_t *file):bahndepot_t(tramdepot, file) {}
	tramdepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t): bahndepot_t(tramdepot, pos, player, t) {}
#else
	tramdepot_t(loadsave_t *file):bahndepot_t(file) {}
	tramdepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t): bahndepot_t(pos,player,t) {}
#endif

	simline_t::linetype get_line_type() const OVERRIDE { return simline_t::tramline; }

	waytype_t get_wegtyp() const OVERRIDE {return tram_wt;}
#ifdef INLINE_OBJ_TYPE
#else
	obj_t::typ get_typ() const OVERRIDE { return tramdepot; }
#endif
	//const char *get_name() const OVERRIDE {return "Tramdepot"; }
};

class monoraildepot_t : public bahndepot_t
{
public:
#ifdef INLINE_OBJ_TYPE
	monoraildepot_t(loadsave_t *file):bahndepot_t(monoraildepot, file) {}
	monoraildepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t): bahndepot_t(monoraildepot, pos, player, t) {}
#else
	monoraildepot_t(loadsave_t *file):bahndepot_t(file) {}
	monoraildepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t): bahndepot_t(pos,player,t) {}
#endif

	simline_t::linetype get_line_type() const OVERRIDE { return simline_t::monorailline; }

	waytype_t get_wegtyp() const OVERRIDE {return monorail_wt;}
#ifdef INLINE_OBJ_TYPE
#else
	obj_t::typ get_typ() const OVERRIDE { return monoraildepot; }
#endif
	//const char *get_name() const OVERRIDE {return "Monoraildepot"; }
};

class maglevdepot_t : public bahndepot_t
{
public:
#ifdef INLINE_OBJ_TYPE
	maglevdepot_t(loadsave_t *file):bahndepot_t(maglevdepot, file) {}
	maglevdepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t): bahndepot_t(maglevdepot, pos, player, t) {}
#else
	maglevdepot_t(loadsave_t *file):bahndepot_t(file) {}
	maglevdepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t): bahndepot_t(pos,player,t) {}
#endif

	simline_t::linetype get_line_type() const OVERRIDE { return simline_t::maglevline; }

	waytype_t get_wegtyp() const OVERRIDE {return maglev_wt;}
#ifdef INLINE_OBJ_TYPE
#else
	obj_t::typ get_typ() const OVERRIDE { return maglevdepot; }
#endif
	//const char *get_name() const OVERRIDE {return "Maglevdepot"; }
};

class narrowgaugedepot_t : public bahndepot_t
{
public:
#ifdef INLINE_OBJ_TYPE
	narrowgaugedepot_t(loadsave_t *file):bahndepot_t(narrowgaugedepot, file) {}
	narrowgaugedepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t): bahndepot_t(narrowgaugedepot, pos, player, t) {}
#else
	narrowgaugedepot_t(loadsave_t *file):bahndepot_t(file) {}
	narrowgaugedepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t): bahndepot_t(pos,player,t) {}
#endif

	simline_t::linetype get_line_type() const OVERRIDE { return simline_t::narrowgaugeline; }

	waytype_t get_wegtyp() const OVERRIDE {return narrowgauge_wt;}
#ifdef INLINE_OBJ_TYPE
#else
	obj_t::typ get_typ() const OVERRIDE { return narrowgaugedepot; }
#endif
	//virtual const char *get_name() const {return "Narrowgaugedepot"; }
	//const char *get_name() const OVERRIDE {return "Narrowgaugedepot"; }
};

/**
 * Depots for street vehicles
 * @see depot_t
 * @see gebaeude_t
 */
class strassendepot_t : public depot_t
{
public:
#ifdef INLINE_OBJ_TYPE
	strassendepot_t(loadsave_t *file) : depot_t(strassendepot, file) {}
	strassendepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t) : depot_t(strassendepot, pos, player, t) {}
#else
	strassendepot_t(loadsave_t *file) : depot_t(file) {}
	strassendepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t) : depot_t(pos,player,t) {}
#endif

	simline_t::linetype get_line_type() const OVERRIDE { return simline_t::truckline; }

	waytype_t get_wegtyp() const OVERRIDE {return road_wt; }
#ifdef INLINE_OBJ_TYPE
#else
	obj_t::typ get_typ() const OVERRIDE { return strassendepot; }
#endif
	///**
	// * Parameters to determine layout and behaviour of the depot_frame_t.
	// */
	//int get_x_placement() const { return -20; }
	//int get_y_placement() const { return -25; }
	//int get_x_grid() const { return 24; }
	//int get_y_grid() const { return 24; }
	//unsigned get_max_convoi_length() const { return 4; }

	//const char *get_name() const OVERRIDE {return "Strassendepot";}
};


/**
 * Depots for ships
 * @see depot_t
 * @see gebaeude_t
 */
class schiffdepot_t : public depot_t
{
public:
#ifdef INLINE_OBJ_TYPE
	schiffdepot_t(loadsave_t *file) : depot_t(schiffdepot, file) {}
	schiffdepot_t(koord3d pos, player_t *player, const building_tile_desc_t *t) : depot_t(schiffdepot, pos, player, t) {}
#else
	schiffdepot_t(loadsave_t *file) : depot_t(file) {}
	schiffdepot_t(koord3d pos, player_t *player, const building_tile_desc_t *t) : depot_t(pos,player,t) {}
#endif

	simline_t::linetype get_line_type() const OVERRIDE { return simline_t::shipline; }

	waytype_t get_wegtyp() const OVERRIDE {return water_wt; }
#ifdef INLINE_OBJ_TYPE
#else
	obj_t::typ get_typ() const OVERRIDE { return schiffdepot; }
#endif
	///**
	// * Parameters to determine layout and behaviour of the depot_frame_t.
	// */
	//int get_x_placement() const { return -1; }
	//int get_y_placement() const { return -11; }
	//int get_x_grid() const { return 60; }
	//int get_y_grid() const { return 46; }

	//unsigned get_max_convoi_length() const { return 4; }
	//const char *get_name() const OVERRIDE {return "Schiffdepot";}
};


/**
 * Depots for aircrafts
 */
class airdepot_t : public depot_t
{
public:
#ifdef INLINE_OBJ_TYPE
	airdepot_t(loadsave_t *file) : depot_t(airdepot, file) {}
	airdepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t) : depot_t(airdepot, pos, player, t) {}
#else
	airdepot_t(loadsave_t *file) : depot_t(file) {}
	airdepot_t(koord3d pos,player_t *player, const building_tile_desc_t *t) : depot_t(pos,player,t) {}
#endif

	simline_t::linetype get_line_type() const OVERRIDE { return simline_t::airline; }

	waytype_t get_wegtyp() const OVERRIDE { return air_wt; }
	///**
	// * Parameters to determine layout and behaviour of the depot_frame_t.
	// */
	//int get_x_placement() const {return -10; }
	//int get_y_placement() const {return -23; }
	//int get_x_grid() const { return 36; }
	//int get_y_grid() const { return 36; }
	//unsigned get_max_convoi_length() const { return 1; }

#ifdef INLINE_OBJ_TYPE
#else
	obj_t::typ get_typ() const OVERRIDE { return airdepot; }
#endif
	//const char *get_name() const OVERRIDE {return "Hangar";}
};

#endif
