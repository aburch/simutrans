/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __HAUS_BESCH_H
#define __HAUS_BESCH_H

#include <assert.h>
#include "bildliste2d_besch.h"
#include "obj_besch_std_name.h"
#include "skin_besch.h"
#include "../dings/gebaeude.h"


class haus_besch_t;
class werkzeug_t;
class checksum_t;

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Das komplette Bild besteht aus Hinter- und Vorgergrund. Außerdem ist
 *      hier die Anzahl der Animationen festgelegt. Diese weiter unten zu
 *      definieren macht kaum Sinn, da die Animationslogik immer das ganze
 *      Tile betrifft.
 *
 *  Kindknoten:
 *   0   Imagelist2D season 0 back
 *   1   Imagelist2D season 0 front
 *   2   Imagelist2D season 1 back
 *   3   Imagelist2D season 1 front
 *	... ...
 */
class haus_tile_besch_t : public obj_besch_t {
	friend class tile_reader_t;

	const haus_besch_t	*haus;

	uint8  seasons;
	uint8  phasen;	    // Wie viele Animationsphasen haben wir?
	uint16 index;

public:
	void set_besch(const haus_besch_t *haus_besch) { haus = haus_besch; }

	const haus_besch_t *get_besch() const { return haus; }

	int get_index() const { return index; }
	int get_seasons() const { return seasons; }
	int get_phasen() const { return phasen; }

	bool has_image() const {
		return get_hintergrund(0,0,0)!=IMG_LEER  ||  get_vordergrund(0,0)!=IMG_LEER;
	}

	image_id get_hintergrund(int phase, int hoehe, int season) const
	{
		season &= (seasons-1);
		bildliste2d_besch_t const* const bl = get_child<bildliste2d_besch_t>(0 + 2 * season);
		if(phase>0 && phase<phasen) {
			if (bild_besch_t const* const bild = bl->get_bild(hoehe, phase)) {
				return bild->get_nummer();
			}
		}
		// here if this phase does not exists ...
		bild_besch_t const* const bild = bl->get_bild(hoehe, 0);
		return bild != NULL ? bild->get_nummer() : IMG_LEER;
	}

	// returns true, if the background is animated
	bool is_hintergrund_phases(int season) const
	{
		season &= (seasons-1);
		bildliste2d_besch_t const* const bl = get_child<bildliste2d_besch_t>(0 + 2 * season);
		const uint16 max_h = bl->get_anzahl();
		for(  uint16 phase=1;  phase<phasen;  phase++  ) {
			for(  uint16 h=0;  h<max_h;  h++  ) {
				if(  bl->get_bild( h, phase )  ) {
					return true;
				}
			}
		}
		return false;
	}

	image_id get_vordergrund(int phase,int season) const
	{
		season &= (seasons-1);
		bildliste2d_besch_t const* const bl = get_child<bildliste2d_besch_t>(1 + 2 * season);
		if(phase>0 && phase<phasen) {
			if (bild_besch_t const* const bild = bl->get_bild(0, phase)) {
				return bild->get_nummer();
			}
		}
		// here if this phase does not exists ...
		bild_besch_t const* const bild = bl->get_bild(0, 0);
		return bild != NULL ? bild->get_nummer() : IMG_LEER;
	}

	koord get_offset() const;

	uint8 get_layout() const;
};

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Die Hausbeschreibung enthält die Komplettbeschrebung eines Gebäudes.
 *      Das sind mehre Tiles und die Attribute für die Spielsteuerung.
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Tile 1
 *	3   Tile 2
 *	... ...
 */
class haus_besch_t : public obj_besch_std_name_t { // Daten für ein ganzes Gebäude
	friend class building_reader_t;

	public:
		/* Unbekannte Gebäude sind nochmal unterteilt */
		enum utyp
		{
			unbekannt         =  0,
			attraction_city   =  1,
			attraction_land   =  2,
			denkmal           =  3,
			fabrik            =  4,
			rathaus           =  5,
			weitere           =  6,
			firmensitz        =  7,
			// from here on only old style flages
			bahnhof           =  8,
			bushalt           =  9,
			ladebucht         = 10,
			hafen             = 11,// this is still current, as it is can be larger than 1x1
			binnenhafen       = 12,
			airport           = 13,
			monorailstop      = 14,
			bahnhof_geb       = 16,
			bushalt_geb       = 17,
			ladebucht_geb     = 18,
			hafen_geb         = 19,
			binnenhafen_geb   = 20,
			airport_geb       = 21,
			monorail_geb      = 22,
			wartehalle        = 30,
			post              = 31,
			lagerhalle        = 32,
			// in these, the extra data points to a waytype
			depot             = 33,
			generic_stop      = 34,
			generic_extension = 35,
			last_haus_typ,
			unbekannt_flag    = 128,
		};

		enum flag_t {
			FLAG_NULL        = 0,
			FLAG_KEINE_INFO  = 1, // was flag FLAG_ZEIGE_INFO
			FLAG_KEINE_GRUBE = 2, // Baugrube oder nicht?
			FLAG_NEED_GROUND = 4, // draw ground below
			FLAG_HAS_CURSOR  = 8  // there is cursor/icon for this
		};

	private:
	gebaeude_t::typ     gtyp;      // Hajo: this is the type of the building
	utyp            utype; // Hajo: if gtyp == gebaeude_t::unbekannt, then this is the real type

	uint16 animation_time;	// in ms
	uint32 extra_data;
		// extra data:
		// minimum population to build for city attractions,
		// waytype for depots
		// player level for headquarters
		// cluster number for city buildings (0 means no clustering)
	koord  groesse;
	flag_t flags;
	uint16 level;          // or passengers;
	uint8  layouts;        // 1 2, 4, 8  or 16
	uint8  enables;		// if it is a stop, what is enabled ...
	uint8  chance;         // Hajo: chance to build, special buildings, only other is weight factor

	climate_bits	allowed_climates;

	// when was this building allowed
	uint16 intro_date;
	uint16 obsolete_date;

	/**
	 * Whether this building can or must be built underground.
	 * Only relevant for stations (generic_stop).
	 * 0 = cannot be built underground
	 * 1 = can only be built underground
	 * 2 = can be built either underground or above ground.
	 */
	uint8 allow_underground;

	bool ist_utyp(utyp u) const {
		return gtyp == gebaeude_t::unbekannt && utype == u;
	}

	werkzeug_t *builder;

public:

	koord get_groesse(int layout = 0) const {
		return (layout & 1) ? koord(groesse.y, groesse.x) : groesse;
	}

	// size of the building
	int get_h(int layout = 0) const {
		return (layout & 1) ? groesse.x: groesse.y;
	}

	int get_b(int layout = 0) const {
		return (layout & 1) ? groesse.y : groesse.x;
	}

	uint8 get_all_layouts() const { return layouts; }

	uint32 get_extra() const { return extra_data; }

	/** Returns waytype used for finance stats (distinguishes between tram track and train track) */
	waytype_t get_finance_waytype() const;

	// ground is transparent
	bool ist_mit_boden() const { return (flags & FLAG_NEED_GROUND) != 0; }

	// no construction stage
	bool ist_ohne_grube() const { return (flags & FLAG_KEINE_GRUBE) != 0; }

	// do not open info for this
	bool ist_ohne_info() const { return (flags & FLAG_KEINE_INFO) != 0; }

	// see gebaeude_t and hausbauer for the different types
	gebaeude_t::typ get_typ() const { return gtyp; }
	utyp get_utyp() const { return utype; }

	bool ist_rathaus()      const { return ist_utyp(rathaus); }
	bool ist_firmensitz()   const { return ist_utyp(firmensitz); }
	bool ist_ausflugsziel() const { return ist_utyp(attraction_land) || ist_utyp(attraction_city); }
	bool ist_fabrik()       const { return ist_utyp(fabrik); }

	bool is_connected_with_town() const;

	/**
	* the level is used in many places: for price, for capacity, ...
	* @author Hj. Malthaner
	*/
	uint16 get_level() const { return level; }

	/**
	 * Mail generation level
	 * @author Hj. Malthaner
	 */
	uint16 get_post_level() const;

	// how often will this appear
	int get_chance() const { return chance; }

	const haus_tile_besch_t *get_tile(int index) const {
		assert(0<=index  &&  index < layouts * groesse.x * groesse.y);
		return get_child<haus_tile_besch_t>(index + 2);
	}

	const haus_tile_besch_t *get_tile(int layout, int x, int y) const;

	// returns true,if building can be rotated
	bool can_rotate() const {
		if(groesse.x!=groesse.y  &&  layouts==1) {
			return false;
		}
		// check for missing tiles after rotation
		for( int x=0;  x<groesse.x;  x++  ) {
			for( int y=0;  y<groesse.y;  y++  ) {
				// only true, if one is missing
				if(get_tile( 0, x, y )->has_image()  ^  get_tile( 1, get_b(1)-y-1, x )->has_image()) {
					return false;
				}
			}
		}
		return true;
	}

	int layout_anpassen(int layout) const;

	/**
	* Skin: cursor (index 0) and icon (index 1)
	* @author Hj. Malthaner
	*/
	const skin_besch_t * get_cursor() const {
		return flags & FLAG_HAS_CURSOR ? get_child<skin_besch_t>(2 + groesse.x * groesse.y * layouts) : 0;
	}

	/**
	* @return introduction month
	* @author Hj. Malthaner
	*/
	uint32 get_intro_year_month() const { return intro_date; }

	/**
	* @return time when obsolete
	* @author prissi
	*/
	uint32 get_retire_year_month() const { return obsolete_date; }

	// true if future
	bool is_future (const uint16 month_now) const {
		return month_now  &&  (intro_date > month_now);
	}

	// true if obsolete
	bool is_retired (const uint16 month_now) const {
		return month_now  &&  (obsolete_date <= month_now);
	}

	// the right house for this area?
	bool is_allowed_climate( climate cl ) const { return ((1<<cl)&allowed_climates)!=0; }

	// the right house for this area?
	bool is_allowed_climate_bits( climate_bits cl ) const { return (cl&allowed_climates)!=0; }

	// for the paltzsucher needed
	climate_bits get_allowed_climate_bits() const { return allowed_climates; }

	/**
	* @return station flags (only used for station buildings and oil riggs)
	* @author prissi
	*/
	int get_enabled() const { return enables; }

	/**
	* @return time for doing one step
	* @author prissi
	*/
	uint16 get_animation_time() const { return animation_time; }

	// default tool for building
	werkzeug_t *get_builder() const {
		return builder;
	}
	void set_builder( werkzeug_t *w )  {
		builder = w;
	}

	void calc_checksum(checksum_t *chk) const;

	bool can_be_built_underground() const { return allow_underground > 0; }
	bool can_be_built_aboveground() const { return allow_underground != 1; }

	uint32 get_clusters() const {
		// Only meaningful for res, com, ind
		if(  gtyp != gebaeude_t::wohnung  &&  gtyp != gebaeude_t::gewerbe  &&  gtyp != gebaeude_t::industrie  ) {
			return 0;
		}
		else {
			return extra_data;
		}
	}
};

ENUM_BITSET(haus_besch_t::flag_t)

#endif
