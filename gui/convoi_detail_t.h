/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "gui_frame.h"
#include "gui_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_speedbar.h"
#include "components/gui_button.h"
#include "components/gui_label.h"                  // 09-Dec-2001      Markus Weber    Added
#include "components/action_listener.h"
#include "../convoihandle_t.h"

class koord;

/**
 * One element of the vehicle list display
 * @autor prissi
 */
class gui_vehicleinfo_t : public gui_container_t
{
private:
    /**
     * Handle des anzuzeigenden Convois.
     * @author Hj. Malthaner
     */
    convoihandle_t cnv;

public:
    /**
     * @param cnv das Handle für den anzuzeigenden Convoi.
     * @author Hj. Malthaner
     */
    gui_vehicleinfo_t(convoihandle_t cnv);

    /**
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    void zeichnen(koord offset);
};




/**
 * Displays an information window for a convoi
 *
 * @author Hj. Malthaner
 * @date 22-Aug-01
 */
class convoi_detail_t : public gui_frame_t , private action_listener_t
{
public:
	enum sort_mode_t { by_destination=0, by_via=1, by_amount_via=2, by_amount=3, SORT_MODES=4 };

private:
	gui_scrollpane_t scrolly;
	gui_vehicleinfo_t veh_info;

	convoihandle_t cnv;
	button_t	sale_button;
	button_t	withdraw_button;
	button_t	retire_button;

public:
    convoi_detail_t(convoihandle_t cnv);

    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author Hj. Malthaner
     */
    void zeichnen(koord pos, koord gr);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author V. Meyer
     */
    const char * get_hilfe_datei() const {return "convoidetail.txt"; }

    /**
     * resize window in response to a resize event
     * @author Hj. Malthaner
     */
    void resize(const koord delta);

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     *
     * Returns true, if action is done and no more
     * components should be triggered.
     * V.Meyer
     */
    bool action_triggered( gui_action_creator_t *komp, value_t extra);

	/**
	 * called when convoi was renamed
	 */
	void update_data() { set_dirty(); }
};
