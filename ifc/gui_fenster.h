/*
 * gui_fenster.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef ifc_gui_fenster_h
#define ifc_gui_fenster_h

#ifndef koord_h
#include "../dataobj/koord.h"
#endif

struct event_t;

/**
 * Farbdaten für die Fensterahmen und -körper.
 * @author Hj. Malthaner
 * @version $Revision: 1.10 $
 */
struct fensterfarben
{
  unsigned char titel;
  unsigned char hell;
  unsigned char mittel;
  unsigned char dunkel;
};


/**
 * Nachdem sich der info_geber_t als zu beschränkt erwiesen hat,
 * soll ueber den gui_fenster_t eine flexiblere Schnittstelle
 * geschaffen werden.
 *
 * @author Hj. Malthaner
 * @date 18.06.2000
 */
class gui_fenster_t
{
public:

    /**
     * Resize modes
     * @author Markus Weber
     * @date   11-May-2002
     */
    enum resize_modes {
      no_resize = 0, diagonal_resize = 3
    }; // vertical_resize = 1, horizonal_resize = 2,


    /**
     * Get resize mode
     * @author Markus Weber
     * @date   25-May-2002
     */
    virtual resize_modes get_resizemode(void) {return no_resize;};


    virtual ~gui_fenster_t() {};


    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return NULL;};


    /**
     * Does this window need a min size button in the title bar?
     * @return true if such a button is needed
     * @author Hj. Malthaner
     */
    virtual bool has_min_sizer() const {return false;};


    /**
     * Does this window need a next button in the title bar?
     * @return true if such a button is needed
     * @author Volker Meyer
     */
    virtual bool has_next() const {return false;};


    /**
     * Does this window need a prev button in the title bar?
     * @return true if such a button is needed
     * @author Volker Meyer
     */
    virtual bool has_prev() const {return has_next();};


    /**
     * in top-level fenstern wird der Name in der Titelzeile dargestellt
     * @return den nicht uebersetzten Namen der Komponente
     * @author Hj. Malthaner
     */
    virtual const char * gib_name() const = 0;


    /**
     * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
     * zurück
     * @author Hj. Malthaner
     */
    virtual fensterfarben gib_fensterfarben() const = 0;


    /**
     * @return gibt wunschgroesse für das Darstellungsfenster zurueck
     * @author Hj. Malthaner
     */
    virtual koord gib_fenstergroesse() const = 0;


    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    virtual void infowin_event(const event_t *ev) = 0;


    /**
     * Fenster neu zeichnen.
     * @author Hj. Malthaner
     */
    virtual void zeichnen(koord pos, koord gr) = 0;
};

#endif
