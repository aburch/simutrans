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
#include "../bauer/hausbauer.h"
#include "../besch/haus_besch.h"
#include "../boden/grund.h"
#include "../dings/gebaeude.h"
#include "../dings/zeiger.h"
#include "../simdepot.h"
#include "loadsave.h"

#include "fahrplan.h"

#include "../tpl/slist_tpl.h"


struct linieneintrag_t schedule_t::dummy_eintrag = { koord3d::invalid, 0, 0 };


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
	for(  uint8 i=0;  i<src->eintrag.get_count();  i++  ) {
		eintrag.append(src->eintrag[i]);
	}
	set_aktuell( src->get_aktuell() );

	abgeschlossen = src->ist_abgeschlossen();
	circular = src->is_circular();
	mirrored = src->is_mirrored();
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
#ifndef _MSC_VER
	struct linieneintrag_t stop = { gr->get_pos(), ladegrad, waiting_time_shift };
#else
	struct linieneintrag_t stop;
	stop.pos = gr->get_pos();
	stop.ladegrad = ladegrad;
	stop.waiting_time_shift = waiting_time_shift;
#endif
	// stored in minivec, so wie have to avoid adding too many
	if(  eintrag.get_count()>=254  ) {
		create_win( new news_img("Maximum 254 stops\nin a schedule!\n"), w_time_delete, magic_none);
		return false;
	}

	if(  ist_halt_erlaubt(gr)  ) {
		eintrag.insert_at(aktuell, stop);
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
#ifndef _MSC_VER
	struct linieneintrag_t stop = { gr->get_pos(), ladegrad, waiting_time_shift };
#else
	struct linieneintrag_t stop;
	stop.pos = gr->get_pos();
	stop.ladegrad = ladegrad;
	stop.waiting_time_shift = waiting_time_shift;
#endif

	// stored in minivec, so wie have to avoid adding too many
	if(eintrag.get_count()>=254) {
		create_win( new news_img("Maximum 254 stops\nin a schedule!\n"), w_time_delete, magic_none);
		return false;
	}

	if(ist_halt_erlaubt(gr)) {
		eintrag.append(stop, 4);
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
	if (eintrag.empty()) {
		return; // nothing to check
		aktuell = 0;
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
	if(  aktuell>=eintrag.get_count()  ) {
		aktuell = max(1,eintrag.get_count())-1;
	}
}



bool schedule_t::remove()
{
	bool ok = eintrag.remove_at(aktuell);
	if(  aktuell>=eintrag.get_count()  ) {
		aktuell = max(1,eintrag.get_count())-1;
	}
	return ok;
}



void schedule_t::rdwr(loadsave_t *file)
{
	xml_tag_t f( file, "fahrplan_t" );

	if(  aktuell>=eintrag.get_count()  ) {
		aktuell = max(1,eintrag.get_count())-1;
	}

	uint8 size = eintrag.get_count();
	if(  file->get_version()<=101000  ) {
		uint32 dummy=aktuell;
		file->rdwr_long(dummy, " ");
		aktuell = (uint8)dummy;

		sint32 maxi=size;
		file->rdwr_long(maxi, " ");
		DBG_MESSAGE("fahrplan_t::rdwr()","read schedule %p with %i entries",this,maxi);
		if(file->get_version()<86010) {
			// old array had different maxi-counter
			maxi ++;
		}
		size = (uint8)max(0,maxi);
	}
	else {
		file->rdwr_byte(aktuell, " ");
		file->rdwr_byte(size, " ");
		if( file->get_version()>=102003 && file->get_experimental_version()>=9 ) {
			file->rdwr_bool(circular, " ");
			file->rdwr_bool(mirrored, " ");
		}
	}
	eintrag.resize(size);

	if(file->get_version()<99012) {
		for(  uint8 i=0; i<size; i++  ) {
			koord3d pos;
			uint32 dummy;
			pos.rdwr(file);
			file->rdwr_long(dummy, "\n");

			struct linieneintrag_t stop;
			stop.pos = pos;
			stop.ladegrad = (sint8)dummy;
			stop.waiting_time_shift = 0;
			eintrag.append(stop);
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
			file->rdwr_byte(eintrag[i].ladegrad, "\n");
			if(file->get_version()>=99018) {
				file->rdwr_byte( eintrag[i].waiting_time_shift, "w" );
			}
		}
	}
	if(file->is_loading()) {
		abgeschlossen = true;
	}
	if(aktuell>=eintrag.get_count()) {
		dbg->error("fahrplan_t::rdwr()","aktuell %i >count %i => aktuell = 0", aktuell, eintrag.get_count() );
		aktuell = 0;
	}
}



void schedule_t::rotate90( sint16 y_size )
{
 	// now we have to rotate all entries ...
	for(  uint8 i = 0;  i<eintrag.get_count();  i++  ) {
		eintrag[i].pos.rotate90(y_size);
	}
}



/*
 * compare this fahrplan with another, passed in fahrplan
 * @author hsiegeln
 */
bool schedule_t::matches(karte_t *welt, const schedule_t *fpl)
{
	if(fpl == NULL) {
		return false;
	}
	// same pointer => equal!
	if(this==fpl) {
		return true;
	}
	// different circular or mirrored settings => not equal
	if ((this->circular != fpl->circular) || (this->mirrored != fpl->mirrored)) {
		return false;
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
		if(f1<eintrag.get_count()  &&  f2<fpl->eintrag.get_count()  &&  fpl->eintrag[f2].pos == eintrag[f1].pos && fpl->eintrag[f2].ladegrad == eintrag[f1].ladegrad && fpl->eintrag[f2].waiting_time_shift == eintrag[f1].waiting_time_shift) {
			// ladegrad/waiting ignored: identical
			f1++;
			f2++;
		}
		else {
			bool ok = false;
			if(  f1<eintrag.get_count()  ) {
				grund_t *gr1 = welt->lookup(eintrag[f1].pos);
				if(  gr1  &&  gr1->get_depot()  ) {
					// skip depot
					f1++;
					ok = true;
				}
			}
			if(  f2<fpl->eintrag.get_count()  ) {
				grund_t *gr2 = welt->lookup(fpl->eintrag[f2].pos);
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



void schedule_t::add_return_way()
{
	if(  eintrag.get_count()<127  &&  eintrag.get_count()>1  ) {
		for(  uint8 maxi=eintrag.get_count()-2;  maxi>0;  maxi--  ) {
			eintrag.append(eintrag[maxi]);
		}
	}
}


/*
 * Increment or decrement the given index according to the given direction.
 * Also switches the direction if necessary.
 * @author yobbobandana
 */
void schedule_t::increment_index(uint8 *index, bool *reversed) const {
	if( !get_count() ) { return; }
	if( *reversed ) {
		if( *index != 0 ) {
			*index = *index - 1;
		} else if( mirrored ) {
			*reversed = false;
			*index = get_count()>1 ? 1 : 0;
		} else {
			*index = get_count()-1;
		}
	} else {
		if( *index < get_count()-1 ) {
			*index = *index + 1;
		} else if( mirrored && get_count()>1 ) {
			*reversed = true;
			*index = get_count()-2;
		} else {
			*index = 0;
		}
	}
}


void schedule_t::sprintf_schedule( cbuffer_t &buf ) const
{
	buf.append( aktuell );
	buf.printf( ",%i,%i", circular, mirrored );
	buf.append( "|" );
	for(  uint8 i = 0;  i<eintrag.get_count();  i++  ) {
		buf.printf( "%s,%i,%i|", eintrag[i].pos.get_str(), (int)eintrag[i].ladegrad, (int)eintrag[i].waiting_time_shift );
	}
}


bool schedule_t::sscanf_schedule( const char *ptr )
{
	const char *p = ptr;
	// first: clear current schedule
	while(  eintrag.get_count()>0  ) {
		remove();
	}
	//  first get aktuell pointer
	aktuell = atoi( p );
	while(  *p  &&  (*p!=','  &&  *p!='|')  ) {
		p++;
	}
	if( *p==',' ) { p++; }
	if( *p && (*p!=','  &&  *p!='|') ) { circular = bool(atoi(p)); }
	while(  *p  &&  (*p!=','  &&  *p!='|')  ) {
		p++;
	}
	if( *p==',' ) { p++; }
	if( *p && (*p!=','  &&  *p!='|') ) { mirrored = bool(atoi(p)); }
	while(  *p  &&  (*p!=','  &&  *p!='|')  ) {
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
#ifndef _MSC_VER
		struct linieneintrag_t stop = { koord3d(values[0],values[1],values[2]), values[3], values[4] };
#else
		struct linieneintrag_t stop;
		stop.pos = koord3d(values[0],values[1],values[2]);
		stop.ladegrad = values[3];
		stop.waiting_time_shift = values[4];
#endif
		eintrag.append( stop );
	}
	return true;
}
