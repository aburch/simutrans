/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_BRUECKE_H
#define OBJ_BRUECKE_H


class karte_t;

#include "../descriptor/bridge_desc.h"
#include "simobj.h"

/**
 * Bridges (visible)
 */
class bruecke_t : public obj_no_info_t
{
private:
	const bridge_desc_t *desc;
	bridge_desc_t::img_t img;

protected:
	void rdwr(loadsave_t *file) OVERRIDE;

public:
	bruecke_t(loadsave_t *file);
	bruecke_t(koord3d pos, player_t *player, const bridge_desc_t *desc, bridge_desc_t::img_t img);

	const char *get_name() const OVERRIDE {return "Bruecke";}
	typ get_typ() const OVERRIDE { return bruecke; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const OVERRIDE { return desc ? desc->get_waytype() : invalid_wt; }

	const bridge_desc_t *get_desc() const { return desc; }

	// we will always replace first way image
	image_id get_image() const OVERRIDE { return IMG_EMPTY; }

	image_id get_front_image() const OVERRIDE;

	void calc_image() OVERRIDE;

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool calc_only_season_change) OVERRIDE { if(  !calc_only_season_change  ) { calc_image(); } return true; }  // depends on snowline only

	void finish_rd() OVERRIDE;

	void cleanup(player_t *player) OVERRIDE;

	void rotate90() OVERRIDE;

	/**
	 * @return NULL wenn OK, ansonsten eine Fehlermeldung
	 */
	const char *is_deletable(const player_t *player) OVERRIDE;
};

#endif
