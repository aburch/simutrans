#ifndef obj_tunnel_h
#define obj_tunnel_h

#include "../simobj.h"
#include "../display/simimg.h"

class tunnel_besch_t;

class tunnel_t : public obj_no_info_t
{
private:
	const tunnel_besch_t *besch;
	image_id image;
	image_id after_bild;
	uint8 broad_type; // Is this a broad tunnel mouth?

public:
	tunnel_t(loadsave_t *file);
	tunnel_t(koord3d pos, player_t *player, const tunnel_besch_t *besch);

	const char *get_name() const {return "Tunnelmuendung";}
#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return tunnel; }
#endif

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const;

	void calc_image();

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool calc_only_season_change) { if(  !calc_only_season_change  ) { calc_image(); } return true; };  // depends on snowline only

	void set_bild( image_id b );
	void set_after_bild( image_id b );
	image_id get_bild() const {return image;}
	image_id get_front_image() const { return after_bild; }

	const tunnel_besch_t *get_besch() const { return besch; }

	void set_besch( const tunnel_besch_t *_besch );

	void rdwr(loadsave_t *file);

	void finish_rd();

	void cleanup(player_t *player);

	uint8 get_broad_type() const { return broad_type; };
	/**
	 * @return NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	virtual const char * is_deletable(const player_t *player);
};

#endif
