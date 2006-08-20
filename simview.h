/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * This file is part of the Simugraph engine and may not be used
 * in other projects without written permission of the author.
 *
 * Usage for Iso-Angband is granted.
 */

#ifndef simview_h
#define simview_h

class karte_modell_t;

/**
 * View-Klasse für Weltmodell.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.8 $
 */
class karte_ansicht_t
{
private:
    karte_modell_t *welt;

protected:

    int scale;

    /**
     * Kartenboden von i,j an Bildschirmkoordinate xpos,ypos zeichnen.
     * @author Hj. Malthaner
     */
    virtual void display_boden(int i, int j, int xpos, int ypos, bool dirty) = 0;

    /**
     * Kartenobjekte von i,j an Bildschirmkoordinate xpos,ypos zeichnen.
     * @author Hj. Malthaner
     */
    virtual void display_dinge(int i, int j, int xpos, int ypos, bool dirty) = 0;

    virtual int gib_anzeige_breite() = 0;
    virtual int gib_anzeige_hoehe() = 0;

public:

    karte_ansicht_t(karte_modell_t *welt);
    virtual void display(bool dirty);
};



#endif
