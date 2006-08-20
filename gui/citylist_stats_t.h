/*
 * citylist_stats_t.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef citylist_stats_t_h
#define citylist_stats_t_h

#include "../ifc/gui_komponente.h"
#include "../simcity.h"


class karte_t;

namespace citylist {
    enum sort_mode_t { by_name=0, by_size, by_growth, SORT_MODES };
};

/**
 * City list stats display
 * @author Hj. Malthaner
 */
class citylist_stats_t : public gui_komponente_t
{
 private:
    karte_t * welt;

    weighted_vector_tpl<stadt_t *> city_list;

 public:
 	static char total_bev_string[128];


    citylist_stats_t(karte_t *welt,const citylist::sort_mode_t& sortby,const bool& sortreverse);

    void sort(const citylist::sort_mode_t& sortby_,const bool& sortreverse_);

    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    virtual void infowin_event(const event_t *);


    /**
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    void zeichnen(koord offset) const;
};

#endif // citylist_stats_t_h
