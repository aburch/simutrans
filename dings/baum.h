/*
 * baum.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef dings_baum_h
#define dings_baum_h

#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../besch/baum_besch.h"
#include "../simcolor.h"
#include "../dataobj/umgebung.h"

/**
 * Bäume in Simutrans.
 * @author Hj. Malthaner
 * @version $Revision: 1.12 $
 * @see ding_t
 */
class baum_t : public ding_t
{
private:
	static stringhashtable_tpl<uint32> besch_names;
	static vector_tpl<const baum_besch_t *> baum_typen;

	static const uint16 random_tree_for_climate(climate cl);

	// month of birth
	sint32 geburt:20;

	// type of tree
	uint32 baumtype:10;

	uint32 season:2;

	void saee_baum();

	/**
	 * Berechnet offsets für gepflanzte Bäume
	 */
	void calc_off();

public:
	// only the load save constructor should be called outside
	// otherwise I suggest use the plant tree function (see below)
	baum_t(karte_t *welt, loadsave_t *file);
	baum_t(karte_t *welt, koord3d pos);
	baum_t(karte_t *welt, koord3d pos, uint16 type);

	void rdwr(loadsave_t *file);

	image_id gib_bild() const;

	// hide trees eventually with transparency
	PLAYER_COLOR_VAL gib_outline_colour() const { return (umgebung_t::hide_trees  &&  umgebung_t::hide_with_transparency) ? (TRANSPARENT25_FLAG | OUTLINE_FLAG | COL_BLACK) : 0; }
	image_id gib_outline_bild() const;

	/**
	 * Berechnet Alter und Bild abhängig vom Alter
	 * @author Hj. Malthaner
	 */
	void calc_bild();

	const char *gib_name() const {return "Baum";}
	enum ding_t::typ gib_typ() const {return baum;}

	bool check_season(const long delta_t);

	void zeige_info();

	void info(cbuffer_t & buf) const;

	void entferne(spieler_t *sp);

	void * operator new(size_t s);
	void operator delete(void *p);

	const baum_besch_t* gib_besch() const { return baum_typen[baumtype]; }

	// static functions to handle trees
	static bool plant_tree_on_coordinate(karte_t *welt, koord pos, const uint8 maximum_count);

	static bool register_besch(baum_besch_t *besch);
	static bool alles_geladen();

	static int gib_anzahl_besch() { return baum_typen.get_count(); }
	static int gib_anzahl_besch(climate cl);
};

#endif
