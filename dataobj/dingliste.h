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
	* Nächster freier Eintrg hinter dem obersten Eleemnt
	* @author Hj. Malthaner
	*/
	uint8 top;

	void set_capacity(uint8 new_cap);

	/**
	* @return ersten freien index
	* @author Hj. Malthaner
	*/
	int grow_capacity();

	void shrink_capacity(uint8 last_index);

	inline uint8 intern_insert_at(ding_t *ding,uint8 pri);

	// this will automatically give the right order for citycars and the like ...
	uint8 intern_add_moving(ding_t *ding);

public:
	dingliste_t();
	~dingliste_t();

	void rdwr(karte_t *welt, loadsave_t *file,koord3d current_pos);

	ding_t * suche(ding_t::typ typ,uint8 start) const;

	/**
	* @param n thing index (unsigned value!)
	* @returns thing at index n or NULL if n is out of bounds
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

	uint8  add(ding_t *obj);
	uint8  remove(ding_t *obj);
	bool loesche_alle(spieler_t *sp,uint8 offset);
	bool ist_da(ding_t *obj) const;

	inline int gib_top() const {return top;}

	/**
	* @returns NULL wenn OK, oder Meldung, warum nicht
	* @author Hj. Malthaner
	*/
	const char * kann_alle_entfernen(const spieler_t *) const;

	/* recalcs all objects onthis tile
	* @author prissi
	*/
	void calc_bild();

	/* display all things, much fast to do it here ...
	*  @author prissi
	*/
	void display_dinge( const sint16 xpos, const sint16 ypos, const uint8 start_offset, const bool dirty ) const;

	// animation, waiting for crossing, all things that could take a while should be done in a step
	void step(const long delta_t, const int steps);

	// start next month (good for toogling a seasons)
	void check_season(const long month);
} GCC_PACKED;

#endif
