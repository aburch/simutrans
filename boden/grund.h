/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_GRUND_H
#define BODEN_GRUND_H


#include "../halthandle_t.h"
#include "../display/simimg.h"
#include "../simcolor.h"
#include "../simconst.h"
#include "../dataobj/koord3d.h"
#include "../dataobj/objlist.h"
#include "../display/clip_num.h"
#include "wege/weg.h"


class player_t;
class depot_t;
class karte_ptr_t;
class cbuffer_t;


/* A map from obj_t subtypes to their enum equivalent
 * Used by grund_t::find<T>()
 */
class air_vehicle_t;
class baum_t;
class bruecke_t;
class crossing_t;
class field_t;
class pedestrian_t;
class gebaeude_t;
class groundobj_t;
class label_t;
class leitung_t;
class pillar_t;
class pumpe_t;
class roadsign_t;
class senke_t;
class signal_t;
class private_car_t;
class road_vehicle_t;
class tunnel_t;
class wayobj_t;
class zeiger_t;

template<typename T> struct map_obj {};
template<> struct map_obj<air_vehicle_t>  { static const obj_t::typ code = obj_t::air_vehicle;  };
template<> struct map_obj<baum_t>         { static const obj_t::typ code = obj_t::baum;         };
template<> struct map_obj<bruecke_t>      { static const obj_t::typ code = obj_t::bruecke;      };
template<> struct map_obj<crossing_t>     { static const obj_t::typ code = obj_t::crossing;     };
template<> struct map_obj<field_t>        { static const obj_t::typ code = obj_t::field;        };
template<> struct map_obj<pedestrian_t>   { static const obj_t::typ code = obj_t::pedestrian;   };
template<> struct map_obj<gebaeude_t>     { static const obj_t::typ code = obj_t::gebaeude;     };
template<> struct map_obj<groundobj_t>    { static const obj_t::typ code = obj_t::groundobj;    };
template<> struct map_obj<label_t>        { static const obj_t::typ code = obj_t::label;        };
template<> struct map_obj<leitung_t>      { static const obj_t::typ code = obj_t::leitung;      };
template<> struct map_obj<pillar_t>       { static const obj_t::typ code = obj_t::pillar;       };
template<> struct map_obj<pumpe_t>        { static const obj_t::typ code = obj_t::pumpe;        };
template<> struct map_obj<roadsign_t>     { static const obj_t::typ code = obj_t::roadsign;     };
template<> struct map_obj<senke_t>        { static const obj_t::typ code = obj_t::senke;        };
template<> struct map_obj<signal_t>       { static const obj_t::typ code = obj_t::signal;       };
template<> struct map_obj<private_car_t>  { static const obj_t::typ code = obj_t::road_user;    };
template<> struct map_obj<road_vehicle_t> { static const obj_t::typ code = obj_t::road_vehicle; };
template<> struct map_obj<tunnel_t>       { static const obj_t::typ code = obj_t::tunnel;       };
template<> struct map_obj<wayobj_t>       { static const obj_t::typ code = obj_t::wayobj;       };
template<> struct map_obj<weg_t>          { static const obj_t::typ code = obj_t::way;          };
template<> struct map_obj<zeiger_t>       { static const obj_t::typ code = obj_t::zeiger;       };


template<typename T> static inline T* obj_cast(obj_t* const d)
{
	return d->get_typ() == map_obj<T>::code ? static_cast<T*>(d) : 0;
}


/**
 * <p>Abstract basic class for ground in Simutrans.</p>
 *
 * <p>All ground types (Land, Water, Roads ...) are derived of
 * the class grund_t in Simutrans. Every tile has a ground type.</p>
 *
 * <p>Der Boden hat Eigenschaften, die abgefragt werden koennen
 * ist_natur(), is_water(), hat_wegtyp(), ist_bruecke().
 * In dieser Basisklasse sind alle Eigenschaften false, sie werden erst
 * in den Subklassen redefiniert.</p>
 */
class grund_t
{
public:
	/**
	 * Flag values for different ground properties
	 */
	enum flag_values {
		keine_flags    = 0,
		dirty          = 1 << 0, ///< was changed => redraw full
		is_kartenboden = 1 << 1,
		has_text       = 1 << 2,
		marked         = 1 << 3, ///< will have a frame
		draw_as_obj    = 1 << 4, ///< is a slope etc => draw as one
		is_halt_flag   = 1 << 5, ///< is a part of a halt
		has_way1       = 1 << 6,
		has_way2       = 1 << 7
	};

	/**
	 * @brief Back wall corner count.
	 *
	 * Number of corners used to produce tile back walls. Visually these corners
	 * are the left, top and right corners of a tile.
	 */
	static size_t const BACK_CORNER_COUNT = 3;

	/**
	 * @brief Back wall count.
	 *
	 * Number of back walls a tile can have. Visually these walls are along the
	 * top left and right edges of the tile.
	 */
	static size_t const BACK_WALL_COUNT = BACK_CORNER_COUNT - 1;

	/**
	 * @brief Number of wall images per wall.
	 *
	 * Number of unique wall image graphics per wall.
	 */
	static uint16 const WALL_IMAGE_COUNT = 11;

	/**
	 * @brief Number of fence images.
	 *
	 * Number of unique fence image graphics available. Unlike walls, fence
	 * images are for an entire tile.
	 */
	static uint16 const FENCE_IMAGE_COUNT = 3;

	/**
	 * @brief Back image ID offset for encoding fences.
	 *
	 * The offset used to encode the fence image into a back image ID. Anything
	 * less than this offset can be considered a wall.
	 */
	static sint8 const BIID_ENCODE_FENCE_OFFSET = (sint8)(WALL_IMAGE_COUNT * WALL_IMAGE_COUNT);

	/**
	 * @brief Maximum distance in tiles that hide test will be performed for.
	 *
	 * Maximum distance in tiles that object hide test will be performed for.
	 * The hide test is needed for correct graphic reproduction of tunnel
	 * entrances and such.
	 */
	static uint16 const MAXIMUM_HIDE_TEST_DISTANCE = 5;

	// just to calculate the offset for skipping the ways ...
	static uint8 offsets[4];

public:
	/** true, when showing a grid
	 */
	static volatile bool show_grid;

	/** underground modes */
	enum _underground_modes {
		ugm_none = 0, // normal view
		ugm_all  = 1, // everything underground visible, grid for grounds
		ugm_level= 2  // overground things visible if their height  <= underground_level
		              // underground things visible if their height == underground_level
	};
	static uint8 underground_mode;
	static sint8 underground_level;

protected:
	/**
	 * List of objects on this tile
	 * Pointer (changes occasionally) + 8 bits + 8 bits (changes often)
	 */
	objlist_t objlist;

	/**
	 * Handle to halt built on this ground
	 */
	halthandle_t this_halt;

	/**
	 * Image number
	 */
	image_id imageid;

	/**
	 * Coordinate (40 bits)
	 */
	koord3d pos;

	/**
	 * Slope (now saved locally), because different grounds need different slopes
	 */
	slope_t::type slope;

	/**
	 * Image of the walls
	 */
	sint8 back_imageid;

	/**
	 * Flags to indicate existence of halts, ways, to mark dirty
	 */
	uint8 flags;


public:
	/**
	 * setzt die Bildnr. des anzuzeigenden Bodens
	 */
	inline void set_image(image_id n) {
		imageid = n;
		set_flag(dirty);
	}

protected:
	/**
	* Pointer to the world of this ground. Static to conserve space.
	* Change to instance variable once more than one world is available.
	*/
	static karte_ptr_t welt;

	// calculates the slope image and sets the draw_as_obj flag correctly
	void calc_back_image(const sint8 hgt,const slope_t::type slope_this);

	// this is the real image calculation, called for the actual ground image
	virtual void calc_image_internal(const bool calc_only_snowline_change) = 0;

public:
	enum typ {
		boden = 1,
		wasser,
		fundament,
		tunnelboden,
		brueckenboden,
		monorailboden
	};

	grund_t(koord3d pos);

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

	/**
	* Set Flags for the newly drawn changed ground
	*/
	inline void set_flag(flag_values flag) {flags |= flag;}

	inline void clear_flag(flag_values flag) {flags &= ~flag;}
	inline bool get_flag(flag_values flag) const {return (flags & flag) != 0;}

	/**
	* Updates snowline dependent grund_t (and derivatives) - none are season dependent
	* Updates season and or snowline dependent objects
	*/
	void check_season_snowline(const bool season_change, const bool snowline_change)
	{
		if(  snowline_change  ) {
			calc_image_internal( snowline_change );
		}

		objlist.check_season( season_change  &&  !snowline_change );
	}

	/**
	 * Updates images after change of underground mode.
	 */
	void check_update_underground()
	{
		if (ist_tunnel()  ||  ist_bruecke()  ||  is_water()) {
			calc_image();
		}
		else {
			calc_back_image( get_disp_height(), get_disp_slope() );
		}
	}

	/**
	 * Sets all objects to dirty to prevent artifacts with smart hide cursor
	 */
	void set_all_obj_dirty() { objlist.set_all_dirty(); }

	/**
	 * Dient zur Neuberechnung des Bildes, wenn sich die Umgebung
	 * oder die Lage (Hang) des grundes geaendert hat.
	 */
	void calc_image();

	/**
	* Return the number of images the ground has.
	* @return The number of images.
	*/
	inline image_id get_image() const {return imageid;}

	/**
	* Returns the number of an eventual foundation
	*/
	image_id get_back_image(int leftback) const;
	virtual void clear_back_image() {back_imageid=0;}

	/**
	* if ground is deleted mark the old spot as dirty
	*/
	void mark_image_dirty();

	/**
	* Return the name of the ground.
	* @return The name of the ground.
	*/
	virtual const char* get_name() const = 0;

	/**
	* Return the ground type.
	* @return The ground type.
	*/
	virtual typ get_typ() const = 0;

	/**
	* Return the ground description texts.
	* @return A description for the ground.
	*/
	const char* get_text() const;

	virtual void info(cbuffer_t & buf) const;

	/**
	* Auffforderung, ein Infofenster zu oeffnen.
	* Oeffnet standardmaessig kein Infofenster.
	*/
	void open_info_window();

	/**
	 * @return player that owns the label to show it in player's colors
	 */
	const player_t* get_label_owner() const;

	/**
	 * Sets the label text (by copying it)
	 * @param text the new text (NULL will remove any label text)
	 */
	void set_text(const char* text);

	virtual bool ist_natur() const {return false;}
	virtual bool is_water() const {return false;}

	/**
	* This is called very often, it must be inlined and therefore
	* cannot be virtual - subclasses must set the flags appropriately!
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
	*/
	leitung_t *get_leitung() const { return (leitung_t *) objlist.get_leitung(); }

	/**
	* Laedt oder speichert die Daten des Untergrundes in eine Datei.
	* @param file Zeiger auf die Datei in die gespeichert werden soll.
	*/
	virtual void rdwr(loadsave_t *file);

	// map rotation
	virtual void rotate90();

	// we must put the text back to their proper location after rotation ...
	static void finish_rotate90();

	// since enlargement will require new hashes
	static void enlarge_map( sint16 new_size_x, sint16 new_size_y );

	void sort_trees();

	/**
	* Gibt die 3d-Koordinaten des Planquadrates zurueck, zu dem der
	* Untergrund gehoert.
	* @return Die Position des Grundes in der 3d-Welt
	*/
	inline const koord3d& get_pos() const { return pos; }

	inline void set_pos(koord3d newpos) { pos = newpos;}

	// slope are now maintained locally
	slope_t::type get_grund_hang() const { return slope; }
	void set_grund_hang(slope_t::type sl) { slope = sl; }

	/**
	 * some ground tiles may be part of halts.
	 */
	void set_halt(halthandle_t halt);
	halthandle_t get_halt() const;
	bool is_halt() const { return flags & is_halt_flag; }

	/**
	 * @return The height of the tile.
	 */
	inline sint8 get_hoehe() const {return pos.z;}

	/**
	 * @param corner slope_t::_corner mask of corners to check.
	 * @return The height of the tile at the requested corner.
	 */
	inline sint8 get_hoehe(slope4_t::type corner) const
	{
		switch(  corner  ) {
			case slope4_t::corner_SW: {
				return pos.z + corner_sw(slope);
				break;
			}
			case slope4_t::corner_SE: {
				return pos.z + corner_se(slope);
				break;
			}
			case slope4_t::corner_NE: {
				return pos.z + corner_ne(slope);
				break;
			}
			default: {
				return pos.z + corner_nw(slope);
				break;
			}
		}
	}

	void set_hoehe(sint8 h) { pos.z = h;}

	// Helper functions for underground modes
	//
	// returns the height for the use in underground-mode,
	// heights above underground_level are cut
	inline sint8 get_disp_height() const {
		return (underground_mode & ugm_level )
			? (pos.z > underground_level ? underground_level : pos.z)
			: pos.z ;
	}

	// returns slope
	// if tile is not visible, 'flat' is returned
	// special care has to be taken of tunnel mouths
	inline slope_t::type get_disp_slope() const {
		return (  (underground_mode & ugm_level)  &&  (pos.z > underground_level  ||  (get_typ()==tunnelboden  &&  ist_karten_boden()  &&  pos.z == underground_level))
							? (slope_t::type)slope_t::flat
							: get_grund_hang() );

		/*switch(underground_mode) {// long version of the return statement above
			case ugm_none: return(get_grund_hang());
			case ugm_all:  return(get_grund_hang()); // get_typ()==tunnelboden && !ist_karten? slope_t::flat : get_grund_hang());
			case ugm_level:return((pos.z > underground_level || (get_typ()==tunnelboden && ist_karten_boden() && pos.z == underground_level))
							? slope_t::flat
							: get_grund_hang());
		}*/
	}

	inline bool is_visible() const {
		if(get_typ()==tunnelboden) {
			switch(underground_mode) {
				case ugm_none: return ist_karten_boden();
				case ugm_all:  return true;
				case ugm_level:return  pos.z == underground_level  ||  pos.z+max(max(corner_sw(slope),corner_se(slope)),max(corner_ne(slope),corner_nw(slope))) == underground_level  ||  (ist_karten_boden()  &&  pos.z <= underground_level);
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
	slope_t::type get_disp_way_slope() const;

	/**
	 * Displays the ground images (including foundations, fences and ways)
	 */
#ifdef MULTI_THREAD
	void display_boden(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, const sint8 clip_num, bool force_show_grid=false) const;
#else
	void display_boden(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width) const;
#endif

	/**
	 * Displays the earth at the border
	 */
	void display_border( sint16 xpos, sint16 ypos, const sint16 raster_tile_width CLIP_NUM_DEF);

	/**
	 * Displays the tile if it's visible.
	 * @see is_karten_boden_visible()
	 */
#ifdef MULTI_THREAD
	void display_if_visible(sint16 xpos, sint16 ypos, const sint16 raster_tile_width, const sint8 clip_num, bool force_show_grid=false);
#else
	void display_if_visible(sint16 xpos, sint16 ypos, const sint16 raster_tile_width);
#endif

	/**
	 * displays everything that is on a tile - the main display routine for objects on tiles
	 * @param is_global set to true, if this is called during the whole screen update
	 */
	void display_obj_all(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, const bool is_global CLIP_NUM_DEF) const;

	/**
	 * similar to above but yields clipping error
	 * => only used for zoom out
	 * @param is_global set to true, if this is called during the whole screen update
	 */
	void display_obj_all_quick_and_dirty(const sint16 xpos, sint16 ypos, const sint16 raster_tile_width, const bool is_global CLIP_NUM_DEF) const;

	/**
	 * displays background images of all non-moving objects on the tile
	 * @param is_global set to true, if this is called during the whole screen update
	 * @param draw_ways if true then draw images of ways
	 * @param visible if false then draw only grids and markers
	 * @return index of first vehicle on the tile
	 */
	uint8 display_obj_bg(const sint16 xpos, const sint16 ypos, const bool is_global, const bool draw_ways, const bool visible  CLIP_NUM_DEF) const;

	/**
	 * displays vehicle (background) images
	 * @param start_offset start with object at this index
	 * @param ribi draws only vehicles driving in this direction (or against this)
	 * @param ontile is true if we are on the tile that defines the clipping
	 */
	uint8 display_obj_vh(const sint16 xpos, const sint16 ypos, const uint8 start_offset, const ribi_t::ribi ribi, const bool ontile  CLIP_NUM_DEF) const;

	/**
	 * displays all foreground images
	 * @param is_global set to true, if this is called during the whole screen update
	 */
	void display_obj_fg(const sint16 xpos, const sint16 ypos, const bool is_global, const uint8 start_offset  CLIP_NUM_DEF) const;

	/**
	 * overlay with signs, good levels and station coverage
	 * resets the dirty flag
	 */
	void display_overlay(sint16 xpos, sint16 ypos);

	inline obj_t *first_obj() const { return objlist.bei(offsets[flags/has_way1]); }
	obj_t *suche_obj(obj_t::typ typ) const { return objlist.suche(typ,0); }
	obj_t *obj_remove_top() { return objlist.remove_last(); }

	template<typename T> T* find(uint start = 0) const { return static_cast<T*>(objlist.suche(map_obj<T>::code, start)); }

	bool obj_add(obj_t *obj) { return objlist.add(obj); }
	bool obj_remove(const obj_t* obj) { return objlist.remove(obj); }
	bool obj_loesche_alle(player_t *player) { return objlist.loesche_alle(player,offsets[flags/has_way1]); }
	bool obj_ist_da(const obj_t* obj) const { return objlist.ist_da(obj); }
	obj_t * obj_bei(uint8 n) const { return objlist.bei(n); }
	uint8  obj_count() const { return objlist.get_top()-offsets[flags/has_way1]; }
	uint8 get_top() const {return objlist.get_top();}

	// moves all object from the old to the new grund_t
	void take_obj_from( grund_t *gr);

	/**
	* @return NULL wenn OK, oder Meldung, warum nicht
	*/
	const char * kann_alle_obj_entfernen(const player_t *player) const { return objlist.kann_alle_entfernen(player,offsets[flags/has_way1]); }

	/**
	* Interface zur Bauen und abfragen von Gebaeuden
	* =============================================
	*/

	/**
	* Falls es hier ein Depot gibt, dieses zurueckliefern
	*/
	depot_t *get_depot() const;

	/*
	* Interface zur Abfrage der Wege
	* ==============================
	* Jeder Boden hat bis zu 2. Special fuer Water: ohne Weg-Objekt werden
	* alle ribis vom weg_t::wassert als gesetzt zurueckgeliefert.
	*/

	/**
	* The only way to get the type (typ) of a way on a tile
	*/
	weg_t *get_weg_nr(int i) const { return (flags&(has_way1<<i)) ? static_cast<weg_t *>(obj_bei(i)) : NULL; }

	/**
	* Inline da sehr oft aufgerufen.
	* Sucht einen Weg vom typ 'typ' auf diesem Untergrund.
	*/
	weg_t *get_weg(waytype_t typ) const {
		if (weg_t* const w = get_weg_nr(0)) {
			const waytype_t wt = w->get_waytype();
			if(wt == typ || (typ == any_wt && wt > 0)) {
				return w;
			}
			else if (wt > typ) {
				// ways are ordered wrt to waytype
				return NULL;
			}
			if (flags & has_way2) {
				// try second way (if exists)
				if (weg_t* const w = get_weg_nr(1)) {
					if (w->get_waytype() == typ) {
						return w;
					}
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
	* @see get_weg
	*/
	uint8 get_styp(waytype_t typ) const
	{
		weg_t *weg = get_weg(typ);
		return (weg) ? weg->get_desc()->get_styp() : 0;
	}

	/**
	* Ermittelt die Richtungsbits furr den weg vom Typ 'typ'.
	* Liefert 0 wenn kein weg des Typs vorhanden ist. Ein Weg kann ggf.
	* auch 0 als Richtungsbits liefern, deshalb kann die Anwesenheit eines
	* Wegs nicht hierurber, sondern mit get_weg(), ermittelt werden.
	* Also beware of water, which always allows all directions ...thus virtual
	*/
	virtual ribi_t::ribi get_weg_ribi(waytype_t typ) const;

	/**
	 * @returns the ribis (unmasked) for waytype @p typ.
	 * These are required e.g. for building. For pathfinding masked ribis are used.
	 */
	virtual ribi_t::ribi get_weg_ribi_unmasked(waytype_t typ) const;

	/**
	* checks a ways on this ground tile and returns the highest speedlimit.
	* only used for the minimap
	*/
	int get_max_speed() const;

	/**
	* only used for bridges, which start at a slope
	*/
	virtual sint8 get_weg_yoff() const { return 0; }


	/// @returns true if there is at least one way on this ground.
	inline bool hat_wege() const { return (flags&(has_way1|has_way2))!=0;}

	/**
	* Kreuzen sich hier 2 verschiedene Wege?
	* Strassenbahnschienen duerfen nicht als Kreuzung erkannt werden!
	*/
	inline bool ist_uebergang() const { return (flags&has_way2)!=0  &&  ((weg_t *)objlist.bei(1))->get_desc()->get_styp()!=type_tram; }

	/**
	* returns the vehicle of a convoi (if there)
	*/
	obj_t *get_convoi_vehicle() const { return objlist.get_convoi_vehicle(); }

	virtual slope_t::type get_weg_hang() const { return get_grund_hang(); }

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
	 * A new way is built with the given ribis. Registered and assigned to the builder.
	 * @param weg     der neue Weg
	 * @param ribi    die neuen ribis
	 * @param player  Player building the way
	 */
	sint64 neuen_weg_bauen(weg_t *weg, ribi_t::ribi ribi, player_t *player);

	/**
	 * Bauhilfsfunktion - die ribis eines vorhandenen weges werden erweitert
	 *
	 * @return bool  true, falls weg vorhanden
	 * @param wegtyp um welchen wegtyp geht es
	 * @param ribi   die neuen ribis
	 */
	bool weg_erweitern(waytype_t wegtyp, ribi_t::ribi ribi);

	/**
	 * Bauhilfsfunktion - einen Weg entfernen
	 *
	 * @param wegtyp   um welchen wegtyp geht es
	 * @param ribi_rem sollen die ribis der nachbar zururckgesetzt werden?
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
	 *      and the "slope_type" of the ground.
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
		const slope_t::type way_slope = get_weg_hang();

		// only on slope height may changes
		if(  way_slope!=slope_t::flat  ) {
			if(ribi & ribi_t::northeast) {
				h += corner_ne(way_slope);
			}
			else {
				h += corner_sw(way_slope);
			}
		}

		/* ground and way slope are only different on tunnelmound or bridgehead
		 * since we already know both slopes, this check is faster than the virtual
		 * call involved in ist_bruecke()
		 */
		if(  way_slope != slope  ) {
			if(  ist_bruecke()  &&  slope  ) {
				// calculate height quicker because we know that slope exists and is north, south, east or west
				h += is_one_high(slope) ? 1 : 2;
			}
		}

		return h;
	}

	/** removes everything from a tile, including a halt but i.e. leave a
	 * powerline ond other stuff
	 */
	bool remove_everything_from_way(player_t *player,waytype_t wt,ribi_t::ribi ribi_rem);

	/// @returns true if this is a dummy ground that is only there for UI purposes
	/// (previews for bridges, elevated ways and tunnels)
	bool is_dummy_ground() const;

	void* operator new(size_t s);
	void  operator delete(void* p, size_t s);
};


#endif
