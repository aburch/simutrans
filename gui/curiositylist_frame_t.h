#ifndef curiositylist_frame_t_h
#define curiositylist_frame_t_h

#include "../gui/gui_frame.h"
#include "../gui/curiositylist_stats_t.h"
#include "../dataobj/translator.h"
#include "ifc/action_listener.h"
#include "gui_label.h"
#include "gui_scrollpane.h"

class karte_t;

/**
 * Curiosity list window
 * @author Hj. Malthaner
 */
class curiositylist_frame_t : public gui_frame_t, private action_listener_t
{
 private:
    static const char *sort_text[SORT_MODES];

    gui_container_t cont;
    gui_scrollpane_t* scrolly;
    curiositylist_stats_t* stats;

    gui_label_t sort_label;
    button_t	sortedby;
    button_t	sorteddir;

    /*
     * All filter settings are static, so they are not reset each
     * time the window closes.
     */
    static sort_mode_t sortby;
    static bool sortreverse;

 public:


    curiositylist_frame_t(karte_t * welt);
    ~curiositylist_frame_t();

    // update list
    void update() { stats->get_unique_attractions(sortby,sortreverse); }

    /**
     * resize window in response to a resize event
     * @author Hj. Malthaner
     */
    virtual void resize(const koord delta);

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     */
    virtual bool action_triggered(gui_komponente_t *);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author V. Meyer
     */
    virtual const char * gib_hilfe_datei() const {return "curiositylist_filter.txt"; }

     /**
     * This function refreshs the station-list
     * @author Markus Weber
     */
    void display_list();

    static sort_mode_t gib_sortierung() { return sortby; }
    static void setze_sortierung(sort_mode_t sm) { sortby = sm; }

    static bool gib_reverse() { return sortreverse; }
    static void setze_reverse(bool reverse) { sortreverse = reverse; }
};

#endif // curiositylist_frame_t_h
