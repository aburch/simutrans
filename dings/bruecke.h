#ifndef dings_bruecke_h
#define dings_bruecke_h

#include "../besch/bruecke_besch.h"

/** bruecke.h
 *
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
	bruecke_t(karte_t *welt, koord3d pos, int y_off, spieler_t *sp, const bruecke_besch_t *besch, bruecke_besch_t::img_t img);
	~bruecke_t();

	image_id gib_after_bild() const {return besch->gib_vordergrund(img); }

	const char *gib_name() const {return "Bruecke";}
	enum ding_t::typ gib_typ() const {return bruecke;}

	const bruecke_besch_t *gib_besch() const { return besch; }

	void calc_bild();

	void laden_abschliessen();

	void zeige_info() {} // show no info
};

#endif
