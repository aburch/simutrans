/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "simdebug.h"
#include "world/simworld.h"
#include "player/simplay.h"
#include "simmesg.h"
#include "display/simimg.h"
#include "obj/signal.h"
#include "obj/tunnel.h"
#include "ground/grund.h"
#include "obj/way/schiene.h"

#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "dataobj/environment.h"

#include "tpl/slist_tpl.h"

#include "old_blockmanager.h"

// only needed for loading old games
class oldsignal_t : public obj_t
{
protected:
	uint8 state;
	uint8 blockend;
	uint8 dir;
	obj_t::typ type;

public:
	oldsignal_t(loadsave_t *file, obj_t::typ type);

	/**
	* return direction or the state of the traffic light
	*/
	ribi_t::ribi get_dir() const { return dir; }

	bool ist_blockiert() const {return blockend != 0;}

	obj_t::typ get_typ() const OVERRIDE { return type; }

	void rdwr(loadsave_t *file) OVERRIDE;

	image_id get_image() const OVERRIDE { return IMG_EMPTY; }
};


static slist_tpl <oldsignal_t *> signale;

//------------------------- old blockmanager ----------------------------
// only there to convert old games to 89.02 and higher

// these two routines for compatibility
oldsignal_t::oldsignal_t(loadsave_t *file, obj_t::typ type) : obj_t ()
{
	this->type = type;
	rdwr(file);
}

void
oldsignal_t::rdwr(loadsave_t *file)
{
	if(!file->is_loading()) {
		dbg->fatal("oldsignal_t::rdwr()","cannot be saved!");
	}
	// loading from blockmanager!
	obj_t::rdwr(file);
	file->rdwr_byte(blockend);
	file->rdwr_byte(state);
	file->rdwr_byte(dir);
}



// now the old block reader
void
old_blockmanager_t::rdwr_block(karte_t *,loadsave_t *file)
{
	sint32 count;
	short int typ = obj_t::signal;

	// load signal
	file->rdwr_long(count);

	for(int i=0; i<count; i++) {
		// read the old signals (only when needed)
		typ=file->rd_obj_id();
		oldsignal_t *sig = new oldsignal_t(file, (obj_t::typ)typ);
		DBG_MESSAGE("oldsignal_t()","on %i,%i with dir=%i blockend=%i",sig->get_pos().x,sig->get_pos().y,sig->get_dir(),sig->ist_blockiert());
		signale.insert( sig );
	}

	// counters
	if(file->is_version_less(88, 6)) {
		// old style
		sint32 dummy = 0;
		file->rdwr_long(dummy);
		file->rdwr_long(dummy);
	}
	else  {
		sint16 dummy;
		file->rdwr_short(dummy);
	}
}



void
old_blockmanager_t::rdwr(karte_t *welt, loadsave_t *file)
{
	signale.clear();
	if(file->is_version_atleast(89, 0)) {
		// nothing to do any more ...
		return;
	}

	assert(file->is_loading());

	// this routine just reads the of signal positions
	// and converts them to the new type>
	sint32 count;
	file->rdwr_long(count);
	for(int i=0; i<count; i++) {
		rdwr_block(welt,file);
	}
	DBG_MESSAGE("old_blockmanager::rdwr","%d old signals loaded",count);
}



void
old_blockmanager_t::finish_rd(karte_t *welt)
{
	DBG_MESSAGE("old_blockmanager::finish_rd()","convert old to new signals" );
	char buf[256];
	const char *err_text=translator::translate("Error restoring old signal near (%i,%i)!");
	int failure=0;
	while (!signale.empty()) {
		oldsignal_t *os1=signale.remove_first();
		oldsignal_t *os2=NULL;
		grund_t *gr=welt->lookup(os1->get_pos());
		grund_t *to=NULL;
		uint8 directions=0;
		waytype_t wt=gr->hat_weg(track_wt) ? track_wt : monorail_wt;
		if(  gr->get_neighbour(to,wt,os1->get_dir())  ) {
			FOR(slist_tpl<oldsignal_t*>, const s, signale) {
				if (s->get_pos() == to->get_pos()) {
					os2 = s;
					break;
				}
			}
			if(os2==NULL) {
				dbg->error("old_blockmanager_t::finish_rd()","old signal near (%i,%i) is unpaired!",gr->get_pos().x,gr->get_pos().y);
				welt->get_message()->add_message(translator::translate("Orphan signal during loading!"),os1->get_pos().get_2d(),message_t::problems);
			}
		}
		else {
			dbg->error("old_blockmanager_t::finish_rd()","old signal near (%i,%i) is unpaired!",gr->get_pos().x,gr->get_pos().y);
			welt->get_message()->add_message(translator::translate("Orphan signal during loading!"),os1->get_pos().get_2d(),message_t::problems);
		}

		// remove second signal from list
		if(os2) {
			signale.remove(os2);
		}

		// now we should have a pair of signals ... or something was very wrong
		grund_t*                new_signal_gr = 0;
		roadsign_desc_t::types type          = roadsign_desc_t::SIGN_SIGNAL;
		ribi_t::ribi dir                      = 0;

		// now find out about type and direction
		if(os2  &&  !os2->ist_blockiert()) {
			// built the signal here, if possible
			grund_t *tmp=to;
			to = gr;
			gr = tmp;
			if(os2->get_typ()==obj_t::old_presignal) {
				type = roadsign_desc_t::SIGN_PRE_SIGNAL;
			}
			else if(os2->get_typ()==obj_t::old_choosesignal) {
				type |= roadsign_desc_t::CHOOSE_SIGN;
			}
			dir = os2->get_dir();
			directions = 1;
		}
		else {
			// gr is already the first choice
			// so we just have to determine the type
			if(os1->get_typ()==obj_t::old_presignal) {
				type = roadsign_desc_t::SIGN_PRE_SIGNAL;
			}
			else if(os1->get_typ()==obj_t::old_choosesignal) {
				type |= roadsign_desc_t::CHOOSE_SIGN;
			}
		}
		// take care of one way
		if(!os1->ist_blockiert()) {
			directions ++;
			dir |= os1->get_dir();
		}

		// now check where we can built best
		if(gr->hat_weg(wt)  &&  ribi_t::is_twoway(gr->get_weg(wt)->get_ribi_unmasked())) {
			new_signal_gr = gr;
		}
		if((new_signal_gr==NULL  ||  !os1->ist_blockiert())  &&  to  &&  to->hat_weg(wt)  &&  ribi_t::is_twoway(to->get_weg(wt)->get_ribi_unmasked())) {
			new_signal_gr = to;
		}
		if(directions==2  &&  new_signal_gr) {
			dir = new_signal_gr->get_weg(wt)->get_ribi_unmasked();
		}

		// found a suitable location, ribi, signal type => construct
		if(new_signal_gr  &&  dir!=0) {
			const roadsign_desc_t *sb=roadsign_t::roadsign_search(type,wt,0);
			if(sb!=NULL) {
				signal_t *sig = new signal_t(new_signal_gr->get_weg(wt)->get_owner(),new_signal_gr->get_pos(),dir,sb);
				new_signal_gr->obj_add(sig);
//DBG_MESSAGE("old_blockmanager::finish_rd()","signal restored at %i,%i with dir %i",gr->get_pos().x,gr->get_pos().y,dir);
			}
			else {
				dbg->error("old_blockmanager_t::finish_rd()","no roadsign for way %x with type %d found!",type,wt);
				sprintf(buf,err_text,os1->get_pos().x,os1->get_pos().y);
				welt->get_message()->add_message(buf,os1->get_pos().get_2d(),message_t::problems);
				failure++;
			}
		}
		else {
			dbg->warning("old_blockmanager_t::finish_rd()","could not restore old signal near (%i,%i), dir=%i",gr->get_pos().x,gr->get_pos().y,dir);
			sprintf(buf,err_text,os1->get_pos().x,os1->get_pos().y);
			welt->get_message()->add_message(buf,os1->get_pos().get_2d(),message_t::problems);
			failure ++;
		}

		os1->set_pos(koord3d::invalid);
		delete os1;
		if(os2) {
			os2->set_pos(koord3d::invalid);
			delete os2;
		}
	}
	if(failure) {
		dbg->warning("old_blockmanager_t::finish_rd()","failed on %d signal pairs.",failure);
	}
}
