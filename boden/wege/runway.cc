/* runway.cc
 *
 * runwayn für Simutrans
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../../simdebug.h"
#include "../../simworld.h"
#include "../../blockmanager.h"
#include "../grund.h"
#include "../../dataobj/loadsave.h"
#include "../../utils/cbuffer_t.h"
#include "../../besch/weg_besch.h"
#include "../../bauer/hausbauer.h"
#include "../../bauer/wegbauer.h"

#include "runway.h"


runway_t::runway_t(karte_t *welt) : weg_t(welt)
{
  // Hajo: set a default
  setze_besch(wegbauer_t::gib_besch("runway_modern"));
}


/* create strasse with minimum topspeed
 * @author prissi
 */
runway_t::runway_t(karte_t *welt,int top_speed) : weg_t(welt)
{
  setze_besch(wegbauer_t::weg_search(weg_t::luft,top_speed));
}


runway_t::runway_t(karte_t *welt, loadsave_t *file) : weg_t(welt)
{
  rdwr(file);
}


/**
 * Destruktor. Entfernt etwaige Debug-Meldungen vom Feld
 *
 * @author Hj. Malthaner
 */
runway_t::~runway_t()
{
}


void runway_t::info(cbuffer_t & buf) const
{
  weg_t::info(buf);

  buf.append("\nRail block ");
  buf.append(bs.get_id());
  buf.append("\n");

  bs->info(buf);

  buf.append("\n\nRibi (unmasked) ");
  buf.append(gib_ribi_unmasked());

  buf.append("\nRibi (masked) ");
  buf.append(gib_ribi());
  buf.append("\n");
}


int runway_t::calc_bild(koord3d pos) const
{
    if(welt->ist_in_kartengrenzen( pos.gib_2d() )) {
        // V.Meyer: weg_position_t removed
        grund_t *gr = welt->lookup(pos);

  if(gr) {
      return weg_t::calc_bild(pos, gib_besch());
  }
    }
    return -1;
}


void
runway_t::betrete(vehikel_basis_t *v)
{
//    printf("runway_t::betrete %d,%d", xpos, ypos);
//    printf("    runway_t::betrete blockstrecke %p\n", bs);

//    assert(bs.is_bound());
//    bs->betrete( v );

}


void
runway_t::verlasse(vehikel_basis_t *v)
{
//    printf("runway_t::verlasse %d,%d", xpos, ypos);
//    printf("    runway_t::verlasse blockstrecke %p\n", bs);

//    assert(bs.is_bound());
//    bs->verlasse( v );
}


void
runway_t::setze_blockstrecke(blockhandle_t bs)
{
    this->bs = bs;
}



bool runway_t::ist_frei() const
{
	if(!bs.is_bound()) {
		dbg->warning("runway_t::ist_frei()","Runway  %p is not bound to a runway block!\n", this);
		return true;
	}
	return bs->ist_frei();
}


void
runway_t::rdwr(loadsave_t *file)
{
	weg_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = gib_besch()->gib_name();
		file->rdwr_str(s, "\n");
	}
	else {
		char bname[128];
		file->rd_str_into(bname, "\n");
		const weg_besch_t *besch = wegbauer_t::gib_besch(bname);
		setze_besch(besch);
	}
}
