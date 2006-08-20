/*
 * simdings.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simdings_h
#define simdings_h

#include "simtypes.h"
#include "dataobj/koord3d.h"

class ding_t;
class ding_info_t;

class cbuffer_t;

template <class K, class V> class ptrhashtable_tpl;


// Klassen

// brauche forward-deklaration
class ding_info_t;
class fabrik_t;
class karte_t;
class convoi_t;
class spieler_t;
struct event_t;

// Klassenhierarchie Spielobjekte

/**
 * Von der Klasse ding_t sind alle Objekte in Simutrans abgeleitet.
 *
 * @author Hj. Malthaner
 * @see planqadrat_t
 */
class ding_t
{
public:
    // flags

    enum flag_values {keine_flags=0, dirty=1, not_on_map=2 };


private:

    /**
     * Dies ist die Koordinate des Planquadrates in der Karte zu
     * dem das Objekt gehört.
     * @author Hj. Malthaner
     */
    koord3d pos;


    /**
     * das bild des dings
     * @author Hj. Malthaner
     */
    uint16 bild;


    /**
     * Dies ist der x-Offset in Bilschirmkoordinaten bei der
     * Darstellung des Objektbildes.
     * @author Hj. Malthaner
     */
    sint8 xoff;

    /**
     * Dies ist der y-Offset in Bilschirmkoordinaten bei der
     * Darstellung des Objektbildes.
     * @author Hj. Malthaner
     */
    sint8 yoff;


    /**
     * Dies ist der Zeiger auf den Besitzer des Objekts.
     * @author Hj. Malthaner
     */
    sint8 besitzer_n:4;


    /**
     * flags fuer Zustaende, etc
     * @author Hj. Malthaner
     */
    uint8 flags:4;


 public:

    /**
     * Do we need to be called every step? Must be of (2^n-1)
     * public for performance reasons, this is accessed _very frequently_ !
     * @author Hj. Malthaner
     */
    enum { max_step_frequency=255 };
    uint8 step_frequency;

 private:


    /**
     * Used by all constructors to initialize all vars with safe values
     * -> single source principle
     * @author Hj. Malthaner
     */
    void init(karte_t *welt);


protected:

    /**
     * Basiskonstruktor
     * @author Hj. Malthaner
     */
    ding_t(karte_t *welt);

    /**
     * setzt ein flag im flag-set des dings. Siehe auch flag_values
     * @author Hj. Malthaner
     */
    inline void set_flag(enum flag_values flag) {flags |= flag;};

    /**
     * Erzeugt ein Info-Fenster für dieses Objekt
     * @author V. Meyer
     */
    virtual ding_info_t *new_info();

    /**
     * Offene Info-Fenster
     * @author V. Meyer
     */
    static ptrhashtable_tpl<ding_t *, ding_info_t *> * ding_infos;

    /**
     * Pointer to the world of this thing. Static to conserve space.
     * Change to instance variable once more than one world is available.
     * @author Hj. Malthaner
     */
    static karte_t *welt;


public:
    /**
     * setzt den Besitzer des dings
     * (public wegen Rathausumbau - V.Meyer)
     * @author Hj. Malthaner
     */
    void setze_besitzer(spieler_t *sp);

    /**
     * Ein Objekt kann einen Besitzer haben.
     * @return Einen Zeiger auf den Besitzer des Objekts oder NULL,
     * wenn das Objekt niemand gehört.
     * @author Hj. Malthaner
     */
    spieler_t * gib_besitzer() const;

    void entferne_ding_info();

    inline void clear_flag(enum flag_values flag) {flags &= ~flag;};
    inline bool get_flag(enum flag_values flag) const {return ((flags & flag) != 0);};

    enum typ {undefined=-1, ding=0, baum=1, zeiger=2,
	      wolke=3, sync_wolke=4, async_wolke=5,

              gebaeude_alt=6,	// früher gebaeude
              gebaeude=7,	// früher hausanim
              signal=8,

	      bruecke=9, tunnel=10, gebaeudefundament=11,
	      bahndepot=12, strassendepot=13, schiffdepot = 14, airdepot = 99, monoraildepot=100, tramdepot=101,

	      raucher=15,
	      leitung = 16, pumpe = 17, senke = 18,
//	      nachrichtending = 19,
//              color_gui = 20,
//	      welt_gui = 21,
//              ki_kontroll_gui = 22,
//	      optionen_gui = 23,

	      lagerhaus = 24,
	      oberleitung = 25,

		tramschiene = 26,

	      // vehikel sind von 32 bis 40
	      automobil=32, waggon=33,
	      schiff=34, aircraft=35, monorailwaggon=36,

	      // individualverkehr
	      verkehr=41,
	      fussgaenger=42,

	      // new cars (not used any more!)
	      // car = 64,

	      // other new dings
	      choosesignal = 95,
	      presignal = 96,
	      roadsign = 97,
	      pillar = 98
	};

     inline const sint8 gib_xoff() const {return xoff;};
     inline const sint8 gib_yoff() const {return yoff;};

     void setze_xoff(int xoff) {
		if(this->xoff != xoff) {
			this->xoff = xoff;
			set_flag(dirty);
		}
	};

     void setze_yoff(int yoff) {
		if(this->yoff != yoff) {
			this->yoff = yoff;
			set_flag(dirty);
		}
	};

    /**
     * Mit diesem Konstruktor werden Objekte aus einer Datei geladen
     *
     * @param welt Zeiger auf die Karte, in die das Objekt geladen werden soll.
     * @param file Dateizeiger auf die Datei, die die Objektdaten enthält.
     * @author Hj. Malthaner
     */
    ding_t(karte_t *welt, loadsave_t *file);

    /**
     * Mit diesem Konstruktor werden Objekte für den Boden[x][y][z] erzeugt,
     * diese Objekte müssem nach der Erzeugung mit
     * plan[x][y][z].obj_add explizit auf das Planquadrat gesezt werden.
     *
     * @param welt Zeiger auf die Karte, zu der das Objekt gehören soll.
     * @param pos Die Koordinate des Planquadrates.
     * @author Hj. Malthaner
     */
    ding_t(karte_t *welt, koord3d pos);

    /**
     * Der Destruktor schließt alle Beobachtungsfenster für dieses Objekt.
     * Er entfernt das Objekt aus der Karte.
     * @author Hj. Malthaner
     */
    virtual ~ding_t();

    /**
     * Zum buchen der Abrisskosten auf das richtige Konto
     * @author Hj. Malthaner
     */
    virtual void entferne(spieler_t *) {};

    /**
     * 'Jedes Ding braucht einen Namen.'
     * @return Gibt den unübersetzten(!) Namen des Objekts zurück.
     * @author Hj. Malthaner
     */
    virtual const char *gib_name() const {return "Ding";};


    /**
     * 'Jedes Ding braucht einen Typ.'
     * @return Gibt den typ des Objekts zurück.
     * @author Hj. Malthaner
     */
    virtual enum ding_t::typ gib_typ() const = 0;


    /**
     * Methode für asynchrone Funktionen eines Objekts.
     * @return false to be deleted after the step, true to live on
     * @author Hj. Malthaner
     */
    virtual bool step(long /*delta_t*/) {return true;};


    /**
     * Jedes Objekt braucht ein Bild.
     * @return Die Nummer des aktuellen Bildes für das Objekt.
     * @author Hj. Malthaner
     */
    virtual int gib_bild() const {return bild;};


    /**
     * Falls etwas nach den Vehikeln gemalt werden muß.
     * @author V. Meyer
     */
    virtual int gib_after_bild() const {return -1;};

    /**
     * manche Objekte haben mehr als ein Bild
     * @param  i Der Index des Bildes, i >= 1, benutze fuer i == 0 bild()
     * @return die nummer des Bildes, -1 wenn kein Bild verfuegbar
     * @author V. Meyer
     */
    virtual int gib_after_bild(int ) const {return -1;};

    /**
     * Setzt ein Bild des Objects, normalerweise ist nur bild 0
     * setzbar.
     * @param n bild index
     * @param bild bild nummer
     * @author Hj. Malthaner
     */
    virtual void setze_bild(int n, int bild);


    /**
     * manche Objekte haben mehr als ein Bild
     * @param  i Der Index des Bildes, i >= 1, benutze fuer i == 0 bild()
     * @return die nummer des Bildes, -1 wenn kein Bild verfuegbar
     * @author Hj. Malthaner
     */
    virtual int gib_bild(int ) const {return -1;};


    /**
     * Ein Objekt kann zu einer Fabrik gehören.
     * @return Einen Zeiger auf die Fabrik zu der das Objekt gehört oder NULL,
     * wenn das Objekt zu keiner Fabrik gehört.
     * @author Hj. Malthaner
     */
    virtual inline fabrik_t* fabrik() const {return NULL;};


    /**
     * Speichert den Zustand des Objekts.
     *
     * @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
     * soll.
     * @author Hj. Malthaner
     */
    virtual void rdwr(loadsave_t *file);


    /**
     * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
     * um das Aussehen des Dings an Boden und Umgebung anzupassen
     *
     * @author Hj. Malthaner
     */
    virtual void laden_abschliessen();


    /**
     * Ein Objekt gehört immer zu einem grund_t
     * @return Die aktuellen Koordinaten des Grundes.
     * @author V. Meyer
     * @see ding_t#ding_t
     */
    inline koord3d gib_pos() const {return pos;};


    virtual void setze_pos(koord3d pos);


    /**
     * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird.
     * @author Hj. Malthaner
     * @see simwin
     */
    virtual void info(cbuffer_t & buf) const;


    /**
     * Öffnet ein neues Beobachtungsfenster für das Objekt.
     * @author Hj. Malthaner
     */
    virtual void zeige_info();
//    void zeige_info(ding_t *dt);

    /**
     * @returns NULL wenn OK, ansonsten eine Fehlermeldung
     * @author Hj. Malthaner
     */
    virtual const char * ist_entfernbar(const spieler_t *sp);


    /**
     * Ding zeichnen
     * @author Hj. Malthaner
     */
    virtual void display(int xpos, int ypos, bool dirty) const;


    /**
     * 2. Teil zeichnen (was hinter Fahrzeugen kommt
     * @author V. Meyer
     */
    void display_after(int xpos, int ypos, bool dirty) const;


    /**
     * Dient zur Neuberechnung des Bildes
     * @author Hj. Malthaner
     */
    virtual void calc_bild() {};
} GCC_PACKED;

#endif
