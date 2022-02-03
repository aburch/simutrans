/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_GEBAEUDE_H
#define OBJ_GEBAEUDE_H


#include "../obj/sync_steppable.h"
#include "simobj.h"
#include "../simcolor.h"

class building_tile_desc_t;
class fabrik_t;
class stadt_t;
class grund_t;

/**
 * Asynchronous or synchronous animations for buildings.
 */
class gebaeude_t : public obj_t, sync_steppable
{
private:
	const building_tile_desc_t *tile;

	/**
	 * Time control for animation progress.
	 */
	uint16 anim_time;

	/**
	 * Is this a sync animated object?
	 */
	uint8 sync:1;

	/**
	 * Boolean flag if a construction site or buildings image
	 * shall be displayed.
	 */
	uint8 zeige_baugrube:1;

	/**
	 * if true, this ptr union contains a factory pointer
	 */
	uint8 is_factory:1;

	uint8 season:3;
	uint8 background_animated:1;

	uint8 remove_ground:1;  // true if ground image can go

	uint8 anim_frame;

	/**
	 * Zeitpunkt an dem das Gebaeude Gebaut wurde
	 */
	uint32 insta_zeit;

	/**
	* either point to a factory or a city
	*/
	union {
		fabrik_t  *fab;
		stadt_t *stadt;
	} ptr;

	/**
	 * Initializes all variables with safe, usable values
	 */
	void init();

protected:
	gebaeude_t();

public:
	gebaeude_t(loadsave_t *file);
	gebaeude_t(koord3d pos,player_t *player, const building_tile_desc_t *t);
	virtual ~gebaeude_t();

	void rotate90() OVERRIDE;

	void add_alter(uint32 a);

	void set_fab(fabrik_t *fd);
	void set_stadt(stadt_t *s);

	fabrik_t* get_fabrik() const { return is_factory ? ptr.fab : NULL; }
	stadt_t* get_stadt() const { return is_factory ? NULL : ptr.stadt; }

	obj_t::typ get_typ() const OVERRIDE { return obj_t::gebaeude; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const OVERRIDE;

	image_id get_image() const OVERRIDE;
	image_id get_image(int nr) const OVERRIDE;
	image_id get_front_image() const OVERRIDE;
	void mark_images_dirty() const;

	image_id get_outline_image() const OVERRIDE;
	FLAGGED_PIXVAL get_outline_colour() const OVERRIDE;

	// caches image at height 0
	void calc_image() OVERRIDE;

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool) OVERRIDE { calc_image(); return true; }

	/**
	 * @return eigener Name oder Name der Fabrik falls Teil einer Fabrik
	 */
	const char *get_name() const OVERRIDE;

	bool is_townhall() const;

	bool is_headquarter() const;

	bool is_monument() const;

	bool is_city_building() const;

	/// fills vector with a list of all tiles with this building
	/// @return number of actual tiles
	uint32 get_tile_list( vector_tpl<grund_t *>& list ) const;

	/// @copydoc obj_t::info
	void info(cbuffer_t & buf) const OVERRIDE;

	void rdwr(loadsave_t *file) OVERRIDE;

	/**
	 * Play animations of animated buildings.
	 * Count-down to replace construction site image by regular image.
	 */
	sync_result sync_step(uint32 delta_t) OVERRIDE;

	/**
	 * @return Den level (die Ausbaustufe) des Gebaudes
	 */
	int get_passagier_level() const;

	int get_mail_level() const;

	void set_tile( const building_tile_desc_t *t, bool start_with_construction );

	const building_tile_desc_t *get_tile() const { return tile; }

	bool is_within_players_network(const player_t* player) const;

	void show_info() OVERRIDE;

	void cleanup(player_t *player) OVERRIDE;

	/// @copydoc obj_t::finish_rd
	void finish_rd() OVERRIDE;

	// currently animated
	bool is_sync() const { return sync; }

	/**
	 * @returns pointer to first tile of a multi-tile building.
	 */
	gebaeude_t* get_first_tile();

	/**
	 * @returns true if both building tiles are part of one (multi-tile) building.
	 */
	bool is_same_building(const gebaeude_t* other) const;
};


template<> inline gebaeude_t* obj_cast<gebaeude_t>(obj_t* const d)
{
	return dynamic_cast<gebaeude_t*>(d);
}

#endif
