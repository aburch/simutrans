#ifndef boden_wege_strasse_h
#define boden_wege_strasse_h

#include "weg.h"

// number of different traffic directions
#define MAX_WAY_STAT_DIRECTIONS 2

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

	/**
	* 0 = calculate automatically
	* 1 = north-south traffic has priority
	* 2 = east-west traffic has priority
	* @author THLeaderH
	*/
	uint8 prior_direction_setting;

	/**
	* array for statistical values
	* store directional statistics to calculate prior_direction
	* direction: 0 = north-south, 1 = east-west
	*/
	sint16 directional_statistics[MAX_WAY_STAT_MONTHS][MAX_WAY_STATISTICS][MAX_WAY_STAT_DIRECTIONS];

	void init_statistics();

public:
	static const way_desc_t *default_strasse;

	strasse_t(loadsave_t *file);
	strasse_t();

	inline waytype_t get_waytype() const {return road_wt;}

	void set_gehweg(bool janein);

	virtual void rdwr(loadsave_t *file);

	/**
	* Overtaking mode (declared in simtypes.h)
	* halt_mode = vehicles can stop on passing lane
	* oneway_mode = condition for one-way road
	* twoway_mode = condition for two-way road
	* loading_only_mode = overtaking a loading convoy only
	* prohibited_mode = overtaking is completely forbidden
	* inverted_mode = vehicles can go only on passing lane
	* @author teamhimeH
	*/
	overtaking_mode_t get_overtaking_mode() const { return overtaking_mode; };
	void set_overtaking_mode(overtaking_mode_t o) { overtaking_mode = o; };

	void set_ribi_mask_oneway(ribi_t::ribi ribi) { ribi_mask_oneway = (uint8)ribi; }
	// used in wegbauer. param @allow is ribi in which vehicles can go. without this, ribi cannot be updated correctly at intersections.
	void update_ribi_mask_oneway(ribi_t::ribi mask, ribi_t::ribi allow);
	ribi_t::ribi get_ribi_mask_oneway() const { return (ribi_t::ribi)ribi_mask_oneway; }
	virtual ribi_t::ribi get_ribi() const;

	virtual void rotate90();

	void book(int amount, way_statistics type, ribi_t::ribi dir);
	void new_month();
	ribi_t::ribi get_prior_direction() const;

	image_id get_front_image() const {return show_masked_ribi ? skinverwaltung_t::ribi_arrow->get_image_id(get_ribi()) : weg_t::get_front_image();}

};

#endif
