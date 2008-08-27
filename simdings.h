/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef simdings_h
#define simdings_h

#include "simtypes.h"
#include "simimg.h"
#include "simcolor.h"
#include "dataobj/koord3d.h"


class cbuffer_t;
class fabrik_t;
class karte_t;
class spieler_t;

/**
 * Von der Klasse ding_t sind alle Objekte in Simutrans abgeleitet.
 * since everything is a ding on the map, we need to make this a compact and fast as possible
 *
 * @author Hj. Malthaner
 * @see planqadrat_t
 */
class ding_t
{
public:
	// flags
	enum flag_values {keine_flags=0, dirty=1, not_on_map=2, is_vehicle=4, is_wayding=8 };

private:
	/**
	* Dies ist die Koordinate des Planquadrates in der Karte zu
	* dem das Objekt gehört.
	* @author Hj. Malthaner
	*/
	koord3d pos;

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

private:
	/**
	* Used by all constructors to initialize all vars with safe values
	* -> single source principle
	* @author Hj. Malthaner
	*/
	void init(karte_t *welt);

protected:
	ding_t(karte_t *welt);

	// since we need often access during loading
	void set_player_nr(sint8 s) { besitzer_n = s; }

	/**
	* Pointer to the world of this thing. Static to conserve space.
	* Change to instance variable once more than one world is available.
	* @author Hj. Malthaner
	*/
	static karte_t *welt;


public:
	// needed for drawinf images
	sint8 get_player_nr() const { return besitzer_n; }

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

	/**
	* setzt ein flag im flag-set des dings. Siehe auch flag_values
	* @author Hj. Malthaner
	*/
	inline void set_flag(enum flag_values flag) {flags |= flag;}
	inline void clear_flag(enum flag_values flag) {flags &= ~flag;}
	inline bool get_flag(enum flag_values flag) const {return ((flags & flag) != 0);}
	enum typ {
		undefined=-1, ding=0, baum=1, zeiger=2,
		wolke=3, sync_wolke=4, async_wolke=5,

		gebaeude=7,	// animated buildings (6 not used any more)
		signal=8,

		bruecke=9, tunnel=10,
		bahndepot=12, strassendepot=13, schiffdepot = 14,

		raucher=15, // obsolete
		leitung = 16, pumpe = 17, senke = 18,
		roadsign = 19, pillar = 20,

		airdepot = 21, monoraildepot=22, tramdepot=23, maglevdepot=24,

		wayobj = 25,
		way = 26, // since 99.04 ways are normal things and stored in the dingliste_t!

		label = 27,	// indicates ownership
		field = 28,
		crossing = 29,
		groundobj = 30, // lakes, stones

		narrowgaugedepot=31,

		// after this only moving stuff
		// vehikel sind von 64 bis 95
		fussgaenger=64,
		verkehr=65,
		automobil=66,
		waggon=67,
		monorailwaggon=68,
		maglevwaggon=69,
		narrowgaugewaggon=70,
		schiff=80,
		aircraft=81,
		movingobj=82,

		// other new dings (obsolete, only used during loading old games
		// lagerhaus = 24, (never really used)
		// gebaeude_alt=6,	(very, very old?)
		old_gebaeudefundament=11,	// wall below buildings, not used any more
		old_automobil=32, old_waggon=33,
		old_schiff=34, old_aircraft=35, old_monorailwaggon=36,
		old_verkehr=41,
		old_fussgaenger=42,
		old_choosesignal = 95,
		old_presignal = 96,
		old_roadsign = 97,
		old_pillar = 98,
		old_airdepot = 99,
		old_monoraildepot=100,
		old_tramdepot=101,
	};

	inline sint8 gib_xoff() const {return xoff;}
	inline sint8 gib_yoff() const {return yoff;}

	// true for all moving objects
	inline bool is_moving() const { return flags&is_vehicle; }

	// true for all moving objects
	inline bool is_way() const { return flags&is_wayding; }

	// while in principle, this should trigger the dirty, it takes just too much time to do it
	// TAKE CARE OF SET IT DIRTY YOURSELF!!!
	inline void setze_xoff(sint8 xoff) {this->xoff = xoff; }
	inline void setze_yoff(sint8 yoff) {this->yoff = yoff; }

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

	karte_t* get_welt() const { return welt; }

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
	virtual void entferne(spieler_t *) {}

	/**
	 * 'Jedes Ding braucht einen Namen.'
	 * @return Gibt den unübersetzten(!) Namen des Objekts zurück.
	 * @author Hj. Malthaner
	 */
	virtual const char *gib_name() const {return "Ding";}

	/**
	 * 'Jedes Ding braucht einen Typ.'
	 * @return Gibt den typ des Objekts zurück.
	 * @author Hj. Malthaner
	 */
	virtual enum ding_t::typ gib_typ() const = 0;

	/*
	* called whenever the snowline height changes
	* return false and the ding_t will be deleted
	* @author prissi
	*/
	virtual bool check_season(const long /*month*/) { return true; }

	/**
	 * called during map rotation
	 * @author priss
	 */
	virtual void rotate90();

	/**
	 * Jedes Objekt braucht ein Bild.
	 * @return Die Nummer des aktuellen Bildes für das Objekt.
	 * @author Hj. Malthaner
	 */
	virtual image_id gib_bild() const = 0;

	/**
	 * give image for height > 0 (max. height currently 3)
	 * IMG_LEER is no images
	 * @author Hj. Malthaner
	 */
	virtual image_id gib_bild(int /*height*/) const {return IMG_LEER;}

	/**
	 * this image is draw after all gib_bild() on this tile
	 * Currently only single height is supported for this feature
	 */
	virtual image_id gib_after_bild() const {return IMG_LEER;}

	/**
	 * if a function return here a value with TRANSPARENT_FLAGS set
	 * then a transparent outline with the color form the lower 8 Bit is drawn
	 * @author kierongreen
	 */
	virtual PLAYER_COLOR_VAL gib_outline_colour() const {return 0;}

	/**
	 * The image, that will be outlined
	 * @author kierongreen
	 */
	virtual PLAYER_COLOR_VAL gib_outline_bild() const {return IMG_LEER;}

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
	virtual void laden_abschliessen() {}

	/**
	 * Ein Objekt gehört immer zu einem grund_t
	 * @return Die aktuellen Koordinaten des Grundes.
	 * @author V. Meyer
	 * @see ding_t#ding_t
	 */
	inline koord3d gib_pos() const {return pos;}

	// only zeiger_t overlays this function, so virtual definition is overkill
	inline void setze_pos(koord3d k) { if(k!=pos) { set_flag(dirty); pos = k;} }

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

	/**
	 * @returns NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	virtual const char *ist_entfernbar(const spieler_t *sp);

	/**
	 * Ding zeichnen
	 * @author Hj. Malthaner
	 */
	void display(int xpos, int ypos, bool dirty) const;

	/**
	 * 2. Teil zeichnen (was hinter Fahrzeugen kommt
	 * @author V. Meyer
	 */
	virtual void display_after(int xpos, int ypos, bool dirty) const;

	/*
	* when a vehicle moves or a cloud moves, it needs to mark the old spot as dirty (to copy to screen)
	* sometimes they have an extra offset, this the yoff parameter
	* @author prissi
	*/
	void mark_image_dirty(image_id bild,sint8 yoff) const;

	/**
	 * Dient zur Neuberechnung des Bildes
	 * @author Hj. Malthaner
	 */
	virtual void calc_bild() {}
} GCC_PACKED;

#endif
