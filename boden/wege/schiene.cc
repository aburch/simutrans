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
#include "../../simconvoi.h"
#include "../../simvehikel.h"
#include "../../blockmanager.h"
#include "../grund.h"

#include "../../dataobj/loadsave.h"
#include "../../dataobj/translator.h"

#include "../../dings/signal.h"

#include "../../utils/cbuffer_t.h"

#include "../../besch/weg_besch.h"
#include "../../bauer/hausbauer.h"
#include "../../bauer/wegbauer.h"

#include "schiene.h"

const weg_besch_t *schiene_t::default_schiene=NULL;


schiene_t::schiene_t(karte_t *welt) : weg_t(welt)
{
	is_electrified = false;
	nr_of_signals = 0;
	reserved = convoihandle_t();
	bs = blockhandle_t();
	setze_besch(schiene_t::default_schiene);
}

/* create schiene with minimum topspeed
 * @author prissi
 */
schiene_t::schiene_t(karte_t *welt,int top_speed) : weg_t(welt)
{
	is_electrified = false;
	reserved = convoihandle_t();
	nr_of_signals = 0;
	bs = blockhandle_t();
	setze_besch(wegbauer_t::weg_search(gib_typ(),top_speed));
}

schiene_t::schiene_t(karte_t *welt, loadsave_t *file) : weg_t(welt)
{
	is_electrified = false;
	nr_of_signals = 0;
	reserved = convoihandle_t();
	bs = blockhandle_t();
	rdwr(file);
}



/**
 * counts signals on this tile;
 * It would be enough for the signals to register and unreigister themselves, but this is more secure ...
 * @author prissi
 */
bool
schiene_t::count_signals()
{
	const grund_t *gr=welt->lookup(gib_pos());
	if(gr) {
		nr_of_signals = 0;
		for( uint8 i=0;  i<gr->obj_count();  i++  ) {
			ding_t *d=gr->obj_bei(i);
			if(d  &&  (d->gib_typ()==ding_t::choosesignal  ||  d->gib_typ()==ding_t::presignal  ||  d->gib_typ()==ding_t::signal)) {
				nr_of_signals ++;
				((signal_t *)d)->set_block(bs);
			}
		}
		return true;
	}
	return false;
}



void
schiene_t::setze_blockstrecke(blockhandle_t bs)
{
	const grund_t *gr=welt->lookup(gib_pos());
	if(gr  &&  nr_of_signals>0) {
		nr_of_signals = 0;
		for( uint8 i=0;  i<gr->obj_count();  i++  ) {
			ding_t *d=gr->obj_bei(i);
			if(d  &&  (d->gib_typ()==ding_t::choosesignal  ||  d->gib_typ()==ding_t::presignal  ||  d->gib_typ()==ding_t::signal)) {
				((signal_t *)d)->set_block(bs);
				nr_of_signals ++;
			}
		}
	}
	this->bs = bs;
}




void schiene_t::info(cbuffer_t & buf) const
{
	weg_t::info(buf);

	if(is_electrified) {
		buf.append(translator::translate("\nelektrified"));
	}
	else {
		buf.append(translator::translate("\nnot elektrified"));
	}

	buf.append(translator::translate("\nRail block"));
	buf.append(bs.get_id());
	buf.append("\n");

	bs->info(buf);
}



/**
 * true, if this rail can be reserved
 * @author prissi
 */
bool
schiene_t::reserve(convoihandle_t c) {
	if(can_reserve(c)) {
		reserved = c;
		return true;
	}
	// reserve anyway ...
	return false;
}



/**
* releases previous reservation
* only true, if there was something to release
* @author prissi
*/
bool
schiene_t::unreserve(convoihandle_t c)
{
	// is this tile reserved by us?
	if(reserved.is_bound()  &&  reserved==c) {
		reserved = convoihandle_t();
		return true;
	}
	return false;
}




/**
* releases previous reservation
* @author prissi
*/
bool
schiene_t::unreserve(vehikel_t *v)
{
	// is this tile empty?
	if(!reserved.is_bound()) {
		return true;
	}
	if(!welt->lookup(gib_pos())->suche_obj(v->gib_typ())) {
		reserved = convoihandle_t();
		return true;
	}
	return false;
}




void
schiene_t::rdwr(loadsave_t *file)
{
	long blocknr;

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
			dbg->warning("schiene_t::schiene_t()","invalid rail block id (id=%d) in saved game", blocknr);
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
			dbg->warning("schiene_t::rwdr()", "Unknown rail %s replaced by a rail %s (old_max_speed %i)", bname, besch->gib_name(), old_max_speed );
		}
		setze_besch(besch);
		if(old_max_speed>0) {
			setze_max_speed(old_max_speed);
		}
		//DBG_MESSAGE("schiene_t::rdwr","track %s at (%i,%i) max_speed %i",bname,gib_pos().x,gib_pos().y,old_max_speed);
	}
}
