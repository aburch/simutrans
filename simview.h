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

class karte_t;

/**
 * View-Klasse für Weltmodell.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.8 $
 */
class karte_ansicht_t
{
private:
    karte_t *welt;

protected:

    int scale;

public:

    karte_ansicht_t(karte_t *welt);
    virtual void display(bool dirty);
};

#endif
