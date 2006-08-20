/*
 * convoi_frame.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef __convoi_frame_h
#define __convoi_frame_h

#include "gui_frame.h"
#include "gui_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_label.h"
#include "components/action_listener.h"                                // 28-Dec-2001  Markus Weber    Added
#include "components/gui_button.h"

class spieler_t;
class vehikel_t;
class ware_besch_t;

/**
 * Displays a scrollable list of all convois of a player
 *
 * @author Hj. Malthaner, Sort/Filtering by V. Meyer
 * @date 15-Jun-01
 */
class convoi_frame_t : public gui_frame_t , private action_listener_t           //28-Dec-01     Markus Weber    Added , private action_listener_t
{
public:
    enum sort_mode_t { nach_name=0, nach_gewinn=1, nach_typ=2, nach_id=3, SORT_MODES=4 };
    enum filter_flag_t { any_filter=1, name_filter=2, typ_filter=4, ware_filter=8, spezial_filter=16,
		sub_filter=32,	// Ab hier beginnen die Unterfilter!
		lkws_filter=32, zuege_filter=64, schiffe_filter=128, aircraft_filter=256,
		noroute_filter=512, nofpl_filter=1024, noincome_filter=2048, indepot_filter=4096, noline_filter=8192 };
private:
    spieler_t *owner;

    karte_t	*welt;

    static const char *sort_text[SORT_MODES];
    /*
     * All gui elements of this dialog:
     */
    gui_container_t cont;
    gui_scrollpane_t scrolly;

    gui_label_t sort_label;
    button_t	sortedby;
    button_t	sorteddir;
    gui_label_t filter_label;
    button_t	filter_on;
    button_t	filter_details;

    /*
     * Child window, if open
     */
    gui_fenster_t *filter_frame;

    /*
     * All filter settings are static, so they are not reset each
     * time the window closes.
     */
    static sort_mode_t sortby;
    static bool sortreverse;

    static int filter_flags;

    static char name_filter_value[64];

    static slist_tpl<const ware_besch_t *> waren_filter;

    /**
     * Compare function using current sort settings for use by
     * qsort().
     * @author V. Meyer
     */
    static int compare_convois(const void *p1, const void *p2);

    /**
     * Check all filters for one convoi.
     * returns true, if it is not filtered away.
     * @author V. Meyer
     */
    static bool passes_filter(convoihandle_t cnv);
public:
    /**
     * Konstruktor. Erzeugt alle notwendigen Subkomponenten.
     * @author Hj. Malthaner
     */
    convoi_frame_t(spieler_t *sp, karte_t *welt);

    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author V. Meyer
     */
    void infowin_event(const event_t *ev);

    /**
     * The filter frame tells us when it is closed.
     * @author V. Meyer
     */
    void filter_frame_closed() { filter_frame = NULL; }

    /**
     * This method is called if the size of the window should be changed
     * @author Markus Weber
     */
    void resize(const koord size_change);                       // 28-Dec-01        Markus Weber Added

    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author Hj. Malthaner
     */
    void zeichnen(koord pos, koord gr);

    /**
     * Displays the current list, checking sorting and filter settings.
     * @author V. Meyer
     */
    void display_list(void);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author V. Meyer
     */
    const char * gib_hilfe_datei() const {return "convoi.txt"; }

    static sort_mode_t gib_sortierung() { return sortby; }
    static void setze_sortierung(sort_mode_t sm) { sortby = sm; }

    static bool gib_reverse() { return sortreverse; }
    static void setze_reverse(bool reverse) { sortreverse = reverse; }

    static bool gib_filter(filter_flag_t filter) { return (filter_flags & filter) != 0; }
    static void setze_filter(filter_flag_t filter, bool on) { filter_flags = on ? (filter_flags | filter) : (filter_flags & ~filter); }

    static char *access_name_filter() { return name_filter_value; }

    static bool gib_ware_filter(const ware_besch_t *ware) { return waren_filter.contains(ware); }
    // mode: 0=off, 1=on, -1=toggle
    static void setze_ware_filter(const ware_besch_t *ware, int mode);
    static void setze_alle_ware_filter(int mode);

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     *
     * Returns true, if action is done and no more
     * components should be triggered.
     * V.Meyer
     */
    bool action_triggered(gui_komponente_t *komp, value_t extra);
};

#endif // __convoi_frame_h
