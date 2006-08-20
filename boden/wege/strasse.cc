/* strasse.cc
 *
 * Strassen für Simutrans
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "strasse.h"
#include "../../simdebug.h"
#include "../../simworld.h"
#include "../../simimg.h"
#include "../grund.h"
#include "../../dataobj/loadsave.h"
#include "../../utils/cbuffer_t.h"
#include "../../besch/weg_besch.h"
#include "../../bauer/hausbauer.h"
#include "../../bauer/wegbauer.h"


void strasse_t::setze_gehweg(bool janein)
{
  gehweg = janein;
  setze_max_speed(janein ? 50 : gib_besch() ? gib_besch()->gib_topspeed(): 130);
}


strasse_t::strasse_t(karte_t *welt, loadsave_t *file) : weg_t (welt)
{
  rdwr(file);
}


strasse_t::strasse_t(karte_t *welt) : weg_t (welt)
{
  setze_gehweg(false);

  // Hajo: set a default
  setze_besch(wegbauer_t::gib_besch("asphalt_road"));
}


int strasse_t::calc_bild(koord3d pos) const
{
    if(welt->ist_in_kartengrenzen( pos.gib_2d() )) {
        // V.Meyer: weg_position_t removed
        grund_t *gr = welt->lookup(pos);

	if(gr && !gr->hat_gebaeude(hausbauer_t::frachthof_besch)) {
	    return weg_t::calc_bild(pos, gib_besch());
	}
    }
    return -1;
}


void strasse_t::info(cbuffer_t & buf) const
{
  weg_t::info(buf);

  buf.append("\nRibi (unmasked) ");
  buf.append(gib_ribi_unmasked());

  buf.append("\nRibi (masked) ");
  buf.append(gib_ribi());
  buf.append("\n");
}


void
strasse_t::rdwr(loadsave_t *file)
{
  weg_t::rdwr(file);
  file->rdwr_bool(gehweg, " \n");

  setze_gehweg(gehweg);


  if(file->get_version() <= 84005) {

    setze_besch(wegbauer_t::gib_besch("asphalt_road"));

  } else {

    if(file->is_saving()) {
      const char *s = gib_besch()->gib_name();
      file->rdwr_str(s, "\n");
    }

    if(file->is_loading()) {
      char bname[128];
      file->rd_str_into(bname, "\n");

      setze_besch(wegbauer_t::gib_besch(bname));

      if(gib_besch() == 0) {
	dbg->warning("strasse_t::rwdr",
		     "description %s for road at %d,%d not found!",
		     bname,
		     gib_pos().x,
		     gib_pos().y);
      }
    }
  }
}
