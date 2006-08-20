/*
 *
 *  ware_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#ifndef __WARE_BESCH_H
#define __WARE_BESCH_H

/*
 *  includes
 */
#include "text_besch.h"

/*
 *  class:
 *      ware_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Text Maßeinheit
 */
class ware_besch_t : public obj_besch_t {
    friend class good_writer_t;
    friend class good_reader_t;

    uint16 value;

    /**
     * Category of the good
     * @author Hj. Malthaner
     */
    uint16 catg;

    /**
     * Bonus for fast transport given in percent!
     * @author Hj. Malthaner
     */
    uint16 speed_bonus;


    /**
     * Weight in KG per unit of this good
     * @author Hj. Malthaner
     */
    uint16 weight_per_unit;


public:
    const char *gib_name() const
    {
        return static_cast<const text_besch_t *>(gib_kind(0))->gib_text();
    }

    const char *gib_copyright() const
    {
        return static_cast<const text_besch_t *>(gib_kind(1))->gib_text();
    }

    const char *gib_mass() const
    {
        return static_cast<const text_besch_t *>(gib_kind(2))->gib_text();
    }

    int gib_preis() const
    {
        return value;
    }


    /**
     * @return speed bonus value of the good
     * @author Hj. Malthaner
     */
    int gib_speed_bonus() const
    {
        return speed_bonus;
    }


    /**
     * @return Category of the good
     * @author Hj. Malthaner
     */
    int gib_catg() const
    {
        return catg;
    }


    /**
     * @return weight in KG per unit of the good
     * @author Hj. Malthaner
     */
    int gib_weight_per_unit() const
    {
        return weight_per_unit;
    }


    /**
     * @return Name of the category of the good
     * @author Hj. Malthaner
     */
    const char * gib_catg_name() const;


    /**
     * Checks if this good can be interchanged with the other, in terms of
     * transportability.
     *
     * Inline because called very often
     *
     * @author Hj. Malthaner
     */
    bool is_interchangeable(const ware_besch_t *other) const
    {
      return (catg) ? (catg == other->catg) : (this == other);
    };
};

#endif // __WARE_BESCH_H
