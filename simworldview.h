/*
 * simworldview.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simworldview_h
#define simworldview_h


#include "simview.h"

class karte_t;

/**
 * Die Ansicht für die Karte von Simutrans
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.6 $
 */
class karte_vollansicht_t : public karte_ansicht_t
{
private:
    karte_t *welt;

protected:

    /**
     * Kartenboden von i,j an Bildschirmkoordinate xpos,ypos zeichnen.
     * @author Hj. Malthaner
     */
    virtual void display_boden(int i, int j, int xpos, int ypos, bool dirty);


    /**
     * Kartenobjekte von i,j an Bildschirmkoordinate xpos,ypos zeichnen.
     * @author Hj. Malthaner
     */
    virtual void display_dinge(int i, int j, int xpos, int ypos, bool dirty);


    virtual int gib_anzeige_breite();
    virtual int gib_anzeige_hoehe();

public:

    karte_vollansicht_t(karte_t *welt);

    void display_menu();

    void display(bool dirty);
};



#endif
