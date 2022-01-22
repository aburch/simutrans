/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_BAUM_H
#define OBJ_BAUM_H


#include "simobj.h"

#include "../bauer/tree_builder.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/weighted_vector_tpl.h"
#include "../descriptor/tree_desc.h"
#include "../simcolor.h"
#include "../dataobj/environment.h"

#include <string>


/**
 * Simulated trees for Simutrans.
 */
class baum_t : public obj_t
{
	friend class tree_builder_t;

public:
	static const uint16 AGE_LIMIT = 704; // in months (58 years 8 months). Must be smaller than 4095.
	static const uint16 SPAWN_PERIOD_START = 512;
	static const uint16 SPAWN_PERIOD_LENGTH = 4;

private:
	static FLAGGED_PIXVAL outline_color;

	/// month of birth
	uint16 geburt;

	/// type of tree (was 9 but for more compact saves now only 254 different tree types are allowed)
	uint8 tree_id;

	uint8 season:3;

	/// z-offset, max TILE_HEIGHT_STEP i.e. 4 bits
	uint8 zoff:4;

	// one bit free ;)

public:
	/// Only the load save constructor should be called outside.
	/// Otherwise I suggest use the plant tree function (@see tree_builder_t)
	baum_t(loadsave_t *file);
	baum_t(koord3d pos);

	/// @param age Must be smaller than 4095
	baum_t(koord3d pos, uint8 type, uint16 age, slope_t::type slope );

	baum_t(koord3d pos, const tree_desc_t *desc);

public:
	/// @copydoc obj_t::get_name
	const char *get_name() const OVERRIDE { return "Baum"; }

	/// @copydoc obj_t::get_typ
	typ get_typ() const OVERRIDE { return baum; }

	/// @copydoc obj_t::rdwr
	/// @deprecated Only used for loading old saves that did save tree offsets
	void rdwr(loadsave_t *file) OVERRIDE;

	/// @copydoc obj_t::finish_rd
	void finish_rd() OVERRIDE;

	/// @copydoc obj_t::get_image
	image_id get_image() const OVERRIDE;

	/// @copydoc obj_t::get_outline_image
	image_id get_outline_image() const OVERRIDE;

	/// @copydoc obj_t::calc_image
	/// Calculates tree image dependent on tree age
	void calc_image() OVERRIDE;

	/// @copydoc obj_t::get_outline_colour
	/// hide trees eventually with transparency
	FLAGGED_PIXVAL get_outline_colour() const OVERRIDE;

	static void recalc_outline_color();

	/// @copydoc obj_t::check_season
	bool check_season(const bool) OVERRIDE;

	/// @copydoc obj_t::rotate90
	void rotate90() OVERRIDE;

	/// re-calculate z-offset if slope of the tile has changed
	void recalc_off();

	/// @copydoc obj_t::show_info
	void show_info() OVERRIDE;

	/// @copydoc obj-t::info
	void info(cbuffer_t & buf) const OVERRIDE;

	/// @copydoc obj_t::cleanup
	void cleanup(player_t *player) OVERRIDE;

public:
	const tree_desc_t *get_desc() const { return tree_builder_t::get_desc_by_id(tree_id); }
	void set_desc(const tree_desc_t *desc) { tree_id = tree_builder_t::get_id_by_desc(desc); }
	uint16 get_desc_id() const { return tree_id; }

	/// @returns age of the tree in months, between 0 and 4095
	uint16 get_age() const;

public:
	void *operator new(size_t s);
	void operator delete(void *p);

private:
	/// calculate offsets for new trees
	void calc_off(slope_t::type slope, sint8 x=-128, sint8 y=-128);
};

#endif
