#ifndef gui_spieler_h
#define gui_spieler_h

#include "../simconst.h"

#include "gui_frame.h"
#include "button.h"
#include "gui_label.h"
#include "ifc/action_listener.h"


class spieler_t;
class karte_t;

/**
 * Ein Menü zur Wahl der automatischen Spieler.
 * @author Hj. Malthaner
 * @version $Revision: 1.7 $
 */
class ki_kontroll_t : public gui_frame_t, private action_listener_t
{
private:
	gui_label_t *ai_income[MAX_PLAYER_COUNT];
	char account_str[MAX_PLAYER_COUNT][32];

	button_t	player_active[MAX_PLAYER_COUNT-2];
	button_t	player_get_finances[MAX_PLAYER_COUNT];
	button_t	player_change_to[MAX_PLAYER_COUNT];
	karte_t *welt;

public:
    ki_kontroll_t(karte_t *welt);
    ~ki_kontroll_t() {};


    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return "players.txt";}

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     */
    virtual bool action_triggered(gui_komponente_t *);

    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author Hj. Malthaner
     */
    virtual void zeichnen(koord pos, koord gr);
};

#endif
