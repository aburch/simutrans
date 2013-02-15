/* completely overhauled by prissi Oct-2005 */

#include <stdio.h>
#include <ctype.h>

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

linieneintrag_t schedule_t::dummy_eintrag(koord3d::invalid, 0, 0, 0, false);

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
	spacing = src->get_spacing();
	bidirectional = src->is_bidirectional();
	mirrored = src->is_mirrored();
	same_spacing_shift = src->is_same_spacing_shift();

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



bool schedule_t::insert(const grund_t* gr, uint16 ladegrad, uint8 waiting_time_shift, sint16 spacing_shift, bool show_failure)
{
	// stored in minivec, so we have to avoid adding too many
	if(eintrag.get_count() >= 254) 
	{
		if(show_failure)
		{
			create_win( new news_img("Maximum 254 stops\nin a schedule!\n"), w_time_delete, magic_none);
		}
		return false;
	}

	if(!gr)
	{
		// This can occur in some cases if a depot is not found.
		return false;
	}

	if(  ist_halt_erlaubt(gr)  ) {
		eintrag.insert_at(aktuell, linieneintrag_t(gr->get_pos(), ladegrad, waiting_time_shift, spacing_shift, false));
		aktuell ++;
		return true;
	}
	else {
		// too many stops or wrong kind of stop
		if(show_failure)
		{
			create_win( new news_img(fehlermeldung()), w_time_delete, magic_none);
		}
		return false;
	}
}



bool schedule_t::append(const grund_t* gr, uint16 ladegrad, uint8 waiting_time_shift, sint16 spacing_shift)
{
	// stored in minivec, so wie have to avoid adding too many
	if(eintrag.get_count()>=254) {
		create_win( new news_img("Maximum 254 stops\nin a schedule!\n"), w_time_delete, magic_none);
		return false;
	}

	if(!gr)
	{
		// This can occur in some cases if a depot is not found.
		return false;
	}

	if(ist_halt_erlaubt(gr)) {
		eintrag.append(linieneintrag_t(gr->get_pos(), ladegrad, waiting_time_shift, spacing_shift, false), 4);
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
		if(file->get_version()>=102003 && file->get_experimental_version() >= 9)
		{
			file->rdwr_bool(bidirectional);
			file->rdwr_bool(mirrored);
		}
	}
	eintrag.resize(size);

	if(file->get_version()<99012) {
		for(  uint8 i=0; i<size; i++  ) {
			koord3d pos;
			uint32 dummy;
			pos.rdwr(file);
			file->rdwr_long(dummy);
			eintrag.append(linieneintrag_t(pos, (uint8)dummy, 0, 0, false));
		}
	}
	else {
		// loading/saving new version
		for(  uint8 i=0;  i<size;  i++  ) {
			if(eintrag.get_count()<=i) {
				eintrag.append( linieneintrag_t() );
				eintrag[i] .waiting_time_shift = 0;
				eintrag[i].spacing_shift = 0;
				eintrag[i].reverse = false;
			}
			eintrag[i].pos.rdwr(file);
			if(file->get_experimental_version() >= 10 && file->get_version() >= 111002)
			{
				file->rdwr_short(eintrag[i].ladegrad);
			}
			else
			{
				// Previous versions had ladegrad as a uint8. 
				uint8 old_ladegrad = (uint8)eintrag[i].ladegrad;
				file->rdwr_byte(old_ladegrad);
				eintrag[i].ladegrad = (uint16)old_ladegrad;
			}
			if(file->get_version()>=99018) {
				file->rdwr_byte(eintrag[i].waiting_time_shift);

				if(file->get_experimental_version() >= 9 && file->get_version() >= 110006) 
				{
					file->rdwr_short(eintrag[i].spacing_shift);
				}

				if(file->get_experimental_version() >= 10)
				{
					file->rdwr_bool(eintrag[i].reverse);
				}
				else
				{
					eintrag[i].reverse = false;
				}
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

	if(file->get_experimental_version() >= 9)
	{
		file->rdwr_short(spacing);
	}

	if(file->get_experimental_version() >= 9 && file->get_version() >= 110006)
	{
		file->rdwr_bool(same_spacing_shift);
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
	if(fpl == NULL) {
		return false;
	}
	// same pointer => equal!
	if(this==fpl) {
		return true;
	}
	// different bidirectional or mirrored settings => not equal
	if ((this->bidirectional != fpl->bidirectional) || (this->mirrored != fpl->mirrored)) {
		return false;
	}
	// unequal count => not equal
	const uint8 min_count = min( fpl->eintrag.get_count(), eintrag.get_count() );
	if(  min_count==0  &&  fpl->eintrag.get_count()!=eintrag.get_count()  ) {
		return false;
	}
	if ( this->spacing != fpl->spacing ) {
		return false;
	}
	if (this->same_spacing_shift != fpl->same_spacing_shift) {
		return false;
	}
	// now we have to check all entries ...
	// we need to do this that complicated, because the last stop may make the difference
	uint16 f1=0, f2=0;
	while(  f1+f2<eintrag.get_count()+fpl->eintrag.get_count()  ) {

		if(		f1<eintrag.get_count()  &&  f2<fpl->eintrag.get_count()
			&& fpl->eintrag[(uint8)f2].pos == eintrag[(uint8)f1].pos 
			&& fpl->eintrag[(uint16)f2].ladegrad == eintrag[(uint16)f1].ladegrad 
			&& fpl->eintrag[(uint8)f2].waiting_time_shift == eintrag[(uint8)f1].waiting_time_shift 
			&& fpl->eintrag[(uint8)f2].spacing_shift == eintrag[(uint8)f1].spacing_shift
		  ) {
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


/*
 * Increment or decrement the given index according to the given direction.
 * Also switches the direction if necessary.
 * @author yobbobandana
 */
void schedule_t::increment_index(uint8 *index, bool *reversed) const {
	if( !get_count() ) 
	{ 
		return; 
	}
	if( *reversed ) 
	{
		if( *index != 0 ) 
		{
			*index = *index - 1;
		} 
		else if( mirrored ) 
		{
			*reversed = false;
			*index = get_count()>1 ? 1 : 0;
		} 
		else 
		{
			*index = get_count()-1;
		}
	} 
	else 
	{
		if( *index < get_count()-1 ) 
		{
			*index = *index + 1;
		} 
		else if( mirrored && get_count()>1 ) 
		{
			*reversed = true;
			*index = get_count()-2;
		} 
		else
		{
			*index = 0;
		}
	}
}


void schedule_t::sprintf_schedule( cbuffer_t &buf ) const
{
	buf.append( aktuell );
	buf.printf( ",%i,%i,%i,%i", bidirectional, mirrored, spacing, same_spacing_shift) ;
	buf.append( "|" );
	buf.append( (int)get_type() );
	buf.append( "|" );
	FOR(minivec_tpl<linieneintrag_t>, const& i, eintrag) 
	{
		buf.printf( "%s,%i,%i,%i,%i|", i.pos.get_str(), i.ladegrad, (int)i.waiting_time_shift, (int)i.spacing_shift, (int)i.reverse );
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
	while ( *p && isdigit(*p) ) { p++; }
	if ( *p && *p == ',' ) { p++; }
	//bidirectional flag
	if( *p && (*p!=','  &&  *p!='|') ) { bidirectional = bool(atoi(p)); }
	while ( *p && isdigit(*p) ) { p++; }
	if ( *p && *p == ',' ) { p++; }
	// mirrored flag
	if( *p && (*p!=','  &&  *p!='|') ) { mirrored = bool(atoi(p)); }
	while ( *p && isdigit(*p) ) { p++; }
	if ( *p && *p == ',' ) { p++; }
	// spacing
	if( *p && (*p!=','  &&  *p!='|') ) { spacing = atoi(p); }
	while ( *p && isdigit(*p) ) { p++; }
	if ( *p && *p == ',' ) { p++; }
	// same_spacing_shift flag
	if( *p && (*p!=','  &&  *p!='|') ) { same_spacing_shift = bool(atoi(p)); }
	while ( *p && isdigit(*p) ) { p++; }
	if ( *p && *p == ',' ) { p++; }

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
		sint16 values[7];
		for(  sint8 i=0;  i<7;  i++  ) {
			values[i] = atoi( p );
			while(  *p  &&  (*p!=','  &&  *p!='|')  ) {
				p++;
			}
			if(  i<6  &&  *p!=','  ) {
				dbg->error( "schedule_t::sscanf_schedule()","incomplete string!" );
				return false;
			}
			if(  i==6  &&  *p!='|'  ) {
				dbg->error( "schedule_t::sscanf_schedule()","incomplete entry termination!" );
				return false;
			}
			p++;
		}
		// ok, now we have a complete entry
		eintrag.append(linieneintrag_t(koord3d(values[0], values[1], values[2]), values[3], values[4], values[5], (bool)values[6]));
	}
	return true;
}

bool schedule_t::is_contained (koord3d pos)
{
	ITERATE(eintrag, i)
	{
		if(pos == eintrag[i].pos)
		{
			return true;
		}
	}
	return false;
}
