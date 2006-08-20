/*
 * grund.h
 *
 * Copyright (c) 1997 - 2001 Hansj÷rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef boden_grund_h
#define boden_grund_h


#include "../halthandle_t.h"
#include "../simimg.h"
#include "../simcolor.h"

#include "../dataobj/koord3d.h"
#include "../dataobj/dingliste.h"

#include "wege/weg.h"

#include "../besch/weg_besch.h"

class spieler_t;
class vehikel_basis_t;
class depot_t;
class karte_t;
class gebaeude_t;
class grund_info_t;
class haus_besch_t;
class cbuffer_t;

template <class K, class V> class ptrhashtable_tpl;


/**
 * <p>Abstrakte Basisklasse für Untergründe in Simutrans.</p>
 *
 * <p>Von der Klasse grund_t sind alle Untergruende (Land, Wasser, Strassen ...)
 * in simu abgeleitet. Jedes Planquadrat hat einen Untergrund.</p>
 *
 * <p>Der Boden hat Eigenschaften, die abgefragt werden koennen
 * ist_natur(), ist_wasser(), hat_wegtyp(), ist_bruecke().
 * In dieser Basisklasse sind alle Eigenschaften false, sie werden erst
 * in den Subklassen redefiniert.</p>
 *
 * @author Hj. Malthaner
 * @version $Revision
 * @see planqadrat_t
 */
class grund_t
{
public:
    enum { MAX_WEGE=2 };


    /**
     * Flag-Werte für das neuzeichnen geänderter Untergründe
     * @author Hj. Malthaner
     */
    enum flag_values {
		keine_flags=0,
		dirty=1,
		new_text=2,
		has_text=4,
		world_spot_dirty = 8,  // Hajo: benutzt von karte_t::ist_dirty(koord3d)
		draw_as_ding = 16, // is a slope etc => draw as one
		is_cover_tile = 32,	// is a ground; however, below is another ground ...
		is_bridge = 64,
		is_tunnel = 128,
		is_in_tunnel = 32
    };


protected:

    /**
     * Offene Info-Fenster
     * @author V. Meyer
     */
    static ptrhashtable_tpl<grund_t *, grund_info_t *> *grund_infos;

protected:

    /**
     * Zusammenfassung des Ding-Container als Objekt
     * @author V. Meyer
     */
    dingliste_t dinge;

    /**
     * The station this ground is bound to
     */
    halthandle_t halt;

    /**
     * Jeder Boden hat im Moment maximal 2 Wege (Kreuzungen sind 1 Weg).
     * Dieses Array darf immer nur bei den niedrigsten Indices gefuellt sein.
     */
    weg_t *wege[MAX_WEGE];

    /**
     * Koordinate in der Karte.
     * @author Hj. Malthaner
     */
    koord3d pos;

    /**
     * 0..100: slopenr, (bild_nr%100), normal ground
     * (bild_nr/100)%17 left slope
     * (bild_nr/1700) right slope
     * @author Hj. Malthaner
     */
    image_id bild_nr;

	/* image of the walls */
    sint8 back_bild_nr;

    /**
     * Flags für das neuzeichnen geänderter Untergründe
     * @author Hj. Malthaner
     */
    uint8 flags;

    /**
     * der Besitzer des Feldes als index in das array der Welt,
     * -1 bedeutet "kein besitzer"
     * @author Hj. Malthaner
     */
    sint8 besitzer_n;

    // slope (now saved locally), because different grounds need differen slopes
    uint8 slope;

    /**
     * Description;
     *      Checks whether there is a way connection from this to gr in dv
     *      direction of the given waytype.
     *
     * Return:
     *      false, if ground is NULL (this case is explitly allowed).
     *      true, if waytyp is weg_t::invalid (this case is explitly allowed)
     *      Check result otherwise
     *
     * Notice:
     *      helper function for "get_neighbour"
     *      Currently private, but it may be possible to make it public, if
     *      neeeded.
     *      Was previously a part of the simposition module.
     *
     * @author: Volker Meyer
     * @date: 21.05.2003
     */
    bool is_connected(const grund_t *gr, weg_t::typ wegtyp, koord dv) const;


    /*
     * Check, whether a may in the given direction may move up or down.
     * Returns 16 (up), 0 (only same height) or -16 (down)
     * @author V. Meyer
     */
    /**
     * Description;
     *      Check, whether it is possible that a way goes up or down in dv
     *      direction. The result depends of the ground type (i.e tunnel entries)
     *      and the "hang_typ" of the ground.
     *
     *      Returns 16, if it is possible to go up
     *      Returns -16, if it is possible to go down
     *      Returns 0 otherwise.
     *
     * Notice:
     *      helper function for "get_neighbour"
     *      Was previously a part of the simposition module.
     *
     * @author: Volker Meyer
     * @date: 21.05.2003
     */
    int get_vmove(koord dir) const;

	// the actual drawing routine
	inline void do_display_boden( const sint16 xpos, const sint16 ypos, const bool dirty ) const;

protected:


    grund_t(karte_t *wl);


public:
    /**
     * setzt die Bildnr. des anzuzeigenden Bodens
     * @author Hj. Malthaner
     */
    inline void setze_bild(image_id n) {bild_nr = n; set_flag(dirty);};


protected:
    /**
     * Pointer to the world of this ground. Static to conserve space.
     * Change to instance variable once more than one world is available.
     * @author Hj. Malthaner
     */
    static karte_t *welt;

	// calculates the slope image and sets the draw_as_ding flag correctly
	void grund_t::calc_back_bild(const sint8 hgt,const sint8 slope_this);

public:
    enum typ {grund, boden, wasser, fundament, tunnelboden, brueckenboden, monorailboden };

    grund_t(karte_t *welt, loadsave_t *file);
    grund_t(karte_t *welt, koord3d pos);

    virtual ~grund_t();


    /**
     * Setzt Flags für das neuzeichnen geänderter Untergründe
     * @author Hj. Malthaner
     */
    inline void set_flag(enum flag_values flag) {flags |= flag;};


    inline void clear_flag(enum flag_values flag) {flags &= ~flag;};
    inline bool get_flag(enum flag_values flag) const {return (flags & flag) != 0;};

    void entferne_grund_info();


    /**
     * Untergruende werden wie asynchrone Objekte gesteppt.
     *
     * Take care, this can be switched off by the user. Don't do
     * anything important for the game herein, just animations or
     * nice effects.
     *
     * @author Hj. Malthaner
     */
    virtual void step() {};


    /**
     * Gibt die Nummer des Bildes des Untergrundes zurueck.
     * @return Die Nummer des Bildes des Untergrundes.
     * @author Hj. Malthaner
     */
    inline const image_id gib_bild() const {return bild_nr;}

    /**
     * Returns the number of an eventual foundation
     * @author prissi
     */
	image_id gib_back_bild(int leftback) const;
	virtual void clear_back_bild() {back_bild_nr=0;}

    /**
     * Gibt den Namen des Untergrundes zurueck.
     * @return Den Namen des Untergrundes.
     * @author Hj. Malthaner
     */
    virtual const char* gib_name() const;


    /**
     * Gibt den Typ des Untergrundes zurueck.
     * @return Der Typ des Untergrundes.
     * @author Hj. Malthaner
     */
    virtual enum typ gib_typ() const {return grund;};


    /**
     * Gibt eine Beschreibung des Untergrundes (informell) zurueck.
     * @return Einen Beschreibungstext zum Untergrund.
     * @author Hj. Malthaner
     */
    const char* gib_text() const;


    /**
     * @return NULL
     * @author Hj. Malthaner
     */
    virtual void info(cbuffer_t & buf) const;


    /**
     * Auffforderung, ein Infofenster zu oeffnen.
     * Oeffnet standardmaessig kein Infofenster.
     * @author Hj. Malthaner
     */
    virtual bool zeige_info() { return false; };


    /**
     * Gibt die Farbe des Beschreibungstexthintergrundes zuurck
     * @return die Farbe des Beschreibungstexthintergrundes.
     * @author Hj. Malthaner
     */
    PLAYER_COLOR_VAL text_farbe() const;


    /**
     * Dient zur Neuberechnung des Bildes, wenn sich die Umgebung
     * oder die Lage (Hang) des grundes geaendert hat.
     * @author Hj. Malthaner
     */
    virtual void calc_bild();


    /**
     * Setzt den Beschreibungstext.
     * @param text Der neue Beschreibungstext.
     * @see grund_t::text
     * @author Hj. Malthaner
     */
    void setze_text(const char *text);


    /**
     * Ermittelt den Besitzer des Untergrundes.
     * @author Hj. Malthaner
     */
    spieler_t * gib_besitzer() const;


    /**
     * Setze den Besitzer des Untergrundes. Gibt false zururck, wenn
     * das setzen fehlgeschlagen ist.
     * @author Hj. Malthaner
     */
    bool setze_besitzer(spieler_t *s);


    virtual bool ist_natur() const {return false;};
    virtual bool ist_wasser() const {return false;};


    /**
     * This is called very often, it must be inlined and therefore
     * cannot be virtual - subclasses must set the flags appropriately!
     * @author Hj. Malthaner
     */
    inline bool ist_bruecke() const {return (flags & is_bridge) != 0;};


    /**
     * This is called very often, it must be inlined and therefore
     * cannot be virtual - subclasses must set the flags appropriately!
     * @author Hj. Malthaner
     */
    inline bool ist_tunnel() const {return (flags & is_tunnel) != 0;};

    /**
     * This is called very often, it must be inlined and therefore
     * cannot be virtual - subclasses must set the flags appropriately!
     * @author Hj. Malthaner
     */
    inline bool ist_im_tunnel() const {return (flags & is_in_tunnel) != 0;};




    int ist_karten_boden() const;

    /**
     * L„dt oder speichert die Daten des Untergrundes in eine Datei.
     * @param file Zeiger auf die Datei in die gespeichert werden soll.
     * @author Hj. Malthaner
     */
    virtual void rdwr(loadsave_t *file);


    /**
     * Gibt die 3d-Koordinaten des Planquadrates zurueck, zu dem der
     * Untergrund gehoert.
     * @return Die Position des Grundes in der 3d-Welt
     * @return Die invalid, falls this == NULL
     * @author Hj. Malthaner
     */
    inline const koord3d & gib_pos() const {return this ? pos : koord3d::invalid;};

    inline void setze_pos(koord3d newpos) { pos = newpos;};

    hang_t::typ gib_grund_hang() const { return (hang_t::typ)slope; }
    virtual bool setze_grund_hang(hang_t::typ sl);

    /**
     * einige sorten untergrund k÷nnen betreten werden. Die Aktionen werden
     * an den zug÷rigen Weg weitergeleitet.
     * @author V. Meyer
     */
    void betrete(vehikel_basis_t *v);

    /**
     * Gegenstueck zu betrete()
     * @author V. Meyer
     */
    void verlasse(vehikel_basis_t *v);


    /**
     * Manche Böden können zu Haltestellen gehören.
     * Der Zeiger auf die Haltestelle wird hiermit gesetzt.
     * @author Hj. Malthaner
     */
    void setze_halt(halthandle_t halt);

    /**
     * Ermittelt, ob dieser Boden zu einer Haltestelle gehört.
     * @return NULL wenn keine Haltestelle, sonst Zeiger auf Haltestelle
     * @author Hj. Malthaner
     */
    const halthandle_t gib_halt() const {return halt;}



    inline short gib_hoehe() const {return pos.z;}

    void setze_hoehe(int h) { pos.z = h;}

    /**
     * Zeichnet Bodenbild des Grundes
     * @author Hj. Malthaner
     */
    void display_boden(const sint16 xpos, sint16 ypos, const bool dirty) const;

    /**
     * Zeichnet Dinge des Grundes. Löscht das Dirty-Flag.
     * @author Hj. Malthaner
     */
    void display_dinge(const sint16 xpos, sint16 ypos, const bool dirty);

    ding_t * suche_obj(ding_t::typ typ) const { return dinge.suche(typ,0); }
    ding_t * suche_obj_ab(ding_t::typ typ,uint8 start) const { return dinge.suche(typ,start); }

    uint8  obj_add(ding_t *obj) { return dinge.add(obj,0); }
    uint8	obj_insert_at(ding_t *obj,uint8 pri) { return dinge.insert_at(obj,pri); }
    uint8  obj_pri_add(ding_t *obj, uint8 pri) { return dinge.add(obj, pri); }
    uint8  obj_remove(ding_t *obj, spieler_t *sp) { set_flag(dirty); return dinge.remove(obj, sp); }
    ding_t *obj_takeout(uint8 pos) { return dinge.remove_at(pos); }
    bool obj_loesche_alle(spieler_t *sp) { return dinge.loesche_alle(sp); }
    bool obj_ist_da(ding_t *obj) const { return dinge.ist_da(obj); }
    ding_t * obj_bei(uint8 n) const { return dinge.bei(n); }
    uint8  obj_count() const { return dinge.count(); }
    uint8 gib_top() const {return dinge.gib_top();}

    /**
     * @returns NULL wenn OK, oder Meldung, warum nicht
     * @author Hj. Malthaner
     */
    const char * kann_alle_obj_entfernen(const spieler_t *sp) const { return dinge.kann_alle_entfernen(sp); }


    /**
     * Interface zur Bauen und abfragen von Gebaeuden
     * =============================================
     */

    /**
     * TRUE, falls es hier ein Gebaeude des speziellen Typs gibt.
     * Dient insbesondere dazu, festzustellen ob eine bestimmte Haltestelle
     * vorhanden ist:
     * @author Volker Meyer
     */
    bool hat_gebaeude(const haus_besch_t *besch) const;

    /**
     * Falls es hier ein Depot gibt, dieses zurueckliefern
     * @author Volker Meyer
     */
    depot_t *gib_depot() const;


    /*
     * Interface zur Abfrage der Wege
     * ==============================
     * Jeder Boden hat bis zu 2. Special fuer Wasser: ohne Weg-Objekt werden
     * alle ribis vom weg_t::wassert als gesetzt zurueckgeliefert.
     */


    /**
     * The only way to get the typ of a way on a tile
     * @author Hj. Malthaner
     */
    weg_t *gib_weg_nr(int i) const { return (this  &&  wege[i]) ? wege[i] : NULL; }


    /**
     * Inline da sehr oft aufgerufen.
     * Sucht einen Weg vom typ 'typ' auf diesem Untergrund.
     * @author Hj. Malthaner
     */
    weg_t *gib_weg(weg_t::typ typ) const {
		if(this) {
			int i = 0;
			while(wege[i] && i<MAX_WEGE) {
				if(wege[i]->gib_typ() == typ) {
					return wege[i];
				}
				i++;
			}
		}
		return NULL;
    }

	/**
		* Returns the system type s_type of a way of type typ at this location
		* Currently only needed for tramways or other different types of rails
		*
		* @author DarioK
		* @see gib_weg
		*/
	const uint8 gib_styp(weg_t::typ typ) const {
		if(this) {
			int i = 0;
			while(wege[i] && i<MAX_WEGE) {
				if(wege[i]->gib_typ() == typ){
					return wege[i]->gib_besch()->gib_styp();
				}
				i++;
			}
		}
		return 0;
	}

     /**
     * Ermittelt die Richtungsbits furr den weg vom Typ 'typ'.
     * Liefert 0 wenn kein weg des Typs vorhanden ist. Ein Weg kann ggf.
     * auch 0 als Richtungsbits liefern, deshalb kann die Anwesenheit eines
     * Wegs nicht hierurber, sondern mit gib_weg(), ermittelt werden.
     * @author Hj. Malthaner
     */
    virtual ribi_t::ribi gib_weg_ribi(weg_t::typ typ) const;

     /**
     * Ermittelt die Richtungsbits furr den weg vom Typ 'typ' unmaskiert.
     * Dies wird beim Bauen ben÷tigt. Furr die Routenfindung werden die
     * maskierten ribis benutzt.
     * @author Hj. Malthaner/V. Meyer
     *
     */
    virtual ribi_t::ribi gib_weg_ribi_unmasked(weg_t::typ typ) const;

    /**
     * Brurckenwege an den Auffahrten sind eine Ebene urber der Planh÷he.
     * Alle anderen liefern hier 0.
     * @author V. Meyer
     */
    virtual int gib_weg_yoff() const { return 0; }

    /**
     * Hat der Boden mindestens ein weg_t-Objekt? Liefert false furr Wasser!
     * @author V. Meyer
     */
    inline bool hat_wege() const { return wege[0] != NULL;}

    /**
     * Kreuzen sich hier 2 verschiedene Wege?
		 * Strassenbahnschienen duerfen nicht als Kreuzung erkannt werden!
     * @author V. Meyer, dariok
     */
	inline bool ist_uebergang() const {return (wege[1]  &&  wege[0]->gib_besch()->gib_wtyp()!=wege[1]->gib_besch()->gib_wtyp()  &&  wege[1]->gib_besch()->gib_styp()!=7); }

    /**
     * Liefert einen Text furr die Ueberschrift des Info-Fensters.
     * @author V. Meyer
     */
    const char * gib_weg_name(weg_t::typ typ = weg_t::invalid) const;


    virtual hang_t::typ gib_weg_hang() const { return gib_grund_hang(); }

    /**
     * Interface zur Bauen der Wege
     * =============================
     */

    /**
     * Bauhilfsfunktion - ein neuer weg wird mit den vorgegebenen ribis
     * eingetragen und der Grund dem Erbauer zugeordnet.
     *
     * @return bool	    true, falls weg nicht vorhanden war
     * @param weg	    der neue Weg
     * @param ribi	    die neuen ribis
     * @param sp	    Spieler, dem der Boden zugeordnet wird
     *
     * @author V. Meyer
     */
    long neuen_weg_bauen(weg_t *weg, ribi_t::ribi ribi, spieler_t *sp);

    /**
     * Bauhilfsfunktion - die ribis eines vorhandenen weges werden erweitert
     *
     * @return bool	    true, falls weg vorhanden
     * @param wegtyp	    um welchen wegtyp geht es
     * @param ribi	    die neuen ribis
     *
     * @author V. Meyer
     */
    bool weg_erweitern(weg_t::typ wegtyp, ribi_t::ribi ribi);


    /**
     * Bauhilfsfunktion - einen Weg entfernen
     *
     * @return bool	    true, falls weg vorhanden war
     * @param wegtyp	    um welchen wegtyp geht es
     * @param ribi_rem  sollen die ribis der nachbar zururckgesetzt werden?
     *
     * @author V. Meyer
     */
    sint32 weg_entfernen(weg_t::typ wegtyp, bool ribi_rem);

    /**
     * Description;
     *      Look for an adjacent way in the given direction. Think of an object
     *      that needs the given waytyp for movement. The object is current at
     *      the ground "this". It wants to move in "dir". The routine checks if
     *      this is possible and returns the destination ground.
     *      Tunnels and bridges are entered and left correctly. This requires
     *      some complex checks, since we have three types of level changes
     *      (tunnel entries, bridge ramps and horizontal bridge start).
     *
     * Notice:
     *      Uses two helper functions "is_connected()" and "get_vmove()"
     *      Was previously a part of the simposition module.
     *
     * Parameters:
     *      If dir is not (-1,0), (1,0), (0,-1) or (0, 1), the function fails
     *      If wegtyp is set to weg_t::invalid, no way checking is performed
     *
     * In case of success:
     *      "to" ist set to the ground found
     *      true is returned
     * In case of failure:
     *      "to" ist not touched
     *      false is returned
     *
     * @author: Volker Meyer
     * @date: 21.05.2003
     */
    bool get_neighbour(grund_t *&to, weg_t::typ type, koord dir) const;

		/**
		 * checks a ways on this ground tile and returns the highest speedlimit.
		 * @author hsiegeln
		 */
    int get_max_speed() const;


	/* remove almost everything on this way */
	bool remove_everything_from_way(spieler_t *sp,weg_t::typ wt,ribi_t::ribi ribi_rem);
} GCC_PACKED;


#endif
