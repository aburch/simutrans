/* bruecke.cc
 *
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */

#include <stdio.h>
#include <string.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../boden/wege/strasse.h"
#include "../boden/wege/schiene.h"
#include "../boden/grund.h"
#include "../simplay.h"
#include "../blockmanager.h"
#include "../simimg.h"
#include "../simmem.h"
#include "../simcosts.h"
#include "../bauer/brueckenbauer.h"
#include "../utils/cbuffer_t.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"

#include "bruecke.h"



bruecke_t::bruecke_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
  besch = NULL;
  rdwr(file);
  setze_bild(0, besch->gib_hintergrund(img));
  step_frequency = 0;
}

bruecke_t::bruecke_t(karte_t *welt, koord3d pos, const int y_off, spieler_t *sp,
		     const bruecke_besch_t *besch, bruecke_besch_t::img_t img) :
 ding_t(welt, pos)
{
	this->besch = besch;
	this->img = img;

	setze_bild(0, besch->gib_hintergrund(img));
	setze_besitzer( sp );
	setze_yoff( height_scaling(y_off) );

	if(gib_besitzer()) {
		gib_besitzer()->buche(-besch->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION);
	}
	step_frequency = 0;
}


bruecke_t::~bruecke_t()
{
	if(gib_besitzer()) {
		gib_besitzer()->buche(-besch->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION);
	}
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void bruecke_t::info(cbuffer_t & buf) const
{
  ding_t::info(buf);

  buf.append("\n");
  buf.append(translator::translate("Max. speed:"));
  buf.append(" ");
  buf.append(besch->gib_topspeed());
  buf.append("km/h\n");

  schiene_t *sch = dynamic_cast <schiene_t *> (welt->lookup(gib_pos())->gib_weg(weg_t::schiene));

  if(sch) {

    buf.append("Rail block ");
    buf.append(sch->gib_blockstrecke().get_id());
    buf.append("\n");

    buf.append("Ribi ");
    buf.append(sch->gib_ribi());
    buf.append("\n");

  }
}



void bruecke_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	const char *s = NULL;

	if(file->is_saving()) {
	s = besch->gib_name();
	}
	file->rdwr_str(s, "");
	file->rdwr_enum(img, "");

	if(file->is_loading()) {
		besch = brueckenbauer_t::gib_besch(s);
		if(besch==0) {
			if(strstr(s,"ail")) {
				besch = brueckenbauer_t::gib_besch("ClassicRail");
				dbg->warning("bruecke_t::rdwr()","Unknown bridge %s replaced by ClassicRail",s);
			}
			else if(strstr(s,"oad")) {
				besch = brueckenbauer_t::gib_besch("ClassicRoad");
				dbg->warning("bruecke_t::rdwr()","Unknown bridge %s replaced by ClassicRoad",s);
			}
			else {
				dbg->fatal("bruecke_t::rdwr()","Unknown bridge %s",s);
			}
		}
		guarded_free(const_cast<char *>(s));
	}
}
