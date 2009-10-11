#ifndef dings_bruecke_h
#define dings_bruecke_h

#include "../besch/bruecke_besch.h"
#include "../simworld.h"

/**
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */

class bruecke_t : public ding_t
{
private:
	const bruecke_besch_t *besch;
	bruecke_besch_t::img_t img;

protected:
	void rdwr(loadsave_t *file);

public:
	bruecke_t(karte_t *welt, loadsave_t *file);
	bruecke_t(karte_t *welt, koord3d pos, spieler_t *sp, const bruecke_besch_t *besch, bruecke_besch_t::img_t img);

	image_id get_after_bild() const {return besch->get_vordergrund(img, get_pos().z+Z_TILE_STEP*(img>=bruecke_besch_t::N_Start  &&  img<=bruecke_besch_t::W_Start) >= welt->get_snowline()); }

	const char *get_name() const {return "Bruecke";}
	enum ding_t::typ get_typ() const {return bruecke;}

	const bruecke_besch_t *get_besch() const { return besch; }

	// we will always replace first way image
	image_id get_bild() const { return IMG_LEER; }

	void calc_bild();

	bool check_season( const long ) { calc_bild(); return true; }

	void laden_abschliessen();

	void entferne(spieler_t *sp);

	void zeige_info() {} // show no info

	void rotate90();
	/**
	 * @returns NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	virtual const char *ist_entfernbar(const spieler_t *sp);
};

#endif
