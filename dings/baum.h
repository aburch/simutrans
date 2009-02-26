/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
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
 */
class baum_t : public ding_t
{
private:
	// month of birth
	sint32 geburt:20;

	// type of tree
	uint32 baumtype:9;

	uint32 season:3;

	// static for administration
	static stringhashtable_tpl<uint32> besch_names;
	static vector_tpl<const baum_besch_t *> baum_typen;

	// static for the forest rule set
	static uint8 forest_base_size;
	static uint8 forest_map_size_divisor;
	static uint8 forest_count_divisor;
	static uint8 forest_boundary_blur;
	static uint8 forest_boundary_thickness;
	static uint16 forest_inverse_spare_tree_density;
	static uint8 max_no_of_trees_on_square;
	static uint16 tree_climates;
	static uint16 no_tree_climates;

	void saee_baum();

	/**
	 * Berechnet offsets für gepflanzte Bäume
	 */
	void calc_off();

	static uint16 random_tree_for_climate_intern(climate cl);

public:
	// only the load save constructor should be called outside
	// otherwise I suggest use the plant tree function (see below)
	baum_t(karte_t *welt, loadsave_t *file);
	baum_t(karte_t *welt, koord3d pos);
	baum_t(karte_t *welt, koord3d pos, uint16 type);
	baum_t(karte_t *welt, koord3d pos, const baum_besch_t *besch);

	void rdwr(loadsave_t *file);

	image_id get_bild() const;

	// hide trees eventually with transparency
	PLAYER_COLOR_VAL get_outline_colour() const { return (umgebung_t::hide_trees  &&  umgebung_t::hide_with_transparency) ? (TRANSPARENT25_FLAG | OUTLINE_FLAG | COL_BLACK) : 0; }
	image_id get_outline_bild() const;

	/**
	 * Berechnet Alter und Bild abhängig vom Alter
	 * @author Hj. Malthaner
	 */
	void calc_bild();

	const char *get_name() const {return "Baum";}
	enum ding_t::typ get_typ() const {return baum;}

	bool check_season(const long delta_t);

	void zeige_info();

	void info(cbuffer_t & buf) const;

	void entferne(spieler_t *sp);

	void * operator new(size_t s);
	void operator delete(void *p);

	const baum_besch_t* get_besch() const { return baum_typen[baumtype]; }

	// static functions to handle trees

	// reads configuration data
	static bool forestrules_init(cstring_t objpathname);

	// distributes trees on a map
	static void distribute_trees(karte_t *welt, int dichte);

	static bool plant_tree_on_coordinate(karte_t *welt, koord pos, const uint8 maximum_count);
	static bool plant_tree_on_coordinate(karte_t *welt, koord pos, const baum_besch_t *besch, const bool check_climate, const bool random_age );

	static bool register_besch(baum_besch_t *besch);
	static bool alles_geladen();

	static uint32 create_forest(karte_t *welt, koord center, koord size );
	static void fill_trees(karte_t *welt, int dichte);


	// return list to beschs
	static const vector_tpl<const baum_besch_t *> *get_all_besch() { return &baum_typen; }

	static const baum_besch_t *random_tree_for_climate(climate cl) { uint16 b = random_tree_for_climate_intern(cl);  return b!=0xFFFF ? baum_typen[b] : NULL; }

	static const baum_besch_t *find_tree( const char *tree_name ) { return baum_typen.empty() ? NULL : baum_typen[besch_names.get(tree_name)]; }

	static int get_anzahl_besch() { return baum_typen.get_count(); }
	static int get_anzahl_besch(climate cl);

};

#endif
