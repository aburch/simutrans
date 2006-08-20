/*
 * ribi.h
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef dataobj_ribi_t_h
#define dataobj_ribi_t_h

class koord;

class ribi_t;


class hang_t {
    static const int flags[16];

    enum { wegbar_ns = 1, wegbar_ow = 2, einfach = 4 };

public:
    /*
     * Eigentlich aus bits zusammengesetzt:
     * Bit 0: gesetzt, falls Suedwestecke erhöht
     * Bit 1: gesetzt, falls Suedostecke erhöht
     * Bit 2: gesetzt, falls Nordostecke erhöht
     * Bit 3: gesetzt, falls Nordwestecke erhöht
     *
     * Und nicht verwirren lassen - der Suedhang ist im Norden hoch
     */
    enum _typ {
	flach=0,
	nord = 3, 	    // Nordhang
	west = 6, 	    // Westhang
	ost = 9,	    // Osthang
	sued = 12,	    // Suedhang
	erhoben = 15	    // Speziell für Brückenanfänge
    };
    typedef signed char typ;

    //
    // Ranges werden nicht geprüft!
    //
    static bool ist_einfach(typ x) { return (flags[x] & einfach) != 0; }
    static bool ist_wegbar(typ x)  { return (flags[x] & (wegbar_ns | wegbar_ow)) != 0; }
    static bool ist_wegbar_ns(typ x)  { return (flags[x] & wegbar_ns) != 0; }
    static bool ist_wegbar_ow(typ x)  { return (flags[x] & wegbar_ow) != 0; }

    // Ein bischen tricky implementiert:
    static bool ist_gegenueber(typ x, typ y) { return ist_einfach(x) && ist_einfach(y) && x + y == erhoben; }
    static typ gegenueber(typ x) { return ist_einfach(x) ? erhoben - x : flach; }
};


/**
 * Hajo: Nach Volkers einführung des NSOW arrays in der Koordinaten klasse
 * brauchen wir etwas entsprechendes für Richtungsbits. Deshalb habe ich hier
 * eine Klasse mit einer Sammlung von Daten für richtungsbits angelegt
 *
 * @author Hansjörg Malthaner
 */
class ribi_t {
    static const int flags[16];

    enum { einfach = 1, gerade_ns = 2, gerade_ow = 4, kurve = 8 };
public:
    /* das enum richtung ist eine verallgemeinerung der richtungsbits auf
     * benannte Konstanten; die Werte sind wie folgt zugeordnet
     * 1=Nord, 2=Ost, 4=Sued, 8=West
     *
     * richtungsbits (ribi) koennen 16 Komb. darstellen.
     */
    enum _ribi {
	keine=0,
	nord = 1,
	ost = 2,
	nordost = 3,
	sued = 4,
	nordsued = 5,
	suedost = 6,
	nordsuedost = 7,
	west = 8,
	nordwest = 9,
	ostwest = 10,
	nordostwest = 11,
	suedwest = 12,
	nordsuedwest = 13,
	suedostwest = 14,
	alle = 15
    };
    typedef unsigned char ribi;

    enum _dir {
      dir_invalid = 0,  // Hajo: maybe we should define a more sensible
                        //       value as invalid direction. 0 is
                        //       safe at least.
	dir_sued = 0,
	dir_west = 1,
	dir_suedwest = 2,
	dir_suedost = 3,
        dir_nord = 4,
	dir_ost = 5,
	dir_nordost = 6,
	dir_nordwest = 7
   };
    typedef unsigned char dir;
private:
    static const ribi rwr[16];
    static const ribi doppelr[16];
    static const dir  dirs[16];
public:
    static const ribi nsow[4];

    //
    // Alle Abfragen über statische Tabellen wg. Performance
    // Ranges werden nicht geprüft!
    //
    static ribi doppelt(ribi x) { return doppelr[x]; }
    static ribi rueckwaerts(ribi x) { return rwr[x]; }
    static bool ist_orthogonal(ribi x, ribi y) { return (doppelt(x) | doppelt(y)) == alle; }
    static bool ist_einfach(ribi x) { return (flags[x] & einfach) != 0; }
    static bool ist_kurve(ribi x) { return (flags[x] & kurve) != 0; }
    static bool ist_gerade(ribi x) { return (flags[x] & (gerade_ns | gerade_ow)) != 0; }
    static bool ist_gerade_ns(ribi x) { return (flags[x] & gerade_ns) != 0; }
    static bool ist_gerade_ow(ribi x) { return (flags[x] & gerade_ow) != 0; }

    static dir gib_dir(ribi x) { return dirs[x]; }
};

//
// Umrechnungen zwische koord, ribi_t und hang_t:
//
//	ribi_t::nord entspricht koord::nord entspricht hang_t::sued !!!
//	-> ich denke aufwaerts, also geht es auf einem Suedhang nach Norden!
//
hang_t::typ  hang_typ(koord dir);   // dir:nord -> hang:sued, ...
ribi_t::ribi ribi_typ(koord dir);
ribi_t::ribi ribi_typ(koord from, koord to);
ribi_t::ribi ribi_typ(hang_t::typ hang);  // nordhang -> sued, ... !

#endif
