/*
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef __simbridge_h
#define __simbridge_h

#include "../simtypes.h"
#include "../dataobj/koord.h"
#include "../dataobj/koord3d.h"

class bruecke_besch_t;
class grund_t;
class karte_ptr_t;
class spieler_t;               // Hajo: 22-Nov-01: Added forward declaration
class weg_besch_t;
class werkzeug_waehler_t;



/**
 * Bridge management: builds bridges, manages list of available bridge types
 * Bridges should not be built directly but always by calling brueckenbauer_t::baue().
 */
class brueckenbauer_t {
private:

	brueckenbauer_t() {} ///< private -> no instance please

	static karte_ptr_t welt;

public:
	/**
	 * Finds the position of the end of the bridge. Does all kind of checks.
	 * Uses two methods:
	 * -#  If ai_bridge==false then looks for end location at a sloped tile.
	 * -#  If ai_bridge==true returns the first location (taking min_length into account)
	 *     for the bridge end (including flat tiles).
	 * @param sp active player, needed to check scenario conditions
	 * @param pos  the position of the start of the bridge
	 * @param zv   desired direction of the bridge
	 * @param besch the description of the bridge
	 * @param error_msg an error message when the search fails.
	 * @param ai_bridge if this bridge is being built by an AI
	 * @param min_length the minimum length of the bridge.
	 * @return the position of the other end of the bridge or koord3d::invalid if no possible end is found
	 */
	static koord3d finde_ende(spieler_t *sp, koord3d pos, const koord zv, const bruecke_besch_t *besch, const char *&error_msg, bool ai_bridge=false, uint32 min_length=0 );

	/**
	 * Checks whether given tile @p gr is suitable for placing bridge ramp.
	 *
	 * @param sp the player wanting to build the  bridge.
	 * @param gr the ground to check.
	 * @return true, if bridge ramp can be built here.
	 */
	static bool ist_ende_ok(spieler_t *sp, const grund_t *gr, waytype_t wt, ribi_t::ribi r );

	/**
	 * Build a bridge ramp.
	 *
	 * @param sp the player wanting to build the bridge
	 * @param end the position of the ramp
	 * @param zv direction the bridge will face
	 * @param besch the bridge description.
	 */
	static void baue_auffahrt(spieler_t *sp, koord3d end, ribi_t::ribi ribi_neu, hang_t::typ weg_hang, const bruecke_besch_t *besch);

	/**
	 * Actually builds the bridge without checks.
	 * Therefore checks should be done before in
	 * brueckenbauer_t::baue().
	 *
	 * @param sp the master builder of the bridge.
	 * @param start start position.
	 * @param end end position
	 * @param zv direction the bridge will face
	 * @param besch bridge description.
	 * @param weg_besch description of the way to be built on the bridge
	 */
	static void baue_bruecke(spieler_t *sp, const koord3d start, const koord3d end, koord zv, const bruecke_besch_t *besch, const weg_besch_t *weg_besch);

	/**
	 * Registers a new bridge type and adds it to the list of build tools.
	 *
	 * @param besch Description of the bridge to register.
	 */
	static void register_besch(bruecke_besch_t *besch);

	/**
	 * Method to retrieve bridge descriptor
	 * @param name name of the bridge
	 * @return bridge descriptor or NULL if not found
	 */
	static const bruecke_besch_t *get_besch(const char *name);

	/**
	 * Builds the bridge and performs all checks.
	 * This is the main construction routine.
	 *
	 * @param sp The player wanting to build the bridge.
	 * @param pos the start of the bridge.
	 * @param besch Description of the bridge to build
	 * @return NULL on success or error message otherwise
	 */
	static const char *baue( spieler_t *sp, koord pos, const bruecke_besch_t *besch);

	/**
	 * Removes a bridge
	 * @param sp the demolisher and owner of the bridge
	 * @param pos position anywhere on a bridge.
	 * @param wegtyp way type of the bridge
	 * @return An error message if the bridge could not be removed, NULL otherwise
	 */
	static const char *remove(spieler_t *sp, koord3d pos, waytype_t wegtyp);

	/**
	 * Find a matching bridge.
	 * @param wtyp way type
	 * @param min_speed desired lower bound for speed limit
	 * @param time current in-game time
	 * @return bridge descriptor or NULL
	 */
	static const bruecke_besch_t *find_bridge(const waytype_t wtyp, const sint32 min_speed,const uint16 time);

	/**
	 * Fill menu with icons for all ways of the given waytype
	 * @param wzw gui object of the toolbar
	 * @param wtyp way type
	 * @param welt the current world
	 */
	static void fill_menu(werkzeug_waehler_t *wzw, const waytype_t wtyp, sint16 sound_ok);
};

#endif
