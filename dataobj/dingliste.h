#ifndef dingliste_h
#define dingliste_h

#include "../simtypes.h"
#include "../simdings.h"


class dingliste_t {
private:
	union {
		ding_t **some;    // valid if capacity > 1
		ding_t *one;      // valid if capacity == 1
	} obj;

	uint8 capacity;

	/**
	* index of the next free entry after the last element
	* @author Hj. Malthaner
	*/
	uint8 top;

	void set_capacity(uint16 new_cap);

	bool grow_capacity();

	void shrink_capacity(uint8 last_index);

	inline void intern_insert_at(ding_t* ding, uint8 pri);

	// this will automatically give the right order for citycars and the like ...
	bool intern_add_moving(ding_t* ding);

	dingliste_t(dingliste_t const&);
	dingliste_t& operator=(dingliste_t const&);
public:
	dingliste_t();
	~dingliste_t();

	void rdwr(karte_t *welt, loadsave_t *file,koord3d current_pos);

	ding_t * suche(ding_t::typ typ,uint8 start) const;

	// since this is often needed, it is defined here
	ding_t * get_leitung() const;
	ding_t * get_convoi_vehicle() const;

	// show all info about the current liste and its objects
	void dump() const;

	/**
	* @param n thing index (unsigned value!)
	* @return thing at index n or NULL if n is out of bounds
	* @author Hj. Malthaner
	*/
	inline ding_t * bei(uint8 n) const
	{
		if(n>top) {
			return NULL;
		}
		return (capacity<=1) ? obj.one : obj.some[n];
	}

	// usually used only for copying by grund_t
	ding_t *remove_last();

	/**
	 * this routine will automatically obey the correct order of things during
	 * insert into dingliste
	 */
	bool add(ding_t* obj);

	bool remove(const ding_t* obj);
	bool loesche_alle(spieler_t *sp,uint8 offset);
	bool ist_da(const ding_t* obj) const;

	inline uint8 get_top() const {return top;}

	/**
	 * sorts the trees according to their offsets
	 */
	void sort_trees(uint8 index, uint8 count);

	/**
	* @return NULL wenn OK, oder Meldung, warum nicht
	* @author Hj. Malthaner
	*/
	const char * kann_alle_entfernen(const spieler_t *, uint8 ) const;

	/* recalcs all objects onthis tile
	* @author prissi
	*/
	void calc_bild();

	/** display all things, faster, but will lead to clipping errors
	 *  @author prissi
	 */
	void display_dinge_quick_and_dirty( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const bool is_global ) const;

	/* display all things, called by the routines in grund_t
	*  @author prissi,dwachs
	*/
	uint8 display_dinge_bg(const sint16 xpos, const sint16 ypos, const uint8 start_offset) const;
	uint8 display_dinge_vh(const sint16 xpos, const sint16 ypos, const uint8 start_offset, const ribi_t::ribi ribi, const bool ontile ) const;
	void display_dinge_fg(const sint16 xpos, const sint16 ypos, const uint8 start_offset, const bool is_global ) const;

	// start next month (good for toogling a seasons)
	void check_season(const long month);
} GCC_PACKED;

#endif
