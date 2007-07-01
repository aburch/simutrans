/*
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

const weg_besch_t *kanal_t::default_kanal=NULL;



kanal_t::kanal_t(karte_t *welt, loadsave_t *file) :  weg_t(welt)
{
    rdwr(file);
}



kanal_t::kanal_t(karte_t *welt) : weg_t (welt)
{
	setze_besch(default_kanal);
}



void
kanal_t::rdwr(loadsave_t *file)
{
	weg_t::rdwr(file);

	if(file->get_version() <= 87000) {
		setze_besch(default_kanal);
		return;
	}

	if(file->is_saving()) {
		const char *s = gib_besch()->gib_name();
		file->rdwr_str(s, "\n");
	}
	else {
		char bname[128];
		file->rd_str_into(bname, "\n");

		const weg_besch_t *besch = wegbauer_t::gib_besch(bname);
		int old_max_speed = gib_max_speed();
		if(besch==NULL) {
			besch = wegbauer_t::gib_besch(translator::compatibility_name(bname));
			dbg->warning("kanal_t::rdwr()", "Unknown channel %s replaced by %s (old_max_speed %i)", bname, besch->gib_name(), old_max_speed );
		}
		if(besch==NULL) {
			besch = default_kanal;
			dbg->warning("kanal_t::rdwr()", "Unknown channel %s replaced by the default %s (old_max_speed %i)", bname, besch->gib_name(), old_max_speed );
		}
		setze_besch(besch);
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
