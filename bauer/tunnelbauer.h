/*
 * simtunnel.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef __simtunnel_h
#define __simtunnel_h

#include "../simtypes.h"
#include "../dataobj/koord.h"
#include "../dataobj/koord3d.h"
#include "../besch/tunnel_besch.h"
#include "../boden/wege/weg.h"

class tabfileobj_t;
class karte_t;                 // Hajo: 22-Nov-01: Added forward declaration
class spieler_t;               // Hajo: 22-Nov-01: Added forward declaration

/**
 * Baut Tunnel. Tunnel sollten nicht direkt instanziiert werden
 * sondern immer vom tunnelbauer_t erzeugt werden.
 *
 * Es gibt keine Instanz - nur statische Methoden.
 *
 * @author V. Meyer
 */
class tunnelbauer_t {
private:
    static koord3d finde_ende(karte_t *welt, koord3d pos, koord zv, weg_t::typ wegtyp);
    static bool baue_tunnel(karte_t *welt, spieler_t *sp, koord3d pos, koord3d end, koord zv, weg_t::typ wegtyp);
    static void baue_einfahrt(karte_t *welt, spieler_t *sp, koord3d end, koord zv, weg_t::typ wegtyp, const weg_besch_t *weg_besch, int &cost);

    tunnelbauer_t() {} // private -> no instance please
public:
    static const tunnel_besch_t *strassentunnel;
    static const tunnel_besch_t *schienentunnel;

    static bool register_besch(const tunnel_besch_t *besch);
    static bool laden_erfolgreich();

    static int baue(spieler_t *sp, karte_t *welt, koord pos, weg_t::typ wegtyp);
    static int baue(spieler_t *sp, karte_t *welt, koord pos, value_t param)
    {return baue(sp, welt, pos, (weg_t::typ)param.i); }

    static const char *remove(karte_t *welt, spieler_t *sp, koord3d pos, weg_t::typ wegtyp);

    static void create_menu(karte_t *welt);
};

#endif
