#ifndef obj_bruecke_h
#define obj_bruecke_h

class karte_t;

#include "../descriptor/bridge_desc.h"
#include "../simobj.h"

/**
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */

class bruecke_t : public obj_no_info_t
{
private:
	const bridge_desc_t *desc;
	bridge_desc_t::img_t img;

	image_id image;

protected:
	void rdwr(loadsave_t *file);

public:
	bruecke_t(loadsave_t *file);
	bruecke_t(koord3d pos, player_t *player, const bridge_desc_t *desc, bridge_desc_t::img_t img);

	const char *get_name() const {return "Bruecke";}
#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return bruecke; }
#endif

#ifdef MULTI_THREAD
	void lock_mutex();
	void unlock_mutex();
#endif

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const { return desc ? desc->get_waytype() : invalid_wt; }

	const bridge_desc_t *get_desc() const { return desc; }

	inline void set_image( image_id b ) { image = b; }
	image_id get_image() const { return image; }

	image_id get_front_image() const;

	void calc_image();

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool calc_only_season_change) { if(  !calc_only_season_change  ) { calc_image(); } return true; }  // depends on snowline only

	void finish_rd();

	void cleanup(player_t *player);

	void rotate90();
	/**
	 * @return NULL when OK, otherwise an error message
	 * @author Hj. Malthaner
	 */
	virtual const char * is_deletable(const player_t *player, bool allow_public = false);
};

#endif
