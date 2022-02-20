/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef WORLD_SIMPLAN_H
#define WORLD_SIMPLAN_H


#include "../halthandle.h"
#include "../ground/grund.h"


class karte_ptr_t;
class grund_t;
class obj_t;

class planquadrat_t;
void swap(planquadrat_t& a, planquadrat_t& b);


/**
 * The map (karte_t) consists of map squares (planquadrat_t).
 * planquadrat_t objects consist of zero or more ground objects (grund_t).
 */
class planquadrat_t
{
	static karte_ptr_t welt;
private:
	/* list of stations that are reaching to this tile (saves lots of time for lookup) */
	halthandle_t *halt_list;

	uint8 ground_size, halt_list_count;

	// stores climate related settings
	uint8 climate_data;

	union DATA {
		grund_t ** some;    // valid if capacity > 1
		grund_t * one;      // valid if capacity == 1
	} data;

public:
	/**
	 * Constructs a planquadrat (tile) with initial capacity of one ground
	 */
	planquadrat_t() { ground_size = 0; climate_data = 0; data.one = NULL; halt_list_count = 0;  halt_list = NULL; }

	~planquadrat_t();

private:
	planquadrat_t(planquadrat_t const&);
	planquadrat_t& operator=(planquadrat_t const&);
	friend void swap(planquadrat_t& a, planquadrat_t& b);

public:
	/**
	* Setzen des "normalen" Bodens auf Kartenniveau
	*/
	void kartenboden_setzen(grund_t *bd, bool startup = false);

	/// Replaces old ground by new ground, deletes old ground.
	void boden_ersetzen(grund_t *old_ground, grund_t *new_ground);

	/// Adds a bridge, tunnel or monorail ground to this map square.
	void boden_hinzufuegen(grund_t *bd);

	/// Removes a bridge, tunnel or monorail ground from this map square.
	bool boden_entfernen(grund_t *bd);

	/**
	* Return either ground tile in this height or NULL if not existing
	* Inline, since called from karte_t::lookup() and thus extremely often
	* @return NULL if not ground in this height
	*/
	inline grund_t *get_boden_in_hoehe(const sint16 z) const {
		if(ground_size==1) {
			// must be valid ground at this point!
			if(  data.one->get_hoehe() == z  ) {
				return data.one;
			}
		}
		else {
			for(  uint8 i = 0;  i < ground_size;  i++  ) {
				if(  data.some[i]->get_hoehe() == z  ) {
					return data.some[i];
				}
			}
		}
		return NULL;
	}

	/**
	* returns normal ground (always first index)
	* @return not defined if no ground (must not happen!)
	*/
	inline grund_t *get_kartenboden() const { return (ground_size<=1) ? data.one : data.some[0]; }

	/**
	* find ground if thing is on this planquadrat (tile)
	* @return grund_t * with thing or NULL
	*/
	grund_t *get_boden_von_obj(obj_t *obj) const;

	/**
	* ground saved at index position idx (zero would be normal ground)
	* Since it is always called from loops or with other checks, no
	* range check is done => if only one ground, range is ignored!
	* @return ground at idx, undefined if ground_size==NULL
	*/
	inline grund_t *get_boden_bei(const unsigned idx) const { return (ground_size<=1 ? data.one : data.some[idx]); }

	/// @returns number of grounds on this map square.
	unsigned int get_boden_count() const { return ground_size; }

	/**
	* returns climate of plan (lowest 3 bits of climate byte)
	*/
	inline climate get_climate() const { return (climate)(climate_data & 7); }

	/**
	* sets plan climate
	*/
	void set_climate(climate cl) {
		climate_data = (climate_data & 0xf8) + (cl & 7);
	}

	/**
	* returns whether this is a transition to next climate (which will then use calculated image rather than overlay)
	*/
	inline bool get_climate_transition_flag() const { return (climate_data >> 3) & 1; }

	/**
	* set whether this is a transition to next climate (which will then use calculated image rather than overlay)
	*/
	void set_climate_transition_flag(bool flag) {
		climate_data = flag ? (climate_data | 0x08) : (climate_data & 0xf7);
	}

	/**
	* returns corners which transition to another climate
	* this has no meaning if tile is a slope with transition to next climate as these corners are fixed
	* therefore for this case to allow double heights 0 = first level transition, 1 = second level transition
	*/
	inline uint8 get_climate_corners() const { return (climate_data >> 4) & 15; }

	/**
	* sets climate transition corners
	* this has no meaning if tile is a slope with transition to next climate as these corners are fixed
	* therefore for this case to allow double heights 0 = first level transition, 1 = second level transition
	*/
	void set_climate_corners(uint8 corners) {
		climate_data = (climate_data & 0x0f) + (corners << 4);
	}

	/**
	* converts boden to correct type, land or water
	*/
	void correct_water();

	/**
	* konvertiert Land zu Water wenn unter Grundwasserniveau abgesenkt
	*/
	void abgesenkt();

	/**
	* Converts water to land when raised above the ground water level
	*/
	void angehoben();

	/**
	* returns halthandle belonging to player if present
	*/
	halthandle_t get_halt(player_t *player) const;

private:
	// these functions are private helper functions for halt_list corrections
	void halt_list_remove( halthandle_t halt );
	void halt_list_insert_at( halthandle_t halt, uint8 pos );

public:
	/**
	 * The following three functions takes about 4 bytes of memory per tile but speed up passenger generation
	 * @param unsorted if true then halt list will be sorted later by call to sort_haltlist, see karte_t::plans_finish_rd.
	 */
	void add_to_haltlist(halthandle_t halt, bool unsorted = false);

	/**
	* removes the halt from a ground
	* however this function check, whether there is really no other part still reachable
	*/
	void remove_from_haltlist(halthandle_t halt);

	/**
	 * sort list of connected halts, ascending wrt distance to this tile
	 */
	void sort_haltlist();


	bool is_connected(halthandle_t halt) const;

	/**
	* returns the internal array of halts
	*/
	const halthandle_t *get_haltlist() const { return halt_list; }
	uint8 get_haltlist_count() const { return halt_list_count; }

	void rdwr(loadsave_t *file, koord pos );

	/**
	* Updates season and/or snowline dependent graphics
	*/
	void check_season_snowline(const bool season_change, const bool snowline_change);

	void display_obj(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, const bool is_global, const sint8 hmin, const sint8 hmax  CLIP_NUM_DEF) const;

	void display_overlay(sint16 xpos, sint16 ypos) const;

	static void toggle_horizontal_clip(CLIP_NUM_DEF0);

	/**
	 * @brief Update this square for underground view.
	 *
	 * Updates this square for underground view. This includes calculating
	 * calculating new back will images as well as water depth texture.
	 *
	 * This method does not modify this square object, but does modify the
	 * grounds it references.
	 */
	void update_underground() const;
};

#endif
