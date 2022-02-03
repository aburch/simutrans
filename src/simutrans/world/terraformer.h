/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef WORLD_TERRAFORMER_H
#define WORLD_TERRAFORMER_H


#include "../simtypes.h"
#include "../tpl/vector_tpl.h"


class karte_t;
class player_t;


/**
 * Class to manage terraform operations.
 * Can be used for raise only or lower only operations, but not mixed.
 *
 * Usual order of calls:
 * 1. add_node()                        (can be repeated)
 * 2. generate_affected_tile_list()
 * 3. can_raise_all() / can_lower_all()
 * 4. apply()                           (to actually apply the changes to the terrain; optional)
 */
class terraformer_t
{
public:
	enum operation_t
	{
		lower = 0,
		raise = 1
	};

private:
	/// Structure to save terraforming operations
	struct node_t
	{
	public:
		node_t();
		node_t(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw, uint8 c);

	public:
		/// compares position
		bool operator==(const node_t &a) const;

		/// compares position
		static bool comp(const node_t &a, const node_t &b);

	public:
		sint16 x;    ///< x-coordinate
		sint16 y;    ///< y-coordinate

		sint8 hsw, hse, hne, hnw;

		uint8 changed;
	};

public:
	terraformer_t(karte_t *world, terraformer_t::operation_t op);

public:
	/// Add tile to be raised/lowered.
	void add_node(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw);

	/// Generate list of all tiles that will be affected.
	void generate_affected_tile_list();

	/// Check whether raise operation would succeed
	const char *can_raise_all(const player_t *player, bool keep_water = false) const;

	/// Check whether lower operation would succeed
	const char *can_lower_all(const player_t *player) const;

	/// Do the raise/lower operations
	int apply();

private:
	/// Internal functions to be used with terraformer_t to propagate terrain changes to neighbouring tiles
	void prepare_raise(const node_t node);
	void prepare_lower(const node_t node);

	/**
	 * Checks whether the heights of the corners of the tile at (@p x, @p y) can be raised.
	 * If the desired height of a corner is lower than its current height, this corner is ignored.
	 * @param player player who wants to lower
	 * @param keep_water returns false if water tiles would be raised above water
	 * @returns NULL if raise_to operation can be performed, an error message otherwise
	 */
	const char *can_raise_tile_to(const node_t &node, const player_t *player, bool keep_water) const;

	/**
	 * Checks whether the heights of the corners of the tile at (@p x, @p y) can be lowered.
	 * If the desired height of a corner is higher than its current height, this corner is ignored.
	 * @param player player who wants to lower
	 * @returns NULL if lower_to operation can be performed, an error message otherwise
	 */
	const char *can_lower_tile_to(const node_t &node, const player_t *player) const;

	/**
	 * Raises heights of the corners of the tile at (@p x, @p y).
	 * New heights for each corner given.
	 * @pre can_raise_to should be called before this method.
	 * @see can_raise_to
	 * @returns count of full raise operations (4 corners raised one level)
	 * @note Clear tile, reset water/land type, calc minimap pixel.
	 */
	int raise_to(const node_t &node);

	/**
	 * Lowers heights of the corners of the tile at (@p x, @p y).
	 * New heights for each corner given.
	 * @pre can_lower_to should be called before this method.
	 * @see can_lower_to
	 * @returns count of full lower operations (4 corners lowered one level)
	 * @note Clear tile, reset water/land type, calc minimap pixel.
	 */
	int lower_to(const node_t &node);

	/**
	 * Checks if the planquadrat (tile) at coordinate (x,y)
	 * can be lowered at the specified height.
	 */
	const char *can_lower_plan_to(const player_t *player, sint16 x, sint16 y, sint8 h) const;

	/**
	 * Checks if the planquadrat (tile) at coordinate (x,y)
	 * can be raised at the specified height.
	 */
	const char *can_raise_plan_to(const player_t *player, sint16 x, sint16 y, sint8 h) const;

private:
	vector_tpl<node_t> list; ///< list of affected tiles
	uint8 actual_flag;       ///< internal flag to iterate through list
	bool ready;              ///< internal flag to signal that the affected tile list is updated
	operation_t op;          ///< Raise or lower?
	karte_t *welt;
};


#endif
