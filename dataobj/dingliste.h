#ifndef dingliste_h
#define dingliste_h

#ifndef simtypes_h
#include "../simtypes.h"
#endif

#ifndef simdings_h
#include "../simdings.h"
#endif


class dingliste_t {
private:
    ding_t  **obj;
    sint16 capacity;

    /**
     * Nächster freier Eintrg hinter dem obersten Eleemnt
     * @author Hj. Malthaner
     */
    uint16 top;

    void set_capacity(const int new_cap);

    /**
     * @return ersten freien index
     * @author Hj. Malthaner
     */
    int grow_capacity(const int pri);


    void shrink_capacity(const int last_index);

public:
    dingliste_t();
    ~dingliste_t();

    void rdwr(karte_t *welt, loadsave_t *file,koord3d current_pos);

    ding_t * suche(ding_t::typ typ) const;


    /**
     * @param n thing index (unsigned value!)
     * @returns thing at index n or NULL if n is out of bounds
     * @author Hj. Malthaner
     */
    inline ding_t * bei(unsigned int n) const { return n < top ? obj[n] : NULL; }


    int  add(ding_t *obj, int pri = 0);
    int  remove(ding_t *obj, spieler_t *sp);
    bool loesche_alle(spieler_t *sp);
    bool ist_da(ding_t *obj) const;
    int  count() const;

    inline int gib_top() const {return top;};

    /**
     * @returns NULL wenn OK, oder Meldung, warum nicht
     * @author Hj. Malthaner
     */
    const char * kann_alle_entfernen(const spieler_t *) const;
};

#endif
