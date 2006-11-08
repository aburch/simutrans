/* schiene.cc
 *
 * Schienen für Simutrans
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../../simdebug.h"
#include "../../simworld.h"
#include "../grund.h"
#include "../../dataobj/loadsave.h"
#include "../../dataobj/translator.h"
#include "../../utils/cbuffer_t.h"
#include "../../besch/weg_besch.h"
#include "../../bauer/hausbauer.h"
#include "../../bauer/wegbauer.h"

#include "monorail.h"

const weg_besch_t *monorail_t::default_monorail=NULL;

void monorail_t::info(cbuffer_t & buf) const
{
	weg_t::info(buf);
}



/**
 * File loading constructor.
 * @author prissi
 */
monorail_t::monorail_t(karte_t *welt, loadsave_t *file) : schiene_t(welt)
{
	rdwr(file);
}



void
monorail_t::rdwr(loadsave_t *file)
{
	schiene_t::rdwr(file);

	if(gib_besch()->gib_wtyp()!=gib_typ()) {
		int old_max_speed=gib_max_speed();
		const weg_besch_t *besch = wegbauer_t::weg_search(gib_waytype(),old_max_speed>0 ? old_max_speed : 120 );
		dbg->warning("monorail_t::rwdr()", "Unknown way replaced by monorail %s (old_max_speed %i)", besch->gib_name(), old_max_speed );
		setze_besch(besch);
		if(old_max_speed>0) {
			setze_max_speed(old_max_speed);
		}
	}
}
