/*
 * Copyright (c) 1997 - 2001 Hj Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef boden_grund_h
#define boden_grund_h


#include "../halthandle_t.h"
#include "../simimg.h"
#include "../simcolor.h"
#include "../simconst.h"
#include "../dataobj/koord3d.h"
#include "../dataobj/dingliste.h"
#include "wege/weg.h"


class spieler_t;
class depot_t;
class karte_t;
class cbuffer_t;


/* A map from ding_t subtypes to their enum equivalent
 * Used by grund_t::find<T>()
 */
class aircraft_t;
class baum_t;
class bruecke_t;
class crossing_t;
class field_t;
class fussgaenger_t;
class gebaeude_t;
class groundobj_t;
class label_t;
class leitung_t;
class pillar_t;
class pumpe_t;
class roadsign_t;
class senke_t;
class signal_t;
class stadtauto_t;
class automobil_t;
class tunnel_t;
class wayobj_t;
class zeiger_t;

template<typename T> struct map_ding {};
template<> struct map_ding<aircraft_t>    { static const ding_t::typ code = ding_t::aircraft;    };
template<> struct map_ding<baum_t>        { static const ding_t::typ code = ding_t::baum;        };
template<> struct map_ding<bruecke_t>     { static const ding_t::typ code = ding_t::bruecke;     };
template<> struct map_ding<crossing_t>    { static const ding_t::typ code = ding_t::crossing;    };
template<> struct map_ding<field_t>       { static const ding_t::typ code = ding_t::field;       };
template<> struct map_ding<fussgaenger_t> { static const ding_t::typ code = ding_t::fussgaenger; };
template<> struct map_ding<gebaeude_t>    { static const ding_t::typ code = ding_t::gebaeude;    };
template<> struct map_ding<groundobj_t>   { static const ding_t::typ code = ding_t::groundobj;   };
template<> struct map_ding<label_t>       { static const ding_t::typ code = ding_t::label;       };
template<> struct map_ding<leitung_t>     { static const ding_t::typ code = ding_t::leitung;     };
template<> struct map_ding<pillar_t>      { static const ding_t::typ code = ding_t::pillar;      };
template<> struct map_ding<pumpe_t>       { static const ding_t::typ code = ding_t::pumpe;       };
template<> struct map_ding<roadsign_t>    { static const ding_t::typ code = ding_t::roadsign;    };
template<> struct map_ding<senke_t>       { static const ding_t::typ code = ding_t::senke;       };
template<> struct map_ding<signal_t>      { static const ding_t::typ code = ding_t::signal;      };
template<> struct map_ding<stadtauto_t>   { static const ding_t::typ code = ding_t::verkehr;     };
template<> struct map_ding<automobil_t>   { static const ding_t::typ code = ding_t::automobil;   };
template<> struct map_ding<tunnel_t>      { static const ding_t::typ code = ding_t::tunnel;      };
template<> struct map_ding<wayobj_t>      { static const ding_t::typ code = ding_t::wayobj;      };
template<> struct map_ding<weg_t>         { static const ding_t::typ code = ding_t::way;         };
template<> struct map_ding<zeiger_t>      { static const ding_t::typ code = ding_t::zeiger;      };


template<typename T> static inline T* ding_cast(ding_t* const d)
{
	return d->get_typ() == map_ding<T>::code ? static_cast<T*>(d) : 0;
}


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

public:
	/** true, when showing a grid
	 * @author prissi
	 */
	static volatile bool show_grid;

	/* underground modes */
	/* @author Dwachs    */
	enum _underground_modes {
		ugm_none = 0,	// normal view
		ugm_all  = 1,   // everything underground visible, grid for grounds
		ugm_level= 2	// overground things visible if their height  <= underground_level
						// underground things visible if their height == underground_level
	};
	static uint8 underground_mode;
	static sint8 underground_level;

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

public:
	/**
	 * setzt die Bildnr. des anzuzeigenden Bodens
	 * @author Hj. Malthaner
	 */
	inline void set_bild(image_id n) {
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

	// this is the real image calculation, called for the actual ground image
	virtual void calc_bild_internal() = 0;

public:
	enum typ { boden = 1, wasser, fundament, tunnelboden, brueckenboden, monorailboden };

	grund_t(karte_t *welt, loadsave_t *file);
	grund_t(karte_t *welt, koord3d pos);

private:
	grund_t(grund_t const&);
	grund_t& operator=(grund_t const&);

public:
	virtual ~grund_t();

	/**
	 * Toggle ground grid display (now only a flag)
	 */
	static void toggle_grid() { grund_t::show_grid = !grund_t::show_grid; }

	/**
	 * Sets the undergroundmode & level
	 */
	static void set_underground_mode(const uint8 ugm, const sint8 level);

	karte_t *get_welt() const {return welt;}

	/**
	* Setzt Flags für das neuzeichnen geänderter Untergründe
	* @author Hj. Malthaner
	*/
	inline void set_flag(flag_values flag) {flags |= flag;}

	inline void clear_flag(flag_values flag) {flags &= ~flag;}
	inline bool get_flag(flag_values flag) const {return (flags & flag) != 0;}

	/**
	* start a new month (and toggle the seasons)
	* @author prissi
	*/
	void check_season(const long month) { calc_bild_internal(); dinge.check_season(month); }

	/**
	 * Dient zur Neuberechnung des Bildes, wenn sich die Umgebung
	 * oder die Lage (Hang) des grundes geaendert hat.
	 * @author Hj. Malthaner
	 */
	void calc_bild();

	/**
	* Gibt die Nummer des Bildes des Untergrundes zurueck.
	* @return Die Nummer des Bildes des Untergrundes.
	* @author Hj. Malthaner
	*/
	inline image_id get_bild() const {return bild_nr;}

	/**
	* Returns the number of an eventual foundation
	* @author prissi
	*/
	image_id get_back_bild(int leftback) const;
	virtual void clear_back_bild() {back_bild_nr=0;}

	/**
	* if ground is deleted mark the old spot as dirty
	*/
	void mark_image_dirty();

	/**
	* Gibt den Namen des Untergrundes zurueck.
	* @return Den Namen des Untergrundes.
	* @author Hj. Malthaner
	*/
	virtual const char* get_name() const = 0;

	/**
	* Gibt den Typ des Untergrundes zurueck.
	* @return Der Typ des Untergrundes.
	* @author Hj. Malthaner
	*/
	virtual typ get_typ() const = 0;

	/**
	* Gibt eine Beschreibung des Untergrundes (informell) zurueck.
	* @return Einen Beschreibungstext zum Untergrund.
	* @author Hj. Malthaner
	*/
	const char* get_text() const;

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
	void zeige_info();

	/**
	* Gibt die Farbe des Beschreibungstexthintergrundes zuurck
	* @return die Farbe des Beschreibungstexthintergrundes.
	* @author Hj. Malthaner
	*/
	PLAYER_COLOR_VAL text_farbe() const;

	/**
	 * Sets the label text (by copying it)
	 * @param text the new text (NULL will remove any label text)
	 */
	void set_text(const char* text);

	virtual bool ist_natur() const {return false;}
	virtual bool ist_wasser() const {return false;}

	/**
	* This is called very often, it must be inlined and therefore
	* cannot be virtual - subclasses must set the flags appropriately!
	* @author Hj. Malthaner
	*/
	inline bool ist_bruecke() const {return get_typ()==brueckenboden;}

	/**
	* true if tunnelboden (hence true also for tunnel mouths)
	* check for visibility in is_visible()
	*/
	inline bool ist_tunnel() const {
		return ( (get_typ()==tunnelboden) );
	}

	/**
	* gives true for grounds inside tunnel (not tunnel mouths)
	* check for visibility in is_visible()
	*/
	inline bool ist_im_tunnel() const {
		return ( get_typ()==tunnelboden && (!ist_karten_boden())) ;
	}

	/* this will be stored locally, since it is called many, many times */
	inline bool ist_karten_boden() const {return (flags&is_kartenboden);}
	void set_kartenboden(bool tf) {if(tf) {flags|=is_kartenboden;} else {flags&=~is_kartenboden;} }

	/**
	* returns powerline here
	* @author Kieron Green
	*/
	leitung_t *get_leitung() const { return (leitung_t *) dinge.get_leitung(); }

	/**
	* Laedt oder speichert die Daten des Untergrundes in eine Datei.
	* @param file Zeiger auf die Datei in die gespeichert werden soll.
	* @author Hj. Malthaner
	*/
	virtual void rdwr(loadsave_t *file);

	// map rotation
	virtual void rotate90();

	// we must put the text back to thier proper location after roation ...
	static void finish_rotate90();

	// since enlargement will require new hases
	static void enlarge_map( sint16 new_size_x, sint16 new_size_y );

	void sort_trees();

	/**
	* Gibt die 3d-Koordinaten des Planquadrates zurueck, zu dem der
	* Untergrund gehoert.
	* @return Die Position des Grundes in der 3d-Welt
	* @author Hj. Malthaner
	*/
	inline const koord3d& get_pos() const { return pos; }

	inline void set_pos(koord3d newpos) { pos = newpos;}

	// slope are now maintained locally
	hang_t::typ get_grund_hang() const { return (hang_t::typ)slope; }
	void set_grund_hang(hang_t::typ sl) { slope = sl; }

	/**
	 * Manche Böden können zu Haltestellen gehören.
	 * @author Hj. Malthaner
	 */
	void set_halt(halthandle_t halt);

	/**
	 * Ermittelt, ob dieser Boden zu einer Haltestelle gehört.
	 * @return NULL wenn keine Haltestelle, sonst Zeiger auf Haltestelle
	 * @author Hj. Malthaner
	 */
	halthandle_t get_halt() const;
	bool is_halt() const { return flags & is_halt_flag; }

	/**
	 * @return The height of the tile.
	 */
	inline sint8 get_hoehe() const {return pos.z;}

	/**
	 * @param corner hang_t::_corner mask of corners to check.
	 * @return The height of the tile at the requested corner.
	 */
	inline sint8 get_hoehe(hang_t::typ corner) const
	{
		return pos.z + (((hang_t::typ)slope & corner )?1:0);
	}

	void set_hoehe(int h) { pos.z = h;}

	// Helper functions for underground modes
	//
	// returns the height for the use in underground-mode,
	// heights above underground_level are cutted
	inline sint8 get_disp_height() const {
		return (underground_mode & ugm_level )
			? (pos.z > underground_level ? underground_level : pos.z)
			: pos.z ;
	}

	// returns slope
	// if tile is not visible, 'flat' is returned
	// special care has to be taken of tunnel mouths
	inline hang_t::typ get_disp_slope() const {
		return (  (underground_mode & ugm_level)  &&  (pos.z > underground_level  ||  (get_typ()==tunnelboden  &&  ist_karten_boden()  &&  pos.z == underground_level))
							? (hang_t::typ)hang_t::flach
							: get_grund_hang() );

		/*switch(underground_mode) {// long version of the return statement above
			case ugm_none: return(get_grund_hang());
			case ugm_all:  return(get_grund_hang()); // get_typ()==tunnelboden && !ist_karten? hang_t::flach : get_grund_hang());
			case ugm_level:return((pos.z > underground_level || (get_typ()==tunnelboden && ist_karten_boden() && pos.z == underground_level))
							? hang_t::flach
							: get_grund_hang());
		}*/
	}

	inline bool is_visible() const {
		if(get_typ()==tunnelboden) {
			switch(underground_mode) {
				case ugm_none: return ist_karten_boden();
				case ugm_all:  return true;
				case ugm_level:return  pos.z == underground_level  ||  (ist_karten_boden()  &&  pos.z <= underground_level);
			}
		}
		else {
			switch(underground_mode) {
				case ugm_none: return true;
				case ugm_all:  return false;
				case ugm_level:return pos.z <= underground_level;
			}
		}
		return(false);
	}

	// the same as above but specialized for kartenboden
	inline bool is_karten_boden_visible() const {
		switch(underground_mode) {
			case ugm_none: return true;
			case ugm_all:  return get_typ()==tunnelboden;
			case ugm_level:return pos.z <= underground_level;
		}
		return(false);
	}
	/**
	 * returns slope of ways as displayed (special cases: bridge ramps, tunnel mouths, undergroundmode etc)
	 */
	hang_t::typ get_disp_way_slope() const;
	/**
	* displays the ground images (including foundations, fences and ways)
	* @author Hj. Malthaner
	*/
	void display_boden(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width) const;

	void display_if_visible(sint16 xpos, sint16 ypos, sint16 raster_tile_width) const;

	/**
	 * displays everything that is on a tile - the main display routine for objects on tiles
	 * @param is_global set to true, if this is called during the whole screen update
	 * @author dwachs
	 */
	void display_dinge_all(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, const bool is_global) const;

	/**
	 * similar to above but yieleds clipping error
	 * => only used for zoom out
	 * @param is_global set to true, if this is called during the whole screen update
	 * @author prissi
	 */
	void display_dinge_all_quick_and_dirty(const sint16 xpos, sint16 ypos, const sint16 raster_tile_width, const bool is_global) const;

	/**
	 * displays background images of all non-moving objects on the tile
	 * @param is_global set to true, if this is called during the whole screen update
	 * @param draw_ways if true then draw images of ways
	 * @param visible if false then draw only grids and markers
	 * @return index of first vehicle on the tile
	 * @author dwachs
	 */
	uint8 display_dinge_bg(const sint16 xpos, const sint16 ypos, const bool is_global, const bool draw_ways, const bool visible) const;

	/**
	 * displays vehicle (background) images
	 * @param start_offset start with object at this index
	 * @param ribi draws only vehicles driving in this direction (or against this)
	 * @param ontile is true if we are on the tile that defines the clipping
	 * @author dwachs
	 */
	uint8 display_dinge_vh(const sint16 xpos, const sint16 ypos, const uint8 start_offset, const ribi_t::ribi ribi, const bool ontile) const;

	/**
	 *  displays all foreground images
	 * @param is_global set to true, if this is called during the whole screen update
	 * @author dwachs
	 */
	void display_dinge_fg(const sint16 xpos, const sint16 ypos, const bool is_global, const uint8 start_offset) const;
	/* overlayer with signs, good levels and station coverage
	 * resets the dirty flag
	 * @author kierongreen
	 */

	void display_overlay(sint16 xpos, sint16 ypos);

	inline ding_t *first_obj() const { return dinge.bei(offsets[flags/has_way1]); }
	ding_t *suche_obj(ding_t::typ typ) const { return dinge.suche(typ,0); }
	ding_t *obj_remove_top() { return dinge.remove_last(); }

	template<typename T> T* find(uint start = 0) const { return static_cast<T*>(dinge.suche(map_ding<T>::code, start)); }

	uint8  obj_add(ding_t *obj) { return dinge.add(obj); }
	uint8 obj_remove(const ding_t* obj) { return dinge.remove(obj); }
	bool obj_loesche_alle(spieler_t *sp) { return dinge.loesche_alle(sp,offsets[flags/has_way1]); }
	bool obj_ist_da(const ding_t* obj) const { return dinge.ist_da(obj); }
	ding_t * obj_bei(uint8 n) const { return dinge.bei(n); }
	uint8  obj_count() const { return dinge.get_top()-offsets[flags/has_way1]; }
	uint8 get_top() const {return dinge.get_top();}

	// moves all object from the old to the new grund_t
	void take_obj_from( grund_t *gr);

	/**
	* @return NULL wenn OK, oder Meldung, warum nicht
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
	depot_t *get_depot() const;

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
	weg_t *get_weg_nr(int i) const { return (flags&(has_way1<<i)) ? static_cast<weg_t *>(obj_bei(i)) : NULL; }

	/**
	* Inline da sehr oft aufgerufen.
	* Sucht einen Weg vom typ 'typ' auf diesem Untergrund.
	* @author Hj. Malthaner
	*/
	weg_t *get_weg(waytype_t typ) const {
		if (weg_t* const w = get_weg_nr(0)) {
			const waytype_t wt = w->get_waytype();
			if(wt == typ) {
				return w;
			}
			else if (wt > typ) {
				// ways are ordered wrt to waytype
				return NULL;
			}
			// try second way (if exists)
			if (weg_t* const w = get_weg_nr(1)) {
				if(w->get_waytype()==typ) {
					return w;
				}
			}
		}
		return NULL;
	}

	bool has_two_ways() const { return flags&has_way2; }

	bool hat_weg(waytype_t typ) const { return get_weg(typ)!=NULL; }

	/**
	* Returns the system type s_type of a way of type typ at this location
	* Currently only needed for tramways or other different types of rails
	*
	* @author DarioK
	* @see get_weg
	*/
	uint8 get_styp(waytype_t typ) const
	{
		weg_t *weg = get_weg(typ);
		return (weg) ? weg->get_besch()->get_styp() : 0;
	}

	/**
	* Ermittelt die Richtungsbits furr den weg vom Typ 'typ'.
	* Liefert 0 wenn kein weg des Typs vorhanden ist. Ein Weg kann ggf.
	* auch 0 als Richtungsbits liefern, deshalb kann die Anwesenheit eines
	* Wegs nicht hierurber, sondern mit get_weg(), ermittelt werden.
	* Also beware of water, which always allows all directions ...thus virtual
	* @author Hj. Malthaner
	*/
	virtual ribi_t::ribi get_weg_ribi(waytype_t typ) const;

	/**
	* Ermittelt die Richtungsbits furr den weg vom Typ 'typ' unmaskiert.
	* Dies wird beim Bauen ben÷tigt. Furr die Routenfindung werden die
	* maskierten ribis benutzt.
	* @author Hj. Malthaner/V. Meyer
	*
	*/
	virtual ribi_t::ribi get_weg_ribi_unmasked(waytype_t typ) const;

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
	virtual sint8 get_weg_yoff() const { return 0; }

	/**
	* Hat der Boden mindestens ein weg_t-Objekt? Liefert false für Wasser!
	* @author V. Meyer
	*/
	inline bool hat_wege() const { return (flags&(has_way1|has_way2))!=0;}

	/**
	* Kreuzen sich hier 2 verschiedene Wege?
	* Strassenbahnschienen duerfen nicht als Kreuzung erkannt werden!
	* @author V. Meyer, dariok
	*/
	inline bool ist_uebergang() const { return (flags&has_way2)!=0  &&  ((weg_t *)dinge.bei(1))->get_besch()->get_styp()!=7; }

	/**
	* returns the vehcile of a convoi (if there)
	* @author V. Meyer
	*/
	ding_t *get_convoi_vehicle() const { return dinge.get_convoi_vehicle(); }

	virtual hang_t::typ get_weg_hang() const { return get_grund_hang(); }

	/*
	 * Search a matching wayobj
	 */
	wayobj_t *get_wayobj( waytype_t wt ) const;

	/**
	* Interface zur Bauen der Wege
	* =============================
	*/

	/**
	 * remove trees and groundobjs on this tile
	 * called before building way or powerline
	 * @return costs
	 */
	sint64 remove_trees();

	/**
	 * Bauhilfsfunktion - ein neuer weg wird mit den vorgegebenen ribis
	 * eingetragen und der Grund dem Erbauer zugeordnet.
	 *
	 * @param weg	    der neue Weg
	 * @param ribi	    die neuen ribis
	 * @param sp	    Spieler, dem der Boden zugeordnet wird
	 *
	 * @author V. Meyer
	 */
	sint64 neuen_weg_bauen(weg_t *weg, ribi_t::ribi ribi, spieler_t *sp);

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
	 *      Uses helper function "get_vmove()"
	 *
	 * Parameters:
	 *      If dir is notsingle direct it will assert
	 *      If wegtyp is set to invalid_wt, no way checking is performed
	 *
	 * In case of success:
	 *      "to" ist set to the ground found
	 *      true is returned
	 * In case of failure:
	 *      "to" ist not touched
	 *      false is returned
	 *
	 */
	bool get_neighbour(grund_t *&to, waytype_t type, ribi_t::ribi r ) const;

	/**
	 * Description;
	 *      Check, whether it is possible that a way goes up or down in ribi
	 *      direction. The result depends of the ground type (i.e tunnel entries)
	 *      and the "hang_typ" of the ground.
	 *
	 *      Returns the height if one moves in direction given by ribi
	 *
	 *      ribi must be a single direction!
	 * Notice:
	 *      helper function for "get_neighbour"
	 *
	 * This is called many times, so it is inline
	 */
	inline sint8 get_vmove(ribi_t::ribi ribi) const {
		sint8 h = pos.z;
		const hang_t::typ way_slope = get_weg_hang();

		// only on slope height may changes
		if(  way_slope  ) {
			if(ribi & ribi_t::nordost) {
				h += corner3(way_slope);
			}
			else {
				h += corner1(way_slope);
			}
		}

		/* ground and way slope are only different on tunnelmound or bridgehead
		 * since we already know both slopes, this check is faster than the virtual
		 * call involved in ist_bruecke()
		 */
		if(  way_slope != slope  ) {
			if(  ist_bruecke()  &&  slope  ) {
				h ++;	// end or start of a bridge
			}
		}

		return h;
	}

	/* removes everything from a tile, including a halt but i.e. leave a
	 * powerline ond other stuff
	 * @author prissi
	 */
	bool remove_everything_from_way(spieler_t *sp,waytype_t wt,ribi_t::ribi ribi_rem);

	void* operator new(size_t s);
	void  operator delete(void* p, size_t s);

};


#endif
