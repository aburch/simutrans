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

/*
 *  includes
 */
#include "bildliste2d_besch.h"
#include "text_besch.h"
#include "../dings/gebaeude.h"
#include "../bauer/hausbauer.h"

#include "intro_dates.h"


/*
 *  forward declarations
 */
class haus_besch_t;
class skin_besch_t;

/*
 *  class:
 *      haus_tile_besch_t()
 *
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
 *	0   Bildliste2D Hintergrund
 *	1   Bildliste2D Vordergrund
 *	... ...
 */
class haus_tile_besch_t : public obj_besch_t {
    friend class tile_writer_t;
    friend class tile_reader_t;

    const haus_besch_t	*haus;

    uint16		phasen;	    // Wie viele Animationsphasen haben wir?
    uint16		index;
public:
	void setze_besch(const haus_besch_t *haus_besch)
	{
		haus = haus_besch;
	}

	const haus_besch_t *gib_besch() const
	{
		return haus;
	}

	int gib_index() const
	{
		return index;
	}

	int gib_phasen() const
	{
		return phasen;
	}

	image_id gib_hintergrund(int phase, int hoehe) const
	{
		if(phase>0 && phase<phasen) {
			const bild_besch_t *bild = static_cast<const bildliste2d_besch_t *>(gib_kind(0))->gib_bild(hoehe, phase);
			if(bild) {
				return bild->bild_nr;
			}
		}
		// here if this phase does not exists ...
		const bild_besch_t *bild = static_cast<const bildliste2d_besch_t *>(gib_kind(0))->gib_bild(hoehe, 0);
		if(bild) {
			return bild->bild_nr;
		}
		return IMG_LEER;
	}

	image_id gib_vordergrund(int phase, int hoehe) const
	{
		if(phase>0 && phase<phasen) {
			const bild_besch_t *bild = static_cast<const bildliste2d_besch_t *>(gib_kind(1))->gib_bild(hoehe, phase);
			if(bild) {
				return bild->bild_nr;
			}
		}
		// here if this phase does not exists ...
		const bild_besch_t *bild = static_cast<const bildliste2d_besch_t *>(gib_kind(1))->gib_bild(hoehe, 0);
		if(bild) {
			return bild->bild_nr;
		}
		return IMG_LEER;
	}

	koord gib_offset() const;

	int gib_layout() const;
};

/*
 *  class:
 *      haus_besch_t()
 *
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
class haus_besch_t : public obj_besch_t {     // Daten für ein ganzes Gebäude
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

    uint16		bauzeit;        // == inhabitants at build time
    koord		groesse;
    flag_t		flags;
    int			level;          // or passengers;
    uint8		layouts;        // 1 2 oder 4
    uint8		enables;		// if it is a stop, what is enabled ...
    uint8		chance;         // Hajo: chance to build, special buildings, only other is weight factor

    // when was this building allowed
    uint16 intro_date;
    uint16 obsolete_date;

    bool ist_utyp(hausbauer_t::utyp utyp) const
    {
	return  gtyp == gebaeude_t::unbekannt &&  this->utyp == utyp;
    }
public:
    gebaeude_t::typ gib_typ() const
    {
        return gtyp;
    }
    hausbauer_t::utyp gib_utyp() const
    {
        return utyp;
    }
    const char *gib_name() const
    {
        return static_cast<const text_besch_t *>(gib_kind(0))->gib_text();
    }
	const char *gib_copyright() const
	{
		if(gib_kind(1)==0) {
			return NULL;
		}
		return static_cast<const text_besch_t *>(gib_kind(1))->gib_text();
	}
    koord gib_groesse(int layout = 0) const
    {
        return (layout & 1) ? koord(groesse.y, groesse.x) : groesse;
    }
    int gib_h(int layout = 0) const
    {
        return (layout & 1) ? groesse.x: groesse.y;
    }
    int gib_b(int layout = 0) const
    {
        return (layout & 1) ? groesse.y : groesse.x;
    }
    int gib_all_layouts() const
    {
        return layouts;
    }
    int gib_bauzeit() const
    {
        return bauzeit;
    }
    bool ist_mit_boden() const
    {
        return (flags & FLAG_NEED_GROUND) != 0;
    }
    bool ist_ohne_grube() const
    {
        return (flags & FLAG_KEINE_GRUBE) != 0;
    }
    bool ist_ohne_info() const
    {
        return (flags & FLAG_KEINE_INFO) != 0;
    }
    bool ist_rathaus() const
    {
        return ist_utyp(hausbauer_t::rathaus);
    }
    bool ist_firmensitz() const
    {
        return ist_utyp(hausbauer_t::firmensitz);
    }
    bool ist_ausflugsziel() const
    {
        return ist_utyp(hausbauer_t::sehenswuerdigkeit) || ist_utyp(hausbauer_t::special);
    }
    bool ist_fabrik() const
    {
        return ist_utyp(hausbauer_t::fabrik);
    }


    int gib_level() const
    {
        return level;
    }


    /**
     * Mail generation level
     * @author Hj. Malthaner
     */
    int gib_post_level() const;


    int gib_chance() const
    {
        return chance;
    }
    const haus_tile_besch_t *gib_tile(int index) const
    {
        return index >= 0 && index < layouts * groesse.x * groesse.y ?
	    static_cast<const haus_tile_besch_t *>(gib_kind(index + 2)) : NULL;
    }
    const haus_tile_besch_t *gib_tile(int layout, int x, int y) const;

    int layout_anpassen(int layout) const;

    /**
     * Skin: cursor (index 0) and icon (index 1)
     * @author Hj. Malthaner
     */
    const skin_besch_t * gib_cursor() const
    {
	return (const skin_besch_t *)(gib_kind(2+groesse.x*groesse.y*layouts));
    }

    /**
     * @return introduction month
     * @author Hj. Malthaner
     */
    int get_intro_year_month() const {
      return intro_date;
    }

    /**
     * @return time when obsolete
     * @author prissi
     */
    int get_retire_year_month() const {
      return obsolete_date;
    }

    /**
     * @return time when obsolete
     * @author prissi
     */
    int get_enabled() const {
      return enables;
    }
};

#endif // __HAUS_BESCH_H
