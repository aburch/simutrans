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

    void set_capacity(unsigned new_cap);

    /**
     * @return ersten freien index
     * @author Hj. Malthaner
     */
    int grow_capacity(uint8 pri);


    void shrink_capacity(uint8 last_index);

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
		if(capacity<=1) {
			return (capacity!=0  &&  n==top-1) ? obj.one : NULL;
		}
		else {
			return (n < top) ? obj.some[n] : NULL;
		}
	}

    uint8  add(ding_t *obj, uint8 pri = 0);
    uint8  remove(ding_t *obj, spieler_t *sp);
    bool loesche_alle(spieler_t *sp);
    bool ist_da(ding_t *obj) const;
    uint8  count() const;

	// take the thing out from the list
	// use this only for temperary removing
	// since it does not shrink list or checks for ownership
	ding_t *remove_at(uint8 pos);

    inline int gib_top() const {return top;};

    /**
     * @returns NULL wenn OK, oder Meldung, warum nicht
     * @author Hj. Malthaner
     */
    const char * kann_alle_entfernen(const spieler_t *) const;
};

#endif
