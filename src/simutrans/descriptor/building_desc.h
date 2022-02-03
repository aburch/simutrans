/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_BUILDING_DESC_H
#define DESCRIPTOR_BUILDING_DESC_H


#include <assert.h>
#include "image_array.h"
#include "obj_base_desc.h"
#include "skin_desc.h"
#include "../dataobj/koord.h"

class building_desc_t;
class tool_t;
class karte_t;
class checksum_t;

/**
 * Data for one tile of a potentially multi-tile building.
 *
 *  Child nodes:
 *   0   Imagelist2D season 0 back
 *   1   Imagelist2D season 0 front
 *   2   Imagelist2D season 1 back
 *   3   Imagelist2D season 1 front
 * ... ...
 */
class building_tile_desc_t : public obj_desc_t {
	friend class tile_reader_t;

	const building_desc_t *building;

	uint8  seasons;
	uint8  phases;  ///< number of animation phases
	uint16 index;

public:
	void set_desc(const building_desc_t *building_desc) { building = building_desc; }

	const building_desc_t *get_desc() const { return building; }

	uint16 get_index() const { return index; }
	uint8 get_seasons() const { return seasons; }
	uint8 get_phases() const { return phases; }

	bool has_image() const {
		return get_background(0,0,0)!=IMG_EMPTY  ||  get_foreground(0,0)!=IMG_EMPTY;
	}

	image_id get_background(uint16 phase, uint16 height, uint8 season) const
	{
		image_array_t const* const imglist = get_child<image_array_t>(0 + 2 * season);
		if(phase>0 && phase<phases) {
			if (image_t const* const image = imglist->get_image(height, phase)) {
				return image->get_id();
			}
		}
		// here if this phase does not exists ...
		image_t const* const image = imglist->get_image(height, 0);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	// returns true, if the background is animated
	bool is_background_animated(uint8 season) const
	{
		image_array_t const* const imglist = get_child<image_array_t>(0 + 2 * season);
		const uint16 max_h = imglist->get_count();
		for(  uint16 phase=1;  phase<phases;  phase++  ) {
			for(  uint16 h=0;  h<max_h;  h++  ) {
				if(  imglist->get_image( h, phase )  ) {
					return true;
				}
			}
		}
		return false;
	}

	image_id get_foreground(uint16 phase, uint8 season) const
	{
		image_array_t const* const imglist = get_child<image_array_t>(1 + 2 * season);
		if(phase>0 && phase<phases) {
			if (image_t const* const image = imglist->get_image(0, phase)) {
				return image->get_id();
			}
		}
		// here if this phase does not exists ...
		image_t const* const image = imglist->get_image(0, 0);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	koord get_offset() const;

	uint8 get_layout() const;
};

/**
 * Data for one building, consists of potentially more than one tile.
 *
 * Child nodes:
 *  0   Name
 *  1   Copyright
 *  2   Tile 1
 *  3   Tile 2
 * ... ...
 */
class building_desc_t : public obj_desc_timelined_t {
	friend class building_reader_t;

public:
		/**
		 * Building types
		 */
		enum btype
		{
			unknown           =  0,
			attraction_city   =  1,
			attraction_land   =  2,
			monument          =  3,
			factory           =  4,
			townhall          =  5,
			others            =  6, ///< monorail foundation
			headquarters      =  7,
			dock              = 11, ///< dock, build on sloped coast
			// in these, the extra data points to a waytype
			depot             = 33,
			generic_stop      = 34,
			generic_extension = 35,
			flat_dock         = 36, ///< dock, but can start on a flat coast line
			// city buildings
			city_res          = 37, ///< residential city buildings
			city_com          = 38, ///< commercial  city buildings
			city_ind          = 39  ///< industrial  city buildings
		};


		enum flag_t {
			FLAG_NULL        = 0,
			FLAG_NO_INFO     = 1 << 0, ///< do not show info window
			FLAG_NO_PIT      = 1 << 1, ///< do not show construction pit
			FLAG_NEED_GROUND = 1 << 2, ///< needs ground drawn below
			FLAG_HAS_CURSOR  = 1 << 3  ///< there is cursor/icon for this
		};
private:
	/**
	 * Old named constants, only used for compatibility to load very old paks.
	 * These will be converted in building_reader_t::register_obj to a valid btype.
	 */
	enum old_building_types_t {
			bahnhof           =  8,
			bushalt           =  9,
			ladebucht         = 10,
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
			mail              = 31,
			lagerhalle        = 32
		};

	building_desc_t::btype type;
	uint16 animation_time; // in ms
	uint32 extra_data;
		// extra data:
		// minimum population to build for city attractions,
		// waytype for depots
		// player level for headquarters
		// cluster number for city buildings (0 means no clustering)
	koord  size;
	flag_t flags;
	uint16 level;               // or passengers;
	uint8  layouts;             // 1 2, 4, 8  or 16
	uint8  enables;             // if it is a stop, what is enabled ...
	uint8  distribution_weight; // chance to build, special buildings, only other is weight factor


	/**
	 * Additional fields for separate capacity/maintenance
	 * If these are not specified in the .dat file, they are set to
	 * PRICE_MAGIC then calculated from the "level" in the old way.
	 */

	sint32 price;
	sint32 maintenance;
	uint16 capacity;

#define PRICE_MAGIC (2147483647)


	climate_bits allowed_climates;

	/**
	 * Whether this building can or must be built underground.
	 * Only relevant for stations (generic_stop).
	 * 0 = cannot be built underground
	 * 1 = can only be built underground
	 * 2 = can be built either underground or above ground.
	 */
	uint8 allow_underground;

	uint16 preservation_year_month;

	bool is_type(building_desc_t::btype u) const {
		return type == u;
	}

	tool_t *builder;

public:

	koord get_size(uint8 layout = 0) const {
		return (layout & 1) ? koord(size.y, size.x) : size;
	}

	// size of the building
	sint16 get_y(uint8 layout = 0) const {
		return (layout & 1) ? size.x: size.y;
	}

	sint16 get_x(uint8 layout = 0) const {
		return (layout & 1) ? size.y : size.x;
	}

	uint8 get_all_layouts() const { return layouts; }

	uint32 get_extra() const { return extra_data; }

	/** Returns waytype used for finance stats (distinguishes between tram track and train track) */
	waytype_t get_finance_waytype() const;

	// ground is transparent
	bool needs_ground() const { return (flags & FLAG_NEED_GROUND) != 0; }

	// no construction stage
	bool no_construction_pit() const { return (flags & FLAG_NO_PIT) != 0; }

	// do not open info for this
	bool no_info_window() const { return (flags & FLAG_NO_INFO) != 0; }

	// never replace this building for renovation (to create historic city centres)
	uint16 no_renovation_month() const { return preservation_year_month; }

	building_desc_t::btype get_type() const { return type; }

	bool is_townhall()      const { return is_type(townhall); }
	bool is_headquarters()   const { return is_type(headquarters); }
	bool is_attraction() const { return is_type(attraction_land) || is_type(attraction_city); }
	bool is_factory()       const { return is_type(factory); }
	bool is_city_building() const { return is_type(city_res) || is_type(city_com) || is_type(city_ind); }
	bool is_transport_building() const { return type > headquarters  && type <= flat_dock; }
	bool is_depot() const { return is_type(depot); }

	bool is_connected_with_town() const;

	/// @returns headquarters level (or -1 if building is not headquarters)
	sint32 get_headquarters_level() const  { return (is_headquarters() ? get_extra() : -1) ; }

	/**
	* the level is used in many places: for price, for capacity, ...
	*/
	uint16 get_level() const { return level; }

	/**
	 * Mail generation level
	 */
	uint16 get_mail_level() const;

	// how often will this appear
	uint8 get_distribution_weight() const { return distribution_weight; }

	const building_tile_desc_t *get_tile(uint16 index) const {
		assert(index < layouts * size.x * size.y);
		return get_child<building_tile_desc_t>(index + 2);
	}

	const building_tile_desc_t *get_tile(uint8 layout, sint16 x, sint16 y) const;

	// returns true,if building can be rotated
	bool can_rotate() const {
		if(size.x!=size.y  &&  layouts==1) {
			return false;
		}
		// check for missing tiles after rotation
		for( sint16 x=0;  x<size.x;  x++  ) {
			for( sint16 y=0;  y<size.y;  y++  ) {
				// only true, if one is missing
				if(get_tile( 0, x, y )->has_image()  ^  get_tile( 1, get_x(1)-y-1, x )->has_image()) {
					return false;
				}
			}
		}
		return true;
	}

	uint8 adjust_layout(uint8 layout) const;

	/**
	* Skin: cursor (index 0) and icon (index 1)
	*/
	const skin_desc_t * get_cursor() const {
		return flags & FLAG_HAS_CURSOR ? get_child<skin_desc_t>(2 + size.x * size.y * layouts) : 0;
	}

	// the right house for this area?
	bool is_allowed_climate( climate cl ) const { return ((1<<cl)&allowed_climates)!=0; }

	// the right house for this area?
	bool is_allowed_climate_bits( climate_bits cl ) const { return (cl&allowed_climates)!=0; }

	// needed by place_finder
	climate_bits get_allowed_climate_bits() const { return allowed_climates; }

	/**
	* @return station flags (only used for station buildings and oil rigs)
	*/
	uint8 get_enabled() const { return enables; }

	/**
	* @return time for doing one step
	*/
	uint16 get_animation_time() const { return animation_time; }

	/**
	* @see above for maintenance/price/capacity variable information
	*/
	sint32 get_maintenance(karte_t *world) const;
	sint32 get_price(karte_t *world) const;
	uint16 get_capacity() const { return capacity; }


	// default tool for building
	tool_t *get_builder() const {
		return builder;
	}
	void set_builder( tool_t *tool )  {
		builder = tool;
	}

	void calc_checksum(checksum_t *chk) const;

	bool can_be_built_underground() const { return allow_underground > 0; }
	bool can_be_built_aboveground() const { return allow_underground != 1; }

	uint32 get_clusters() const {
		// Only meaningful for res, com, ind
		return is_city_building() ? extra_data : 0;
	}
};

ENUM_BITSET(building_desc_t::flag_t)

#endif
