/*
 * car_t.h
 *
 * Copyright (c) 2003 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef car_t_h
#define car_t_h

#include "../simvehikel.h"

class car_group_t;
class vehikel_besch_t;

/**
 * This class represents a single car of a car group.
 *
 * @author Hj. Malthaner
 */
class car_t : public vehikel_basis_t
{
 public:

  static bool ist_blockwechsel(koord3d pos1, koord3d pos2);

 private:

    /**
     * aktuelle Bewegungsrichtung, vector
     * @author Hj. Malthaner
     */
    int dx, dy;
    int hoff;


    /**
     * Temporary value. Pathfinding is done before changing the sqaure.
     * next_pos is already known and calculated. next_pos will become
     * current pos in the next movement step. Thus we need to store the
     * next_next_pos until the next movement step.
     * @author Hj. Malthaner
     */
    koord3d next_next_pos;

    /**
     * We head for this coordinate. It must always be adjacent
     * to our current position.
     * @author Hj. Malthaner
     */
    koord3d next_pos;

    koord3d prev_pos;


    /**
     * What kind of vehicle are we? Full info here!
     * @author Hj. Malthaner
     */
    const vehikel_besch_t *kind;


    /**
     * Our car group
     * @author Hj. Malthaner
     */
    car_group_t *group;


    /**
     * aktuelle Bewegungsrichtung, bits
     * @author Hj. Malthaner
     */
    ribi_t::ribi fahrtrichtung;


    void verlasse_feld();
    void betrete_feld();


 protected:

    virtual int  gib_dx() const {return dx;};
    virtual int  gib_dy() const {return dy;};
    virtual int  gib_hoff() const {return hoff;};
    virtual bool ist_fahrend() const {return true;};

    virtual bool hop_check() {return true;};
    virtual void hop();

    virtual void calc_bild();

    car_t(karte_t *welt);

 public:

    virtual ribi_t::ribi gib_fahrtrichtung() const {return fahrtrichtung;};
    virtual weg_t::typ gib_wegtyp() const;


    koord3d get_prev_pos() const {return prev_pos;};
    koord3d get_next_pos() const {return next_pos;};


    /**
     * Sets the driving direction
     * @author Hj. Malthaner
     */
    void set_next_next_pos(koord3d npos);
    void set_next_pos(koord3d npos);
    void set_prev_pos(koord3d npos);



    void set_kind(const vehikel_besch_t *vb);


    car_t(karte_t *welt, koord3d pos, car_group_t *group);


    /**
     * Calculates driving direction. Also sets direction vetor.
     * Needs valid prev_pos and next_pos
     * @author Hj. Malthaner
     */
    void calc_fahrtrichtung();


    /**
     * Swaps state with another car
     * @author Hj. Malthaner
     */
    void swap_state(car_t *other);


    /**
     * Turn car by 180°
     * @author Hj. Malthaner
     */
    void turn();


    /**
     * Sets the car at its current position (gib_pos()) onto the map
     * @author Hj. Malthaner
     */
    void add_to_map();


    const char *gib_name() const;
    enum ding_t::typ gib_typ() const;


    /**
     * Move one step
     * @author Hj. Malthaner
     */
    void step();


    /**
     * Öffnet ein neues Beobachtungsfenster für das Objekt.
     * @author Hj. Malthaner
     */
    virtual void zeige_info();


    /**
     * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird.
     * @author Hj. Malthaner
     * @see simwin
     */
    virtual char *info(char *buf) const;


    void rdwr(loadsave_t *file);


    /**
     * Debug info
     */
    void dump() const;
};

#endif // car_t_h
