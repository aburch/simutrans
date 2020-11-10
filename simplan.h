/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMPLAN_H
#define SIMPLAN_H


#include "halthandle_t.h"
#include "boden/grund.h"


class karte_ptr_t;
class grund_t;
class obj_t;
class stadt_t;

class planquadrat_t;
void swap(planquadrat_t& a, planquadrat_t& b);

struct nearby_halt_t
{
	uint8 distance;
	halthandle_t halt;
};


/**
 * Die Karte ist aus Planquadraten zusammengesetzt.
 * Planquadrate speichern Untergründe (Böden) der Karte.
 * @author Hj. Malthaner
 */
class planquadrat_t
{
	static karte_ptr_t welt;
private:
	/* list of stations that are reaching to this tile (saves lots of time for lookup) */
	nearby_halt_t *halt_list;

	uint8 ground_size, halt_list_count;

	/**
	 * If this tile belongs to a city, a pointer to that city.
	 * This saves much lookup time
	 */
	stadt_t* city;

	// stores climate related settings
	uint8 climate_data;

	union DATA {
		grund_t ** some;    // valid if capacity > 1
		grund_t * one;      // valid if capacity == 1
	} data;

public:
	/**
	 * Constructs a planquadrat (tile) with initial capacity of one ground
	 * @author Hansjörg Malthaner
	 */
	planquadrat_t() { ground_size = 0; climate_data = 0; data.one = NULL; halt_list_count = 0;  halt_list = NULL; city = NULL; }

	~planquadrat_t();

private:
	planquadrat_t(planquadrat_t const&);
	planquadrat_t& operator=(planquadrat_t const&);
	friend void swap(planquadrat_t& a, planquadrat_t& b);

public:
	/**
	* Setzen des "normalen" Bodens auf Kartenniveau
	* @author V. Meyer
	*/
	void kartenboden_setzen(grund_t *bd);

	/**
	* Ersetzt Boden alt durch neu, löscht Boden alt.
	* @author Hansjörg Malthaner
	*/
	void boden_ersetzen(grund_t *alt, grund_t *neu);

	/**
	* Setzen einen Brücken- oder Tunnelbodens
	* @author V. Meyer
	*/
	void boden_hinzufuegen(grund_t *bd);

	/**
	* Löschen eines Brücken- oder Tunnelbodens
	* @author V. Meyer
	*/
	bool boden_entfernen(grund_t *bd);

	/**
	* Return either ground tile in this height or NULL if not existing
	* Inline, since called from karte_t::lookup() and thus extremely often
	* @return NULL if not ground in this height
	* @author Hj. Malthaner
	*/
	inline grund_t *get_boden_in_hoehe(const sint16 z) const {
		if(  ground_size == 1  )
		{
			// must be valid ground at this point!
		    return data.one->get_hoehe() == z ? data.one : NULL;
		}
		for( grund_t **gr = data.some, **end = data.some + ground_size;  gr != end; ++gr ) {
			if(  (*gr)->get_hoehe() == z  ) {
				return *gr;
			}
		}
		return NULL;
	}

	/**
	* returns normal ground (always first index)
	* @return not defined if no ground (must not happen!)
	* @author Hansjörg Malthaner
	*/
	inline grund_t *get_kartenboden() const { return (ground_size<=1) ? data.one : data.some[0]; }

	/**
	* find ground if thing is on this planquadrat (tile)
	* @return grund_t * with thing or NULL
	* @author V. Meyer
	*/
	grund_t *get_boden_von_obj(obj_t *obj) const;

	/**
	* ground saved at index position idx (zero would be normal ground)
	* Since it is always called from loops or with other checks, no
	* range check is done => if only one ground, range is ignored!
	* @return ground at idx, undefined if ground_size==NULL
	* @author Hj. Malthaner
	*/
	inline grund_t *get_boden_bei(const unsigned idx) const { return (ground_size<=1 ? data.one : data.some[idx]); }

	/**
	* @return Anzahl der Böden dieses Planquadrats
	* @author Hj. Malthaner
	*/
	unsigned int get_boden_count() const { return ground_size; }

	/**
	* returns climate of plan (lowest 3 bits of climate byte)
	* @author Kieron Green
	*/
	inline climate get_climate() const { return (climate)(climate_data & 7); }

	/**
	* sets plan climate
	* @author Kieron Green
	*/
	void set_climate(climate cl) {
		climate_data = (climate_data & 0xf8) + (cl & 7);
	}

	/**
	* returns whether this is a transition to next climate (which will then use calculated image rather than overlay)
	* @author Kieron Green
	*/
	inline bool get_climate_transition_flag() const { return (climate_data >> 3) & 1; }

	/**
	* set whether this is a transition to next climate (which will then use calculated image rather than overlay)
	* @author Kieron Green
	*/
	void set_climate_transition_flag(bool flag) {
		climate_data = flag ? (climate_data | 0x08) : (climate_data & 0xf7);
	}

	/**
	* returns corners which transition to another climate
	* this has no meaning if tile is a slope with transition to next climate as these corners are fixed
	* therefore for this case to allow double heights 0 = first level transition, 1 = second level transition
	* @author Kieron Green
	*/
	inline uint8 get_climate_corners() const { return (climate_data >> 4) & 15; }

	stadt_t* get_city() const { return city; }
	void set_city(stadt_t* value) { city = value; }

	/**
	* sets climate transition corners
	* this has no meaning if tile is a slope with transition to next climate as these corners are fixed
	* therefore for this case to allow double heights 0 = first level transition, 1 = second level transition
	* @author Kieron Green
	*/
	void set_climate_corners(uint8 corners) {
		climate_data = (climate_data & 0x0f) + (corners << 4);
	}

	/**
	* converts boden to correct type, land or water
	* @author Kieron Green
	*/
	void correct_water();

	/**
	* konvertiert Land zu Water wenn unter Grundwasserniveau abgesenkt
	* @author Hj. Malthaner
	*/
	void abgesenkt();

	/**
	* Converts water to land when raised above the ground water level
	* @author Hj. Malthaner
	*/
	void angehoben();

	/**
	* returns halthandle belonging to player player
	* returns a random halt if player is NULL
	* @return NULL if no halt present
	* @author Kieron Green
	*/
	halthandle_t get_halt(player_t *player) const;

private:
	// these functions are private helper functions for halt_list corrections
	void halt_list_remove(halthandle_t halt);
	void halt_list_insert_at(halthandle_t halt, uint8 pos, uint8 distance);

public:
	/*
	* The following three functions takes about 4 bytes of memory per tile but speed up passenger generation
	* @author prissi
	*/
	void add_to_haltlist(halthandle_t halt);

	/**
	* removes the halt from a ground
	* however this function check, whether there is really no other part still reachable
	* @author prissi
	*/
	void remove_from_haltlist(halthandle_t halt);

	uint8 get_connected(halthandle_t halt) const;
	bool is_connected(halthandle_t halt) const { return get_connected(halt) < 255; }

	/**
	* returns the internal array of halts
	* @author prissi
	*/
	const nearby_halt_t *get_haltlist() const { return halt_list; }
	uint8 get_haltlist_count() const { return halt_list_count; }

	void rdwr(loadsave_t *file, koord pos );

	/**
	* Updates season and/or snowline dependent graphics
	*/
	void check_season_snowline(const bool season_change, const bool snowline_change);

	void display_obj(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, const bool is_global, const sint8 hmin, const sint8 hmax  CLIP_NUM_DEF) const;

	void display_tileoverlay(sint16 xpos, sint16 ypos, const sint8 hmin, const sint8 hmax) const;

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

	bool is_being_deleted() const { return data.one == NULL; }
};

#endif
