/*
 *
 *  haus_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#ifndef __HAUS_BESCH_H
#define __HAUS_BESCH_H

#include "bildliste2d_besch.h"
#include "obj_besch_std_name.h"
#include "../dings/gebaeude.h"
#include "../bauer/hausbauer.h"
#include "intro_dates.h"


class haus_besch_t;
class skin_besch_t;

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
    friend class tile_writer_t;
    friend class tile_reader_t;

    const haus_besch_t	*haus;

    uint8		seasons;
    uint8		phasen;	    // Wie viele Animationsphasen haben wir?
    uint16		index;

public:
	void setze_besch(const haus_besch_t *haus_besch) { haus = haus_besch; }

	const haus_besch_t *gib_besch() const { return haus; }

	int gib_index() const { return index; }
	int get_seasons() const { return seasons; }
	int gib_phasen() const { return phasen; }

	image_id gib_hintergrund(int phase, int hoehe,int season) const
	{
		if(season>=seasons) {
			season = seasons-1;
		}
		if(phase>0 && phase<phasen) {
			const bild_besch_t *bild = static_cast<const bildliste2d_besch_t *>(gib_kind(0+2*season))->gib_bild(hoehe, phase);
			if(bild) {
				return bild->bild_nr;
			}
		}
		// here if this phase does not exists ...
		const bild_besch_t *bild = static_cast<const bildliste2d_besch_t *>(gib_kind(0+2*season))->gib_bild(hoehe, 0);
		if(bild) {
			return bild->bild_nr;
		}
		return IMG_LEER;
	}

	image_id gib_vordergrund(int phase,int season) const
	{
		if(season>=seasons) {
			season = seasons-1;
		}
		if(phase>0 && phase<phasen) {
			const bild_besch_t *bild = static_cast<const bildliste2d_besch_t *>(gib_kind(1+2*season))->gib_bild(0, phase);
			if(bild) {
				return bild->bild_nr;
			}
		}
		// here if this phase does not exists ...
		const bild_besch_t *bild = static_cast<const bildliste2d_besch_t *>(gib_kind(1+2*season))->gib_bild(0, 0);
		if(bild) {
			return bild->bild_nr;
		}
		return IMG_LEER;
	}

	koord gib_offset() const;

	int gib_layout() const;
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
	friend class building_writer_t;
	friend class building_reader_t;

	enum flag_t {
		FLAG_NULL = 0,
		FLAG_KEINE_INFO = 1,       // was flag FLAG_ZEIGE_INFO
		FLAG_KEINE_GRUBE = 2 ,      // Baugrube oder nicht?
		FLAG_NEED_GROUND = 4	// draw ground below
	};

	gebaeude_t::typ     gtyp;      // Hajo: this is the type of the building
	hausbauer_t::utyp   utyp;      // Hajo: if gtyp ==  gebaeude_t::unbekannt, then this is the real type

	uint16		animation_time;	// in ms
	uint16		bauzeit;        // == inhabitants at build time
	koord		groesse;
	flag_t		flags;
	int			level;          // or passengers;
	uint8		layouts;        // 1 2 oder 4
	uint8		enables;		// if it is a stop, what is enabled ...
	uint8		chance;         // Hajo: chance to build, special buildings, only other is weight factor

	climate_bits	allowed_climates;

	// when was this building allowed
	uint16 intro_date;
	uint16 obsolete_date;

	bool ist_utyp(hausbauer_t::utyp utyp) const
	{
		return  gtyp == gebaeude_t::unbekannt &&  this->utyp == utyp;
	}

public:

	koord gib_groesse(int layout = 0) const
	{
		return (layout & 1) ? koord(groesse.y, groesse.x) : groesse;
	}

	// size of the building
	int gib_h(int layout = 0) const
	{
		return (layout & 1) ? groesse.x: groesse.y;
	}
	int gib_b(int layout = 0) const
	{
		return (layout & 1) ? groesse.y : groesse.x;
	}

    int gib_all_layouts() const { return layouts; }

    int gib_bauzeit() const { return bauzeit; }

	// ground is transparent
	bool ist_mit_boden() const { return (flags & FLAG_NEED_GROUND) != 0; }

	// no construction stage
	bool ist_ohne_grube() const { return (flags & FLAG_KEINE_GRUBE) != 0; }

	// do not open info for this
	bool ist_ohne_info() const { return (flags & FLAG_KEINE_INFO) != 0; }

	// see gebaeude_t and hausbauer for the different types
	gebaeude_t::typ gib_typ() const { return gtyp; }
	hausbauer_t::utyp gib_utyp() const { return utyp; }

	bool ist_rathaus() const { return ist_utyp(hausbauer_t::rathaus); }
	bool ist_firmensitz() const { return ist_utyp(hausbauer_t::firmensitz); }
	bool ist_ausflugsziel() const { return ist_utyp(hausbauer_t::sehenswuerdigkeit) || ist_utyp(hausbauer_t::special); }
	bool ist_fabrik() const { return ist_utyp(hausbauer_t::fabrik); }

	/**
	* the level is used in many places: for price, for capacity, ...
	* @author Hj. Malthaner
	*/
	int gib_level() const { return level; }

	/**
	 * Mail generation level
	 * @author Hj. Malthaner
	 */
	int gib_post_level() const;

	// how often will this appear
	int gib_chance() const { return chance; }

	const haus_tile_besch_t *gib_tile(int index) const
	{
		if(index >= 0 && index < layouts * groesse.x * groesse.y) {
			return static_cast<const haus_tile_besch_t *>(gib_kind(index + 2));
		}
		// we should never get here; I am not sure, if an assert would be better ...
		dbg->error("haus_tile_besch_t::gib_tile()","index=%d, max %d",index,layouts * groesse.x * groesse.y);
		return NULL;
	}

	const haus_tile_besch_t *gib_tile(int layout, int x, int y) const;

	int layout_anpassen(int layout) const;

	/**
	* Skin: cursor (index 0) and icon (index 1)
	* @author Hj. Malthaner
	*/
	const skin_besch_t * gib_cursor() const { return (const skin_besch_t *)(gib_kind(2+groesse.x*groesse.y*layouts)); }

	/**
	* @return introduction month
	* @author Hj. Malthaner
	*/
	int get_intro_year_month() const { return intro_date; }

	/**
	* @return time when obsolete
	* @author prissi
	*/
	int get_retire_year_month() const { return obsolete_date; }

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
};

#endif
