/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef _simdepot_h
#define _simdepot_h

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
 * @author Hj. Malthaner
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
	 *
	 * @author Volker Meyer
	 * @date  30.05.2003
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

	void rdwr(loadsave_t *file);

	// text for the tabs is defaulted to the train names
	virtual const char * get_electrics_name() { return "Electrics_tab"; };
	virtual const char * get_passenger_name() { return "Pas_tab"; }
	virtual const char * get_zieher_name() { return "Lokomotive_tab"; }
	virtual const char * get_haenger_name() { return "Waggon_tab"; }
	virtual const char * get_passenger2_name() { return "Pas2_tab"; }

	/**
	 * Access to convoi list.
	 * @author Volker Meyer
	 * @date  30.05.2003
	 */
	unsigned convoi_count() const { return convois.get_count(); }

	convoihandle_t get_convoi(unsigned int icnv) const { return icnv < convoi_count() ? convois.at(icnv) : convoihandle_t(); }

	convoihandle_t add_convoi(bool local_execution);

	slist_tpl<convoihandle_t> const& get_convoy_list() { return convois; }

	// checks if cnv can be copied by using only stored vehicles and non-obsolete purchased vehicles
	bool check_obsolete_inventory(convoihandle_t cnv);

	/**
	 * copies convoi and its schedule or line
	 * @author hsiegeln
	 */
	convoihandle_t copy_convoi(convoihandle_t old_cnv, bool local_execution);

	/**
	 * Let convoi leave the depot.
	 * If not possible, a message is displayed and the function returns false.
	 * @param if local_execution is true, this method creates pop-ups in case of errors
	 * @author Volker Meyer, Dwachs
	 * @date  09.06.2003 / 27.04.2010
	 */
	bool start_convoi(convoihandle_t cnv, bool local_execution);

	bool start_all_convoys();

	/**
	 * Destroy the convoi and put the vehicles in the vehicles list (sell==false),
	 * or sell all immediately (sell==true).
	 * @author Volker Meyer
	 * @date  09.06.2003
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
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void append_vehicle(convoihandle_t &cnv, vehicle_t* veh, bool infront, bool local_execution);

	/**
	 * Remove the vehicle at given position from the convoi and put it in the
	 * vehicle list.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void remove_vehicle(convoihandle_t cnv, int ipos);
	void remove_vehicles_to_end(convoihandle_t cnv, int ipos);

	/**
	 * Access to vehicles not bound to a convoi. They are not ordered
	 * in any way.
	 * @author Volker Meyer
	 * @date  30.05.2003
	 */
	unsigned vehicle_count() const { return vehicles.get_count(); }
	slist_tpl<vehicle_t*> & get_vehicle_list() { return vehicles; }

	/**
	 * A new vehicle is bought and added to the vehicle list.
	 * The new vehicle in the list is returned.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	vehicle_t* buy_vehicle(const vehicle_desc_t* info, uint16 livery_scheme_index);

	// This upgrades a vehicle in the convoy to the type specified.
	// @author: jamespetts, February 2010
	void upgrade_vehicle(convoihandle_t cnv, const vehicle_desc_t* vb);

	/**
	 * Sell a vehicle from the vehicle list.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void sell_vehicle(vehicle_t* veh);

	/**
	 * Access to vehicle types which can be bought in the depot.
	 * @author Volker Meyer
	 */
	slist_tpl<vehicle_desc_t*> const & get_vehicle_type();

	/**
	 * Returns the waytype for a certain vehicle; only way to distinguish differnt depots ...
	 * @author prissi
	 */
	virtual waytype_t get_wegtyp() const { return invalid_wt; }

	/**
	 * A convoi arrived at the depot and is added to the convoi list.
	 * If fpl_adjust is true, the current depot is removed from schedule.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void convoi_arrived(convoihandle_t cnv, bool fpl_adjust);

	/**
	 * Öffnet ein neues Beobachtungsfenster für das Objekt.
	 * @author Hj. Malthaner
	 */
	void show_info();

	/**
	 * Can object be removed?
	 * @return NULL when OK, otherwise an error message
	 * @author Hj. Malthaner
	 */
	virtual const char *  is_deletable(const player_t *player);

	/**
	 * identifies the oldest vehicle of a certain type
	 * @return NULL if no vehicle is found
	 * @author hsiegeln (stolen from Hajo)
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
	* @author Bernd Gabriel
	* @date 26.06.2009
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
 *
 * @author Hj. Malthaner
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

	virtual simline_t::linetype get_line_type() const { return simline_t::trainline; }

	void rdwr_vehicles(loadsave_t *file) { depot_t::rdwr_vehicle(vehicles,file); }

	virtual waytype_t get_wegtyp() const {return track_wt;}
#ifdef INLINE_OBJ_TYPE
#else
	virtual ding_t::typ get_typ() const {return bahndepot;}
#endif
	///**
	// * Parameters to determine layout and behaviour of the depot_frame_t.
	// * @author Volker Meyer
	// * @date  09.06.2003
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

	virtual simline_t::linetype get_line_type() const { return simline_t::tramline; }

	virtual waytype_t get_wegtyp() const {return tram_wt;}
#ifdef INLINE_OBJ_TYPE
#else
	virtual obj_t::typ get_typ() const { return tramdepot; }
#endif
	//virtual const char *get_name() const {return "Tramdepot"; }
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

	virtual simline_t::linetype get_line_type() const { return simline_t::monorailline; }

	virtual waytype_t get_wegtyp() const {return monorail_wt;}
#ifdef INLINE_OBJ_TYPE
#else
	virtual obj_t::typ get_typ() const { return monoraildepot; }
#endif
	//virtual const char *get_name() const {return "Monoraildepot"; }
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

	virtual simline_t::linetype get_line_type() const { return simline_t::maglevline; }

	virtual waytype_t get_wegtyp() const {return maglev_wt;}
#ifdef INLINE_OBJ_TYPE
#else
	virtual obj_t::typ get_typ() const { return maglevdepot; }
#endif
	//virtual const char *get_name() const {return "Maglevdepot"; }
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

	virtual simline_t::linetype get_line_type() const { return simline_t::narrowgaugeline; }

	virtual waytype_t get_wegtyp() const {return narrowgauge_wt;}
#ifdef INLINE_OBJ_TYPE
#else
	virtual obj_t::typ get_typ() const { return narrowgaugedepot; }
#endif
	//virtual const char *get_name() const {return "Narrowgaugedepot"; }
};

/**
 * Depots for street vehicles
 *
 * @author Hj. Malthaner
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

	virtual simline_t::linetype get_line_type() const { return simline_t::truckline; }

	virtual waytype_t get_wegtyp() const {return road_wt; }
#ifdef INLINE_OBJ_TYPE
#else
	obj_t::typ get_typ() const {return strassendepot;}
#endif
	///**
	// * Parameters to determine layout and behaviour of the depot_frame_t.
	// * @author Volker Meyer
	// * @date  09.06.2003
	// */
	//int get_x_placement() const { return -20; }
	//int get_y_placement() const { return -25; }
	//int get_x_grid() const { return 24; }
	//int get_y_grid() const { return 24; }
	//unsigned get_max_convoi_length() const { return 4; }

	//const char *get_name() const {return "Strassendepot";}
};


/**
 * Depots for ships
 *
 * @author Hj. Malthaner
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

	virtual simline_t::linetype get_line_type() const { return simline_t::shipline; }

	virtual waytype_t get_wegtyp() const {return water_wt; }
#ifdef INLINE_OBJ_TYPE
#else
	obj_t::typ get_typ() const {return schiffdepot;}
#endif
	///**
	// * Parameters to determine layout and behaviour of the depot_frame_t.
	// * @author Volker Meyer
	// * @date  09.06.2003
	// */
	//int get_x_placement() const { return -1; }
	//int get_y_placement() const { return -11; }
	//int get_x_grid() const { return 60; }
	//int get_y_grid() const { return 46; }

	//unsigned get_max_convoi_length() const { return 4; }
	//const char *get_name() const {return "Schiffdepot";}
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

	virtual simline_t::linetype get_line_type() const { return simline_t::airline; }

	virtual waytype_t get_wegtyp() const { return air_wt; }
	///**
	// * Parameters to determine layout and behaviour of the depot_frame_t.
	// * @author Volker Meyer
	// * @date  09.06.2003
	// */
	//int get_x_placement() const {return -10; }
	//int get_y_placement() const {return -23; }
	//int get_x_grid() const { return 36; }
	//int get_y_grid() const { return 36; }
	//unsigned get_max_convoi_length() const { return 1; }

#ifdef INLINE_OBJ_TYPE
#else
	obj_t::typ get_typ() const { return airdepot; }
#endif
	//const char *get_name() const {return "Hangar";}
};

#endif
