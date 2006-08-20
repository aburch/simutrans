/*
 * boden.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef boden_boden_h
#define boden_boden_h

#include "grund.h"

/**
 * Der Boden ist der 'Natur'-Untergrund in Simu. Er kann einen Besitzer haben.
 *
 * @author Hj. Malthaner
 */

class boden_t : public grund_t
{
private:
    static bool show_grid;

public:
    /**
     * Toggle ground grid display
     * @author Hj. Malthaner
     */
    static void toggle_grid();

	/**
	 * Toggle ground seasons
	 * @author Hj. Malthaner
	 */
    static void toggle_season(int season);

    boden_t(karte_t *welt, loadsave_t *file);
    boden_t(karte_t *welt, koord3d pos);

    ~boden_t();

    inline bool ist_natur() const { return !hat_wege(); };

    /**
     * Öffnet ein Info-Fenster für diesen Boden
     * @author Hj. Malthaner
     */
    virtual bool zeige_info();


    /**
     * Needed to synchronize map ground with map height. Should do
     * nothing for ground types which are not linked to map height
     * @author Hj. Malthaner
     */
    virtual void sync_height();


    void calc_bild();
    inline const char *gib_name() const;
    inline enum typ gib_typ() const {return boden;};


    void * operator new(size_t s);
    void operator delete(void *p);
};



#endif
