#ifndef obj_pillar_h
#define obj_pillar_h

#include "../simobj.h"
#include "../besch/bruecke_besch.h"

/**
 * Bridge piece (visible)
 *
 * Hj. Malthaner
 */
class loadsave_t;
class karte_t;

class pillar_t : public obj_t
{
	const bruecke_besch_t *besch;
	uint8 dir;
	bool asymmetric;
	image_id bild;

protected:
	void rdwr(loadsave_t *file);

public:
	pillar_t(loadsave_t *file);
	pillar_t(koord3d pos, player_t *player, const bruecke_besch_t *besch, bruecke_besch_t::img_t img, int hoehe);

	const char* get_name() const { return "Pillar"; }
	typ get_typ() const { return obj_t::pillar; }

	const bruecke_besch_t* get_besch() const { return besch; }

	image_id get_image() const { return asymmetric ? IMG_LEER : bild; }

	// asymmetric pillars are placed at the southern/eastern boundary of the tile
	// thus the images have to be displayed after vehicles
	image_id get_front_image() const { return asymmetric ? bild : IMG_LEER;}

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
