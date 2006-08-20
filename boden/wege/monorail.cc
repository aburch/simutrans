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
#include "../../blockmanager.h"
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

	buf.append(translator::translate("\nRail block"));
	buf.append(gib_blockstrecke().get_id());
	buf.append("\n");

	gib_blockstrecke()->info(buf);
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
	int blocknr;

	weg_t::rdwr(file);

	if(file->is_saving()) {
		blocknr = blockmanager::gib_manager()->gib_block_nr(bs);

		if(blocknr < 0) {
			dbg->error("schiene_t::speichern()",
				"A railroad track was saved which has an invalid rail block id!\n"
				"This saved game may be not loadable again!!!");
		}
	}
	file->rdwr_long(blocknr, "\n");

	if(file->is_loading()) {
		blockhandle_t bs=blockmanager::gib_manager()->gib_strecke_bei(blocknr);
		if(!bs.is_bound()) {
			// will try to repair them after loading everything ...
			dbg->warning("monorail_t::monorail_t()","invalid rail block id (id=%d) in saved game", blocknr);
			bs = blockstrecke_t::create(welt);
			blockmanager::gib_manager()->repair_block(bs);
		}
		setze_blockstrecke(bs);
	}

	uint8 dummy = is_electrified;
	file->rdwr_byte(dummy, "\n");
	is_electrified = dummy;

	if(file->is_saving()) {
		const char *s = gib_besch()->gib_name();
		file->rdwr_str(s, "\n");
	}
	else {
		char bname[128];
		file->rd_str_into(bname, "\n");

		int old_max_speed=gib_max_speed();
		const weg_besch_t *besch = wegbauer_t::gib_besch(bname);
		if(besch==NULL) {
			besch = wegbauer_t::weg_search(gib_typ(),old_max_speed>0 ? old_max_speed : 120 );
			dbg->warning("monorail_t::rwdr()", "Unknown rail %s replaced by a rail %s (old_max_speed %i)", bname, besch->gib_name(), old_max_speed );
		}
		setze_besch(besch);
		if(old_max_speed>0) {
			setze_max_speed(old_max_speed);
		}
		//DBG_MESSAGE("schiene_t::rdwr","track %s at (%i,%i) max_speed %i",bname,gib_pos().x,gib_pos().y,old_max_speed);
	}
}
