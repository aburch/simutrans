/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_OBJLIST_H
#define DATAOBJ_OBJLIST_H


#include "../simtypes.h"
#include "../obj/simobj.h"


/**
 * All things including ways are stored in this structure.
 * The entries are packed, i.e. the first free entry is at the top.
 * To save memory, a single element (like in the case for houses or a single tree)
 * is stored directly (obj.one) and capacity==1. Otherwise obj.some points to an
 * array.
 * The objects are sorted according to their drawing order.
 * ways are always first.
 */
class objlist_t
{
private:
	union {
		obj_t **some;    // valid if capacity > 1
		obj_t *one;      // valid if capacity == 1 (and NULL if top or capacity==0!)
	} obj;

	/**
	 * Number of items which can be stored without expanding
	 * zero indicates empty list
	 */
	uint8 capacity;

	/**
	 * 0-based index of the next free entry after the last element
	 * therefore also the count of number of items which are stored
	 */
	uint8 top;

	void set_capacity(uint16 new_cap);

	bool grow_capacity();

	void shrink_capacity(uint8 last_index);

	inline void intern_insert_at(obj_t* new_obj, uint8 pri);

	// only used internal for loading. DO NOT USE OTHERWISE! Use add instead!
	bool append(obj_t *obj);

	// this will automatically give the right order for citycars and the like ...
	bool intern_add_moving(obj_t* new_obj);

	objlist_t(objlist_t const&);
	objlist_t& operator=(objlist_t const&);

public:
	objlist_t();
	~objlist_t();

	void rdwr(loadsave_t *file,koord3d current_pos);

	obj_t * suche(obj_t::typ typ,uint8 start) const;

	// since this is often needed, it is defined here
	obj_t * get_leitung() const;
	obj_t * get_convoi_vehicle() const;

	/**
	* @param n thing index (unsigned value!)
	* @return thing at index n or NULL if n is out of bounds
	*/
	inline obj_t * bei(uint8 n) const
	{
		if(  n >= top  ) {
			return NULL;
		}
		return (capacity<=1) ? obj.one : obj.some[n];
	}

	// usually used only for copying by grund_t
	obj_t *remove_last();

	/// This routine will automatically obey the correct order of things during insertion.
	bool add(obj_t *obj);
	bool remove(const obj_t* obj);
	bool loesche_alle(player_t *player,uint8 offset);
	bool ist_da(const obj_t* obj) const;

	inline uint8 get_top() const {return top;}

	/**
	 * sorts the trees according to their offsets
	 */
	void sort_trees(uint8 index, uint8 count);

	/**
	* @return NULL when OK, or message, why not?
	*/
	const char * kann_alle_entfernen(const player_t *, uint8 ) const;

	/** recalcs all objects on this tile
	*/
	void calc_image();

	/**
	 * Sets all objects dirty to prevent artifacts with smart hide cursor
	 */
	void set_all_dirty();

	/**
	 * Called whenever the season or snowline height changes
	 */
	void check_season(const bool calc_only_season_change);

	/** display all things, faster, but will lead to clipping errors
	 */
#ifdef MULTI_THREAD
	void display_obj_quick_and_dirty( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const sint8 clip_num ) const;
#else
	void display_obj_quick_and_dirty( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const bool is_global ) const;
#endif

	/**
	* display all things, called by the routines in grund_t
	*/
	uint8 display_obj_bg(const sint16 xpos, const sint16 ypos, const uint8 start_offset  CLIP_NUM_DEF) const;
	uint8 display_obj_vh(const sint16 xpos, const sint16 ypos, const uint8 start_offset, const ribi_t::ribi ribi, const bool ontile  CLIP_NUM_DEF) const;

#ifdef MULTI_THREAD
	void display_obj_fg(const sint16 xpos, const sint16 ypos, const uint8 start_offset, const sint8 clip_num ) const;

	void display_obj_overlay(const sint16 xpos, const sint16 ypos) const;
#else
	void display_obj_fg(const sint16 xpos, const sint16 ypos, const uint8 start_offset, const bool is_global ) const;
#endif
} GCC_PACKED;

#endif
