/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../gui/simwin.h"
#include "../simtypes.h"
#include "../world/simworld.h"
#include "../simhalt.h"
#include "../display/simimg.h"

#include "../utils/cbuffer.h"
#include "../utils/int_math.h"
#include "../gui/messagebox.h"
#include "../descriptor/building_desc.h"
#include "../ground/grund.h"
#include "../obj/gebaeude.h"
#include "../player/simplay.h"
#include "../obj/depot.h"
#include "loadsave.h"
#include "translator.h"

#include "schedule.h"

#include "../tpl/slist_tpl.h"


schedule_entry_t schedule_t::dummy_entry(koord3d::invalid, 0, 0);


// copy all entries from schedule src to this and adjusts current_stop
void schedule_t::copy_from(const schedule_t *src)
{
	// make sure, we can access both
	if(  src==NULL  ) {
		dbg->fatal("schedule_t::copy_to()","cannot copy from NULL");
	}
	entries.clear();
	FOR(minivec_tpl<schedule_entry_t>, const& i, src->entries) {
		entries.append(i);
	}
	set_current_stop( src->get_current_stop() );

	editing_finished = src->is_editing_finished();
}



bool schedule_t::is_stop_allowed(const grund_t *gr) const
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
			// tram rails are track internally
			ok = gr->hat_weg(track_wt);
		}
	}

	if(  ok  ) {
		// ok, we can go here; but we must also check, that we are not entering a foreign depot
		depot_t *dp = gr->get_depot();
		ok &= (dp==NULL  ||  (int)dp->get_tile()->get_desc()->get_extra()==my_waytype);
	}

	return ok;
}



/* returns a valid halthandle if there is a next halt in the schedule;
 * it may however not be allowed to load there, if the owner mismatches!
 */
halthandle_t schedule_t::get_next_halt( player_t *player, halthandle_t halt ) const
{
	if(  entries.get_count()>1  ) {
		for(  uint i=1;  i < entries.get_count();  i++  ) {
			halthandle_t h = haltestelle_t::get_halt( entries[ (current_stop+i) % entries.get_count() ].pos, player );
			if(  h.is_bound()  &&  h != halt  ) {
				return h;
			}
		}
	}
	return halthandle_t();
}


/* returns a valid halthandle if there is a previous halt in the schedule;
 * it may however not be allowed to load there, if the owner mismatches!
 */
halthandle_t schedule_t::get_prev_halt( player_t *player ) const
{
	if(  entries.get_count()>1  ) {
		for(  uint i=1;  i < entries.get_count()-1u;  i++  ) {
			halthandle_t h = haltestelle_t::get_halt( entries[ (current_stop+entries.get_count()-i) % entries.get_count() ].pos, player );
			if(  h.is_bound()  ) {
				return h;
			}
		}
	}
	return halthandle_t();
}


bool schedule_t::insert(const grund_t* gr, uint8 minimum_loading, uint16 waiting_time )
{
	// too many stops or wrong kind of stop
	if (entries.get_count()>=254  ||  !is_stop_allowed(gr)) {
		return false;
	}

	entries.insert_at(current_stop, schedule_entry_t(gr->get_pos(), minimum_loading, waiting_time));
	current_stop ++;
	make_current_stop_valid();
	return true;
}



bool schedule_t::append(const grund_t* gr, uint8 minimum_loading, uint16 waiting_time)
{
	// too many stops or wrong kind of stop
	if (entries.get_count()>=254  ||  !is_stop_allowed(gr)) {
		return false;
	}
	entries.append(schedule_entry_t(gr->get_pos(), minimum_loading, waiting_time), 4);
	return true;
}



// cleanup a schedule
void schedule_t::make_valid()
{
	remove_double_entries();
	if(  entries.get_count() == 1 ) {
		// schedules with one entry not allowed
		entries.clear();
	}
	make_current_stop_valid();
}

void schedule_t::remove_double_entries()
{
	if(  entries.get_count() < 2  ) {
		return; // nothing to check
	}

	// first and last must not be the same!
	koord3d lastpos = entries.back().pos;
	// now we have to check all entries ...
	for(  uint8 i=0;  i<entries.get_count();  i++  ) {
		if(  entries[i].pos == lastpos  ) {
			// ignore double entries just one after the other
			entries.remove_at(i);
			if(  i<current_stop  ) {
				current_stop --;
			}
			i--;
		} else if(  entries[i].pos == koord3d::invalid  ) {
			// ignore double entries just one after the other
			entries.remove_at(i);
		}
		else {
			// next pos for check
			lastpos = entries[i].pos;
		}
	}
}



bool schedule_t::remove()
{
	bool ok = entries.remove_at(current_stop);
	make_current_stop_valid();
	return ok;
}



void schedule_t::remove_entry( uint8 delete_enty )
{
	if( current_stop > delete_enty ) {
		current_stop--;
	}
	entries.remove_at(delete_enty);
	make_current_stop_valid();
}



void schedule_t::move_entry_forward( uint8 cur )
{
	if( entries.get_count() <= 1 ) {
		return;
	}
	// not last entry
	if( cur < entries.get_count()-1 ) {
		// just append everything
		entries.insert_at( cur+2, entries[ cur ] );
		entries.remove_at( cur );
	}
	else {
		// last entry, just append everything
		entries.insert_at( 0, entries[cur] );
		entries.remove_at( cur+1 );
	}

	// if cur was not at end of list then cur and other changed places
	uint8 other = (cur + entries.get_count() + 1 ) % entries.get_count();

	if (cur == entries.get_count()-1) {
		// all entries moved down one index
		current_stop = (current_stop + 1 + entries.get_count()) % entries.get_count();
	}
	else if (current_stop == other) {
		current_stop = cur;
	}
	else if (current_stop == cur) {
		current_stop = other;
	}
}



void schedule_t::move_entry_backward( uint8 cur )
{
	if( entries.get_count() <= 1 ) {
		return;
	}

	if( cur==0 ) {
		//first entry
		entries.append( entries[0] );
		entries.remove_at( 0 );
	}
	else {
		// now move all to new position afterwards
		entries.insert_at( cur-1, entries[ cur ] );
		entries.remove_at( cur+1 );
	}
	// if cur was not at start of list then cur and other changed places
	uint8 other = (cur + entries.get_count() - 1 ) % entries.get_count();

	if (cur == 0) {
		// all entries moved up one index
		current_stop = (current_stop - 1 + entries.get_count()) % entries.get_count();
	}
	else if (current_stop == other) {
		current_stop = cur;
	}
	else if (current_stop == cur) {
		current_stop = other;
	}
}



void schedule_t::rdwr(loadsave_t *file)
{
	xml_tag_t f( file, "fahrplan_t" );

	assert(!file->is_loading() || entries.empty());

	make_current_stop_valid();

	uint8 size = entries.get_count();
	if(  file->is_version_less(101, 1)  ) {
		uint32 dummy=current_stop;
		file->rdwr_long(dummy);
		current_stop = (uint8)dummy;

		sint32 maxi=size;
		file->rdwr_long(maxi);
		DBG_MESSAGE("schedule_t::rdwr()","read schedule %p with %i entries",this,maxi);
		if(file->is_version_less(86, 10)) {
			// old array had different maxi-counter
			maxi ++;
		}
		size = (uint8)max(0,maxi);
	}
	else {
		file->rdwr_byte(current_stop);
		file->rdwr_byte(size);
	}
	entries.resize(size);

	if(file->is_version_less(99, 12)) {
		for(  uint8 i=0; i<size; i++  ) {
			koord3d pos;
			uint32 dummy;
			pos.rdwr(file);
			file->rdwr_long(dummy);
			entries.append(schedule_entry_t(pos, (uint8)dummy, 0));
		}
	}
	else {
		// loading/saving new version
		for(  uint8 i=0;  i<size;  i++  ) {
			if(entries.get_count()<=i) {
				entries.append( schedule_entry_t() );
				entries[i] .waiting_time = 0;
			}
			entries[i].pos.rdwr(file);
			file->rdwr_byte(entries[i].minimum_loading);
			if(file->is_version_atleast(99, 18)) {
				if( file->is_version_atleast( 122, 1 ) ) {
					file->rdwr_short(entries[i].waiting_time);
				}
				else if(file->is_loading()) {
					uint8 wl=0;
					file->rdwr_byte(wl);
					if( entries[i].minimum_loading <= 100 ) {
						if( wl > 0 ) {
							// old value: maximum waiting time in 1/2^(16-n) parts of a month
							entries[ i ].waiting_time = 65535u / (1 << (16u - wl));
						}
					}
					else if( entries[i].minimum_loading > 100 ) {
						// hack to store absolute departure times in old games
						entries[ i ].waiting_time = wl << 8;
					}
				}
				else {
					uint8 wl = entries[ i ].waiting_time >> 8;	// loosing precision, but what can we do ...
					if( entries[ i ].minimum_loading <= 100  &&  entries[ i ].waiting_time > 0  ) {
						wl = log2( (uint32)entries[ i ].waiting_time )+1;
					}
					file->rdwr_byte(wl);
				}
			}
		}
	}
	if(file->is_loading()) {
		editing_finished = true;
	}
	if(current_stop>=entries.get_count()  ) {
		if (!entries.empty()) {
			dbg->error("schedule_t::rdwr()","current_stop %i >count %i => current_stop = 0", current_stop, entries.get_count() );
		}
		current_stop = 0;
	}
}


void schedule_t::rotate90( sint16 y_size )
{
	// now we have to rotate all entries ...
	FOR(minivec_tpl<schedule_entry_t>, & i, entries) {
		i.pos.rotate90(y_size);
	}
}


/**
 * compare this schedule (schedule) with another, passed in schedule
 */
bool schedule_t::matches(karte_t *welt, const schedule_t *schedule)
{
	if(  schedule == NULL  ) {
		return false;
	}
	// same pointer => equal!
	if(  this==schedule  ) {
		return true;
	}
	// no match for empty schedules
	if(  schedule->entries.empty()  ||  entries.empty()  ) {
		return false;
	}
	// now we have to check all entries ...
	// we need to do this that complicated, because the last stop may make the difference
	uint16 f1=0, f2=0;
	while(  f1+f2<entries.get_count()+schedule->entries.get_count()  ) {
		if(f1<entries.get_count()  &&  f2<schedule->entries.get_count()  &&  schedule->entries[(uint8)f2].pos == entries[(uint8)f1].pos) {
			// minimum_loading/waiting ignored: identical
			f1++;
			f2++;
		}
		else {
			bool ok = false;
			if(  f1<entries.get_count()  ) {
				grund_t *gr1 = welt->lookup(entries[(uint8)f1].pos);
				if(  gr1  &&  gr1->get_depot()  ) {
					// skip depot
					f1++;
					ok = true;
				}
			}
			if(  f2<schedule->entries.get_count()  ) {
				grund_t *gr2 = welt->lookup(schedule->entries[(uint8)f2].pos);
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
	return f1==entries.get_count()  &&  f2==schedule->entries.get_count();
}



/**
 * Ordering based on halt id
 */
class HaltIdOrdering
{
public:
	bool operator()(const halthandle_t& a, const halthandle_t& b) const { return a.get_id() < b.get_id(); }
};


/**
 * compare this schedule (schedule) with another, ignoring order and exact positions and waypoints
 */
bool schedule_t::similar( const schedule_t *schedule, const player_t *player )
{
	if(  schedule == NULL  ) {
		return false;
	}
	// same pointer => equal!
	if(  this == schedule  ) {
		return true;
	}
	// unequal count => not equal
	const uint8 min_count = min( schedule->entries.get_count(), entries.get_count() );
	if(  min_count == 0  ) {
		return false;
	}
	// now we have to check all entries: So we add all stops to a vector we will iterate over
	vector_tpl<halthandle_t> halts;
	for(  uint8 idx = 0;  idx < this->entries.get_count();  idx++  ) {
		koord3d p = this->entries[idx].pos;
		halthandle_t halt = haltestelle_t::get_halt( p, player );
		if(  halt.is_bound()  ) {
			halts.insert_unique_ordered( halt, HaltIdOrdering() );
		}
	}
	vector_tpl<halthandle_t> other_halts;
	for(  uint8 idx = 0;  idx < schedule->entries.get_count();  idx++  ) {
		koord3d p = schedule->entries[idx].pos;
		halthandle_t halt = haltestelle_t::get_halt( p, player );
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



void schedule_t::add_return_way(bool append_mirror)
{
	if(  append_mirror  ) {
		// add mirror entries
		if(  entries.get_count()<127  &&  entries.get_count()>1  ) {
			for(  uint8 maxi=entries.get_count()-2;  maxi>0;  maxi--  ) {
				entries.append(entries[maxi]);
			}
		}
	}
	else {
		// invert
		if(  entries.get_count()>1  ) {
			for(  uint8 i=0;  i<(entries.get_count()-1)/2;  i++  ) {
				schedule_entry_t temp = entries[i];
				entries[i] = entries[entries.get_count()-i-1];
				entries[ entries.get_count()-i-1] = temp;
			}
		}
	}
}



void schedule_t::sprintf_schedule( cbuffer_t &buf ) const
{
	buf.printf("%u|%d|", current_stop, (int)get_type());
	FOR(minivec_tpl<schedule_entry_t>, const& i, entries) {
		buf.printf("%s,%i,%i|", i.pos.get_str(), (int)i.minimum_loading, (int)i.waiting_time);
	}
}


bool schedule_t::sscanf_schedule( const char *ptr )
{
	const char *p = ptr;
	// first: clear current schedule
	while (!entries.empty()) {
		remove();
	}
	if ( p == NULL  ||  *p == 0) {
		// empty string
		return false;
	}
	//  first get current_stop pointer
	current_stop = atoi( p );
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
		entries.append(schedule_entry_t(koord3d(values[0], values[1], (sint8)values[2]), (uint8)values[3], (uint16)values[4]));
	}
	make_valid();
	return true;
}


void schedule_t::gimme_stop_name(cbuffer_t& buf, karte_t* welt, player_t const* const player_, schedule_entry_t const& entry, int const max_chars)
{
	const char *p;
	halthandle_t halt = haltestelle_t::get_halt(entry.pos, player_);
	if(halt.is_bound()) {
		p = halt->get_name();
	}
	else {
		const grund_t* gr = welt->lookup(entry.pos);
		if(gr==NULL) {
			p = translator::translate("Invalid coordinate");
		}
		else if(gr->get_depot() != NULL) {
			p = translator::translate("Depot");
		}
		else {
			p = translator::translate("Wegpunkt");
		}
	}
	// finally append
	if(max_chars > 0  &&  strlen(p)>(unsigned)max_chars) {
		buf.printf("%.*s...", max_chars - 3, p);
	}
	else {
		buf.append(p);
	}
	// position (when no length restriction)
	if (max_chars <= 0) {
		buf.printf(" (%s)", entry.pos.get_str());
	}
}
