#ifndef gui_tab_panel_h
#define gui_tab_panel_h

#include "../../ifc/gui_action_creator.h"

/**
 * Eine Klasse für Registerkartenartige Aufteilung von gui_komponente_t
 * Objekten.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.7 $
 */
class gui_tab_panel_t : public gui_komponente_action_creator_t
{
private:
    slist_tpl <gui_komponente_t *> tabs;
    slist_tpl <const char *> namen;

    int active_tab;

public:
    enum { HEADER_VSIZE = 18};

    gui_tab_panel_t();

    /**
     * Fügt eine neue Registerkarte hinzu.
     * @param c die Komponente für die Rgisterkarte
     * @param name der Name der Registerkarte für die Komponente
     * @author Hj. Malthaner
     */
    void add_tab(gui_komponente_t *c, const char *name);

    /**
     * Gibt die aktuell angezeigte Komponente zurück.
     * @author Hj. Malthaner
     */
    gui_komponente_t * gib_aktives_tab() const { return tabs.at(active_tab); }

    int get_active_tab_index() { return active_tab; }

    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    void infowin_event(const event_t *ev);

    /**
     * Zeichnet die Registerkarten
     * @author Hj. Malthaner
     */
    void zeichnen(koord offset) const;

    /**
     * Resizing must be propagated!
     * @author Volker Meyer
     * @date  18.06.2003
     */
    void setze_groesse(koord groesse);
};

#endif
