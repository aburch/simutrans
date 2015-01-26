#ifndef obj_pillar_h
#define obj_pillar_h

#include "../simobj.h"

/**
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */
class bruecke_besch_t;
class loadsave_t;
class karte_t;

class pillar_t : public obj_t
{
	const bruecke_besch_t *besch;
	uint8 dir;
	bool asymmetric;
	image_id image;

protected:
	void rdwr(loadsave_t *file);

public:
	pillar_t(loadsave_t *file);
	pillar_t(koord3d pos, player_t *player, const bruecke_besch_t *besch, bruecke_besch_t::img_t img, int hoehe);

	const char* get_name() const { return "Pillar"; }
#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return obj_t::pillar; }
#endif

	const bruecke_besch_t* get_besch() const { return besch; }

	image_id get_bild() const { return asymmetric ? IMG_LEER : image; }

	// asymmetric pillars are placed at the southern/eastern boundary of the tile
	// thus the images have to be displayed after vehicles
	image_id get_front_image() const { return asymmetric ? image : IMG_LEER;}

	// needs to check for hiding asymmetric pillars
	void calc_image();

	/**
	 * @return Einen Beschreibungsstring fuer das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 */
	void show_info();

	void rotate90();
};

#endif
