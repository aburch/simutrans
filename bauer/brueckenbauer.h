/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BAUER_BRUECKENBAUER_H
#define BAUER_BRUECKENBAUER_H


#include "../simtypes.h"
#include "../dataobj/koord.h"
#include "../dataobj/koord3d.h"

class bridge_desc_t;
class grund_t;
class karte_ptr_t;
class player_t;
class way_desc_t;
class tool_selector_t;



/**
 * Bridge management: builds bridges, manages list of available bridge types
 * Bridges should not be built directly but always by calling bridge_builder_t::build().
 */
class bridge_builder_t {
private:

	bridge_builder_t() {} ///< private -> no instance please

	static karte_ptr_t welt;

	static bool is_blocked(koord3d pos, ribi_t::ribi check_ribi, const char *&error_msg);
	static bool is_monorail_junction(koord3d pos, player_t *player, const bridge_desc_t *desc, const char *&error_msg);
public:
	/**
	 * Finds the position of the end of the bridge. Does all kind of checks.
	 * Uses two methods:
	 * -#  If ai_bridge==false then looks for end location at a sloped tile.
	 * -#  If ai_bridge==true returns the first location (taking min_length into account)
	 *     for the bridge end (including flat tiles).
	 * @param player active player, needed to check scenario conditions
	 * @param pos  the position of the start of the bridge
	 * @param zv   desired direction of the bridge
	 * @param desc the description of the bridge
	 * @param error_msg an error message when the search fails.
	 * @param bridge_height on success, the height of the bridge that we can build
	 * @param ai_bridge if this bridge is being built by an AI
	 * @param min_length the minimum length of the bridge.
	 * @return the position of the other end of the bridge or koord3d::invalid if no possible end is found
	 */
	static koord3d find_end_pos(player_t *player, koord3d pos, const koord zv, const bridge_desc_t *desc, const char *&error_msg, sint8 &bridge_height, bool ai_bridge=false, uint32 min_length=0, bool high_bridge = false );

	/**
	 * Checks whether given tile @p gr is suitable for placing bridge ramp.
	 *
	 * @param player the player wanting to build the  bridge.
	 * @param gr the ground to check.
	 * @param r is ribi_t::none of the direction from bridge to ground.
	 * @return true, if bridge ramp can be built here.
	 */
	static bool can_place_ramp(player_t *player, const grund_t *gr, waytype_t wt, ribi_t::ribi r );

	/**
	 * Checks if a bridge starts on @p gr
	 *
	 * @param gr the ground to check.
	 * @return true, if bridge ends/starts here
	 */
	static bool is_start_of_bridge( const grund_t *gr );

	/**
	 * Build a bridge ramp.
	 *
	 * @param player the player wanting to build the bridge
	 * @param end the position of the ramp
	 * @param desc the bridge description.
	 */
	static void build_ramp(player_t *player, koord3d end, ribi_t::ribi ribi_neu, slope_t::type weg_hang, const bridge_desc_t *desc);

	/**
	 * Actually builds the bridge without checks.
	 * Therefore checks should be done before in
	 * bridge_builder_t::build().
	 *
	 * @param player the master builder of the bridge.
	 * @param start start position.
	 * @param end end position
	 * @param zv direction the bridge will face
	 * @param bridge_height the height above start.z that the bridge will have
	 * @param desc bridge description.
	 * @param way_desc description of the way to be built on the bridge
	 */
	static void build_bridge(player_t *player, const koord3d start, const koord3d end, koord zv, sint8 bridge_height, const bridge_desc_t *desc, const way_desc_t *way_desc);

	/**
	 * Registers a new bridge type and adds it to the list of build tools.
	 *
	 * @param desc Description of the bridge to register.
	 */
	static void register_desc(bridge_desc_t *desc);

	/**
	 * Method to retrieve bridge descriptor
	 * @param name name of the bridge
	 * @return bridge descriptor or NULL if not found
	 */
	static const bridge_desc_t *get_desc(const char *name);

	/**
	 * Builds the bridge and performs all checks.
	 * This is the main construction routine.
	 *
	 * @param player The player wanting to build the bridge.
	 * @param pos the start of the bridge.
	 * @param desc Description of the bridge to build
	 * @return NULL on success or error message otherwise
	 */
	static const char *build( player_t *player, const koord3d pos, const bridge_desc_t *desc);

	/**
	 * Removes a bridge
	 * @param player the demolisher and owner of the bridge
	 * @param pos position anywhere on a bridge.
	 * @param wegtyp way type of the bridge
	 * @return An error message if the bridge could not be removed, NULL otherwise
	 */
	static const char *remove(player_t *player, koord3d pos, waytype_t wegtyp);

	/**
	 * Find a matching bridge.
	 * @param wtyp way type
	 * @param min_speed desired lower bound for speed limit
	 * @param time current in-game time
	 * @return bridge descriptor or NULL
	 */
	static const bridge_desc_t *find_bridge(const waytype_t wtyp, const sint32 min_speed,const uint16 time);

	/**
	 * Fill menu with icons for all ways of the given waytype
	 * @param tool_selector gui object of the toolbar
	 * @param wtyp way type
	 */
	static void fill_menu(tool_selector_t *tool_selector, const waytype_t wtyp, sint16 sound_ok);

	/**
	 * Returns a list with available bridge types.
	 */
	static const vector_tpl<const bridge_desc_t *>& get_available_bridges(const waytype_t wtyp);
};

#endif
