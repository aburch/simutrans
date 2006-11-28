/*
 * gui_resizer.h
 *
 * Copyright (c) 2001 Hansjˆrg Malthaner, Markus Weber
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_components_gui_resizer_h
#define gui_components_gui_resizer_h

#include "../../ifc/gui_action_creator.h"


/**
 * Ein Resize-Button
 *
 * @date 15-Dec-01
 * @author Markus Weber
 */
class gui_resizer_t : public gui_komponente_action_creator_t
{
public:
  enum type { vertical_resize, horizonal_resize, diagonal_resize };

private:
    enum type type;

    int vresize;
    int hresize;
    bool followmouse;

public:
    /**
     * Konstruktor
     *
     * @author Markus Weber
     */
    gui_resizer_t();


    void set_type(enum type type) { this->type = type; }

    /**
     * Zeichnet die Komponente
     * @author Markus Weber
     */
    void zeichnen(koord offset);

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
     * Definiert, ob das Resize-Steuerelement der Maus w‰hrend des Drag-Vorgangs folgt.
     * @param follow values: true = Der Resize-Button folgt der Maus
     *                       false = Der Resize-Button folgt der Maus nicht. Der Button muﬂ w‰hrend des Drag-Vorgangs per Code verschoben werden
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
};

#endif
