/*
 * sound_frame.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "gui_frame.h"
#include "scrollbar.h"
#include "gui_label.h"
#include "button.h"
#include "ifc/action_listener.h"
#include "ifc/scrollbar_listener.h"

/**
 * Soundkontrollfenster für Simutrans.
 *
 * @author Hj. Malthaner, Owen Rudge
 * @date 09-Apr-01
 * @version $Revision: 1.8 $
 */
class sound_frame_t : public gui_frame_t, scrollbar_listener_t, action_listener_t
{
private:
    scrollbar_t digi;
    scrollbar_t midi;
    gui_label_t dlabel;
    gui_label_t mlabel;
    button_t nextbtn;
    button_t prevbtn;
    gui_label_t curlabel;
    gui_label_t cplaying;


    char song_buf[128];
    const char *make_song_name();

public:

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return "sound.txt";};


    /**
     * Konstruktor. Erzeugt alle notwendigen Subkomponenten.
     * @author Hj. Malthaner
     */
    sound_frame_t();



    /**
     * This method is called if the slider is moved.
     * The value ranges from 0 to (and including) range.
     * @author Hj. Malthaner
     */
    void scrollbar_moved(scrollbar_t *sb, int range, int value);
    bool action_triggered(gui_komponente_t *komp);

    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author Hj. Malthaner
     */
    void zeichnen(koord pos, koord gr);
};
