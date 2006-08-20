/* dock.cc
 *
 * Eine Sorte Wasser die zu einer Haltestelle gehört
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../../simworld.h"
#include "../../simimg.h"

#include "../../besch/grund_besch.h"
#include "../../besch/weg_besch.h"

#include "../../bauer/wegbauer.h"

#include "kanal.h"

kanal_t::kanal_t(karte_t *welt, loadsave_t *file) :  weg_t(welt)
{
    rdwr(file);
}


kanal_t::kanal_t(karte_t *welt) : weg_t (welt)
{
}


void
kanal_t::rdwr(loadsave_t *file)
{
	weg_t::rdwr(file);

	if(file->get_version() <= 87000) {
		setze_besch( wegbauer_t::weg_search(weg_t::wasser,20 ) );
	}
	else {

		if(file->is_saving()) {
			const char *s = gib_besch()->gib_name();
			file->rdwr_str(s, "\n");
		}
		else {
			char bname[128];
			file->rd_str_into(bname, "\n");

			const weg_besch_t *besch = wegbauer_t::gib_besch(bname);
			if(besch==NULL) {
				int old_max_speed=gib_max_speed();
				besch = wegbauer_t::weg_search(weg_t::wasser,old_max_speed>0 ? old_max_speed : 120 );
				dbg->warning("strasse_t::rwdr()", "Unknown channel %s replaced by a channel %s (old_max_speed %i)", bname, besch->gib_name(), old_max_speed );
			}
			setze_besch(besch);
		}
	}
}



void kanal_t::info(cbuffer_t & buf) const
{
  weg_t::info(buf);

  buf.append("\nRibi (unmasked) ");
  buf.append(gib_ribi_unmasked());

  buf.append("\nRibi (masked) ");
  buf.append(gib_ribi());
  buf.append("\n");
}



int kanal_t::calc_bild(koord3d pos) const
{
	if(welt->ist_in_kartengrenzen( pos.gib_2d() )) {
		if(gib_besch()!=NULL) {
			return weg_t::calc_bild(pos, gib_besch());
		}
		return grund_besch_t::wasser->gib_bild(hang_t::flach);
	}
	return -1;
}
