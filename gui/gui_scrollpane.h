/*
 * gui_scrollpane.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "../ifc/gui_komponente.h"

class scrollbar_t;


/**
 * Eine Klasse doe Scrollbars für andere gui_komponent_t bereitstellt.
 *
 * @author Hj. Malthaner
 * @date 03-Mar-01
 * @version $Revision: 1.7 $
 */

#ifndef gui_scrollpane_h
#define gui_scrollpane_h


class gui_scrollpane_t : public gui_komponente_t
{
private:
    /**
     * Die zu scrollende Komponente
     * @author Hj. Malthaner
     */
    gui_komponente_t *komp;


    /**
     * Scrollbar X
     * @author Hj. Malthaner
     */
    scrollbar_t *scroll_x;


    /**
     * Scrollbar X
     * @author Hj. Malthaner
     */
    scrollbar_t *scroll_y;

	bool b_show_scroll_x;
	bool b_show_scroll_y;
	bool b_has_size_corner;
public:

    /**
     * Basic constructor
     * @param komp Die zu scrollende Komponente
     * @author Hj. Malthaner
     */
    gui_scrollpane_t(gui_komponente_t *komp);


    /**
     * Destruktor
     * @author Hj. Malthaner
     */
    ~gui_scrollpane_t();


    /**
     * Bei Scrollpanes _muss_ diese Methode zum setzen der Groesse
     * benutzt werden.
     * @author Hj. Malthaner
     */
    void setze_groesse(koord groesse);

    /**
     * Setzt Positionen der Scrollbars
     * @author Hj. Malthaner
     */
    void setze_scroll_position(int x, int y);

    int get_scroll_x() const;
    int get_scroll_y() const;

    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    virtual void infowin_event(const event_t *ev);


    /**
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    virtual void zeichnen(koord offset) const;

    void set_show_scroll_x(bool yesno) { b_show_scroll_x = yesno; };

    void set_show_scroll_y(bool yesno) { b_show_scroll_y = yesno; };

    void set_size_corner(bool yesno) { b_has_size_corner = yesno; };

};

#endif
