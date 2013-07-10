/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Author: V. Meyer
 */
#ifndef __MARKER_H
#define __MARKER_H

#include "../tpl/ptrhashtable_tpl.h"

class grund_t;

class marker_t {
    // Hajo: added bit mask, because it allows a more efficient
    // implementation (use & instead of %)
    enum { bit_unit = (8 * sizeof(unsigned char)),
           bit_mask = (8 * sizeof(unsigned char))-1 };

    unsigned char *bits;
    int	bits_groesse;

    int cached_groesse;

    ptrhashtable_tpl <const grund_t *, bool> more;
public:
    marker_t(int welt_groesse_x,int welt_groesse_y) : bits(NULL) { init(welt_groesse_x, welt_groesse_y); }
    ~marker_t();

    void init(int welt_groesse_x,int welt_groesse_y);

    void markiere(const grund_t *gr);
    void unmarkiere(const grund_t *gr);
    bool ist_markiert(const grund_t *gr) const;

    void unmarkiere_alle();

};

#endif
