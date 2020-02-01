/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef boden_wege_weg_h
#define boden_wege_weg_h

#include "../../display/simimg.h"
#include "../../simtypes.h"
#include "../../simobj.h"
#include "../../descriptor/way_desc.h"
#include "../../dataobj/koord3d.h"
#include "../../tpl/minivec_tpl.h"
#include "../../tpl/koordhashtable_tpl.h"
#include "../../simskin.h"

class karte_t;
class way_desc_t;
class cbuffer_t;
class player_t;
class signal_t;
class gebaeude_t;
class stadt_t;
template <class T> class vector_tpl;


// maximum number of months to store information
#define MAX_WAY_STAT_MONTHS 2

enum way_statistics {
	WAY_STAT_GOODS,		///< number of goods transported over this way
	WAY_STAT_CONVOIS,	///< number of convois that passed this way
	WAY_STAT_WAITING,	///< Number of vehicles waiting in a traffic jam on this way
	MAX_WAY_STATISTICS
};


/**
 * <p>Der Weg ist die Basisklasse fuer all Verkehrswege in Simutrans.
 * Wege "gehören" immer zu einem Grund. Sie besitzen Richtungsbits sowie
 * eine Maske fuer Richtungsbits.</p>
 *
 * <p>Ein Weg gehört immer zu genau einer Wegsorte</p>
 *
 * <p>Kreuzungen werden dadurch unterstützt, daß ein Grund zwei Wege
 * enthalten kann (prinzipiell auch mehrere möglich.</p>
 *
 * <p>Wetype -1 ist reserviert und kann nicht für Wege benutzt werden<p>
 *
 * @author Hj. Malthaner
 */
class weg_t : public obj_no_info_t
{
public:
	/**
	* Get list of all ways
	* @author Hj. Malthaner
	*/
	static const vector_tpl <weg_t *> & get_alle_wege();
	static const uint32 get_all_ways_count();
	static void clear_list_of__ways();

	enum {
		HAS_SIDEWALK   = 0x01,
		IS_ELECTRIFIED = 0x02,
		HAS_SIGN       = 0x04,
		HAS_SIGNAL     = 0x08,
		HAS_WAYOBJ     = 0x10,
		HAS_CROSSING   = 0x20,
		IS_DIAGONAL    = 0x40, // marker for diagonal image
		IS_SNOW = 0x80	// marker, if above snowline currently
	};

	struct runway_directions
	{
		bool runway_36_18;
		bool runway_9_27;

		runway_directions(bool run_36_18, bool run_9_27)
		{
			runway_36_18 = run_36_18;
			runway_9_27 = run_9_27;
		}
	};

private:
	/**
	* array for statistical values
	* MAX_WAY_STAT_MONTHS: [0] = actual value; [1] = last month value
	* MAX_WAY_STATISTICS: see #define at top of file
	* @author hsiegeln
	*/
	sint16 statistics[MAX_WAY_STAT_MONTHS][MAX_WAY_STATISTICS];

	/**
	* Way type description
	* @author Hj. Malthaner
	*/
	const way_desc_t * desc;

	/**
	* Richtungsbits für den Weg. Norden ist oben rechts auf dem Monitor.
	* 1=Nord, 2=Ost, 4=Sued, 8=West
	* @author Hj. Malthaner
	*/
	uint8 ribi:4;

	/**
	* ask for ribi (Richtungsbits => Direction Bits)
	* @author Hj. Malthaner
	*/
	uint8 ribi_maske:4;

	/**
	* flags like walkway, electrification, road sings
	* @author Hj. Malthaner
	*/
	uint8 flags;

	/**
	* max speed; could not be taken for desc, since other object may modify the speed
	* @author Hj. Malthaner
	*/
	uint16 max_speed;

	/**
	* Likewise for weight
	* @author: jamespetts
	*/
	uint32 max_axle_load;

	uint32 bridge_weight_limit;

	image_id image;
	image_id foreground_image;

	/**
	* Initializes all member variables
	* @author Hj. Malthaner
	*/
	void init();

	/**
	* initializes statistic array
	* @author hsiegeln
	*/
	void init_statistics();

	/*
	 * Way constraints for, e.g., loading gauges, types of electrification, etc.
	 * @author: jamespetts (modified by Bernd Gabriel)
	 */
	way_constraints_of_way_t way_constraints;

	// BG, 24.02.2012 performance enhancement avoid virtual method call, use inlined get_waytype()
	waytype_t    wtyp;


	/* These are statistics showing when this way was last built and when it was last renewed.
	 * @author: jamespetts
	 */
	uint16 creation_month_year;
	uint16 last_renewal_month_year;

	/* This figure gives the condition of the way: UINT32_MAX = new; 0 = unusable.
	 * @author: jamespetts
	 */
	uint32 remaining_wear_capacity;

	/*
	* If this flag is true, players may not delete this way even if it is unowned unless they
	* build a diversionary route. Makes the way usable by all players regardless of ownership
	* and access settings. Permits upgrades but not downgrades, and prohibits private road signs.
	* @author: jamespetts
	*/
	bool public_right_of_way:1;

	// Whether the way is in a degraded state.
	bool degraded:1;

protected:

	enum image_type { image_flat, image_slope, image_diagonal, image_switch };

	/**
	 * initializes both front and back images
	 * switch images are set in schiene_t::reserve
	 */
	void set_images(image_type typ, uint8 ribi, bool snow, bool switch_nw=false);


	/* This is the way with which this way will be replaced when it comes time for renewal.
	 * NULL = do not replace.
	 * @author: jamespetts
	 */
	const way_desc_t *replacement_way;

public:

	/*
	 * Degrade the way owing to excessive wear without renewal.
	 */
	void degrade();

	inline weg_t(waytype_t waytype, loadsave_t*) : obj_no_info_t(obj_t::way), wtyp(waytype) { init(); }
	inline weg_t(waytype_t waytype) : obj_no_info_t(obj_t::way), wtyp(waytype) { init(); }

	// This was in strasse_t, but being there possibly caused heap corruption.
	minivec_tpl<gebaeude_t*> connected_buildings;
	
	// Likewise, out of caution, put this here for the same reason.
	typedef koordhashtable_tpl<koord, koord3d> private_car_route_map;
	private_car_route_map private_car_routes;

	virtual ~weg_t();

#ifdef MULTI_THREAD
	void lock_mutex();
	void unlock_mutex();
#endif

	/**
	 * Actual image recalculation
	 */
	void calc_image();

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool calc_only_season_change);

	/**
	* Setzt die erlaubte Höchstgeschwindigkeit
	* @author Hj. Malthaner
	*/
	void set_max_speed(sint32 s) { max_speed = s; }

	void set_max_axle_load(uint32 w) { max_axle_load = w; }
	void set_bridge_weight_limit(uint32 value) { bridge_weight_limit = value; }

	// Resets constraints to their base values. Used when removing way objects.
	void reset_way_constraints() { way_constraints = desc->get_way_constraints(); }

	void clear_way_constraints() { way_constraints.set_permissive(0); way_constraints.set_prohibitive(0); }

	/* Way constraints: determines whether vehicles
	 * can travel on this way. This method decodes
	 * the byte into bool values. See here for
	 * information on bitwise operations:
	 * http://www.cprogramming.com/tutorial/bitwise_operators.html
	 * @author: jamespetts
	 * */

	const way_constraints_of_way_t& get_way_constraints() const { return way_constraints; }
	void add_way_constraints(const way_constraints_of_way_t& value) { way_constraints.add(value); }
	void remove_way_constraints(const way_constraints_of_way_t& value) { way_constraints.remove(value); }

	/**
	* Ermittelt die erlaubte Höchstgeschwindigkeit
	* @author Hj. Malthaner
	*/
	sint32 get_max_speed() const { return max_speed; }

	uint32 get_max_axle_load() const { return max_axle_load; }
	uint32 get_bridge_weight_limit() const { return bridge_weight_limit; }

	/**
	* Setzt neue Description. Ersetzt alte Höchstgeschwindigkeit
	* mit wert aus Description.
	*
	* Sets a new description. Replaces old with maximum speed
	* worth of description and updates the maintenance cost.
	* @author Hj. Malthaner
	*/
	void set_desc(const way_desc_t *b, bool from_saved_game = false);
	const way_desc_t *get_desc() const { return desc; }

	// returns a way with the matching type
	static weg_t *alloc(waytype_t wt);

	// returns a string with the "official name of the waytype"
	static const char *waytype_to_string(waytype_t wt);

	virtual void rdwr(loadsave_t *file);

	/**
	* Info-text for this way
	* @author Hj. Malthaner
	*/
	virtual void info(cbuffer_t & buf, bool is_bridge = false) const;

	/**
	 * @return NULL if OK, otherwise an error message
	 * @author Hj. Malthaner
	 */
	virtual const char * is_deletable(const player_t *player, bool allow_public = false);

	/**
	* Wetype zurückliefern
	*/
	waytype_t get_waytype() const { return wtyp; }

	inline bool is_rail_type() const { return wtyp == track_wt || wtyp == maglev_wt || wtyp == tram_wt || wtyp == narrowgauge_wt || wtyp == monorail_wt;  }

	/**
	* 'Jedes Ding braucht einen Typ.'
	* @return the object type.
	* @author Hj. Malthaner
	*/
	//typ get_typ() const { return obj_t::way; }

	/**
	* Die Bezeichnung des Wegs
	* @author Hj. Malthaner
	*/
	const char *get_name() const { return desc->get_name(); }

	/**
	* Add direction bits (ribi) for a way.
	*
	* Nachdem die ribis geändert werden, ist das weg_image des
	* zugehörigen Grundes falsch (Ein Aufruf von grund_t::calc_image()
	* zur Reparatur muß folgen).
	* @param ribi Richtungsbits
	*/
	void ribi_add(ribi_t::ribi ribi) { this->ribi |= (uint8)ribi;}

	/**
	* Remove direction bits (ribi) on a way.
	*
	* Nachdem die ribis geändert werden, ist das weg_image des
	* zugehörigen Grundes falsch (Ein Aufruf von grund_t::calc_image()
	* zur Reparatur muß folgen).
	* @param ribi Richtungsbits
	*/
	void ribi_rem(ribi_t::ribi ribi) { this->ribi &= (uint8)~ribi;}

	/**
	* Set direction bits (ribi) for the way.
	*
	* Nachdem die ribis geändert werden, ist das weg_image des
	* zugehörigen Grundes falsch (Ein Aufruf von grund_t::calc_image()
	* zur Reparatur muß folgen).
	* @param ribi Richtungsbits
	*/
	void set_ribi(ribi_t::ribi ribi) { this->ribi = (uint8)ribi;}

	/**
	* Get the unmasked direction bits (ribi) for the way (without signals or other ribi changer).
	*/
	ribi_t::ribi get_ribi_unmasked() const { return (ribi_t::ribi)ribi; }

	/**
	* Get the masked direction bits (ribi) for the way (with signals or other ribi changer).
	*/
	virtual ribi_t::ribi get_ribi() const { return (ribi_t::ribi)(ribi & ~ribi_maske); }

	/**
	* für Signale ist es notwendig, bestimmte Richtungsbits auszumaskieren
	* damit Fahrzeuge nicht "von hinten" über Ampeln fahren können.
	* @param ribi Richtungsbits
	*/
	void set_ribi_maske(ribi_t::ribi ribi) { ribi_maske = (uint8)ribi; }
	ribi_t::ribi get_ribi_maske() const { return (ribi_t::ribi)ribi_maske; }

	/**
	 * called during map rotation
	 * @author priss
	 */
	virtual void rotate90();

	/**
	* book statistics - is called very often and therefore inline
	* @author hsiegeln
	*/
	void book(int amount, way_statistics type) { statistics[0][type] += amount; }

	/**
	* return statistics value
	* always returns last month's value
	* @author hsiegeln
	*/
	int get_statistics(int type) const { return statistics[1][type]; }

	/**
	* new month
	* @author hsiegeln
	*/
	void new_month();

	void check_diagonal();

	void count_sign();

	/* flag query routines */
	void set_gehweg(const bool yesno) { flags = (yesno ? flags | HAS_SIDEWALK : flags & ~HAS_SIDEWALK); }
	inline bool hat_gehweg() const { return flags & HAS_SIDEWALK; }

	void set_electrify(bool janein) {janein ? flags |= IS_ELECTRIFIED : flags &= ~IS_ELECTRIFIED;}
	inline bool is_electrified() const {return flags&IS_ELECTRIFIED; }

	inline bool has_sign() const {return flags&HAS_SIGN; }
	inline bool has_signal() const {return flags&HAS_SIGNAL; }
	inline bool has_wayobj() const {return flags&HAS_WAYOBJ; }
	inline bool is_crossing() const {return flags&HAS_CROSSING; }
	inline bool is_diagonal() const {return flags&IS_DIAGONAL; }
	inline bool is_snow() const {return flags&IS_SNOW; }

	// this is needed during a change from crossing to tram track
	void clear_crossing() { flags &= ~HAS_CROSSING; }

	/**
	 * Clear the has-sign flag when roadsign or signal got deleted.
	 * As there is only one of signal or roadsign on the way we can safely clear both flags.
	 */
	void clear_sign_flag() { flags &= ~(HAS_SIGN | HAS_SIGNAL); }

	inline void set_image( image_id b ) { image = b; }
	image_id get_image() const {return image;}

	inline void set_after_image( image_id b ) { foreground_image = b; }
	virtual image_id get_front_image() const { return foreground_image; }

	// correct maintenance
	void finish_rd();

	// Should a city adopt this, if it is being built/upgrade by player player?
	bool should_city_adopt_this(const player_t* player);

	bool is_public_right_of_way() const { return public_right_of_way; }
	void set_public_right_of_way(bool arg=true) { public_right_of_way = arg; }

	bool is_degraded() const { return degraded; }

	uint16 get_creation_month_year() const { return creation_month_year; }
	uint16 get_last_renewal_monty_year() const { return last_renewal_month_year; }

	uint32 get_remaining_wear_capacity() const { return remaining_wear_capacity; }
	uint32 get_condition_percent() const;

	bool is_height_restricted() const;

	/**
	 * Called by a convoy or a city car when it passes over a way
	 * to cause the way to be subject to the specified amount
	 * of wear, denominated in Standard Axles (8t) * 10,000.
	 */
	void wear_way(uint32 wear);

	void set_replacement_way(const way_desc_t* replacement) { replacement_way = replacement; }
	const way_desc_t* get_replacement_way() const { return replacement_way; }

	/**
	 * Renew the way automatically when it is worn out.
	 */
	bool renew();

	signal_t* get_signal(ribi_t::ribi direction_of_travel) const;

	bool is_junction() const { return ribi_t::is_threeway(get_ribi_unmasked()); }

	runway_directions get_runway_directions() const;
	uint32 get_runway_length(bool is_36_18) const; 

	void increment_traffic_stopped_counter() { statistics[0][WAY_STAT_WAITING] ++; }
	uint32 get_congestion_percentage() const { return statistics[0][WAY_STAT_CONVOIS] + statistics[1][WAY_STAT_CONVOIS] ? ((statistics[0][WAY_STAT_WAITING] + statistics[1][WAY_STAT_WAITING]) * 100) / (statistics[0][WAY_STAT_CONVOIS] + statistics[1][WAY_STAT_CONVOIS]) : 0; }

} GCC_PACKED;

#endif
