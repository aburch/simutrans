#ifndef factorylist_frame_t_h
#define factorylist_frame_t_h

#include "gui_frame.h"
#include "components/gui_label.h"
#include "../dataobj/translator.h"
#include "factorylist_stats_t.h"


class gui_scrollpane_t;
class factorylist_stats_t;
class karte_t;


/**
 * Factory list window
 * @author Hj. Malthaner
 */
class factorylist_frame_t : public gui_frame_t, private action_listener_t
{
 private:

    static const char *sort_text[factorylist::SORT_MODES];

    gui_label_t sort_label;
    button_t	sortedby;
    button_t	sorteddir;
    factorylist_stats_t stats;
    gui_scrollpane_t scrolly;

    /*
     * All filter settings are static, so they are not reset each
     * time the window closes.
     */
    static factorylist::sort_mode_t sortby;
    static bool sortreverse;

 public:

    factorylist_frame_t(karte_t * welt);


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
    virtual const char * gib_hilfe_datei() const {return "factorylist_filter.txt"; }

    static factorylist::sort_mode_t gib_sortierung() { return sortby; }
    static void setze_sortierung(const factorylist::sort_mode_t& sm) { sortby = sm; }

    static bool gib_reverse() { return sortreverse; }
    static void setze_reverse(const bool& reverse) { sortreverse = reverse; }
};

#endif // factorylist_frame_t_h
