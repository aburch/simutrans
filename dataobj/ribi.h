/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef dataobj_ribi_t_h
#define dataobj_ribi_t_h

#include "../simtypes.h"

class koord;
class koord3d;

class hang_t {
#ifndef DOUBLE_GROUNDS
	static const int flags[16];
#else
	static const int flags[81];
#endif

	enum { wegbar_ns = 1, wegbar_ow = 2, einfach = 4 };

public:
	typedef sint8 typ;

	/*
	 * Eigentlich aus bits zusammengesetzt:
	 * Bit 0: gesetzt, falls Suedwestecke erhöht
	 * Bit 1: gesetzt, falls Suedostecke erhöht
	 * Bit 2: gesetzt, falls Nordostecke erhöht
	 * Bit 3: gesetzt, falls Nordwestecke erhöht
	 *
	 * Und nicht verwirren lassen - der Suedhang ist im Norden hoch
	 */

#ifndef DOUBLE_GROUNDS
	enum _typ {
		flach=0,
		nord = 3, 	    // Nordhang
		west = 6, 	    // Westhang
		ost = 9,	    // Osthang
		sued = 12,	    // Suedhang
		erhoben = 15	    // Speziell für Brückenanfänge (prissi: unsued, I think)
	};

	// Ein bischen tricky implementiert:
	static bool ist_gegenueber(typ x, typ y) { return ist_einfach(x) && ist_einfach(y) && x + y == erhoben; }
	static typ gegenueber(typ x) { return ist_einfach(x) ? erhoben - x : flach; }
	static typ rotate90(typ x) { return ( (x&1) ? 8|(x>>1) : x>>1); }

#else
	enum _typ {
		flach=0,
		nord = 3+1, 	    // Nordhang
		west = 9+3, 	    // Westhang
		ost = 27+1,	    // Osthang
		sued = 27+9,	    // Suedhang
		erhoben = 80	    // Speziell für Brückenanfänge  (prissi: unsued, I think)
	};

	// Ein bischen tricky implementiert:
	static bool ist_gegenueber(typ x, typ y) { return ist_einfach(x) && ist_einfach(y) && x + y == 40; }
	static typ gegenueber(typ x) { return ist_einfach(x) ? 40 - x : flach; }
	static bool ist_doppelt(typ x) { return (flags[x] == einfach); }
#endif

	//
	// Ranges werden nicht geprüft!
	//
	static bool ist_einfach(typ x) { return (flags[x] & einfach) != 0; }
	static bool ist_wegbar(typ x)  { return (flags[x] & (wegbar_ns | wegbar_ow)) != 0; }
	static bool ist_wegbar_ns(typ x)  { return (flags[x] & wegbar_ns) != 0; }
	static bool ist_wegbar_ow(typ x)  { return (flags[x] & wegbar_ow) != 0; }
	static int get_flags(typ x) {return flags[x]; }
};



/**
 * Hajo: Nach Volkers einführung des NSOW arrays in der Koordinaten klasse
 * brauchen wir etwas entsprechendes für Richtungsbits. Deshalb habe ich hier
 * eine Klasse mit einer Sammlung von Daten für richtungsbits angelegt
 *
 * Hajo: After Volkers introduction of the NSOW array in the coordinates class
 * we need something similar for direction bits. That is why I have a class with 
 * a collection of data created for direction bits.
 *
 * @author Hansjörg Malthaner
 */
class ribi_t {
	static const int flags[16];

	enum { einfach = 1, gerade_ns = 2, gerade_ow = 4, kurve = 8, twoway=16, threeway=32 };

public:
	/* das enum richtung ist eine verallgemeinerung der richtungsbits auf
	 * benannte Konstanten; die Werte sind wie folgt zugeordnet
	 * 1=Nord, 2=Ost, 4=Sued, 8=West
	 *
	 * richtungsbits (ribi) koennen 16 Komb. darstellen.
	 *
	 * the enum direction is a generalization of the 
	 * direction bits to designated constants, the 
	 * values are as follows 1 = North, 2 = East, 4 = South, 8 = West
	 *
	 * direction bits (ribi) can 16 Komb. represent.
	 */
	enum _ribi {
		keine=0, //None
		nord = 1, //North
		ost = 2, //East
		nordost = 3, //North-East
		sued = 4, // South
		nordsued = 5, 
		suedost = 6, //South-East
		nordsuedost = 7, 
		west = 8, //West
		nordwest = 9, //North-West
		ostwest = 10, 
		nordostwest = 11,
		suedwest = 12, //South-West
		nordsuedwest = 13, 
		suedostwest = 14, 
		alle = 15 //"Everything".
	};
	typedef uint8 ribi;

	enum _dir {
		dir_invalid = 0,
		dir_sued = 0,
		dir_west = 1,
		dir_suedwest = 2,
		dir_suedost = 3,
		dir_nord = 4,
		dir_ost = 5,
		dir_nordost = 6,
		dir_nordwest = 7
	};
	typedef uint8 dir;

private:
	static const ribi fwrd[16];
	static const ribi rwr[16];
	static const ribi doppelr[16];
	static const dir  dirs[16];

public:
	static const ribi nsow[4];
	static const ribi layout_to_ribi[4];	// building layout to ribi (for four rotations, for two use doppelt()!
	//
	// Alle Abfragen über statische Tabellen wg. Performance
	// Ranges werden nicht geprüft!
	//
	static ribi doppelt(ribi x) { return doppelr[x]; }
	static ribi rueckwaerts(ribi x) { return rwr[x]; }
	static ribi get_forward(ribi x) { return fwrd[x]; }	// all ribis, that are in front of this thing
	static ribi rotate90(ribi x) { return ((x&8) ? 1|((x<<1)&0x0E) : x<<1); }

	static bool is_twoway(ribi x) { return (flags[x]&twoway)!=0; }
	static bool is_threeway(ribi x) { return (flags[x]&threeway)!=0; }

	static bool ist_exakt_orthogonal(ribi x, ribi y);
	static bool ist_orthogonal(ribi x, ribi y) { return (doppelr[x] | doppelr[y]) == alle; }
	static bool ist_einfach(ribi x) { return (flags[x] & einfach) != 0; }
	static bool ist_kurve(ribi x) { return (flags[x] & kurve) != 0; }
	static bool ist_gerade(ribi x) { return (flags[x] & (gerade_ns | gerade_ow)) != 0; }
	static bool ist_gerade_ns(ribi x) { return (flags[x] & gerade_ns) != 0; }
	static bool ist_gerade_ow(ribi x) { return (flags[x] & gerade_ow) != 0; }
	static bool ist_kreuzung(ribi x) { return x > 0 && flags[x] == 0; }

	static dir get_dir(ribi x) { return dirs[x]; }

};

//
// Umrechnungen zwische koord, ribi_t und hang_t:
//
//	ribi_t::nord entspricht koord::nord entspricht hang_t::sued !!!
//	-> ich denke aufwaerts, also geht es auf einem Suedhang nach Norden!
// I think upwards, so it goes on a southern slope to the north! (Google)
//
hang_t::typ  hang_typ(koord dir);   // dir:nord -> hang:sued, ...
ribi_t::ribi ribi_typ(koord dir);
ribi_t::ribi ribi_typ(koord from, koord to);
ribi_t::ribi ribi_typ(koord3d dir);
ribi_t::ribi ribi_typ(koord3d from, koord3d to);
ribi_t::ribi ribi_typ(hang_t::typ hang);  // nordhang -> sued, ... !

#endif
