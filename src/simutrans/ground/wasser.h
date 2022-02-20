/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GROUND_WASSER_H
#define GROUND_WASSER_H


#include "grund.h"


/**
 * The water ground models rivers and lakes in Simutrans.
 */
class wasser_t : public grund_t
{
protected:
	/// cache ribis to tiles connected by water
	ribi_t::ribi ribi;
	/// helper variable, ribis to canal tiles, where ships cannot turn left or right
	ribi_t::ribi canal_ribi;
	/// helper variable ribis for displaying (tiles with harbour are not considered connected to water for displaying)
	ribi_t::ribi display_ribi;

	/// @copydoc grund_t::calc_image_internal
	/// This method also recalculates ribi and cache_ribi!
	void calc_image_internal(const bool calc_only_snowline_change) OVERRIDE;

public:
	wasser_t(loadsave_t *file, koord pos);
	wasser_t(koord3d pos);

public:
	/// @copydoc grund_t::is_water
	inline bool is_water() const OVERRIDE { return true; }

	/// returns correct directions for water and none for the rest
	ribi_t::ribi get_weg_ribi(waytype_t typ) const OVERRIDE { return (typ==water_wt) ? ribi : (ribi_t::ribi)ribi_t::none; }
	ribi_t::ribi get_weg_ribi_unmasked(waytype_t typ) const OVERRIDE  { return (typ==water_wt) ? ribi : (ribi_t::ribi)ribi_t::none; }

	/// @copydoc grund_t::get_name
	const char* get_name() const OVERRIDE;

	/// @copydoc grund_t::get_typ
	grund_t::typ get_typ() const OVERRIDE { return wasser; }

	/// recalculates the water-specific ribis
	void recalc_ribis();

	/// recalculates the water-specific ribis for this and all neighbouring water tiles
	void recalc_water_neighbours();

	/// @copydoc grund_t::rotate90
	void rotate90() OVERRIDE;

	ribi_t::ribi get_canal_ribi() const { return canal_ribi; }
	ribi_t::ribi get_display_ribi() const { return display_ribi; }

	// static stuff from here on for water animation
	static int stage;
	static bool change_stage;

	static void prepare_for_refresh();
};

#endif
