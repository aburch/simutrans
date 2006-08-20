/*
 * gui_resizer.h
 *
 * Copyright (c) 2001 Hansjörg Malthaner, Markus Weber
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_components_gui_resizer_h
#define gui_components_gui_resizer_h

#include "../../ifc/gui_komponente.h"
#include "../ifc/action_listener.h"
#include "../../tpl/slist_tpl.h"

struct event_t;

/**
 * Ein Resize-Button
 *
 * @date 15-Dec-01
 * @author Markus Weber
 */
class gui_resizer_t : public gui_komponente_t
{

public:
  enum type
    { vertical_resize, horizonal_resize, diagonal_resize };


    /**
     * Add a new listener.
     * @author Hj. Malthaner
     */
   void add_listener(action_listener_t * l) {
	listeners.insert( l );
    }


    /**
     * Konstruktor
     *
     * @author Markus Weber
     */
    gui_resizer_t();


    /**
     * Destruktor
     *
     * @author Markus Weber
     */
    ~gui_resizer_t();


    void set_type(enum type type);

    /**
     * Zeichnet die Komponente
     * @author Markus Weber
     */
    void zeichnen(koord offset) const;

     /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Markus Weber
     */
    void infowin_event(const event_t *ev);


     /**
     * Returns true, if the resize-button would be automatically follows the mouse.
     * @author Markus Weber
     */
    bool get_followmouse() const{return followmouse;}

     /**
     * Defines if the resizebutton follows the mouse on dragging or not.
     * Definiert, ob das Resize-Steuerelement der Maus während des Drag-Vorgangs folgt.
     * @param follow values: true = Der Resize-Button folgt der Maus
     *                       false = Der Resize-Button folgt der Maus nicht. Der Button muß während des Drag-Vorgangs per Code verschoben werden
     * @author Markus Weber
     */
    void set_followmouse(bool follow) {followmouse =  follow;}

     /**
     * Returns how far the resize-button was dragged. I.e. 5 means the buttons was dragged 5 pixels down. -5 means, the buttons was dragged 5 pixels up.
     * @author Markus Weber
     */
    int get_vresize() const{return vresize;}


    /**
     * Returns how far the resize-button was dragged. I.e. 5 means the buttons was dragged 5 pixels to the right. -5 means, the buttons was dragged 5 pixels to the left.
     * @author Markus Weber
     */
    int get_hresize() const{return hresize;}


     /**
     * Cancel the current resize operation
     * @author Markus Weber
     */
    void cancelresize();




private:
    enum type type;


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

    /**
     * @return true wenn x,y innerhalb der Buttonflaeche liegt, d.h. der
     * Button getroffen wurde, false wenn x, y ausserhalb liegt
     * @author Hj. Malthaner
     */
    /*bool getroffen(int x,int y)       // x and y doesn't inlcude useable data
    {
        bool pressed;
        pressed = gui_komponente_t::getroffen(x, y);
	return pressed;
    };*/

    int vresize;
    int hresize;
    bool followmouse;

};

#endif
