#ifndef gui_tab_panel_h
#define gui_tab_panel_h

#include "../ifc/gui_komponente.h"
#include "../tpl/slist_tpl.h"
#include "ifc/action_listener.h"

/**
 * Eine Klasse für Registerkartenartige Aufteilung von gui_komponente_t
 * Objekten.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.7 $
 */
class tab_panel_t : public gui_komponente_t
{
private:
    slist_tpl <gui_komponente_t *> tabs;
    slist_tpl <const char *> namen;

    int active_tab;

    /**
     * Our listeners.
     * @author Hj. Malthaner
     */
    slist_tpl <action_listener_t *> listeners;

    /**
     * Inform all listeners that an action was triggered.
     * @author Hj. Malthaner
     */
    void call_listeners();

public:
    enum { HEADER_VSIZE = 18};

    tab_panel_t();

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
    gui_komponente_t * gib_aktives_tab() const;

    int get_active_tab_index() { return active_tab; }

    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    virtual void infowin_event(const event_t *ev);

    /**
     * Zeichnet die Registerkarten
     * @author Hj. Malthaner
     */
    virtual void zeichnen(koord offset) const;
    /**
     * Resizing must be propagated!
     * @author Volker Meyer
     * @date  18.06.2003
     */
    virtual void setze_groesse(koord groesse);

    /**
     * Add a new listener to this text input field.
     * @author Hj. Malthaner
     */
    void add_listener(action_listener_t * l) {
		listeners.insert( l );
    }
};


#endif
