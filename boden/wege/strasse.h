/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_WEGE_STRASSE_H
#define BODEN_WEGE_STRASSE_H


#include "weg.h"
//#include "../../tpl/minivec_tpl.h"

class fabrik_t;
//class gebaeude_t;

/**
 * Cars are able to drive on roads.
 *
 * @author Hj. Malthaner
 */
class strasse_t : public weg_t
{
public:
	static bool show_masked_ribi;

private:
	/**
	* @author THLeaderH
	*/
	overtaking_mode_t overtaking_mode;

	/**
	* Mask used by oneway_mode road
	* @author THLeaderH
	*/
	uint8 ribi_mask_oneway:4;

public:
	static const way_desc_t *default_strasse;

	// Being here rather than in weg_t might have caused heap corruption.
	//minivec_tpl<gebaeude_t*> connected_buildings;

	strasse_t(loadsave_t *file);
	strasse_t();

	//inline waytype_t get_waytype() const {return road_wt;}

	void set_gehweg(bool janein);

	void rdwr(loadsave_t *file) OVERRIDE;

	/**
	* Overtaking mode (declared in simtypes.h)
	* halt_mode = vehicles can stop on passing lane
	* oneway_mode = condition for one-way road
	* twoway_mode = condition for two-way road
	* prohibited_mode = overtaking is completely forbidden
	* @author teamhimeH
	*/
	overtaking_mode_t get_overtaking_mode() const { return overtaking_mode; };
	void set_overtaking_mode(overtaking_mode_t o, player_t* calling_player);

	void set_ribi_mask_oneway(ribi_t::ribi ribi) { ribi_mask_oneway = (uint8)ribi; }
	// used in wegbauer. param @allow is ribi in which vehicles can go. without this, ribi cannot be updated correctly at intersections.
	void update_ribi_mask_oneway(ribi_t::ribi mask, ribi_t::ribi allow, player_t* calling_player);
	ribi_t::ribi get_ribi_mask_oneway() const { return (ribi_t::ribi)ribi_mask_oneway; }
	ribi_t::ribi get_ribi() const OVERRIDE;

	void rotate90() OVERRIDE;

	image_id get_front_image() const OVERRIDE
	{
		if (show_masked_ribi && overtaking_mode <= oneway_mode) {
			return skinverwaltung_t::ribi_arrow->get_image_id(get_ribi());
		}
		else {
			return weg_t::get_front_image();
		}
	}

	PLAYER_COLOR_VAL get_outline_colour() const OVERRIDE
	{
		uint8 restriction_colour;
		switch (overtaking_mode)
		{
			case halt_mode:
			case prohibited_mode:
			case oneway_mode:
				restriction_colour = overtaking_mode_to_color(overtaking_mode);
				break;
			case invalid_mode:
			case twoway_mode:
			default:
				return 0;
		}
		return (show_masked_ribi && restriction_colour) ? TRANSPARENT75_FLAG | OUTLINE_FLAG | restriction_colour : 0;
	}

	static uint8 overtaking_mode_to_color(overtaking_mode_t o) {
		switch (o)
		{
			// Do not set the lightest color to make a difference between the tile color and the text color
			case halt_mode:
				return COL_LIGHT_PURPLE - 1;
			case prohibited_mode:
				return COL_ORANGE + 1;
			case oneway_mode:
				return 22;
			case invalid_mode:
			case twoway_mode:
			default:
				return COL_WHITE-1;
		}
		return 0;
	}
};

#endif
