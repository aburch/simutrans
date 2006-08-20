/* bruecke.cc
 *
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */

#include <string.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simmem.h"
#include "../simimg.h"

#include "../bauer/brueckenbauer.h"

#include "../boden/grund.h"

#include "../gui/thing_info.h"

#include "../utils/cbuffer_t.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"

#include "../dings/pillar.h"
#include "../dings/bruecke.h"



pillar_t::pillar_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	besch = NULL;
	rdwr(file);
	if(besch) {
		setze_bild(0,besch->gib_hintergrund((bruecke_besch_t::img_t)dir) );
	}
	step_frequency = 0;
}



pillar_t::pillar_t(karte_t *welt, koord3d pos, spieler_t *sp, const bruecke_besch_t *besch, bruecke_besch_t::img_t img, int hoehe) : ding_t(welt, pos)
{
	this->besch = besch;
	this->dir = (uint8)img;
	setze_yoff(-hoehe);
	setze_bild(0,besch->gib_hintergrund(img) );
	setze_besitzer( sp );
	step_frequency = 0;
}


pillar_t::~pillar_t()
{
}



/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
ding_info_t *pillar_t::new_info()
{
	planquadrat_t *plan=welt->access(gib_pos().gib_2d());
	for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
		grund_t *bd=plan->gib_boden_bei(i);
		if(bd->ist_bruecke()) {
			bruecke_t *br=dynamic_cast<bruecke_t *>(bd->suche_obj(ding_t::bruecke));
			if(br  &&  br->gib_besch()==besch) {
//				create_win(-1, -1, new ding_infos->get(this), w_autodelete);
//				br->zeige_info();
    				return new ding_infowin_t(welt, br);
			}
		}
	}
	return NULL;
}



void pillar_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	const char *s = NULL;

	if(file->is_saving()) {
	s = besch->gib_name();
	}
	file->rdwr_str(s, "");
	file->rdwr_byte(dir,"\n");

	if(file->is_loading()) {
		besch = brueckenbauer_t::gib_besch(s);
		if(besch==0) {
			if(strstr(s,"ail")) {
				besch = brueckenbauer_t::gib_besch("ClassicRail");
				dbg->warning("pillar_t::rdwr()","Unknown bridge %s replaced by ClassicRail",s);
			}
			else if(strstr(s,"oad")) {
				besch = brueckenbauer_t::gib_besch("ClassicRoad");
				dbg->warning("pillar_t::rdwr()","Unknown bridge %s replaced by ClassicRoad",s);
			}
			else {
				dbg->fatal("pillar_t::rdwr()","Unknown bridge %s",s);
			}
		}
		guarded_free(const_cast<char *>(s));
	}
}
