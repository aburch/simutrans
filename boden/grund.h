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
#include "../simdepot.h"

#include "../dataobj/koord3d.h"
#include "../dataobj/dingliste.h"

#include "wege/weg.h"

#include "../besch/weg_besch.h"

class spieler_t;
class depot_t;
class karte_t;
class grund_info_t;
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
	/**
	 * Flag-Werte für das neuzeichnen geänderter Untergründe
	 * @author Hj. Malthaner
	 */
	enum flag_values {
		keine_flags=0,
		dirty=1, // was changed => redraw full
		is_kartenboden=2,
		has_text=4,
		marked = 8,  // will have a frame
		draw_as_ding = 16, // is a slope etc => draw as one
		is_halt_flag = 32,	// is a part of a halt
		has_way1 = 64,
		has_way2 = 128
	};

	// just to calculate the offset for skipping the ways ...
	static uint8 offsets[4];

protected:
	/**
	 * Offene Info-Fenster
	 * @author V. Meyer
	 */
	static ptrhashtable_tpl<grund_t*, grund_info_t*> grund_infos;

	static bool show_grid;

public:
	/* true, when only underground should be visible
	 * @author kierongreen
	 */
	static bool underground_mode;

protected:
	/**
	 * Zusammenfassung des Ding-Container als Objekt
	 * @author V. Meyer
	 */
	dingliste_t dinge;

	/**
	 * Koordinate in der Karte.
	 * @author Hj. Malthaner
	 */
	koord3d pos;

	/**
	 * Flags für das neuzeichnen geänderter Untergründe
	 * @author Hj. Malthaner
	 */
	uint8 flags;

	/**
	 * 0..100: slopenr, (bild_nr%100), normal ground
	 * (bild_nr/100)%17 left slope
	 * (bild_nr/1700) right slope
	 * @author Hj. Malthaner
	 */
	image_id bild_nr;

	/* image of the walls */
	sint8 back_bild_nr;

	// slope (now saved locally), because different grounds need differen slopes
	uint8 slope;

	/**
	 * Description;
	 *      Checks whether there is a way connection from this to gr in dv
	 *      direction of the given waytype.
	 *
	 * Return:
	 *      false, if ground is NULL (this case is explitly allowed).
	 *      true, if waytype_t is invalid_wt (this case is explitly allowed)
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
	bool is_connected(const grund_t *gr, waytype_t wegtyp, koord dv) const;

	/**
	 * Description;
	 *      Check, whether it is possible that a way goes up or down in dv
	 *      direction. The result depends of the ground type (i.e tunnel entries)
	 *      and the "hang_typ" of the ground.
	 *
	 *      Returns 1, if it is possible to go up
	 *      Returns -1, if it is possible to go down
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

protected:
	grund_t(karte_t *wl);

public:
  /**
   * setzt die Bildnr. des anzuzeigenden Bodens
   * @author Hj. Malthaner
   */
	inline void setze_bild(image_id n)
	{
		bild_nr = n;
		set_flag(dirty);
	}


protected:
  /**
   * Pointer to the world of this ground. Static to conserve space.
   * Change to instance variable once more than one world is available.
   * @author Hj. Malthaner
   */
  static karte_t *welt;

	// calculates the slope image and sets the draw_as_ding flag correctly
	void calc_back_bild(const sint8 hgt,const sint8 slope_this);

public:
	enum typ {grund, boden, wasser, fundament, tunnelboden, brueckenboden, monorailboden };

	grund_t(karte_t *welt, loadsave_t *file);
	grund_t(karte_t *welt, koord3d pos);

	virtual ~grund_t();

	/**
	 * Toggle ground grid display (now only a flag)
	 */
	static void toggle_grid() { grund_t::show_grid = !grund_t::show_grid; }

	/**
	 * Toggle ground grid display (now only a flag)
	 */
	static void toggle_underground_mode() { grund_t::underground_mode = !grund_t::underground_mode; grund_t::show_grid = grund_t::underground_mode; }

	karte_t *gib_welt() const {return welt;}

	/**
	* Setzt Flags für das neuzeichnen geänderter Untergründe
	* @author Hj. Malthaner
	*/
	inline void set_flag(enum flag_values flag) {flags |= flag;}

	inline void clear_flag(enum flag_values flag) {flags &= ~flag;}
	inline bool get_flag(enum flag_values flag) const {return (flags & flag) != 0;}

	void entferne_grund_info();

	/**
	* start a new month (and toggle the seasons)
	* @author prissi
	*/
	void check_season(const long month) { calc_bild(); dinge.check_season(month); }

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
	virtual const char* gib_name() const = 0;

	/**
	* Gibt den Typ des Untergrundes zurueck.
	* @return Der Typ des Untergrundes.
	* @author Hj. Malthaner
	*/
	virtual enum grund_t::typ gib_typ() const {return grund;}

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
	virtual bool zeige_info() { return false; }

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

	virtual bool ist_natur() const {return false;}
	virtual bool ist_wasser() const {return false;}

	/**
	* This is called very often, it must be inlined and therefore
	* cannot be virtual - subclasses must set the flags appropriately!
	* @author Hj. Malthaner
	*/
	inline bool ist_bruecke() const {return gib_typ()==brueckenboden;}

	/**
	* This is called very often, it must be inlined and therefore
	* cannot be virtual - subclasses must set the flags appropriately!
	* @author Hj. Malthaner
	*/
	inline bool ist_tunnel() const {return ((gib_typ()==tunnelboden)^grund_t::underground_mode);}

	/**
	* This is called very often, it must be inlined and therefore
	* cannot be virtual - subclasses must set the flags appropriately!
	* @author Hj. Malthaner
	*/
	inline bool ist_im_tunnel() const {return (gib_typ()==tunnelboden  &&  ist_karten_boden()==0)^grund_t::underground_mode;}

	/* this will be stored locally, since it is called many, many times */
	inline uint8 ist_karten_boden() const {return (flags&is_kartenboden);}
	void set_kartenboden(bool tf) {if(tf) {flags|=is_kartenboden;} else {flags&=~is_kartenboden;} }

	/**
	* Laedt oder speichert die Daten des Untergrundes in eine Datei.
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
	inline const koord3d & gib_pos() const {return this ? pos : koord3d::invalid;}

	inline void setze_pos(koord3d newpos) { pos = newpos;}

	// slope are now maintained locally
	hang_t::typ gib_grund_hang() const { return (hang_t::typ)slope; }
	void setze_grund_hang(hang_t::typ sl) { slope = sl; }

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
	const halthandle_t gib_halt() const;
	const uint8 is_halt() const {return (flags&is_halt_flag);}

	inline short gib_hoehe() const {return pos.z;}

	void setze_hoehe(int h) { pos.z = h;}

	/**
	* Zeichnet Bodenbild des Grundes
	* @author Hj. Malthaner
	*/
	void display_boden(const sint16 xpos, sint16 ypos) const;

	/**
	* Zeichnet Dinge des Grundes. Löscht das Dirty-Flag.
	* @author Hj. Malthaner
	*/
	void display_dinge(const sint16 xpos, sint16 ypos, const bool called_from_simview) const;

	/**
	* Draw signs on a tile
	* @author Hj. Mathaner
	*/
	void display_overlay(const sint16 xpos, const sint16 ypos, const bool reset_dirty);

	inline ding_t *first_obj() const { return dinge.bei(offsets[flags/has_way1]); }
	ding_t *suche_obj(ding_t::typ typ) const { return dinge.suche(typ,0); }
	ding_t *suche_obj_ab(ding_t::typ typ,uint8 start) const { return dinge.suche(typ,start); }
	ding_t *obj_remove_top() { return dinge.remove_last(); }

	uint8  obj_add(ding_t *obj) { return dinge.add(obj); }
	uint8 obj_remove(const ding_t* obj) { return dinge.remove(obj); }
	bool obj_loesche_alle(spieler_t *sp) { return dinge.loesche_alle(sp,offsets[flags/has_way1]); }
	bool obj_ist_da(const ding_t* obj) const { return dinge.ist_da(obj); }
	ding_t * obj_bei(uint8 n) const { return dinge.bei(n); }
	uint8  obj_count() const { return dinge.gib_top()-offsets[flags/has_way1]; }
	uint8 gib_top() const {return dinge.gib_top();}

	// moves all object from the old to the new grund_t
	void take_obj_from( grund_t *gr);

	/**
	* @returns NULL wenn OK, oder Meldung, warum nicht
	* @author Hj. Malthaner
	*/
	const char * kann_alle_obj_entfernen(const spieler_t *sp) const { return dinge.kann_alle_entfernen(sp,offsets[flags/has_way1]); }

	/**
	* Interface zur Bauen und abfragen von Gebaeuden
	* =============================================
	*/

	/**
	* Falls es hier ein Depot gibt, dieses zurueckliefern
	* @author Volker Meyer
	*/
	depot_t *gib_depot() const { return dynamic_cast<depot_t *>(first_obj()); }

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
	weg_t *gib_weg_nr(int i) const { return (flags&(has_way1<<i)) ? static_cast<weg_t *>(obj_bei(i)) : NULL; }

	/**
	* Inline da sehr oft aufgerufen.
	* Sucht einen Weg vom typ 'typ' auf diesem Untergrund.
	* @author Hj. Malthaner
	*/
	weg_t *gib_weg(waytype_t typ) const {
		if(flags&has_way1) {
			weg_t *w=(weg_t *)obj_bei(0);
			if(w->gib_waytype()==typ) {
				return w;
			}
		}
		if(flags&has_way2) {
			weg_t *w=(weg_t *)obj_bei(1);
			if(w->gib_waytype()==typ) {
				return w;
			}
		}
		return NULL;
	}

	bool hat_weg(waytype_t typ) const { return gib_weg(typ)!=NULL; }

	/**
	* Returns the system type s_type of a way of type typ at this location
	* Currently only needed for tramways or other different types of rails
	*
	* @author DarioK
	* @see gib_weg
	*/
	const uint8 gib_styp(waytype_t typ) const {
		weg_t *weg = gib_weg(typ);
		return (weg) ? weg->gib_besch()->gib_styp() : 0;
	}

	/**
	* Ermittelt die Richtungsbits furr den weg vom Typ 'typ'.
	* Liefert 0 wenn kein weg des Typs vorhanden ist. Ein Weg kann ggf.
	* auch 0 als Richtungsbits liefern, deshalb kann die Anwesenheit eines
	* Wegs nicht hierurber, sondern mit gib_weg(), ermittelt werden.
	* Also beware of water, which always allows all directions ...thus virtual
	* @author Hj. Malthaner
	*/
	virtual ribi_t::ribi gib_weg_ribi(waytype_t typ) const;

	/**
	* Ermittelt die Richtungsbits furr den weg vom Typ 'typ' unmaskiert.
	* Dies wird beim Bauen ben÷tigt. Furr die Routenfindung werden die
	* maskierten ribis benutzt.
	* @author Hj. Malthaner/V. Meyer
	*
	*/
	virtual ribi_t::ribi gib_weg_ribi_unmasked(waytype_t typ) const;

	/**
	* checks a ways on this ground tile and returns the highest speedlimit.
	* only used for the minimap
	* @author hsiegeln
	*/
	int get_max_speed() const;

	/**
	* only used for bridges, which start at a slope
	* @author V. Meyer
	*/
	virtual int gib_weg_yoff() const { return 0; }

	/**
	* Hat der Boden mindestens ein weg_t-Objekt? Liefert false furr Wasser!
	* @author V. Meyer
	*/
	inline bool hat_wege() const { return (flags&(has_way1|has_way2))!=0;}

	/**
	* Kreuzen sich hier 2 verschiedene Wege?
	* Strassenbahnschienen duerfen nicht als Kreuzung erkannt werden!
	* @author V. Meyer, dariok
	*/
	inline bool ist_uebergang() const { return (flags&has_way2)!=0  &&  ((weg_t *)dinge.bei(1))->gib_besch()->gib_styp()!=7; }

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
	bool weg_erweitern(waytype_t wegtyp, ribi_t::ribi ribi);

	/**
	* Bauhilfsfunktion - einen Weg entfernen
	*
	* @return bool	    true, falls weg vorhanden war
	* @param wegtyp	    um welchen wegtyp geht es
	* @param ribi_rem  sollen die ribis der nachbar zururckgesetzt werden?
	*
	* @author V. Meyer
	*/
	sint32 weg_entfernen(waytype_t wegtyp, bool ribi_rem);

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
	*      If wegtyp is set to invalid_wt, no way checking is performed
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
	bool get_neighbour(grund_t *&to, waytype_t type, koord dir) const;

	/* remove almost everything on this way */
	bool remove_everything_from_way(spieler_t *sp,waytype_t wt,ribi_t::ribi ribi_rem);
};


#endif
