/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_TUNNEL_H
#define OBJ_TUNNEL_H


#include "simobj.h"
#include "../display/simimg.h"

class tunnel_desc_t;

class tunnel_t : public obj_no_info_t
{
private:
	const tunnel_desc_t *desc;
	image_id image;
	image_id foreground_image;
	uint8 broad_type; // Is this a broad tunnel mouth?

public:
	tunnel_t(loadsave_t *file);
	tunnel_t(koord3d pos, player_t *player, const tunnel_desc_t *desc);

	const char *get_name() const OVERRIDE {return "Tunnelmuendung";}
	typ get_typ() const OVERRIDE { return tunnel; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const OVERRIDE;

	void calc_image() OVERRIDE;

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool calc_only_season_change) OVERRIDE { if(  !calc_only_season_change  ) { calc_image(); } return true; }  // depends on snowline only

	void set_image( image_id b );
	void set_foreground_image( image_id b );
	image_id get_image() const OVERRIDE {return image;}
	image_id get_front_image() const OVERRIDE { return foreground_image; }

	const tunnel_desc_t *get_desc() const { return desc; }

	void set_desc( const tunnel_desc_t *_desc ) { desc = _desc; }

	void rdwr(loadsave_t *file) OVERRIDE;

	void finish_rd() OVERRIDE;

	void cleanup(player_t *player) OVERRIDE;


	uint8 get_broad_type() const { return broad_type; }

	/**
	 * @return NULL when OK, otherwise an error message
	 */
	const char *is_deletable(const player_t *player) OVERRIDE;
};

#endif
