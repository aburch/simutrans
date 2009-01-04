#ifndef dings_zeiger_h
#define dings_zeiger_h

#include "../simdings.h"
#include "../simtypes.h"
#include "../simimg.h"

/**
 * Objekte dieser Klasse dienen sowohl als Landschaftszeiger im UI
 * als auch zur Markierung/Reservierung von Planquadraten.
 *
 * @author Hj. Malthaner
 */
class zeiger_t : public ding_t
{
private:
	koord area;
	uint8 center;
	bool changed;
	ribi_t::ribi richtung;
	image_id bild;

public:
	void set_richtung(ribi_t::ribi r);

	ribi_t::ribi get_richtung() const {return richtung;}

	zeiger_t(karte_t *welt, loadsave_t *file);
	zeiger_t(karte_t *welt, koord3d pos, spieler_t *sp);

	/**
	 * we want to be able to highlight the current tile .. thus only use this routine
	 * @author Hj. Malthaner
	 */
	void change_pos(koord3d k);

	const char *get_name() const {return "Zeiger";}
	enum ding_t::typ get_typ() const {return zeiger;}

	void set_area( koord area, uint8 center );
	bool area_changed();

	void set_bild( image_id b );
	image_id get_bild() const {return bild;}
};

#endif
