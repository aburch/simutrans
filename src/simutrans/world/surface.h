/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef WORLD_SURFACE_H
#define WORLD_SURFACE_H


#include "simplan.h"

#include "../dataobj/koord.h"
#include "../dataobj/koord3d.h"
#include "../tpl/array2d_tpl.h"


class grund_t;
class player_t;


/// Contains all in-game objects like terrain, buildings and vehicles.
/// Provides low-level access to the in-game world (e.g. for terraforming or map height lookup)
/// Most functions come in two variants: a normal one and a *_nocheck one.
/// The *_nocheck variant should be used if the 2d coordinate is already known to be valid.
class surface_t
{
protected:
	/**
	 * For performance reasons we have the map grid size cached locally, comes from the environment (Einstellungen)
	 * @brief Cached map grid size.
	 * @note Valid grid coords are within (0..x,0..y)
	 */
	koord cached_grid_size;

	/**
	 * For performance reasons we have the map size cached locally, comes from the environment (Einstellungen).
	 * @brief Cached map tile size.
	 * @note Valid tile coords are (0..x,0..y)
	 * @note These values are one less than the size values of the grid.
	 */
	koord cached_size;

	/// @brief The maximum of cached_size.x and cached_size.y.
	/// Maximum size for waiting bars etc.
	sint16 cached_size_max;

	/// the maximum allowed world height. No tile/grid point can be higher than this limit.
	sint8 max_allowed_height;

	/// the minimum allowed world height. No tile/grid point can be lower than this limit.
	sint8 min_allowed_height;

	// @note min_allowed_height <= min_height <= groundwater < max_height <= max_allowed_height
public:
	/// cache the current maximum and minimum height on the map
	sint8 max_height, min_height;

protected:
	/// Water level height.
	sint8 groundwater;

	/// Current snow line height, changes between winter and summer snow line height.
	sint8 snowline;

	/// Current season.
	/// @note 0=winter, 1=spring, 2=summer, 3=autumn
	uint8 season;

	/**
	 * @name Map data structures
	 *       These variables represent the simulated map.
	 * @{
	 */

	/**
	 * Array containing all the map tiles.
	 * @see cached_size
	 */
	planquadrat_t *plan = NULL;

	/**
	 * Array representing the height of each point of the grid.
	 * @see cached_grid_size
	 */
	sint8 *grid_hgts = NULL;

	/**
	 * Array representing the height of water on each point of the grid.
	 * @see cached_grid_size
	 */
	sint8 *water_hgts = NULL;
	/** @} */

	/// Table for fast conversion from height to climate.
	uint8 height_to_climate[256] = {};
	uint8 num_climates_at_height[256] = {};

	/// Contains the intended climate for a tile
	/// (needed to restore tiles after height changes)
	array2d_tpl<uint8> climate_map;

	/// Contains the intended climate for a tile
	/// (needed to restore tiles after height changes)
	array2d_tpl<uint8> humidity_map;

public:
	surface_t();
	~surface_t();

public:
	/// Returns the number of tiles of the map in x and y direction.
	/// @note Valid tile coords are within (0..x-1, 0..y-1)
	inline koord get_size() const { return cached_grid_size; }

	/// Maximum size for waiting bars etc.
	inline sint16 get_size_max() const { return cached_size_max; }

	/**
	 * @return True if the specified coordinate is inside the world tiles(planquadrat_t) limits, false otherwise.
	 * @param x,y Position
	 * @note Inline because called very frequently!
	 */
	inline bool is_within_limits(sint16 x, sint16 y) const {
		// since negative values will make the whole result negative, we can use bitwise or
		// This is faster, since pentiums and other long pipeline processors do not like jumps
		// return x>=0 &&  y>=0  &&  cached_size.x>=x  &&  cached_size.y>=y;
		return (x|y|(cached_size.x-x)|(cached_size.y-y))>=0;
	}

	/**
	 * @return True if the specified coordinate is inside the world tiles(planquadrat_t) limits, false otherwise.
	 * @param k Position
	 * @note Inline because called very frequently!
	 */
	inline bool is_within_limits(koord k) const { return is_within_limits(k.x, k.y); }

	/**
	 * @return True if the specified coordinate is inside the world height grid limits, false otherwise.
	 * @param x,y Position
	 * @note Inline because called very frequently!
	 */
	inline bool is_within_grid_limits(sint16 x, sint16 y) const {
		// Since negative values will make the whole result negative, we can use bitwise or
		// This is faster, since pentiums and other long pipeline processors do not like jumps
		// return x>=0 &&  y>=0  &&  cached_grid_size.x>=x  &&  cached_grid_size.y>=y;
		return (x|y|(cached_grid_size.x-x)|(cached_grid_size.y-y))>=0;
	}

	/**
	 * @return True if the specified coordinate is inside the world height grid limits, false otherwise.
	 * @param k (x,y) coordinate.
	 * @note Inline because called very frequently!
	 */
	inline bool is_within_grid_limits(const koord &k) const { return is_within_grid_limits(k.x, k.y); }

	/**
	 * @return True if the specified coordinate is inside the world height grid limits, false otherwise.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @note Inline because called very frequently!
	 */
	inline bool is_within_grid_limits(uint16 x, uint16 y) const {
		return (x<=(unsigned)cached_grid_size.x && y<=(unsigned)cached_grid_size.y);
	}

	/// @returns the closest coordinate to @p outside_pos that is within the world
	koord get_closest_coordinate(koord outside_pos);

public:
	/**
	 * @return grund an pos/hoehe
	 * @note Inline because called very frequently!
	 */
	inline grund_t *lookup(const koord3d &pos) const
	{
		const planquadrat_t *plan = access(pos.x, pos.y);
		return plan ? plan->get_boden_in_hoehe(pos.z) : NULL;
	}

	/**
	 * @return grund an pos/hoehe
	 * @note Inline because called very frequently!
	 * For speed no checks are performed that 2d coordinates are valid
	 * (This function may still return NULL if there is no ground at pos.z)
	 */
	inline grund_t *lookup_nocheck(const koord3d &pos) const
	{
		return access_nocheck(pos.x, pos.y)->get_boden_in_hoehe(pos.z);
	}

	/**
	 * This function takes grid coordinates as a parameter and a desired height (koord3d).
	 * Will return the ground_t object that intersects with it in its northern corner if possible.
	 * If that tile doesn't exist, returns the one that intersects with it in another corner.
	 *
	 * @param pos Grid coordinates to check for, the z points to the desired height.
	 * @see lookup_kartenboden_gridcoords
	 * @see corner_to_operate
	 * @note Inline because called very frequently!
	 */
	inline grund_t *lookup_gridcoords(const koord3d &pos) const
	{
		if ( ( pos.x != cached_grid_size.x )  &&  ( pos.y != cached_grid_size.y ) ){
			return lookup(koord3d(pos.x, pos.y, pos.z));
		}

		if ( ( pos.x == cached_grid_size.x )  &&  ( pos.y == cached_grid_size.y ) ) {
			return lookup(koord3d(pos.x-1, pos.y-1, pos.z));
		}
		else if ( pos.x == cached_grid_size.x ) {
			return lookup(koord3d(pos.x-1, pos.y, pos.z));
		}
		return lookup(koord3d(pos.x, pos.y-1, pos.z));
	}

	/**
	 * This function takes 2D grid coordinates as a parameter, will return the ground_t object that
	 * intersects with it in it's north corner if possible. If that tile doesn't exist, returns
	 * the one that intersects with it in other corner.
	 * @param pos Grid coordinates to check for.
	 * @see corner_to_operate
	 * @see lookup_gridcoords
	 * @return The requested tile, or the immediately adjacent in case of lower border.
	 * @note Inline because called very frequently!
	 */
	inline grund_t *lookup_kartenboden_gridcoords(const koord &pos) const
	{
		if ( ( pos.x != cached_grid_size.x )  &&  ( pos.y != cached_grid_size.y ) ){
			return lookup_kartenboden(pos);
		}

		if ( ( pos.x == cached_grid_size.x )  &&  ( pos.y == cached_grid_size.y ) ) {
			return access(koord(pos.x-1, pos.y-1))->get_kartenboden();
		}
		else if ( pos.x == cached_grid_size.x ) {
			return access(koord(pos.x-1, pos.y))->get_kartenboden();
		}
		return access(koord(pos.x, pos.y-1))->get_kartenboden();
	}

public:
	/**
	 * @return grund at the bottom (where house will be build)
	 * @note Inline because called very frequently! - nocheck for more speed
	 */
	inline grund_t *lookup_kartenboden_nocheck(const sint16 x, const sint16 y) const
	{
		return plan[x+y*cached_grid_size.x].get_kartenboden();
	}

	inline grund_t *lookup_kartenboden_nocheck(const koord &pos) const { return lookup_kartenboden_nocheck(pos.x, pos.y); }

	/**
	 * @return grund at the bottom (where house will be build)
	 * @note Inline because called very frequently!
	 */
	inline grund_t *lookup_kartenboden(const sint16 x, const sint16 y) const
	{
		return is_within_limits(x, y) ? plan[x+y*cached_grid_size.x].get_kartenboden() : NULL;
	}

	inline grund_t *lookup_kartenboden(const koord &pos) const { return lookup_kartenboden(pos.x, pos.y); }

public:
	inline planquadrat_t *access_nocheck(sint16 x, sint16 y) const {
		return &plan[x + y*cached_grid_size.x];
	}

	inline planquadrat_t *access_nocheck(koord k) const { return access_nocheck(k.x, k.y); }

	inline planquadrat_t *access(sint16 x, sint16 y) const {
		return is_within_limits(x, y) ? &plan[x + y*cached_grid_size.x] : NULL;
	}

	inline planquadrat_t *access(koord k) const { return access(k.x, k.y); }

public:
	/**
	 * @return Height at the grid point x,y - versions without checks for speed
	 */
	inline sint8 lookup_hgt_nocheck(sint16 x, sint16 y) const {
		return grid_hgts[x + y*(cached_grid_size.x+1)];
	}

	inline sint8 lookup_hgt_nocheck(koord k) const { return lookup_hgt_nocheck(k.x, k.y); }

	/**
	 * @return Height at the grid point x,y
	 */
	inline sint8 lookup_hgt(sint16 x, sint16 y) const {
		return is_within_grid_limits(x, y) ? grid_hgts[x + y*(cached_grid_size.x+1)] : groundwater;
	}

	inline sint8 lookup_hgt(koord k) const { return lookup_hgt(k.x, k.y); }

	/**
	 * Sets grid height.
	 * Never set grid_hgts manually, always use this method!
	 */
	void set_grid_hgt_nocheck(sint16 x, sint16 y, sint8 hgt) { grid_hgts[x + y*(uint32)(cached_grid_size.x+1)] = hgt; }

	inline void set_grid_hgt_nocheck(koord k, sint8 hgt) { set_grid_hgt_nocheck(k.x, k.y, hgt); }

public:
	/// @return water height - versions without checks for speed
	inline sint8 get_water_hgt_nocheck(sint16 x, sint16 y) const {
		return water_hgts[x + y * (cached_grid_size.x)];
	}

	inline sint8 get_water_hgt_nocheck(koord k) const { return get_water_hgt_nocheck(k.x, k.y); }

	/// @return water height
	inline sint8 get_water_hgt(sint16 x, sint16 y) const {
		return is_within_limits( x, y ) ? water_hgts[x + y * (cached_grid_size.x)] : groundwater;
	}

	inline sint8 get_water_hgt(koord k) const { return get_water_hgt(k.x, k.y); }

	/// Sets water height.
	/// @param x,y tile position
	void set_water_hgt_nocheck(sint16 x, sint16 y, sint8 hgt) { water_hgts[x + y * cached_grid_size.x] = hgt; }

	inline void set_water_hgt_nocheck(koord k, sint8 hgt) { water_hgts[k.x + k.y * cached_grid_size.x] = hgt; }

public:
	/**
	 * Returns the current waterline height.
	 */
	sint8 get_groundwater() const { return groundwater; }

	/// @returns the minimum allowed height on the map.
	sint8 get_min_allowed_height() const { return min_allowed_height; }

	/// @returns the maximum allowed world height.
	sint8 get_max_allowed_height() const { return max_allowed_height; }

public:
	/// @return true, wenn Platz an Stelle @p pos mit Groesse @p dim dim Water ist
	bool is_water(koord pos, koord dim) const;

	/// @return true, if square in place (i,j) with size w, h is constructible.
	bool square_is_free(koord k, sint16 w, sint16 h, int *last_y, climate_bits cl) const;

public:
	/// @returns Minimum height of the planquadrat (tile) at @p k.
	/// For speed no checks performed that coordinates are valid
	sint8 min_hgt_nocheck(koord k) const;

	/// @return Maximum height of the planquadrats (tile) at @p k.
	/// For speed no checks performed that coordinates are valid
	sint8 max_hgt_nocheck(koord k) const;

	/// @return Minimum height of the planquadrat (tile) at @p k.
	sint8 min_hgt(koord k) const;

	/// @return Maximum height of the planquadrat (tile) at @p k.
	sint8 max_hgt(koord k) const;

public:
	void get_height_slope_from_grid(koord k, sint8 &hgt, slope_t::type &slope);

	/**
	 * Fills array with corner heights of neighbours
	 */
	void get_neighbour_heights(const koord k, sint8 neighbour_height[8][4]) const;

	//
	// Terraforming related
	//
public:
	/**
	 * Checks if the whole planquadrat (tile) at coordinates (@p x, @p y) height can
	 * be changed ( for example, water height can't be changed ).
	 */
	bool is_plan_height_changeable(sint16 x, sint16 y) const;

	/**
	 * Increases the height of the grid coordinate (@p x, @p y) by one.
	 * @param pos Grid coordinate.
	 */
	int grid_raise(const player_t *player, koord pos, const char *&err);

	/**
	 * Decreases the height of the grid coordinate (@p x, @p y) by one.
	 * @param pos Grid coordinate.
	 */
	int grid_lower(const player_t *player, koord pos, const char *&err);

	/**
	 * Raise grid point (@p x,@p y). Changes @ref grid_hgts only, used during map creation/enlargement.
	 * @see clean_up
	 */
	void raise_grid_to(sint16 x, sint16 y, sint8 h);

	/**
	 * Lower grid point (@p x,@p y). Changes @ref grid_hgts only, used during map creation/enlargement.
	 * @see clean_up
	 */
	void lower_grid_to(sint16 x, sint16 y, sint8 h);

	// mostly used by AI: Ask to flatten a tile
	bool can_flatten_tile(player_t *player, koord k, sint8 hgt, bool keep_water=false, bool make_underwater_hill=false);
	bool flatten_tile(player_t *player, koord k, sint8 hgt, bool keep_water=false, bool make_underwater_hill=false, bool justcheck=false);

public:
	/**
	 * Returns the natural slope a a position using the grid.
	 * @note No checking, and only using the grind for calculation.
	 */
	slope_t::type calc_natural_slope( const koord k ) const;

	/**
	 * @return The natural slope at a position.
	 * @note Uses the corner height for the best slope.
	 */
	slope_t::type recalc_natural_slope( const koord k, sint8 &new_height ) const;

	/**
	 * @return The corner that needs to be raised/lowered on the given coordinates.
	 * @param pos Grid coordinate to check.
	 * @note Inline because called very frequently!
	 * @note Will always return north-west except on border tiles.
	 * @pre pos has to be a valid grid coordinate, undefined otherwise.
	 */
	inline slope4_t::type get_corner_to_operate(const koord &pos) const
	{
		// Normal tile
		if ( ( pos.x != cached_grid_size.x )  &&  ( pos.y != cached_grid_size.y ) ){
			return slope4_t::corner_NW;
		}
		// Border on south-east
		if ( is_within_limits(pos.x-1, pos.y) ) {
			return(slope4_t::corner_NE);
		}
		// Border on south-west
		if ( is_within_limits(pos.x, pos.y-1) ) {
			return(slope4_t::corner_SW);
		}
		// Border on south
		return (slope4_t::corner_SE);
	}

protected:
	/**
	 * Initializes the height_to_climate field from settings.
	 */
	void init_height_to_climate();

	/// Rotates climate and water transitions for a tile
	void rotate_transitions(koord k);

public:
	/// Recalculate climate and water transitions for a tile
	void recalc_transitions(koord k);

public:
	/**
	 * Returns the climate for a given height, ruled by world creation settings.
	 * Used to determine climate when terraforming, loading old games, etc.
	 */
	climate get_climate_at_height(sint16 height) const
	{
		const sint16 h=height-groundwater;
		if(h<0) {
			return water_climate;
		}
		else if(  (uint)h >= lengthof(height_to_climate)  ) {
			return arctic_climate;
		}
		return (climate)height_to_climate[h];
	}

	/**
	 * returns the current climate for a given koordinate
	 */
	inline climate get_climate(koord k) const {
		const planquadrat_t *plan = access(k);
		return plan ? plan->get_climate() : water_climate;
	}

	/**
	 * sets the current climate for a given koordinate
	 */
	inline void set_climate(koord k, climate cl, bool recalc) {
		planquadrat_t *plan = access(k);
		if(  plan  ) {
			plan->set_climate(cl);
			if(  recalc  ) {
				recalc_transitions(k);
				for(  int i = 0;  i < 8;  i++  ) {
					recalc_transitions( k + koord::neighbours[i] );
				}
			}
		}
	}

	/// Calculates appropriate climate for a tile
	void calc_climate(koord k, bool recalc);

private:
	/**
	 * Dummy method, to generate compiler error if someone tries to call get_climate( int ),
	 * as the int parameter will silently be cast to koord...
	 */
	climate get_climate(sint16) const = delete;
};


#endif
