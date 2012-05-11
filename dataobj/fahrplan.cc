/* completely overhauled by prissi Oct-2005 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simwin.h"
#include "../simtypes.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../simimg.h"

#include "../utils/cbuffer_t.h"
#include "../gui/messagebox.h"
#include "../besch/haus_besch.h"
#include "../boden/grund.h"
#include "../dings/gebaeude.h"
#include "../simdepot.h"
#include "loadsave.h"

#include "fahrplan.h"

#include "../tpl/slist_tpl.h"


linieneintrag_t schedule_t::dummy_eintrag(koord3d::invalid, 0, 0);


schedule_t::schedule_t(loadsave_t* const file)
{
	rdwr(file);
	if(file->is_loading()) {
		cleanup();
	}
}



// copy all entries from schedule src to this and adjusts aktuell
void schedule_t::copy_from(const schedule_t *src)
{
	// make sure, we can access both
	if(  src==NULL  ) {
		dbg->fatal("fahrplan_t::copy_to()","cannot copy from NULL");
		return;
	}
	eintrag.clear();
	FOR(minivec_tpl<linieneintrag_t>, const& i, src->eintrag) {
		eintrag.append(i);
	}
	set_aktuell( src->get_aktuell() );

	abgeschlossen = src->ist_abgeschlossen();
}



bool schedule_t::ist_halt_erlaubt(const grund_t *gr) const
{
	// first: check, if we can go here
	waytype_t const my_waytype = get_waytype();
	bool ok = gr->hat_weg(my_waytype);
	if(  !ok  ) {
		if(  my_waytype==air_wt  ) {
			// everywhere is ok but not on stops (we have to load at airports only ...)
			ok = !gr->get_halt().is_bound();
		}
		else if(  my_waytype==water_wt  &&  gr->get_typ()==grund_t::wasser  ) {
			ok = true;
		}
		else if(  my_waytype==tram_wt  ) {
			// tram rails are track iternally
			ok = gr->hat_weg(track_wt);
		}
	}

	if(  ok  ) {
		// ok, we can go here; but we must also check, that we are not entring a foreign depot
		depot_t *dp = gr->get_depot();
		ok &=  (dp==NULL  ||  dp->get_tile()->get_besch()->get_extra()==my_waytype);
	}

	return ok;
}



bool schedule_t::insert(const grund_t* gr, uint8 ladegrad, uint8 waiting_time_shift )
{
	// stored in minivec, so wie have to avoid adding too many
	if(  eintrag.get_count()>=254  ) {
		create_win( new news_img("Maximum 254 stops\nin a schedule!\n"), w_time_delete, magic_none);
		return false;
	}

	if(  ist_halt_erlaubt(gr)  ) {
		eintrag.insert_at(aktuell, linieneintrag_t(gr->get_pos(), ladegrad, waiting_time_shift));
		aktuell ++;
		return true;
	}
	else {
		// too many stops or wrong kind of stop
		create_win( new news_img(fehlermeldung()), w_time_delete, magic_none);
		return false;
	}
}



bool schedule_t::append(const grund_t* gr, uint8 ladegrad, uint8 waiting_time_shift)
{
	// stored in minivec, so wie have to avoid adding too many
	if(eintrag.get_count()>=254) {
		create_win( new news_img("Maximum 254 stops\nin a schedule!\n"), w_time_delete, magic_none);
		return false;
	}

	if(ist_halt_erlaubt(gr)) {
		eintrag.append(linieneintrag_t(gr->get_pos(), ladegrad, waiting_time_shift), 4);
		return true;
	}
	else {
		DBG_MESSAGE("fahrplan_t::append()","forbidden stop at %i,%i,%i",gr->get_pos().x, gr->get_pos().x, gr->get_pos().z );
		// error
		create_win( new news_img(fehlermeldung()), w_time_delete, magic_none);
		return false;
	}
}



// cleanup a schedule
void schedule_t::cleanup()
{
	if(  eintrag.empty()  ) {
		return; // nothing to check
	}

	// first and last must not be the same!
	koord3d lastpos = eintrag.back().pos;
	// now we have to check all entries ...
	for(  uint8 i=0;  i<eintrag.get_count();  i++  ) {
		if(  eintrag[i].pos == lastpos  ) {
			// ingore double entries just one after the other
			eintrag.remove_at(i);
			if(  i<aktuell  ) {
				aktuell --;
			}
			i--;
		} else if(  eintrag[i].pos == koord3d::invalid  ) {
			// ingore double entries just one after the other
			eintrag.remove_at(i);
		}
		else {
			// next pos for check
			lastpos = eintrag[i].pos;
		}
	}
	make_aktuell_valid();
}



bool schedule_t::remove()
{
	bool ok = eintrag.remove_at(aktuell);
	make_aktuell_valid();
	return ok;
}



void schedule_t::rdwr(loadsave_t *file)
{
	xml_tag_t f( file, "fahrplan_t" );

	make_aktuell_valid();

	uint8 size = eintrag.get_count();
	if(  file->get_version()<=101000  ) {
		uint32 dummy=aktuell;
		file->rdwr_long(dummy);
		aktuell = (uint8)dummy;

		sint32 maxi=size;
		file->rdwr_long(maxi);
		DBG_MESSAGE("fahrplan_t::rdwr()","read schedule %p with %i entries",this,maxi);
		if(file->get_version()<86010) {
			// old array had different maxi-counter
			maxi ++;
		}
		size = (uint8)max(0,maxi);
	}
	else {
		file->rdwr_byte(aktuell);
		file->rdwr_byte(size);
	}
	eintrag.resize(size);

	if(file->get_version()<99012) {
		for(  uint8 i=0; i<size; i++  ) {
			koord3d pos;
			uint32 dummy;
			pos.rdwr(file);
			file->rdwr_long(dummy);
			eintrag.append(linieneintrag_t(pos, (uint8)dummy, 0));
		}
	}
	else {
		// loading/saving new version
		for(  uint8 i=0;  i<size;  i++  ) {
			if(eintrag.get_count()<=i) {
				eintrag.append( linieneintrag_t() );
				eintrag[i] .waiting_time_shift = 0;
			}
			eintrag[i].pos.rdwr(file);
			file->rdwr_byte(eintrag[i].ladegrad);
			if(file->get_version()>=99018) {
				file->rdwr_byte(eintrag[i].waiting_time_shift);
			}
		}
	}
	if(file->is_loading()) {
		abgeschlossen = true;
	}
	if(aktuell>=eintrag.get_count()  ) {
		if (!eintrag.empty()) {
			dbg->error("fahrplan_t::rdwr()","aktuell %i >count %i => aktuell = 0", aktuell, eintrag.get_count() );
		}
		aktuell = 0;
	}
}



void schedule_t::rotate90( sint16 y_size )
{
	// now we have to rotate all entries ...
	FOR(minivec_tpl<linieneintrag_t>, & i, eintrag) {
		i.pos.rotate90(y_size);
	}
}



/*
 * compare this fahrplan with another, passed in fahrplan
 * @author hsiegeln
 */
bool schedule_t::matches(karte_t *welt, const schedule_t *fpl)
{
	if(  fpl == NULL  ) {
		return false;
	}
	// same pointer => equal!
	if(  this==fpl  ) {
		return true;
	}
	// unequal count => not equal
	const uint8 min_count = min( fpl->eintrag.get_count(), eintrag.get_count() );
	if(  min_count==0  &&  fpl->eintrag.get_count()!=eintrag.get_count()  ) {
		return false;
	}
	// now we have to check all entries ...
	// we need to do this that complicated, because the last stop may make the difference
	uint16 f1=0, f2=0;
	while(  f1+f2<eintrag.get_count()+fpl->eintrag.get_count()  ) {
		if(f1<eintrag.get_count()  &&  f2<fpl->eintrag.get_count()  &&  fpl->eintrag[(uint8)f2].pos == eintrag[(uint8)f1].pos) {
			// ladegrad/waiting ignored: identical
			f1++;
			f2++;
		}
		else {
			bool ok = false;
			if(  f1<eintrag.get_count()  ) {
				grund_t *gr1 = welt->lookup(eintrag[(uint8)f1].pos);
				if(  gr1  &&  gr1->get_depot()  ) {
					// skip depot
					f1++;
					ok = true;
				}
			}
			if(  f2<fpl->eintrag.get_count()  ) {
				grund_t *gr2 = welt->lookup(fpl->eintrag[(uint8)f2].pos);
				if(  gr2  &&  gr2->get_depot()  ) {
					ok = true;
					f2++;
				}
			}
			// no depot but different => do not match!
			if(  !ok  ) {
				/* in principle we could also check for same halt; but this is dangerous,
				 * since a rebuilding of a single square might change that
				 */
				return false;
			}
		}
	}
	return f1==eintrag.get_count()  &&  f2==fpl->eintrag.get_count();
}



/**
 * Ordering based on halt id
 */
class HaltIdOrdering
{
public:
	bool operator()(const halthandle_t& a, const halthandle_t& b) const { return a.get_id() < b.get_id(); }
};


/*
 * compare this fahrplan with another, ignoring order and exact positions and waypoints
 * @author prissi
 */
bool schedule_t::similar( karte_t *welt, const schedule_t *fpl, const spieler_t *sp )
{
	if(  fpl == NULL  ) {
		return false;
	}
	// same pointer => equal!
	if(  this == fpl  ) {
		return true;
	}
	// unequal count => not equal
	const uint8 min_count = min( fpl->eintrag.get_count(), eintrag.get_count() );
	if(  min_count == 0  ) {
		return false;
	}
	// now we have to check all entries: So we add all stops to a vector we will iterate over
	vector_tpl<halthandle_t> halts;
	for(  uint8 idx = 0;  idx < this->eintrag.get_count();  idx++  ) {
		koord3d p = this->eintrag[idx].pos;
		halthandle_t halt = haltestelle_t::get_halt( welt, p, sp );
		if(  halt.is_bound()  ) {
			halts.insert_unique_ordered( halt, HaltIdOrdering() );
		}
	}
	vector_tpl<halthandle_t> other_halts;
	for(  uint8 idx = 0;  idx < fpl->eintrag.get_count();  idx++  ) {
		koord3d p = fpl->eintrag[idx].pos;
		halthandle_t halt = haltestelle_t::get_halt( welt, p, sp );
		if(  halt.is_bound()  ) {
			other_halts.insert_unique_ordered( halt, HaltIdOrdering() );
		}
	}
	// now compare them
	if(  other_halts.get_count() != halts.get_count()  ) {
		return false;
	}
	// number of unique halt similar => compare them now
	for(  uint32 idx = 0;  idx < halts.get_count();  idx++  ) {
		if(  halts[idx] != other_halts[idx]  ) {
			return false;
		}
	}
	return true;
}



void schedule_t::add_return_way()
{
	if(  eintrag.get_count()<127  &&  eintrag.get_count()>1  ) {
		for(  uint8 maxi=eintrag.get_count()-2;  maxi>0;  maxi--  ) {
			eintrag.append(eintrag[maxi]);
		}
	}
}



void schedule_t::sprintf_schedule( cbuffer_t &buf ) const
{
	buf.printf("%u|%d|", aktuell, (int)get_type());
	FOR(minivec_tpl<linieneintrag_t>, const& i, eintrag) {
		buf.printf("%s,%i,%i|", i.pos.get_str(), (int)i.ladegrad, (int)i.waiting_time_shift);
	}
}


bool schedule_t::sscanf_schedule( const char *ptr )
{
	const char *p = ptr;
	// first: clear current schedule
	while (!eintrag.empty()) {
		remove();
	}
	//  first get aktuell pointer
	aktuell = atoi( p );
	while(  *p  &&  *p!='|'  ) {
		p++;
	}
	if(  *p!='|'  ) {
		dbg->error( "schedule_t::sscanf_schedule()","incomplete entry termination!" );
		return false;
	}
	p++;
	//  then schedule type
	int type = atoi( p );
	//  .. check for correct type
	if(  type != (int)get_type()) {
		dbg->error( "schedule_t::sscanf_schedule()","schedule has wrong type (%d)! should have been %d.", type, get_type() );
		return false;
	}
	while(  *p  &&  *p!='|'  ) {
		p++;
	}
	if(  *p!='|'  ) {
		dbg->error( "schedule_t::sscanf_schedule()","incomplete entry termination!" );
		return false;
	}
	p++;
	// now scan the entries
	while(  *p>0  ) {
		sint16 values[5];
		for(  sint8 i=0;  i<5;  i++  ) {
			values[i] = atoi( p );
			while(  *p  &&  (*p!=','  &&  *p!='|')  ) {
				p++;
			}
			if(  i<4  &&  *p!=','  ) {
				dbg->error( "schedule_t::sscanf_schedule()","incomplete string!" );
				return false;
			}
			if(  i==4  &&  *p!='|'  ) {
				dbg->error( "schedule_t::sscanf_schedule()","incomplete entry termination!" );
				return false;
			}
			p++;
		}
		// ok, now we have a complete entry
		eintrag.append(linieneintrag_t(koord3d(values[0], values[1], values[2]), values[3], values[4]));
	}
	return true;
}
