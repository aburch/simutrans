/*
 * baum.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef dings_baum_h
#define dings_baum_h

#include "../tpl/stringhashtable_tpl.h"

class mempool_t;
class baum_besch_t;

/**
 * Bäume in Simutrans.
 * @author Hj. Malthaner
 * @version $Revision: 1.12 $
 * @see ding_t
 */
class baum_t : public ding_t
{
private:

    static mempool_t *mempool;

    static stringhashtable_tpl<const baum_besch_t *> besch_names;
    static slist_tpl<const baum_besch_t *> baum_typen;

    /**
     * Geburtsdatum des Baumes
     * @author Hj. Malthaner
     */
    unsigned long geburt;

    const baum_besch_t *besch;

    void saee_baum();

    /**
     * Berechnet offsets für gepflanzte Bäume
     */
    void calc_off();


    /**
     * Berechnet Bild abhängig vom Alter
     * @author Hj. Malthaner
     */
    void calc_bild(const unsigned long alter);


    /**
     * Berechnet Alter und Bild abhängig vom Alter
     * @author Hj. Malthaner
     */
    void calc_bild();


    const baum_besch_t *gib_aus_liste(int level);

    int gib_bild() const;

public:

    /**
     * Set to true to hide all trees. "Hiding" is implemented by showing the
     * first pic which should be very small.
     * @author Volker Meyer
     * @date  10.06.2003
     */
    static bool hide;


    /**
     * Tomas:  this function is used during map creation
     */
    static bool plant_tree_on_coordinate(karte_t *welt,
					 koord pos,
					 unsigned char maximum_count);


    baum_t(karte_t *welt, loadsave_t *file);
    baum_t(karte_t *welt, koord3d pos);
    baum_t(karte_t *welt, koord3d pos, const baum_besch_t *besch);

    const char *gib_name() const {return "Baum";};
    enum ding_t::typ gib_typ() const {return baum;};

    bool step(long delta_t);

    virtual void rdwr(loadsave_t *file);

    /*
     * Öffnet ein neues Beobachtungsfenster für das Objekt.
     * @author Hj. Malthaner
     */
    virtual void zeige_info();

    /**
     * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird.
     * @author Hj. Malthaner
     */
    void info(cbuffer_t & buf) const;


    void entferne(spieler_t *sp);

    void * operator new(size_t s);
    void operator delete(void *p);

    const baum_besch_t *gib_besch() { return besch; }

    static bool register_besch(baum_besch_t *besch);
    static bool alles_geladen();

    static int gib_anzahl_besch();
};

#endif
