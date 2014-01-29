/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef simobjs_h
#define simobjs_h

#include "simtypes.h"
#include "display/simimg.h"
#include "simcolor.h"
#include "dataobj/koord3d.h"


class cbuffer_t;
class karte_ptr_t;
class spieler_t;

/**
 * Base class of all objects on the map, obj == thing
 * Since everything is a 'obj' on the map, we need to make this as compact and fast as possible.
 *
 * @author Hj. Malthaner
 */
class obj_t
{
public:
	// flags
	enum flag_values {
		keine_flags=0,  /// no special properties
		dirty=1,        /// mark image dirty when drawing
		not_on_map=2,   /// this object is not placed on any tile (e.g. vehicles in a depot)
		is_vehicle=4,   /// this object is a vehicle obviously
		highlight=8      /// for drawing some highlighted outline
	};

	// display only outline with player color on owner stuff
	static bool show_owner;

private:
	obj_t(obj_t const&);
	obj_t& operator=(obj_t const&);

	/**
	 * Coordinate of position
	 */
	koord3d pos;

	/**
	 * x-offset of the object on the tile
	 * used for drawing object image
	 * if xoff and yoff are between 0 and OBJECT_OFFSET_STEPS-1 then
	 * the top-left corner of the image is within tile boundaries
	 */
	sint8 xoff;

	/**
	 * y-offset of the object on the tile
	 */
	sint8 yoff;

	/**
	 * Owner of the object (1 - public player, 15 - unowned)
	 */
	uint8 besitzer_n:4;

	/**
	 * @see flag_values
	 */
	uint8 flags:4;

private:
	/**
	* Used by all constructors to initialize all vars with safe values
	* -> single source principle
	* @author Hj. Malthaner
	*/
	void init();

protected:
	obj_t();

	// since we often need access during loading
	void set_player_nr(uint8 s) { besitzer_n = s; }

	/**
	* Pointer to the world of this thing. Static to conserve space.
	* Change to instance variable once more than one world is available.
	* @author Hj. Malthaner
	*/
	static karte_ptr_t welt;


public:
	// needed for drawing images
	sint8 get_player_nr() const { return besitzer_n; }

	/**
	 * sets owner of object
	 */
	void set_besitzer(spieler_t *sp);

	/**
	 * returns owner of object
	 */
	spieler_t * get_besitzer() const;

	/**
	 * routines to set, clear, get bit flags
	 * @author Hj. Malthaner
	 */
	inline void set_flag(flag_values flag) {flags |= flag;}
	inline void clear_flag(flag_values flag) {flags &= ~flag;}
	inline bool get_flag(flag_values flag) const {return ((flags & flag) != 0);}

	/// all the different types of objects
	enum typ {
		undefined=-1, obj=0, baum=1, zeiger=2,
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
		way = 26, // since 99.04 ways are normal things and stored in the objliste_t!

		label = 27,	// indicates ownership
		field = 28,
		crossing = 29,
		groundobj = 30, // lakes, stones

		narrowgaugedepot=31,

		// after this only moving stuff
		// reserved values for vehicles: 64 to 95
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

		// other new objs (obsolete, only used during loading old games
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

	inline sint8 get_xoff() const {return xoff;}
	inline sint8 get_yoff() const {return yoff;}

	// true for all moving objects
	inline bool is_moving() const { return flags&is_vehicle; }

	// while in principle, this should trigger the dirty, it takes just too much time to do it
	// TAKE CARE OF SET IT DIRTY YOURSELF!!!
	inline void set_xoff(sint8 xoff) {this->xoff = xoff; }
	inline void set_yoff(sint8 yoff) {this->yoff = yoff; }

	/**
	 * Constructor to load object from file
	 * @author Hj. Malthaner
	 */
	obj_t(loadsave_t *file);

	/**
	 * Constructor to set position of object
	 * This does *not* add the object to the tile
	 * @author Hj. Malthaner
	 */
	obj_t(koord3d pos);

	/**
	 * Destructor: removes object from tile, should close any inspection windows
	 * @author Hj. Malthaner
	 */
	virtual ~obj_t();

	/**
	 * Routine for cleanup if object is removed (ie book maintenance, cost for removal)
	 * @author Hj. Malthaner
	 */
	virtual void entferne(spieler_t *) {}

	/**
	 * @returns untranslated name of object
	 * @author Hj. Malthaner
	 */
	virtual const char *get_name() const {return "Ding";}

	/**
	 * @return object type
	 * @author Hj. Malthaner
	 * @see typ
	 */
	virtual typ get_typ() const = 0;

	/**
	 * waytype associated with this object
	 */
	virtual waytype_t get_waytype() const { return invalid_wt; }

	/**
	 * called whenever the snowline height changes
	 * return false and the obj_t will be deleted
	 * @author prissi
	 */
	virtual bool check_season(const long /*month*/) { return true; }

	/**
	 * called during map rotation
	 * @author priss
	 */
	virtual void rotate90();

	/**
	 * Every object needs an image.
	 * @return number of current image for that object
	 * @author Hj. Malthaner
	 */
	virtual image_id get_bild() const = 0;

	/**
	 * give image for height > 0 (max. height currently 3)
	 * IMG_LEER is no images
	 * @author Hj. Malthaner
	 */
	virtual image_id get_bild(int /*height*/) const {return IMG_LEER;}

	/**
	 * this image is drawn after all get_bild() on this tile
	 * Currently only single height is supported for this feature
	 */
	virtual image_id get_after_bild() const {return IMG_LEER;}

	/**
	 * if a function returns a value here with TRANSPARENT_FLAGS set
	 * then a transparent outline with the color from the lower 8 bit is drawn
	 * @author kierongreen
	 */
	virtual PLAYER_COLOR_VAL get_outline_colour() const {return 0;}

	/**
	 * The image, that will be outlined
	 * @author kierongreen
	 */
	virtual image_id get_outline_bild() const { return IMG_LEER; }

	/**
	 * Save and Load of object data in one routine
	 * @author Hj. Malthaner
	 */
	virtual void rdwr(loadsave_t *file);

	/**
	 * Called after the world is completely loaded from savegame
	 *
	 * @author Hj. Malthaner
	 */
	virtual void laden_abschliessen() {}

	/**
	 * @return position
	 */
	inline koord3d get_pos() const {return pos;}

	/**
	 * set position - you would not have guessed it :)
	 */
	inline void set_pos(koord3d k) { if(k!=pos) { set_flag(dirty); pos = k;} }

	/**
	 * put description of object into the buffer
	 * (used for certain windows)
	 * @author Hj. Malthaner
	 * @see simwin
	 */
	virtual void info(cbuffer_t & buf) const;

	/**
	 * Opens a new info window for the object
	 * @author Hj. Malthaner
	 */
	virtual void zeige_info();

	/**
	 * @return NULL if OK, otherwise an error message
	 * @author Hj. Malthaner
	 */
	virtual const char *ist_entfernbar(const spieler_t *sp);

	/**
	 * Draw backgroung image of object
	 * (everything that could be potentially behind vehicles)
	 */
#ifdef MULTI_THREAD
	void display(int xpos, int ypos, const sint8 clip_num) const;
#else
	void display(int xpos, int ypos) const;
#endif

	/**
	 * Draw foreground image
	 * (everything that is in front of vehicles)
	 */
#ifdef MULTI_THREAD
	virtual void display_after(int xpos, int ypos, const sint8 clip_num) const;
#else
	virtual void display_after(int xpos, int ypos, bool is_global) const;
#endif

#ifdef MULTI_THREAD
	/**
	 * Draw overlays
	 * (convoi tooltips)
	 */
	virtual void display_overlay(int /*xpos*/, int /*ypos*/) const { return; }
#endif

	/**
	* When a vehicle moves or a cloud moves, it needs to mark the old spot as dirty (to copy to screen).
	* This routine already takes position, and offsets (x_off, y_off) into account.
	* @param yoff extra y-offset, in most cases 0.
	* @author prissi
	*/
	void mark_image_dirty(image_id bild, sint16 yoff) const;

	/**
	 * Function for recalculating the image.
	 * @author Hj. Malthaner
	 */
	virtual void calc_bild() {}
};


/**
 * Template to do casting of pointers based on obj_t::typ
 * as a replacement of the slower dynamic_cast<>
 */
template<typename T> static T* obj_cast(obj_t*);

template<typename T> static inline T const* obj_cast(obj_t const* const d)
{
	return obj_cast<T>(const_cast<obj_t*>(d));
}


/**
 * Game objects that do not have description windows (for instance zeiger_t, wolke_t)
 */
class obj_no_info_t : public obj_t
{
public:
	obj_no_info_t(loadsave_t* file) : obj_t(file) {}

	obj_no_info_t(koord3d pos) : obj_t(pos) {}

	void zeige_info() {}

protected:
	obj_no_info_t() : obj_t() {}
};

#endif
