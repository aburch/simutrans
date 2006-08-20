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

#include "schiene.h"

const weg_besch_t *schiene_t::default_schiene=NULL;


void schiene_t::setze_elektrisch(bool yesno)
{
  is_electrified = yesno;
}


uint8 schiene_t::ist_elektrisch() const
{
  return is_electrified;
}


schiene_t::schiene_t(karte_t *welt) : weg_t(welt)
{
	is_electrified = false;
	setze_besch(schiene_t::default_schiene);
}


/* create schiene with minimum topspeed
 * @author prissi
 */
schiene_t::schiene_t(karte_t *welt,int top_speed) : weg_t(welt)
{
	is_electrified = false;
	setze_besch(wegbauer_t::weg_search(weg_t::schiene,top_speed));
}


schiene_t::schiene_t(karte_t *welt, loadsave_t *file) : weg_t(welt)
{
	is_electrified = false;
	rdwr(file);
}


/**
 * Destruktor. Entfernt etwaige Debug-Meldungen vom Feld
 *
 * @author Hj. Malthaner
 */
schiene_t::~schiene_t()
{
}


void schiene_t::info(cbuffer_t & buf) const
{
	weg_t::info(buf);

	if(is_electrified) {
		buf.append(translator::translate("\nelektrified"));
	} else {
		buf.append(translator::translate("\nnot elektrified"));
	}

  buf.append(translator::translate("\nRail block"));
  buf.append(bs.get_id());
  buf.append("\n");

  bs->info(buf);
}



void
schiene_t::betrete(vehikel_basis_t *v)
{
//    printf("schiene_t::betrete %d,%d", xpos, ypos);
//    printf("    schiene_t::betrete blockstrecke %p\n", bs);

    assert(bs.is_bound());
    bs->betrete( v );

}


void
schiene_t::verlasse(vehikel_basis_t *v)
{
//    printf("schiene_t::verlasse %d,%d", xpos, ypos);
//    printf("    schiene_t::verlasse blockstrecke %p\n", bs);

    assert(bs.is_bound());
    bs->verlasse( v );
}


void
schiene_t::setze_blockstrecke(blockhandle_t bs)
{
    this->bs = bs;
}



bool schiene_t::ist_frei() const
{
    if(!bs.is_bound()) {
  dbg->warning("schiene_t::ist_frei()",
         "Rail track %p is not bound to a rail block!\n", this);
  return true;
    }

    return bs->ist_frei();
}


void
schiene_t::rdwr(loadsave_t *file)
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
		if(blocknr < 0) {
			dbg->error("schiene_t::schiene_t()",
				"A railroad track in the saved game was saved with an invalid rail block id (id=%d)!\n"
				"rail will be given a new block-id", blocknr);
			// Hack wg. invalid railblock ids - kein abort mehr
			setze_blockstrecke(blockstrecke_t::create(welt));
		}
		else {
			setze_blockstrecke(blockmanager::gib_manager()->gib_strecke_bei(blocknr));
		}
	}

	file->rdwr_byte(is_electrified, "\n");

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
			besch = wegbauer_t::weg_search(weg_t::schiene,old_max_speed>0 ? old_max_speed : 120 );
			dbg->warning("schiene_t::rwdr()", "Unknown rail %s replaced by a rail %s (old_max_speed %i)", bname, besch->gib_name(), old_max_speed );
		}
		setze_besch(besch);
		if(old_max_speed>0) {
			setze_max_speed(old_max_speed);
		}
		//DBG_MESSAGE("schiene_t::rdwr","track %s at (%i,%i) max_speed %i",bname,gib_pos().x,gib_pos().y,old_max_speed);
	}
}
