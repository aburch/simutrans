/*
 * Support for bridges
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

#include "../dataobj/loadsave.h"
#include "../dings/pillar.h"
#include "../dings/bruecke.h"



pillar_t::pillar_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	besch = NULL;
	hide = false;
	rdwr(file);
}



pillar_t::pillar_t(karte_t *welt, koord3d pos, spieler_t *sp, const bruecke_besch_t *besch, bruecke_besch_t::img_t img, int hoehe) : ding_t(welt, pos)
{
	this->besch = besch;
	this->dir = (uint8)img;
	setze_yoff(-hoehe);
	setze_besitzer( sp );
	hide = false;
	if(hoehe==0  &&  besch->has_pillar_asymmetric()) {
		hang_t::typ h = welt->lookup(pos)->gib_grund_hang();
		if(h==hang_t::nord  ||  h==hang_t::west) {
			hide = true;
		}
	}
}



// check for asymmetric pillars
void pillar_t::laden_abschliessen()
{
	hide = false;
	if(gib_yoff()==0  &&  besch->has_pillar_asymmetric()) {
		hang_t::typ h = welt->lookup(gib_pos())->gib_grund_hang();
		if(h==hang_t::nord  ||  h==hang_t::west) {
			hide = true;
		}
	}
}



/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void pillar_t::zeige_info()
{
	planquadrat_t *plan=welt->access(gib_pos().gib_2d());
	for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
		grund_t *bd=plan->gib_boden_bei(i);
		if(bd->ist_bruecke()) {
			bruecke_t* br = bd->find<bruecke_t>();
			if(br  &&  br->gib_besch()==besch) {
   				br->zeige_info();
			}
		}
	}
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
		}
		guarded_free(const_cast<char *>(s));
	}
}



void pillar_t::rotate90()
{
	ding_t::rotate90();
	// may need to hide/show asymmetric pillars
	if(gib_yoff()==0  &&  besch->has_pillar_asymmetric()) {
		// we must hide, if prevous NS and visible
		// we must show, if preivous NS and hidden
		if(hide) {
			hide = (dir==bruecke_besch_t::OW_Pillar);
		}
		else {
			hide = (dir==bruecke_besch_t::NS_Pillar);
		}
	}
	// the rotated image parameter is just one in front/back
	dir = (dir == bruecke_besch_t::NS_Pillar) ? bruecke_besch_t::OW_Pillar : bruecke_besch_t::NS_Pillar;
}
