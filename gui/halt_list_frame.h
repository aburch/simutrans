/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef __halt_list_frame_h
#define __halt_list_frame_h

#include "gui_frame.h"
#include "gui_container.h"
#include "halt_list_stats.h"
#include "components/gui_scrollpane.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"

class spieler_t;
class ware_besch_t;

/**
 * Displays a scrollable list of all stations of a player
 *
 * @author Markus Weber
 * @date 02-Jan-02
 */

class halt_list_frame_t : public gui_frame_t , private action_listener_t
{
public:
	enum sort_mode_t { nach_name, nach_wartend, nach_typ, SORT_MODES };
    enum filter_flag_t { any_filter=1, name_filter=2, typ_filter=4,
		spezial_filter=8, ware_ab_filter=16, ware_an_filter=32,
		sub_filter=64,	// Ab hier beginnen die Unterfilter!
		frachthof_filter=64,
		bahnhof_filter=128,
		bushalt_filter=256,
		dock_filter=512,
		airport_filter=1024,
		ueberfuellt_filter=2048,
		ohneverb_filter=4096
    };

private:
    spieler_t *m_sp;						//13-Feb-02	Added

    static const char *sort_text[SORT_MODES];

	vector_tpl<halt_list_stats_t> stops;

	/*
     * All gui elements of this dialog:
     */

	scrollbar_t vscroll;

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

    static slist_tpl<const ware_besch_t *> waren_filter_ab;
    static slist_tpl<const ware_besch_t *> waren_filter_an;
    /**
     * Compare function using current sort settings for use by
     * qsort().
     * @author V. Meyer
     */
    static int compare_halts(const void *p1, const void *p2);

    /**
     * Check all filters for one halt.
     * returns true, if it is not filtered away.
     * @author V. Meyer
     */
    static bool passes_filter(halthandle_t halt);

public:
    halt_list_frame_t(spieler_t *sp);

	~halt_list_frame_t();

    /**
     * The filter frame tells us when it is closed.
     * @author V. Meyer
     */
    void filter_frame_closed() { filter_frame = NULL; }

	// must be handled, because we could not use scrollpane
	void infowin_event(const event_t*);

	/**
     * This method is called if the size of the window should be changed
     * @author Markus Weber
     */
    void resize(const koord size_change);

	/**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author Hj. Malthaner
     */
    void zeichnen(koord pos, koord gr);

    /**
     * This function refreshs the station-list
     * @author Markus Weber
     */
    void display_list();						//13-Feb-02	Added

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author V. Meyer
     */
    const char * gib_hilfe_datei() const {return "haltlist.txt"; }

    static sort_mode_t gib_sortierung() { return sortby; }
    static void setze_sortierung(sort_mode_t sm) { sortby = sm; }

    static bool gib_reverse() { return sortreverse; }
    static void setze_reverse(bool reverse) { sortreverse = reverse; }

    static bool gib_filter(filter_flag_t filter) { return (filter_flags & filter) != 0; }
    static void setze_filter(filter_flag_t filter, bool on) { filter_flags = on ? (filter_flags | filter) : (filter_flags & ~filter); }

    static char *access_name_filter() { return name_filter_value; }

    static bool gib_ware_filter_ab(const ware_besch_t *ware) { return waren_filter_ab.contains(ware); }
    static void setze_ware_filter_ab(const ware_besch_t *ware, int mode);
    static void setze_alle_ware_filter_ab(int mode);

    static bool gib_ware_filter_an(const ware_besch_t *ware) { return waren_filter_an.contains(ware); }
    static void setze_ware_filter_an(const ware_besch_t *ware, int mode);
    static void setze_alle_ware_filter_an(int mode);

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

#endif
